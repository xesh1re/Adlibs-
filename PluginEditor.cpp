#include "PluginEditor.h"

namespace
{
    constexpr int kWidth  = 780;
    constexpr int kHeight = 540;

    struct KnobDef { const char* id; const char* nombre; const char* descripcion; };

    // Orden: fila 1 (entrada / color) y fila 2 (espacio: eco + reverb)
    const KnobDef kDefs[] =
    {
        { "entrada", "Entrada",
          "Sube o baja el volumen de tu voz antes de los efectos. Si suena muy distorsionada sin querer, bájalo." },
        { "drive", "Distorsión",
          "Ensucia y agranda tu adlib. Poquito le da calor y cuerpo; al máximo suena agresivo y rasposo." },
        { "tono", "Tono",
          "Controla qué tan brillante u oscuro suena. Hacia la izquierda suaviza los agudos; hacia la derecha abre el brillo." },
        { "lofi", "Lo-Fi",
          "Le da un efecto sucio de radio o teléfono, como grabado en baja calidad. Perfecto para adlibs con textura." },
        { "doble", "Doble",
          "Duplica tu voz con un pequeño movimiento para que suene más ancha y como si hubiera dos tomas cantando." },
        { "salida", "Salida",
          "Volumen final del plugin. Úsalo para compensar si los efectos te subieron o bajaron el nivel." },
        { "ecoTiempo", "Eco Tiempo",
          "Qué tan separados suenan los ecos. Valores cortos dan un rebote rápido; largos, repeticiones espaciadas." },
        { "ecoRepeticion", "Eco Repetición",
          "Cuántas veces se repite el eco. Al máximo se repite muchísimo, ideal para finales de frase." },
        { "ecoMezcla", "Eco Mezcla",
          "Cuánto eco se escucha junto a tu voz. En cero no hay eco; sube para que los adlibs floten detrás de la voz principal." },
        { "revTamano", "Reverb Tamaño",
          "El tamaño del espacio donde suena tu voz: de un cuarto pequeño a una catedral enorme." },
        { "revOscuridad", "Reverb Oscuridad",
          "Qué tan oscura es la reverb. Más alto apaga los agudos del eco del espacio y se siente más suave y lejana." },
        { "revMezcla", "Reverb Mezcla",
          "Cuánta reverb se mezcla con tu voz. Sube para que el adlib suene lejano, etéreo y dramático." },
    };
}

//==============================================================================
AdlibSakuraEditor::AdlibSakuraEditor (AdlibSakuraProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&laf);
    setSize (kWidth, kHeight);

    for (const auto& def : kDefs)
    {
        auto k = std::make_unique<Knob>();
        k->descripcion = juce::String (juce::CharPointer_UTF8 (def.descripcion));

        auto& s = k->slider;
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                               juce::MathConstants<float>::pi * 2.8f, true);
        s.setTooltip (k->descripcion);
        s.setBufferedToImage (true);
        s.addMouseListener (this, false);

        k->label.setText (juce::String (juce::CharPointer_UTF8 (def.nombre)), juce::dontSendNotification);
        k->label.setJustificationType (juce::Justification::centred);
        k->label.setFont (juce::Font (juce::FontOptions (15.f, juce::Font::bold)));

        k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.apvts, def.id, s);

        addAndMakeVisible (s);
        addAndMakeVisible (k->label);
        knobs.push_back (std::move (k));
    }

    descLabel.setJustificationType (juce::Justification::centred);
    descLabel.setFont (juce::Font (juce::FontOptions (15.f)));
    descLabel.setMinimumHorizontalScale (1.f);
    descLabel.setText ("Pasa el mouse sobre un knob para ver qué hace.", juce::dontSendNotification);
    addAndMakeVisible (descLabel);

    petals.resize (26);
    for (auto& pt : petals)
        resetPetal (pt, true);

    startTimerHz (30);
}

AdlibSakuraEditor::~AdlibSakuraEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void AdlibSakuraEditor::resetPetal (Petal& p, bool anywhere)
{
    p.x     = rng.nextFloat() * (float) kWidth;
    p.y     = anywhere ? rng.nextFloat() * (float) kHeight : -20.f - rng.nextFloat() * 60.f;
    p.size  = 5.f + rng.nextFloat() * 6.f;
    p.vx    = -0.2f + rng.nextFloat() * 0.5f;
    p.vy    = 0.6f + rng.nextFloat() * 0.9f;
    p.rot   = rng.nextFloat() * juce::MathConstants<float>::twoPi;
    p.vrot  = -0.05f + rng.nextFloat() * 0.1f;
    p.phase = rng.nextFloat() * juce::MathConstants<float>::twoPi;
    p.alpha = 0.55f + rng.nextFloat() * 0.35f;
}

void AdlibSakuraEditor::timerCallback()
{
    for (auto& p : petals)
    {
        p.phase += 0.05f;
        p.x += p.vx + std::sin (p.phase) * 0.6f;
        p.y += p.vy;
        p.rot += p.vrot;

        if (p.y > (float) kHeight + 20.f || p.x < -30.f || p.x > (float) kWidth + 30.f)
            resetPetal (p, false);
    }
    repaint();
}

