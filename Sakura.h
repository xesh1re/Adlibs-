#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
// Paleta rosa pálido + utilidades para dibujar flores de cerezo (sakura)
//==============================================================================
namespace sakura
{
    inline const juce::Colour fondoArriba { 0xfffff5f8 };
    inline const juce::Colour fondoAbajo  { 0xfffbdbe5 };
    inline const juce::Colour petaloClaro { 0xffffe6ee };
    inline const juce::Colour petaloMedio { 0xfff6b3c8 };
    inline const juce::Colour rosaFuerte  { 0xffe0698f };
    inline const juce::Colour texto       { 0xff7a3b50 };
    inline const juce::Colour rama        { 0xff8a5a5f };

    // Pétalo de cerezo apuntando hacia arriba, con la muesquita típica en la punta
    inline juce::Path makePetal (float r)
    {
        juce::Path p;
        p.startNewSubPath (0.f, 0.f);
        p.cubicTo (-0.65f * r, -0.25f * r, -0.55f * r, -1.0f * r, -0.14f * r, -1.0f * r);
        p.lineTo (0.f, -0.84f * r);
        p.lineTo (0.14f * r, -1.0f * r);
        p.cubicTo (0.55f * r, -1.0f * r, 0.65f * r, -0.25f * r, 0.f, 0.f);
        p.closeSubPath();
        return p;
    }

    // Flor de 5 pétalos
    inline void drawFlower (juce::Graphics& g, juce::Point<float> c, float r, float rot,
                            juce::Colour fill, juce::Colour edge, float alpha = 1.f)
    {
        const auto petal = makePetal (r);

        for (int k = 0; k < 5; ++k)
        {
            const auto t = juce::AffineTransform::rotation (rot + (float) k * juce::MathConstants<float>::twoPi / 5.f)
                               .translated (c);
            g.setColour (fill.withMultipliedAlpha (alpha));
            g.fillPath (petal, t);
            g.setColour (edge.withMultipliedAlpha (alpha));
            g.strokePath (petal, juce::PathStrokeType (0.9f), t);
        }

        g.setColour (rosaFuerte.withMultipliedAlpha (0.85f * alpha));
        g.fillEllipse (c.x - r * 0.11f, c.y - r * 0.11f, r * 0.22f, r * 0.22f);
    }

    //==========================================================================
    // Los knobs son flores de cerezo que giran al moverlos
    //==========================================================================
    class SakuraLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        SakuraLookAndFeel()
        {
            setColour (juce::Slider::textBoxTextColourId, texto);
            setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
            setColour (juce::Label::textColourId, texto);
            setColour (juce::TooltipWindow::backgroundColourId, petaloClaro);
            setColour (juce::TooltipWindow::textColourId, texto);
            setColour (juce::TooltipWindow::outlineColourId, petaloMedio);
        }

        void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                               float startAngle, float endAngle, juce::Slider&) override
        {
            auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
            const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.f;
            const auto c = bounds.getCentre();
            const float angle = startAngle + pos * (endAngle - startAngle);

            // Riel
            juce::Path track;
            track.addCentredArc (c.x, c.y, radius, radius, 0.f, startAngle, endAngle, true);
            g.setColour (petaloMedio.withAlpha (0.35f));
            g.strokePath (track, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Valor
            juce::Path val;
            val.addCentredArc (c.x, c.y, radius, radius, 0.f, startAngle, angle, true);
            g.setColour (rosaFuerte);
            g.strokePath (val, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Sombra suave + flor
            g.setColour (juce::Colour (0x22a0506e));
            g.fillEllipse (c.x - radius * 0.62f, c.y - radius * 0.55f, radius * 1.24f, radius * 1.24f);
            drawFlower (g, c, radius * 0.84f, angle, petaloClaro, petaloMedio);

            // Indicador
            juce::Path ptr;
            ptr.addRoundedRectangle (-1.6f, -radius * 0.72f, 3.2f, radius * 0.34f, 1.6f);
            g.setColour (rosaFuerte);
            g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (c));
        }
    };
}
