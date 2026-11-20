#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AdlibSakuraProcessor::createLayout()
{
    using APF  = juce::AudioParameterFloat;
    using Attr = juce::AudioParameterFloatAttributes;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto pctText = [] (float v, int) { return juce::String (juce::roundToInt (v * 100.f)) + " %"; };
    auto pctParse = [] (const juce::String& t) { return t.getFloatValue() / 100.f; };
    auto dbText  = [] (float v, int) { return juce::String (v, 1) + " dB"; };
    auto msText  = [] (float v, int) { return juce::String (juce::roundToInt (v)) + " ms"; };

    auto addPct = [&] (const char* id, const char* name, float def)
    {
        layout.add (std::make_unique<APF> (juce::ParameterID { id, 1 }, name,
                                           juce::NormalisableRange<float> (0.f, 1.f, 0.001f), def,
                                           Attr().withStringFromValueFunction (pctText)
                                                 .withValueFromStringFunction (pctParse)));
    };

    auto addDb = [&] (const char* id, const char* name, float def)
    {
        layout.add (std::make_unique<APF> (juce::ParameterID { id, 1 }, name,
                                           juce::NormalisableRange<float> (-24.f, 12.f, 0.1f), def,
                                           Attr().withStringFromValueFunction (dbText)));
    };

    addDb  ("entrada",       "Entrada",           0.f);
    addPct ("drive",         "Distorsión",        0.30f);
    addPct ("tono",          "Tono",              0.70f);
    addPct ("lofi",          "Lo-Fi",             0.00f);
    addPct ("doble",         "Doble",             0.30f);
    addDb  ("salida",        "Salida",            0.f);

    layout.add (std::make_unique<APF> (juce::ParameterID { "ecoTiempo", 1 }, "Eco Tiempo",
                                       juce::NormalisableRange<float> (40.f, 900.f, 1.f, 0.6f), 320.f,
                                       Attr().withStringFromValueFunction (msText)));
    layout.add (std::make_unique<APF> (juce::ParameterID { "ecoRepeticion", 1 }, "Eco Repetición",
                                       juce::NormalisableRange<float> (0.f, 0.9f, 0.001f), 0.35f,
                                       Attr().withStringFromValueFunction ([] (float v, int)
                                           { return juce::String (juce::roundToInt (v * 100.f)) + " %"; })
                                             .withValueFromStringFunction (pctParse)));
    addPct ("ecoMezcla",     "Eco Mezcla",        0.25f);
    addPct ("revTamano",     "Reverb Tamaño",     0.60f);
    addPct ("revOscuridad",  "Reverb Oscuridad",  0.50f);
    addPct ("revMezcla",     "Reverb Mezcla",     0.30f);

    return layout;
}

//==============================================================================
AdlibSakuraProcessor::AdlibSakuraProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

bool AdlibSakuraProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in  = layouts.getMainInputChannelSet();

    if (out != juce::AudioChannelSet::stereo())
        return false;

    return in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono();
}

void AdlibSakuraProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    chorus.prepare (spec);
    chorus.setRate (0.9f);
    chorus.setDepth (0.35f);
    chorus.setCentreDelay (16.f);
    chorus.setFeedback (0.f);
    chorus.reset();

    echo.setMaximumDelayInSamples ((int) (sampleRate * 2.0));
    echo.prepare (spec);
    echo.reset();

    reverb.setSampleRate (sampleRate);
    reverb.reset();

    toneState[0] = toneState[1] = 0.f;
    echoFbState[0] = echoFbState[1] = 0.f;

    const double rampSeconds = 0.03;
    sIn.reset (sampleRate, rampSeconds);       sIn.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (param ("entrada")));
    sDrive.reset (sampleRate, rampSeconds);    sDrive.setCurrentAndTargetValue (param ("drive"));
    sEchoTime.reset (sampleRate, 0.15);        sEchoTime.setCurrentAndTargetValue (param ("ecoTiempo"));
    sEchoMix.reset (sampleRate, rampSeconds);  sEchoMix.setCurrentAndTargetValue (param ("ecoMezcla"));
    sOut.reset (sampleRate, rampSeconds);      sOut.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (param ("salida")));
}