//==============================================================================
void AdlibSakuraEditor::drawBranch (juce::Graphics& g, int w, int h)
{
    juce::Random r (42);
    using P = juce::Point<float>;

    auto bez = [] (P a, P b, P c, P d, float t)
    {
        const float u = 1.f - t;
        return a * (u * u * u) + b * (3.f * u * u * t) + c * (3.f * u * t * t) + d * (t * t * t);
    };

    const float W = (float) w;
    const P p0 (W + 20.f, 22.f), p1 (W * 0.86f, 6.f), p2 (W * 0.64f, 92.f), p3 (W * 0.46f, 52.f);

    // Flores grandes y tenues de fondo (esquinas inferiores)
    sakura::drawFlower (g, { 40.f, (float) h - 40.f }, 70.f, 0.4f, sakura::petaloMedio, sakura::petaloMedio, 0.18f);
    sakura::drawFlower (g, { W - 40.f, (float) h - 30.f }, 85.f, 1.1f, sakura::petaloMedio, sakura::petaloMedio, 0.18f);

    // Rama principal
    juce::Path mainBranch;
    mainBranch.startNewSubPath (p0);
    mainBranch.cubicTo (p1, p2, p3);
    g.setColour (sakura::rama);
    g.strokePath (mainBranch, juce::PathStrokeType (7.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    std::vector<P> spots;
    bool down = true;

    for (float t = 0.10f; t < 0.98f; t += 0.15f)
    {
        const P a = bez (p0, p1, p2, p3, t);
        const float dx = -(15.f + r.nextFloat() * 40.f);
        const float dy = (down ? 1.f : -0.6f) * (14.f + r.nextFloat() * 42.f);
        down = ! down;
        const P b (a.x + dx, juce::jmax (8.f, a.y + dy));
        const P ctrl ((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f - 8.f);

        juce::Path sub;
        sub.startNewSubPath (a);
        sub.quadraticTo (ctrl, b);
        g.setColour (sakura::rama);
        g.strokePath (sub, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        spots.push_back (b);
        spots.push_back (P (ctrl.x, ctrl.y - 4.f));
        spots.push_back (a);
    }
    spots.push_back (p3);

    // Flores y capullos
    const juce::Colour tonos[] = { sakura::petaloClaro, juce::Colour (0xffffd3e1), sakura::petaloMedio.brighter (0.2f) };
    for (auto& s : spots)
    {
        const float size = 9.f + r.nextFloat() * 9.f;
        sakura::drawFlower (g, s, size, r.nextFloat() * juce::MathConstants<float>::twoPi,
                            tonos[r.nextInt (3)], sakura::petaloMedio);
    }

    for (int i = 0; i < 6; ++i)
    {
        const P a = bez (p0, p1, p2, p3, 0.05f + r.nextFloat() * 0.9f);
        g.setColour (sakura::rosaFuerte.withAlpha (0.75f));
        g.fillEllipse (a.x - 3.f, a.y + 5.f + r.nextFloat() * 8.f, 6.f, 6.f);
    }
}

//==============================================================================
void AdlibSakuraEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (sakura::fondoArriba, 0.f, 0.f,
                                             sakura::fondoAbajo, 0.f, (float) getHeight(), false));
    g.fillAll();

    g.drawImageAt (branchImage, 0, 0);

    // Título
    const auto kanji = juce::String (juce::CharPointer_UTF8 ("\xe6\xa1\x9c"));   // 桜
    g.setColour (sakura::texto);
    g.setFont (juce::Font (juce::FontOptions (34.f, juce::Font::bold)));
    g.drawText ("Adlib Sakura  " + kanji, 30, 16, 400, 44, juce::Justification::centredLeft);

    g.setColour (sakura::rosaFuerte);
    g.setFont (juce::Font (juce::FontOptions (14.f)));
    g.drawText ("efectos para adlibs", 32, 56, 300, 20, juce::Justification::centredLeft);

    // Panel de descripción
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.fillRoundedRectangle (descPanel, 12.f);
    g.setColour (sakura::petaloMedio);
    g.drawRoundedRectangle (descPanel, 12.f, 1.5f);
}

void AdlibSakuraEditor::paintOverChildren (juce::Graphics& g)
{
    for (const auto& p : petals)
    {
        g.setColour (sakura::petaloMedio.withAlpha (p.alpha));
        g.fillPath (petalPath, juce::AffineTransform::scale (p.size, p.size * 1.2f)
                                   .rotated (p.rot)
                                   .translated (p.x, p.y));
    }
}

void AdlibSakuraEditor::resized()
{
    branchImage = juce::Image (juce::Image::ARGB, getWidth(), getHeight(), true);
    {
        juce::Graphics gi (branchImage);
        drawBranch (gi, getWidth(), getHeight());
    }

    auto area = getLocalBounds().reduced (30);
    area.removeFromTop (90);

    auto descArea = area.removeFromBottom (64);
    descPanel = descArea.reduced (0, 6).toFloat();
    descLabel.setBounds (descArea.reduced (14, 8));

    const int cols = 6, rows = 2;
    const int cellW = area.getWidth() / cols;
    const int cellH = area.getHeight() / rows;

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int r = (int) i / cols, c = (int) i % cols;
        auto cell = juce::Rectangle<int> (area.getX() + c * cellW, area.getY() + r * cellH, cellW, cellH).reduced (4);
        knobs[i]->label.setBounds (cell.removeFromTop (22));
        knobs[i]->slider.setBounds (cell);
    }
}

//==============================================================================
void AdlibSakuraEditor::mouseEnter (const juce::MouseEvent& e)
{
    for (auto& k : knobs)
        if (e.eventComponent == &k->slider)
            descLabel.setText (k->label.getText() + ": " + k->descripcion, juce::dontSendNotification);
}

void AdlibSakuraEditor::mouseExit (const juce::MouseEvent&)
{
    descLabel.setText ("Pasa el mouse sobre un knob para ver qué hace.", juce::dontSendNotification);
}
