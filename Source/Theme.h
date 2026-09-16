#pragma once
#include <JuceHeader.h>

//==============================================================================
//  VISUAL IDENTITY SPEC  (theme.md)
//  Shared across every plugin in the range. Only ONE accent may be re-tinted
//  per plugin; neutrals, layout and control shapes stay identical everywhere.
//==============================================================================

namespace PCTheme
{
    // ---- Core palette -------------------------------------------------------
    const juce::Colour bgBase      { 0xff0E1116 }; // background base
    const juce::Colour panel       { 0xff171B22 }; // panel surface
    const juce::Colour panelRaise  { 0xff1E232B }; // raised control fill (derived)
    const juce::Colour edge        { 0xff2A303A }; // panel edge / bevel
    const juce::Colour accent      { 0xffE8532A }; // primary accent - burnt orange
    const juce::Colour accent2     { 0xff4FB6C4 }; // secondary accent - muted teal
    const juce::Colour textPrimary { 0xffE6E8EC };
    const juce::Colour textMuted   { 0xff8A929E };
    const juce::Colour signalOn    { 0xff7BC96F }; // success / signal-on
    const juce::Colour warning     { 0xffF2C14E }; // warning / near clip
    const juce::Colour clipRed     { 0xffE2483A }; // clip
    const juce::Colour knobTop     { 0xff232833 };
    const juce::Colour knobBottom  { 0xff14181F };
    const juce::Colour shadow      { 0x8c000000 }; // rgba(0,0,0,0.55)

    // ---- Metrics (8px base grid) -------------------------------------------
    namespace Metrics
    {
        constexpr int grid          = 8;
        constexpr int windowPad     = 16;
        constexpr int headerHeight  = 32;
        constexpr int buttonHeight  = 28;
        constexpr int knobSmall     = 36;
        constexpr int knobDefault   = 48;
        constexpr int knobLarge     = 64;
        constexpr float radiusWindow = 6.0f;
        constexpr float radiusPanel  = 4.0f;
        constexpr float radiusSmall  = 2.0f;
        constexpr float arcGap       = 4.0f;
        constexpr float arcThickness = 3.0f;
        constexpr float labelSize    = 11.0f;
        constexpr float valueSize    = 13.0f;
        constexpr float tracking     = 0.08f; // +0.08em on uppercase labels
        constexpr int   animMs       = 80;    // value changes ease-out over 80ms
        constexpr int   tooltipMs    = 400;   // tooltips appear after 400ms
    }

    // ---- Typography ---------------------------------------------------------
    // "Inter" (fallback "Space Grotesk", then the system sans) for UI text,
    // "JetBrains Mono" (fallback "IBM Plex Mono", then any mono) for readouts.
    const juce::String& uiTypeface();
    const juce::String& monoTypeface();

    juce::Font labelFont      (float height = Metrics::labelSize); // weight 500
    juce::Font headerFont     (float height = Metrics::labelSize); // weight 600
    juce::Font valueFont      (float height = Metrics::valueSize);
    juce::Font monoFont       (float height = 11.0f);
    juce::Font titleFont      (float height = 14.0f);

    // ---- Text helpers -------------------------------------------------------
    /** Draws text with per-glyph letter-spacing (tracking), expressed in em. */
    void  drawTracked (juce::Graphics&, const juce::String& text, juce::Rectangle<float> area,
                       juce::Justification just, const juce::Font&, float trackingEm = Metrics::tracking);
    float trackedWidth (const juce::String& text, const juce::Font&, float trackingEm = Metrics::tracking);

    // ---- Shared chrome ------------------------------------------------------
    /** Uppercase section header with the 2x12px accent bar to its left. */
    void drawSectionHeader (juce::Graphics&, juce::Rectangle<int> area, const juce::String& name,
                            juce::Colour accentColour = accent);
    /** 1px horizontal separator - sections are never boxes inside boxes. */
    void drawSeparator (juce::Graphics&, int x1, int x2, int y);
    /** Signature 2px, 12px, 45deg accent notch in the window's top-left corner. */
    void drawSignatureNotch (juce::Graphics&, juce::Rectangle<int> window, juce::Colour accentColour = accent);
    /** Version stamp, 9px muted mono, bottom-right. */
    void drawVersionStamp (juce::Graphics&, juce::Rectangle<int> window, const juce::String& version);
    /** Soft drop shadow: 8px blur, y+2. */
    void dropShadow (juce::Graphics&, juce::Rectangle<float> bounds, float radius);

    /** Meter gradient sample: teal (low) -> orange (nominal) -> yellow -> red (clip). */
    juce::Colour meterColour (float norm);

    /** Eases a displayed value towards its target over Metrics::animMs. */
    float easeTowards (float current, float target, int timerHz);
}