//==============================================================================
void AdlibSakuraProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int totalIn = getTotalNumInputChannels();

    for (int c = totalIn; c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);

    if (buffer.getNumChannels() < 2)
        return;

    // Si entra mono, lo duplicamos a estéreo
    if (totalIn == 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, n);

    float* data[2] = { buffer.getWritePointer (0), buffer.getWritePointer (1) };

    // ---- Parámetros ----
    sIn.setTargetValue (juce::Decibels::decibelsToGain (param ("entrada")));
    sDrive.setTargetValue (param ("drive"));
    sEchoTime.setTargetValue (param ("ecoTiempo"));
    sEchoMix.setTargetValue (param ("ecoMezcla"));
    sOut.setTargetValue (juce::Decibels::decibelsToGain (param ("salida")));

    const float tone      = param ("tono");
    const float cutoff    = 700.f * std::pow (28.f, tone);           // 700 Hz .. ~19.6 kHz
    const float toneCoef  = std::exp (-juce::MathConstants<float>::twoPi * cutoff / (float) sr);

    const float lofi      = param ("lofi");
    const float levels    = std::pow (2.f, 16.f - lofi * 12.f);       // 16 bits -> 4 bits

    const float feedback  = param ("ecoRepeticion");
    const float fbCoef    = std::exp (-juce::MathConstants<float>::twoPi * 4500.f / (float) sr); // repeticiones más oscuras

    // ---- 1) Entrada + Distorsión + Tono + Lo-Fi ----
    for (int i = 0; i < n; ++i)
    {
        const float gIn   = sIn.getNextValue();
        const float drive = sDrive.getNextValue();
        const float g     = 1.f + drive * drive * 40.f;
        const float comp  = 1.f / std::pow (g, 0.35f);

        for (int ch = 0; ch < 2; ++ch)
        {
            float y = std::tanh (data[ch][i] * gIn * g) * comp;

            toneState[ch] = (1.f - toneCoef) * y + toneCoef * toneState[ch];
            y = toneState[ch];

            if (lofi > 0.001f)
                y = std::round (y * levels) / levels;

            data[ch][i] = y;
        }
    }

    // ---- 2) Doble (chorus estéreo) ----
    chorus.setMix (param ("doble") * 0.6f);
    {
        juce::dsp::AudioBlock<float> block (buffer);
        auto stereoBlock = block.getSubsetChannelBlock (0, 2);
        juce::dsp::ProcessContextReplacing<float> ctx (stereoBlock);
        chorus.process (ctx);
    }

    // ---- 3) Eco (el canal derecho va un poco más lento: efecto "ping-pong" suave) ----
    for (int i = 0; i < n; ++i)
    {
        const float timeMs = sEchoTime.getNextValue();
        const float mix    = sEchoMix.getNextValue();
        const float dSamp[2] = { timeMs * 0.001f * (float) sr, timeMs * 0.001f * (float) sr * 1.33f };

        for (int ch = 0; ch < 2; ++ch)
        {
            const float in = data[ch][i];
            echo.pushSample (ch, in + echoFbState[ch] * feedback);
            const float del = echo.popSample (ch, dSamp[ch], true);

            echoFbState[ch] = (1.f - fbCoef) * del + fbCoef * echoFbState[ch];
            data[ch][i] = in + del * mix;
        }
    }

    // ---- 4) Reverb ----
    {
        juce::Reverb::Parameters rp;
        rp.roomSize   = param ("revTamano");
        rp.damping    = param ("revOscuridad");
        rp.wetLevel   = param ("revMezcla") * 0.33f;   // JUCE multiplica el wet x3
        rp.dryLevel   = 0.5f;                          // JUCE multiplica el dry x2 -> ganancia 1
        rp.width      = 1.f;
        rp.freezeMode = 0.f;
        reverb.setParameters (rp);
        reverb.processStereo (data[0], data[1], n);
    }

    // ---- 5) Salida (con un recorte de seguridad) ----
    for (int i = 0; i < n; ++i)
    {
        const float gOut = sOut.getNextValue();
        for (int ch = 0; ch < 2; ++ch)
            data[ch][i] = juce::jlimit (-2.f, 2.f, data[ch][i] * gOut);
    }
}

//==============================================================================
juce::AudioProcessorEditor* AdlibSakuraProcessor::createEditor()
{
    return new AdlibSakuraEditor (*this);
}

void AdlibSakuraProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AdlibSakuraProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AdlibSakuraProcessor();
}
