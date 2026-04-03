#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Custom LookAndFeel inspired by OB-8 / Moog / Prophet-5 aesthetic
class SynthLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SynthLookAndFeel()
    {
        // Dark theme base
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffcccccc));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff333333));
        setColour(juce::Slider::thumbColourId, juce::Colours::white);

        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
        setColour(juce::ComboBox::textColourId, juce::Colours::white);
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff555555));
        setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffaaaaaa));

        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff2a2a2a));
        setColour(juce::PopupMenu::textColourId, juce::Colours::white);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff555555));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // --- Outer ring (metallic rim) ---
        {
            auto outerRadius = radius;
            juce::ColourGradient rimGradient(
                juce::Colour(0xff888888), centreX, centreY - outerRadius,
                juce::Colour(0xff3a3a3a), centreX, centreY + outerRadius, false);
            g.setGradientFill(rimGradient);
            g.fillEllipse(centreX - outerRadius, centreY - outerRadius,
                          outerRadius * 2.0f, outerRadius * 2.0f);
        }

        // --- Inner knob body ---
        {
            auto innerRadius = radius * 0.78f;
            juce::ColourGradient knobGradient(
                juce::Colour(0xff4a4a4a), centreX, centreY - innerRadius,
                juce::Colour(0xff1a1a1a), centreX, centreY + innerRadius, false);
            g.setGradientFill(knobGradient);
            g.fillEllipse(centreX - innerRadius, centreY - innerRadius,
                          innerRadius * 2.0f, innerRadius * 2.0f);
        }

        // --- Grip notch (subtle ring) ---
        {
            auto gripRadius = radius * 0.65f;
            g.setColour(juce::Colour(0xff2a2a2a));
            g.drawEllipse(centreX - gripRadius, centreY - gripRadius,
                          gripRadius * 2.0f, gripRadius * 2.0f, 1.0f);
        }

        // --- Pointer line ---
        {
            juce::Path pointer;
            auto pointerLength = radius * 0.75f;
            auto pointerThickness = 2.5f;

            // Use the fill colour for MIDI learn highlighting
            auto fillColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
            bool isHighlighted = (fillColour != juce::Colour(0xffcccccc)
                                  && fillColour != juce::Colour(0xff4a9eff));

            if (isHighlighted)
                g.setColour(fillColour);
            else
                g.setColour(juce::Colour(0xffe8e8e8));

            pointer.addRoundedRectangle(-pointerThickness * 0.5f,
                                         -pointerLength,
                                         pointerThickness,
                                         pointerLength * 0.6f,
                                         1.0f);

            g.fillPath(pointer, juce::AffineTransform::rotation(angle)
                                    .translated(centreX, centreY));
        }

        // --- Position dot at tip ---
        {
            auto dotRadius = 2.5f;
            auto dotDist = radius * 0.55f;
            auto dotX = centreX + std::sin(angle) * dotDist;
            auto dotY = centreY - std::cos(angle) * dotDist;

            auto fillColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
            bool isHighlighted = (fillColour != juce::Colour(0xffcccccc)
                                  && fillColour != juce::Colour(0xff4a9eff));

            g.setColour(isHighlighted ? fillColour : juce::Colour(0xffffffff));
            g.fillEllipse(dotX - dotRadius, dotY - dotRadius,
                          dotRadius * 2.0f, dotRadius * 2.0f);
        }
    }

    // --- Custom combo box ---
    void drawComboBox(juce::Graphics& g, int width, int height, bool,
                      int, int, int, int, juce::ComboBox&) override
    {
        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(bounds, 3.0f);
        g.setColour(juce::Colour(0xff555555));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f);

        // Arrow
        auto arrowZone = juce::Rectangle<int>(width - 20, 0, 16, height).toFloat();
        juce::Path arrow;
        arrow.addTriangle(arrowZone.getCentreX() - 4.0f, arrowZone.getCentreY() - 2.0f,
                          arrowZone.getCentreX() + 4.0f, arrowZone.getCentreY() - 2.0f,
                          arrowZone.getCentreX(), arrowZone.getCentreY() + 3.0f);
        g.setColour(juce::Colour(0xffaaaaaa));
        g.fillPath(arrow);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        auto baseColour = backgroundColour;

        if (shouldDrawButtonAsDown)
            baseColour = baseColour.brighter(0.1f);
        else if (shouldDrawButtonAsHighlighted)
            baseColour = baseColour.brighter(0.05f);

        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(baseColour.brighter(0.2f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

    // --- Draw wood panel ---
    static void drawWoodPanel(juce::Graphics& g, juce::Rectangle<int> area, bool isLeft)
    {
        // Base wood colour gradient
        juce::Colour darkWood(0xff3d2010);
        juce::Colour midWood(0xff5c3318);
        juce::Colour lightWood(0xff7a4422);

        juce::ColourGradient woodGradient(
            isLeft ? midWood : lightWood,
            static_cast<float>(area.getX()), static_cast<float>(area.getY()),
            isLeft ? lightWood : midWood,
            static_cast<float>(area.getRight()), static_cast<float>(area.getBottom()),
            false);
        woodGradient.addColour(0.3, darkWood);
        woodGradient.addColour(0.7, midWood);

        g.setGradientFill(woodGradient);
        g.fillRect(area);

        // Wood grain lines
        g.setColour(darkWood.withAlpha(0.3f));
        auto rng = juce::Random(42); // deterministic grain
        for (int i = 0; i < 20; ++i)
        {
            float yPos = static_cast<float>(area.getY()) + rng.nextFloat() * static_cast<float>(area.getHeight());
            float thickness = 0.5f + rng.nextFloat() * 1.5f;
            g.drawLine(static_cast<float>(area.getX()), yPos,
                       static_cast<float>(area.getRight()), yPos, thickness);
        }

        // Edge shadow
        if (isLeft)
        {
            juce::ColourGradient shadow(
                juce::Colours::black.withAlpha(0.4f),
                static_cast<float>(area.getRight() - 3), 0.0f,
                juce::Colours::transparentBlack,
                static_cast<float>(area.getRight() - 12), 0.0f, false);
            g.setGradientFill(shadow);
            g.fillRect(area.removeFromRight(12));
        }
        else
        {
            juce::ColourGradient shadow(
                juce::Colours::black.withAlpha(0.4f),
                static_cast<float>(area.getX() + 3), 0.0f,
                juce::Colours::transparentBlack,
                static_cast<float>(area.getX() + 12), 0.0f, false);
            g.setGradientFill(shadow);
            g.fillRect(area.removeFromLeft(12));
        }
    }

    // --- Draw brushed metal faceplate ---
    static void drawFaceplate(juce::Graphics& g, juce::Rectangle<int> area)
    {
        // Base dark charcoal
        juce::ColourGradient faceGradient(
            juce::Colour(0xff2d2d32), static_cast<float>(area.getX()),
            static_cast<float>(area.getY()),
            juce::Colour(0xff1e1e22), static_cast<float>(area.getX()),
            static_cast<float>(area.getBottom()), false);
        g.setGradientFill(faceGradient);
        g.fillRect(area);

        // Brushed metal texture (horizontal lines)
        auto rng = juce::Random(123);
        for (int i = 0; i < 80; ++i)
        {
            float yPos = static_cast<float>(area.getY()) + rng.nextFloat() * static_cast<float>(area.getHeight());
            float alpha = 0.02f + rng.nextFloat() * 0.04f;
            g.setColour(juce::Colours::white.withAlpha(alpha));
            g.drawLine(static_cast<float>(area.getX()), yPos,
                       static_cast<float>(area.getRight()), yPos, 0.5f);
        }
    }

    // --- Draw section header strip ---
    static void drawSectionHeader(juce::Graphics& g, juce::Rectangle<int> area,
                                   const juce::String& title)
    {
        // Header strip background
        g.setColour(juce::Colour(0xff1a1a1e));
        g.fillRect(area);

        // Bottom accent line (amber)
        g.setColour(juce::Colour(0xffcc7700));
        g.fillRect(area.getX(), area.getBottom() - 2, area.getWidth(), 2);

        // Title text
        g.setColour(juce::Colour(0xffdddddd));
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(title, area.withTrimmedLeft(4).withTrimmedBottom(2),
                   juce::Justification::centredLeft);
    }
};
