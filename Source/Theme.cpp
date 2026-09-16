#include "Theme.h"

namespace PCTheme
{
    namespace
    {
        juce::String pickTypeface (const juce::StringArray& preferred, bool wantMono)
        {
            const auto available = juce::Font::findAllTypefaceNames();
            for (const auto& name : preferred)
                if (available.contains (name)) return name;

            if (wantMono)
            {
                const juce::StringArray monoFallbacks { "Consolas", "Menlo", "DejaVu Sans Mono",
                                                        "Courier New", "monospace" };
                for (const auto& name : monoFallbacks)
                    if (available.contains (name)) return name;
                return juce::Font::getDefaultMonospacedFontName();
            }

            const juce::StringArray sansFallbacks { "Segoe UI Variable Text", "Segoe UI", "Helvetica Neue",
                                                    "Helvetica", "Arial", "DejaVu Sans" };
            for (const auto& name : sansFallbacks)
                if (available.contains (name)) return name;
            return juce::Font::getDefaultSansSerifFontName();
        }

        void buildTracked (juce::GlyphArrangement& ga, const juce::String& text,
                           const juce::Font& f, float extraPerGlyph)
        {
            ga.addLineOfText (f, text, 0.0f, 0.0f);
            const int n = ga.getNumGlyphs();
            for (int i = 1; i < n; ++i)
                ga.moveRangeOfGlyphs (i, 1, extraPerGlyph * (float) i, 0.0f);
        }
    }

    const juce::String& uiTypeface()
    {
        static const juce::String name = pickTypeface ({ "Inter", "Inter Display", "Space Grotesk" }, false);
        return name;
    }

    const juce::String& monoTypeface()
    {
        static const juce::String name = pickTypeface ({ "JetBrains Mono", "IBM Plex Mono" }, true);
        return name;
    }

    juce::Font labelFont (float height)
    {
        return juce::Font (juce::FontOptions (uiTypeface(), height, juce::Font::plain));
    }

    juce::Font headerFont (float height)
    {
        return juce::Font (juce::FontOptions (uiTypeface(), height, juce::Font::bold));
    }

    juce::Font valueFont (float height)
    {
        return juce::Font (juce::FontOptions (uiTypeface(), height, juce::Font::plain));
    }

    juce::Font monoFont (float height)
    {
        return juce::Font (juce::FontOptions (monoTypeface(), height, juce::Font::plain));
    }

    juce::Font titleFont (float height)
    {
        return juce::Font (juce::FontOptions (uiTypeface(), height, juce::Font::bold));
    }

    float trackedWidth (const juce::String& text, const juce::Font& f, float trackingEm)
    {
        if (text.isEmpty()) return 0.0f;
        juce::GlyphArrangement ga;
        buildTracked (ga, text, f, trackingEm * f.getHeight());
        return ga.getBoundingBox (0, ga.getNumGlyphs(), true).getWidth();
    }

    void drawTracked (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                      juce::Justification just, const juce::Font& f, float trackingEm)
    {
        if (text.isEmpty()) return;

        juce::GlyphArrangement ga;
        buildTracked (ga, text, f, trackingEm * f.getHeight());
        const auto bb = ga.getBoundingBox (0, ga.getNumGlyphs(), true);

        float x = area.getX();
        const int hFlags = just.getOnlyHorizontalFlags();
        if ((hFlags & juce::Justification::horizontallyCentred) != 0) x = area.getCentreX() - bb.getWidth() * 0.5f;
        else if ((hFlags & juce::Justification::right) != 0)          x = area.getRight() - bb.getWidth();

        float baseline = area.getCentreY() + (f.getAscent() - f.getDescent()) * 0.5f;
        const int vFlags = just.getOnlyVerticalFlags();
        if ((vFlags & juce::Justification::top) != 0)          baseline = area.getY() + f.getAscent();
        else if ((vFlags & juce::Justification::bottom) != 0)  baseline = area.getBottom() - f.getDescent();

        ga.draw (g, juce::AffineTransform::translation (x - bb.getX(), baseline));
    }

    void drawSectionHeader (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& name,
                            juce::Colour accentColour)
    {
        auto r = area.toFloat();
        // 2px wide, 12px tall accent bar to the left of the header text
        const float barH = 12.0f;
        g.setColour (accentColour);
        g.fillRect (juce::Rectangle<float> (r.getX(), r.getCentreY() - barH * 0.5f, 2.0f, barH));

        g.setColour (textPrimary);
        drawTracked (g, name.toUpperCase(), r.withTrimmedLeft (8.0f),
                     juce::Justification::centredLeft, headerFont (Metrics::labelSize));
    }

    void drawSeparator (juce::Graphics& g, int x1, int x2, int y)
    {
        g.setColour (edge);
        g.drawHorizontalLine (y, (float) x1, (float) x2);
    }

    void drawSignatureNotch (juce::Graphics& g, juce::Rectangle<int> window, juce::Colour accentColour)
    {
        // 12px long, 45deg, 2px thick, tucked into the top-left corner.
        const float inset = 4.0f;
        const float len = 12.0f / juce::MathConstants<float>::sqrt2;
        const juce::Point<float> a ((float) window.getX() + inset, (float) window.getY() + inset + len);
        const juce::Point<float> b ((float) window.getX() + inset + len, (float) window.getY() + inset);
        g.setColour (accentColour);
        g.drawLine (a.x, a.y, b.x, b.y, 2.0f);
    }

    void drawVersionStamp (juce::Graphics& g, juce::Rectangle<int> window, const juce::String& version)
    {
        g.setColour (textMuted.withAlpha (0.7f));
        g.setFont (monoFont (9.0f));
        g.drawText (version, window.removeFromBottom (14).withTrimmedRight (10),
                    juce::Justification::centredRight, false);
    }

    void dropShadow (juce::Graphics& g, juce::Rectangle<float> bounds, float radius)
    {
        // 8px blur, y+2 offset, approximated with concentric fading strokes.
        for (int i = 8; i >= 1; --i)
        {
            const float spread = (float) i;
            g.setColour (shadow.withMultipliedAlpha (0.055f));
            g.drawRoundedRectangle (bounds.expanded (spread).translated (0.0f, 2.0f),
                                    radius + spread, 1.0f);
        }
    }

    juce::Colour meterColour (float norm)
    {
        norm = juce::jlimit (0.0f, 1.0f, norm);
        if (norm < 0.45f) return accent2.interpolatedWith (accent, norm / 0.45f);
        if (norm < 0.80f) return accent.interpolatedWith (warning, (norm - 0.45f) / 0.35f);
        return warning.interpolatedWith (clipRed, (norm - 0.80f) / 0.20f);
    }

    float easeTowards (float current, float target, int timerHz)
    {
        if (timerHz <= 0) return target;
        // exp decay tuned so ~95% of the distance is covered in Metrics::animMs
        const float dt = 1000.0f / (float) timerHz;
        const float k = 1.0f - std::exp (-3.0f * dt / (float) Metrics::animMs);
        const float next = current + (target - current) * juce::jlimit (0.0f, 1.0f, k);
        return std::abs (target - next) < 1.0e-4f ? target : next;
    }
}
