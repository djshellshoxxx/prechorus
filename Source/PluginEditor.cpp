// PreChorus™
// Copyright © 2026 Sheldon Davidson. All rights reserved.
// Proprietary source code. See LICENSE and COPYRIGHT-TRADEMARK.md.
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "PluginEditor.h"
#include "DemoSource.h"

namespace PCColours
{
    juce::Colour swellColour (float toneHz, float bassCutHz)
    {
        const float toneNorm = juce::jlimit (0.0f, 1.0f, (std::log10 (juce::jmax (200.0f, toneHz)) - 2.3f) / 2.0f);
        const float bassNorm = juce::jlimit (0.0f, 1.0f, (std::log10 (juce::jmax (20.0f, bassCutHz)) - 1.3f) / 1.7f);

        juce::Colour c1 = accent; // Choral violet
        juce::Colour c2 = neon;   // Cyber cyan
        juce::Colour c3 = hitCol; // Climax amber

        auto blend1 = c1.interpolatedWith (c2, toneNorm);
        return blend1.interpolatedWith (c3, bassNorm * 0.4f);
    }
}

// ---------------- LookAndFeel ----------------

PCLookAndFeel::PCLookAndFeel() { refreshColours(); }

void PCLookAndFeel::refreshColours()
{
    setColour (juce::TextEditor::backgroundColourId, PCColours::panel);
    setColour (juce::TextEditor::textColourId, PCColours::text);
    setColour (juce::TextEditor::outlineColourId, PCColours::outline);
    setColour (juce::TextEditor::focusedOutlineColourId, PCColours::neon);
    setColour (juce::ListBox::backgroundColourId, PCColours::panel);
    setColour (juce::AlertWindow::backgroundColourId, PCColours::panel2);
    setColour (juce::AlertWindow::textColourId, PCColours::text);
    setColour (juce::Slider::textBoxTextColourId, PCColours::textDim);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, PCColours::accent.withAlpha (0.4f));
    setColour (juce::Label::textColourId, PCColours::text);
    setColour (juce::TextButton::textColourOffId, PCColours::text);
    setColour (juce::TextButton::textColourOnId, PCColours::text);
    setColour (juce::ComboBox::backgroundColourId, PCColours::panel);
    setColour (juce::ComboBox::textColourId, PCColours::text);
    setColour (juce::ComboBox::outlineColourId, PCColours::outline);
    setColour (juce::PopupMenu::backgroundColourId, PCColours::panel2);
    setColour (juce::PopupMenu::textColourId, PCColours::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, PCColours::accent.withAlpha (0.4f));
}

void PCLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                      float pos, float startAngle, float endAngle, juce::Slider& s)
{
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (2.5f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto center = bounds.getCentre();
    const float arcRadius = radius - 4.0f;
    const float currentAngle = startAngle + pos * (endAngle - startAngle);

    juce::Path bgArc;
    bgArc.addCentredArc (center.x, center.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (PCColours::outline);
    g.strokePath (bgArc, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (pos > 0.001f)
    {
        juce::Path valArc;
        valArc.addCentredArc (center.x, center.y, arcRadius, arcRadius, 0.0f, startAngle, currentAngle, true);
        g.setColour (s.findColour (juce::Slider::rotarySliderFillColourId));
        g.strokePath (valArc, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float dialRadius = arcRadius - 4.5f;
    g.setColour (PCColours::panel2);
    g.fillEllipse (center.x - dialRadius, center.y - dialRadius, dialRadius * 2.0f, dialRadius * 2.0f);
    g.setColour (PCColours::outline.brighter (0.08f));
    g.drawEllipse (center.x - dialRadius, center.y - dialRadius, dialRadius * 2.0f, dialRadius * 2.0f, 1.0f);

    if (s.hasKeyboardFocus (false))
    {
        g.setColour (PCColours::neon);
        g.drawEllipse (bounds.reduced (0.5f), 1.6f);
    }

    juce::Path p;
    const float pLen = dialRadius * 0.65f;
    p.startNewSubPath (center.x + std::sin (currentAngle) * (dialRadius * 0.15f),
                       center.y - std::cos (currentAngle) * (dialRadius * 0.15f));
    p.lineTo (center.x + std::sin (currentAngle) * pLen,
              center.y - std::cos (currentAngle) * pLen);
    g.setColour (PCColours::text);
    g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void PCLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool isOver, bool isDown)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    juce::Colour fill = b.findColour (juce::TextButton::buttonColourId, true);
    if (! b.isColourSpecified (juce::TextButton::buttonColourId))
        fill = PCColours::panel2;

    if (isDown)       fill = fill.brighter (0.2f);
    else if (isOver)  fill = fill.brighter (0.08f);

    g.setColour (fill);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (b.getToggleState() ? PCColours::accent : PCColours::outline);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

void PCLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool isOver, bool)
{
    auto r = b.getLocalBounds().toFloat();
    const float boxSize = 14.0f;
    auto box = juce::Rectangle<float> (r.getX() + 2.0f, r.getCentreY() - boxSize * 0.5f, boxSize, boxSize);

    g.setColour (PCColours::panel2);
    g.fillRoundedRectangle (box, 3.0f);
    g.setColour (isOver ? PCColours::accent : PCColours::outline);
    g.drawRoundedRectangle (box, 3.0f, 1.0f);

    if (b.getToggleState())
    {
        g.setColour (PCColours::accent);
        g.fillRoundedRectangle (box.reduced (2.5f), 2.0f);
    }

    g.setColour (b.getToggleState() ? PCColours::text : PCColours::textDim);
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (boxSize + 6.0f), juce::Justification::centredLeft);
}

juce::Font PCLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return juce::Font (juce::FontOptions (9.5f, juce::Font::bold));
}

juce::Label* PCLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (juce::Font (juce::FontOptions (9.5f)));
    l->setColour (juce::Label::textColourId, PCColours::textDim);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    return l;
}

void PCLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool isDown, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<int> (0, 0, w, h).toFloat().reduced (0.5f);
    g.setColour (isDown ? PCColours::panel2.brighter (0.1f) : PCColours::panel2);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (PCColours::outline);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    juce::Path arrow;
    const float ax = w - 12.0f;
    const float ay = h * 0.5f;
    arrow.startNewSubPath (ax - 3.0f, ay - 2.0f);
    arrow.lineTo (ax, ay + 1.5f);
    arrow.lineTo (ax + 3.0f, ay - 2.0f);
    g.setColour (PCColours::textDim);
    g.strokePath (arrow, juce::PathStrokeType (1.5f));
}

void PCLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (1, 1, box.getWidth() - 16, box.getHeight() - 2);
    label.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    label.setColour (juce::Label::textColourId, PCColours::text);
}

// ---------------- 32-Voice Constellation Visualizer ----------------

void VoiceOrbitVisualizer::timerCallback()
{
    if (! reducedMotion) phase += 0.025f;
    const float lvl = proc.getOutputLevel();
    if (! reducedMotion && lvl - smoothedOut > 0.12f) flashRing = 1.0f;
    smoothedOut += (lvl - smoothedOut) * 0.35f;
    flashRing = reducedMotion ? 0.0f : flashRing * 0.88f;
    repaint();
}

void VoiceOrbitVisualizer::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (3.0f);
    g.setColour (PCColours::panel);
    g.fillRoundedRectangle (b, 8.0f);
    g.setColour (PCColours::outline);
    g.drawRoundedRectangle (b, 8.0f, 1.0f);

    const auto center = b.getCentre();
    const float maxRadius = juce::jmin (b.getWidth(), b.getHeight()) * 0.42f;

    const int voices = juce::jlimit (1, 32, (int) proc.param (IDs::voiceCount));
    const float panSpread = proc.param (IDs::panSpread);
    const float orbitAmt = proc.param (IDs::orbit);
    const float space = proc.param (IDs::space);
    const float liveMeter = proc.getLiveInputMeter();
    const float outLvl = juce::jlimit (0.0f, 1.0f, smoothedOut * 1.8f);
    const float pitchConverge = proc.param (IDs::pitchConverge);
    const float timeConverge = proc.param (IDs::timeConverge);
    const float turb = proc.param (IDs::turbulence);
    const bool isFrozen = proc.param (IDs::freeze) > 0.5f;
    const bool isRevConv = proc.param (IDs::revConverge) > 0.5f;

    for (int r = 1; r <= 3; ++r)
    {
        const float rad = maxRadius * (r / 3.0f);
        g.setColour (PCColours::outline.interpolatedWith (PCColours::neon, outLvl * 0.5f).withAlpha (0.35f + outLvl * 0.25f));
        g.drawEllipse (center.x - rad, center.y - rad, rad * 2.0f, rad * 2.0f, 1.0f);
    }

    if (space > 0.05f)
    {
        g.setColour (PCColours::accent.withAlpha (0.05f * space));
        g.fillEllipse (center.x - maxRadius, center.y - maxRadius, maxRadius * 2.0f, maxRadius * 2.0f);
    }

    const juce::Colour col = isFrozen ? PCColours::freezeCol : PCColours::swellColour (proc.param (IDs::tone), proc.param (IDs::basscut));

    for (int v = 0; v < voices; ++v)
    {
        const float vNorm = (voices > 1) ? (float) v / (float) (voices - 1) : 0.5f;
        const float orbitAngle = vNorm * juce::MathConstants<float>::twoPi + phase * (1.0f + orbitAmt * 1.5f);

        float distNorm = 0.35f + 0.65f * (1.0f - timeConverge * 0.5f);
        if (isRevConv) distNorm = 0.9f - distNorm * 0.5f;
        if (isFrozen) distNorm = 0.65f;

        const float radX = maxRadius * distNorm * (0.6f + 0.4f * std::abs ((vNorm * 2.0f - 1.0f) * panSpread));
        const float radY = maxRadius * distNorm * 0.75f;

        const float jitter = (turb > 0.01f) ? std::sin (phase * 6.0f + (float) v) * turb * 5.0f : 0.0f;
        const float x = center.x + std::cos (orbitAngle) * radX + jitter;
        const float y = center.y + std::sin (orbitAngle) * radY;

        g.setColour (col.withAlpha (0.15f + pitchConverge * 0.2f + outLvl * 0.3f));
        g.drawLine (x, y, center.x, center.y, 1.0f + outLvl * 1.2f);

        const float nodeSize = 3.5f + 3.0f * liveMeter + 5.0f * outLvl;
        const juce::Colour nodeCol = col.interpolatedWith (PCColours::neon, vNorm).interpolatedWith (PCColours::hitCol, outLvl * 0.6f);
        g.setColour (nodeCol);
        g.fillEllipse (x - nodeSize * 0.5f, y - nodeSize * 0.5f, nodeSize, nodeSize);
    }

    // Expanding shockwave ring on playback transient onset (audio-reactive flash)
    if (flashRing > 0.02f)
    {
        const float ringRad = maxRadius * (0.25f + flashRing * 0.65f);
        g.setColour (PCColours::hitCol.withAlpha (flashRing * 0.55f));
        g.drawEllipse (center.x - ringRad, center.y - ringRad, ringRad * 2.0f, ringRad * 2.0f, 1.5f + flashRing * 3.0f);
    }

    const float coreSize = 8.0f + 14.0f * liveMeter + 20.0f * outLvl;
    const juce::Colour coreCol = PCColours::accent.interpolatedWith (PCColours::hitCol, outLvl);
    g.setColour (coreCol.withAlpha (0.25f + outLvl * 0.25f));
    g.fillEllipse (center.x - coreSize * 0.5f, center.y - coreSize * 0.5f, coreSize, coreSize);
    g.setColour (coreCol);
    g.fillEllipse (center.x - 3.0f, center.y - 3.0f, 6.0f, 6.0f);

    g.setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
    g.setColour (isFrozen ? PCColours::freezeCol : PCColours::textDim);
    g.drawText (isFrozen ? "SWARM FROZEN" : "32-VOICE CONSTELLATION", b.withTrimmedTop (4), juce::Justification::centredTop);
}

// ---------------- Waveform Display ----------------

WaveformDisplay::WaveformDisplay (PreChorusProcessor& p) : proc (p)
{
    startTimerHz (30);
}

juce::Rectangle<float> WaveformDisplay::plot() const
{
    return getLocalBounds().toFloat().reduced (8.0f);
}

float WaveformDisplay::envSpanFraction() const
{
    // The volume envelope spans the start of the swell to the hit, then holds its end level.
    return (hitIndex > 0 && total > 0 && hitIndex < total) ? (float) hitIndex / (float) total : 1.0f;
}

float WaveformDisplay::volY (float level) const
{
    auto p = plot();
    return p.getBottom() - juce::jlimit (0.0f, 1.0f, level) * p.getHeight();
}

void WaveformDisplay::rebuild()
{
    swellPath.clear();
    hitPath.clear();
    cached = proc.getRendered();
    if (cached == nullptr || cached->audio.getNumSamples() == 0) return;

    auto p = plot();
    const int numSamples = cached->audio.getNumSamples();
    total = numSamples;
    hitIndex = cached->hitIndex;
    const float w = p.getWidth();
    const float h = p.getHeight();
    const float midY = p.getCentreY();
    const float halfH = h * 0.45f;

    const float* l = cached->audio.getReadPointer (0);
    const float* r = cached->audio.getNumChannels() > 1 ? cached->audio.getReadPointer (1) : l;
    const int split = hitIndex >= 0 ? hitIndex : numSamples;

    swellPath.startNewSubPath (p.getX(), midY);
    for (float px = 0; px < w; ++px)
    {
        const int s0 = (int) ((px / w) * numSamples);
        const int s1 = (int) (((px + 1.0f) / w) * numSamples);
        if (s0 >= split) break;

        float maxV = 0.0f;
        for (int i = s0; i < juce::jmin (s1, split); ++i)
            maxV = juce::jmax (maxV, std::abs (l[i]), std::abs (r[i]));
        swellPath.lineTo (p.getX() + px, midY - maxV * halfH);
    }
    for (float px = w - 1.0f; px >= 0.0f; --px)
    {
        const int s0 = (int) ((px / w) * numSamples);
        const int s1 = (int) (((px + 1.0f) / w) * numSamples);
        if (s0 >= split) continue;

        float maxV = 0.0f;
        for (int i = s0; i < juce::jmin (s1, split); ++i)
            maxV = juce::jmax (maxV, std::abs (l[i]), std::abs (r[i]));
        swellPath.lineTo (p.getX() + px, midY + maxV * halfH);
    }
    swellPath.closeSubPath();

    if (hitIndex >= 0 && hitIndex < numSamples)
    {
        const float hitPx = (float) hitIndex / (float) numSamples * w;
        hitPath.startNewSubPath (p.getX() + hitPx, midY);
        for (float px = hitPx; px < w; ++px)
        {
            const int s0 = (int) ((px / w) * numSamples);
            const int s1 = (int) (((px + 1.0f) / w) * numSamples);
            float maxV = 0.0f;
            for (int i = juce::jmax (s0, hitIndex); i < juce::jmin (s1, numSamples); ++i)
                maxV = juce::jmax (maxV, std::abs (l[i]), std::abs (r[i]));
            hitPath.lineTo (p.getX() + px, midY - maxV * halfH);
        }
        for (float px = w - 1.0f; px >= hitPx; --px)
        {
            const int s0 = (int) ((px / w) * numSamples);
            const int s1 = (int) (((px + 1.0f) / w) * numSamples);
            float maxV = 0.0f;
            for (int i = juce::jmax (s0, hitIndex); i < juce::jmin (s1, numSamples); ++i)
                maxV = juce::jmax (maxV, std::abs (l[i]), std::abs (r[i]));
            hitPath.lineTo (p.getX() + px, midY + maxV * halfH);
        }
        hitPath.closeSubPath();
    }
}

void WaveformDisplay::timerCallback()
{
    // The processor re-renders asynchronously on its own timer; pick up each new render here.
    if (proc.getRendered() != cached) { rebuild(); repaint(); }

    const int ph = proc.getPlayheadPosition();
    const float t = proc.param (IDs::tone);
    const float b = proc.param (IDs::basscut);
    const float v0 = proc.param (IDs::volStart);
    const float v1 = proc.param (IDs::volEnd);
    const float vt = proc.param (IDs::volTension);

    if (ph != lastPlayhead || ! juce::exactlyEqual (t, lastTone) || ! juce::exactlyEqual (b, lastBass) || ! juce::exactlyEqual (v0, lastV0)
        || ! juce::exactlyEqual (v1, lastV1) || ! juce::exactlyEqual (vt, lastVT))
    {
        lastPlayhead = ph; lastTone = t; lastBass = b;
        lastV0 = v0; lastV1 = v1; lastVT = vt;
        repaint();
    }
}

void WaveformDisplay::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (PCColours::panel);
    g.fillRoundedRectangle (b, 8.0f);
    g.setColour (PCColours::outline);
    g.drawRoundedRectangle (b, 8.0f, 1.0f);

    auto p = plot();

    if (cached != nullptr && cached->beats > 0)
    {
        g.setColour (PCColours::outline.withAlpha (0.45f));
        for (int i = 1; i < cached->beats; ++i)
        {
            const float x = p.getX() + (float) i / (float) cached->beats * p.getWidth();
            g.drawVerticalLine ((int) x, p.getY(), p.getBottom());
        }
    }

    g.setColour (PCColours::outline.withAlpha (0.3f));
    g.drawHorizontalLine ((int) p.getCentreY(), p.getX(), p.getRight());

    const juce::Colour col = PCColours::swellColour (proc.param (IDs::tone), proc.param (IDs::basscut));
    g.setColour (col.withAlpha (0.85f));
    g.fillPath (swellPath);

    if (! hitPath.isEmpty())
    {
        g.setColour (PCColours::hitCol.withAlpha (0.9f));
        g.fillPath (hitPath);
    }

    const float v0 = proc.param (IDs::volStart);
    const float v1 = proc.param (IDs::volEnd);
    const float vt = proc.param (IDs::volTension);
    juce::Path volPath;
    volPath.startNewSubPath (p.getX(), volY (v0));
    const float span = envSpanFraction();
    for (int i = 1; i <= 64; ++i)
    {
        const float norm = (float) i / 64.0f;
        const float lvl = v0 + (v1 - v0) * tensionCurve (norm, vt);
        volPath.lineTo (p.getX() + norm * span * p.getWidth(), volY (lvl));
    }
    if (span < 0.999f) volPath.lineTo (p.getRight(), volY (v1));   // the end level holds after the hit
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.strokePath (volPath, juce::PathStrokeType (1.8f));

    g.setColour (PCColours::neon);
    g.fillEllipse (p.getX() - 4.0f, volY (v0) - 4.0f, 8.0f, 8.0f);
    g.fillEllipse (p.getX() + span * p.getWidth() - 4.0f, volY (v1) - 4.0f, 8.0f, 8.0f);

    const float midLvl = v0 + (v1 - v0) * tensionCurve (0.5f, vt);
    g.setColour (PCColours::accent);
    g.fillEllipse (p.getX() + 0.5f * span * p.getWidth() - 3.5f, volY (midLvl) - 3.5f, 7.0f, 7.0f);

    const int ph = proc.getPlayheadPosition();
    if (ph >= 0 && total > 0 && ph < total)
    {
        const float phX = p.getX() + ((float) ph / (float) total) * p.getWidth();
        const float lvl = juce::jlimit (0.0f, 1.0f, proc.getOutputLevel() * 1.8f);
        g.setColour (juce::Colours::white.interpolatedWith (PCColours::hitCol, lvl));
        g.drawVerticalLine ((int) phX, p.getY(), p.getBottom());
        if (lvl > 0.05f)
        {
            g.setColour (PCColours::hitCol.withAlpha (lvl * 0.35f));
            g.fillRect (phX - 2.0f - lvl * 3.0f, p.getY(), 4.0f + lvl * 6.0f, p.getHeight());
        }
    }
}

const juce::String& WaveformDisplay::dragParam (Drag d)
{
    static const juce::String none;
    return d == Drag::volStart ? IDs::volStart : d == Drag::volEnd ? IDs::volEnd : d == Drag::volTension ? IDs::volTension : none;
}

void WaveformDisplay::mouseDown (const juce::MouseEvent& e)
{
    auto p = plot();
    downPos = e.position;

    const float v0 = proc.param (IDs::volStart);
    const float v1 = proc.param (IDs::volEnd);
    const float vt = proc.param (IDs::volTension);
    const float midLvl = v0 + (v1 - v0) * tensionCurve (0.5f, vt);

    const float span = envSpanFraction();
    if (e.position.getDistanceFrom ({ p.getX(), volY (v0) }) < 12.0f) drag = Drag::volStart;
    else if (e.position.getDistanceFrom ({ p.getX() + span * p.getWidth(), volY (v1) }) < 12.0f) drag = Drag::volEnd;
    else if (e.position.getDistanceFrom ({ p.getX() + 0.5f * span * p.getWidth(), volY (midLvl) }) < 12.0f) { drag = Drag::volTension; downA = vt; }
    else { drag = Drag::none; proc.triggerPreview(); }
    if (drag != Drag::none) proc.beginGesture (dragParam (drag));
}

void WaveformDisplay::mouseDrag (const juce::MouseEvent& e)
{
    auto p = plot();
    const float lvl = juce::jlimit (0.0f, 1.0f, (p.getBottom() - e.position.y) / p.getHeight());

    if (drag == Drag::volStart) { proc.setParam (IDs::volStart, lvl); repaint(); }
    else if (drag == Drag::volEnd) { proc.setParam (IDs::volEnd, lvl); repaint(); }
    else if (drag == Drag::volTension)
    {
        const float diff = (downPos.y - e.position.y) / 50.0f;
        proc.setParam (IDs::volTension, juce::jlimit (-1.0f, 1.0f, downA + diff));
        repaint();
    }
}

void WaveformDisplay::mouseUp (const juce::MouseEvent&)
{
    if (drag != Drag::none) proc.endGesture (dragParam (drag));
    drag = Drag::none;
}

// ---------------- Drag Out Pad ----------------

void DragOutPad::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (over ? PCColours::panel2.brighter (0.15f) : PCColours::panel2);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (over ? PCColours::neon : PCColours::outline);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    g.setColour (over ? PCColours::neon : PCColours::text);
    g.drawText ("DRAG TO DAW", getLocalBounds(), juce::Justification::centred);
    if (hasKeyboardFocus (false)) { g.setColour (PCColours::neon); g.drawRoundedRectangle (r, 4.0f, 1.6f); }
}

void DragOutPad::mouseDrag (const juce::MouseEvent&)
{
    if (dragging) return;
    dragging = true;
    auto tempFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                        .getChildFile ("PreChorus_Swarm." + PreChorusProcessor::exportExtension (proc.exportSettings));
    if (proc.exportAudio (tempFile))
    {
        juce::StringArray files;
        files.add (tempFile.getFullPathName());
        if (auto* dnc = findParentComponentOfClass<juce::DragAndDropContainer>())
            dnc->performExternalDragDropOfFiles (files, false);
    }
    dragging = false;
}

// ---------------- Pitch Tension Curve Box ----------------

void TensionBox::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);

    g.setColour (PCColours::panel2);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (PCColours::outline);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    // Center crosshair (neutral / linear reference)
    g.setColour (PCColours::outline.withAlpha (0.6f));
    g.drawLine (r.getX(), r.getBottom(), r.getRight(), r.getY(), 1.0f);

    const float t = proc.param (paramId);
    juce::Path curve;
    const int steps = 24;
    for (int i = 0; i <= steps; ++i)
    {
        const float x = (float) i / (float) steps;
        const float y = 1.0f - tensionCurve (x, t);
        const auto pt = juce::Point<float> (r.getX() + x * r.getWidth(), r.getY() + y * r.getHeight());
        if (i == 0) curve.startNewSubPath (pt); else curve.lineTo (pt);
    }
    g.setColour (PCColours::accent);
    g.strokePath (curve, juce::PathStrokeType (1.8f));

    g.setFont (juce::Font (juce::FontOptions (7.5f, juce::Font::bold)));
    g.setColour (PCColours::textDim);
    g.drawText ("BEND", getLocalBounds().removeFromBottom (10), juce::Justification::centred);
}

// ---------------- Help Overlay ----------------

HelpOverlay::HelpOverlay()
{
    setAlwaysOnTop (true);
    body.setMultiLine (true);
    body.setReadOnly (true);
    body.setCaretVisible (false);
    body.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    body.setColour (juce::TextEditor::textColourId, PCColours::text);
    body.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    body.setFont (juce::Font (juce::FontOptions (11.5f)));
    body.setText (
        "PRECHORUS v" PRECHORUS_VERSION_STRING " - 32-Voice Swarm & Convergence Engine by Circuit Drift Labs\n"
        "Formats: VST3, CLAP, Standalone (Windows x64); AU on macOS builds. No network access, telemetry or licence checks.\n\n"
        "The Signature Sound: a cloud of related voices becoming progressively more recognizable "
        "and coherent until they meet the original drop or event.\n\n"
        "QUICK WORKFLOW:\n"
        "1. LOAD (or drag a file onto the window), capture live input, or pick OPTIONS > Load demo source.\n"
        "2. Pick a preset (PRESETS) or a character.  3. Shape the swarm and convergence (FULL VIEW shows every control).\n"
        "4. Press Space/PLAY, or send MIDI notes, to audition.  5. EXPORT or drag the DRAG TO DAW pad onto a track.\n\n"
        "SOURCE MODES:\n"
        "- Live Capture: uses the active capture history slot (ARM / LIVE CAPTURE record from the plug-in input).\n"
        "- Loaded Sample: uses the loaded WAV/AIFF/FLAC/MP3/OGG file (length limit: OPTIONS > Max source length).\n"
        "- Hybrid Layer: mixes the loaded file and the active capture slot at equal level.\n"
        "- Slice Scatter: splits the loaded file at its transients (8 equal slices if it has none) and spreads them across the voices.\n"
        "Capture: 8 history slots (1-8). LOCK protects the active take; new takes go to the next free slot. Smart trim removes silence "
        "around each take (OPTIONS); RETAKE records the last take again. The meter next to LOCK shows the input level and the red capture threshold.\n\n"
        "KEYBOARD SHORTCUTS:\n"
        "- Space: preview.   - Esc: stop preview, cancel MIDI learn, or close this help.\n"
        "- Ctrl/Cmd+Z: undo.   - Ctrl/Cmd+Shift+Z or Ctrl/Cmd+Y: redo (sound-design edits only).\n"
        "- R: randomize.   - G: new swarm seed.   - B: switch A/B slot.   - H / F1: this help.\n"
        "- Tab moves between controls; with a knob focused use Up/Down (Shift = fine), PageUp/PageDown, Home/End.\n\n"
        "PRESETS: the browser has 24 factory presets with tags (Vocal, EDM, Dubstep, Pop, FX, Subtle, Extreme), search, favorites (star), "
        "sort by name or recent, INIT, user presets (right-click to rename or delete) and shareable .pcbank banks (IMPORT / EXPORT BANK). "
        "The < > buttons step through the list. Presets only change sound-design controls, never your audio or capture history. "
        "User presets live in Documents/Circuit Drift Labs/PreChorus/Presets (.pcpreset, versioned XML).\n\n"
        "RANDOMIZE: R or RANDOM is the classic randomize. The small button beside it picks a theme (Subtle, Wide, Dark, Wild, Tight, Cinematic; the same "
        "seed and theme always give the same sound) and lets you LOCK groups (Swarm, Convergence, Physics, Tone, Mix & Stutter, Character & Layers) "
        "so randomize leaves them alone.\n\n"
        "A/B COMPARE: the A/B button swaps between two sound-design snapshots; COPY copies the current one to the other slot. "
        "A/B MORPH blends between them (store A, press A>B, edit B, then move the knob; it re-renders when you stop moving).\n\n"
        "KEYTRACK: when on, MIDI notes transpose the swarm (C4 = original pitch). With Hit on note (PDC) the impact stays on the note for every pitch.\n\n"
        "TARGET SOURCE: MIDI Note (default), Next Bar Line (starts the swarm early so the impact lands on every bar line, "
        "obeying the Sequence setting), Loop End (impact on the host loop end) or Input Transient (starts when the input passes TRANSIENT). "
        "The automatic modes need the host to be playing and do not use PDC latency. DETECT chooses whether Ducking, Threshold capture and "
        "Input Transient listen to the main input or the sidechain bus (enable the sidechain in your DAW's routing).\n\n"
        "BUILD STUTTER: a rhythmic gate over the second half of the swell, accelerating 1/8 > 1/16 > 1/32 into the hit. The grid can be Straight, "
        "Triplet, Dotted or Custom (click the 16 steps; the right-most step is the one before the hit); SWING delays every second step.\n\n"
        "LAYERS: RISER (noise sweep into the hit), REV CYMBAL (reverse swell ending on the hit), SUB DROP (62 > 30 Hz on the hit) and IMPACT "
        "are synthesised into the render; LAYER TONE sets their brightness.\n\n"
        "CHARACTER MODES:\n"
        "- Clean Digital: transparent linear interpolation.  - Analog Ensemble: warm saturation and slow pitch drift.\n"
        "- Bucket-Brigade: dark clock roll-off and clock noise.  - Tape Choir: wow & flutter with tape saturation.\n"
        "- Dimension: wide cross-coupled chorus with a solid mono centre.  - String Ensemble: Solina-style dual LFO.\n"
        "- Granular Cloud: Hann-windowed micro-grains (GRAIN MS).  - Lo-Fi Choral: 4-bit crush + ~11 kHz sample-rate reduction.\n"
        "- Vowel Choir: formant filters gliding a-e-i-o-u.  - Stadium Crowd: diffuse, noisy, detuned voices.  - Glitch Scatter: scrambled, reversed and muted fragments.\n"
        "FORMANT shifts the vocal timbre of the swarm without changing its pitch.\n\n"
        "MIDI LEARN: right-click a knob > MIDI learn, then move a CC, the mod wheel, aftertouch or pitch bend. Mappings are saved with the project. "
        "Sound controls re-render the swarm, so mapped changes are applied a few times per second; not every host forwards CC to effects.\n\n"
        "EXPORT & DRAG-TO-DAW: EXPORT writes the rendered preview with SWARM WET / HIT DRY levels applied. The small button beside EXPORT sets the "
        "format (WAV 16-bit with dither, 24-bit, 32-bit float, FLAC 24-bit), sample rate, normalise and tail trimming, and exports stems (Swarm and Hit). "
        "DRAG TO DAW uses the same settings.\n\n"
        "STEMS & SIDECHAIN IN THE HOST: PreChorus offers optional extra outputs (Swarm, Hit, Dry) and a sidechain input. Enable them in your DAW's routing; "
        "the main output is always Swarm + Hit + Dry.\n\n"
        "HIT ON NOTE (PDC): reports the swell length as latency (max 20 s) and delays the dry input to match, so the target hit lands exactly on the MIDI note.\n\n"
        "SWARM VIEW: shows every voice of the current render (rows ordered by pan, colour = pitch offset) with the convergence curve and the playhead.\n\n"
        "TRIM IN / OUT crop the rendered result. The white volume curve runs from the start of the swell to the hit (and holds its end level after it); "
        "drag its points on the waveform. Double-click any knob to reset it. Options: tooltips, reduced motion, high contrast, window size.\n"
    );
    addAndMakeVisible (body);
    addAndMakeVisible (closeButton);
    siteLink.setFont (juce::Font (juce::FontOptions (11.0f)), false);
    siteLink.setColour (juce::HyperlinkButton::textColourId, PCColours::neon);
    addAndMakeVisible (siteLink);
    closeButton.onClick = [this] { setVisible (false); };
}

void HelpOverlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.75f));
    auto r = getLocalBounds().reduced (50).toFloat();
    g.setColour (PCColours::panel);
    g.fillRoundedRectangle (r, 12.0f);
    g.setColour (PCColours::accent);
    g.drawRoundedRectangle (r, 12.0f, 1.5f);
}

void HelpOverlay::resized()
{
    auto r = getLocalBounds().reduced (60);
    auto bottom = r.removeFromBottom (32);
    siteLink.setBounds (bottom.removeFromLeft (200));
    closeButton.setBounds (bottom.withSizeKeepingCentre (120, 30));
    r.removeFromBottom (10);
    body.setBounds (r);
}

// ---------------- PreChorus Editor ----------------

PreChorusEditor::PreChorusEditor (PreChorusProcessor& p)
    : AudioProcessorEditor (&p), proc (p), waveform (p), swarmView (p), stepEditor (p), levelBar (p), visualizer (p), dragPad (p),
      pitchTension (p, IDs::pitchTension)
{
    PCColours::applyHighContrast (proc.highContrast);
    lnf.refreshColours();
    setLookAndFeel (&lnf);
    applyTooltipSetting();

    title.setText ("PRECHORUS", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, PCColours::text);
    addAndMakeVisible (title);

    subtitle.setText ("SWARM & CONVERGENCE ENGINE", juce::dontSendNotification);
    subtitle.setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
    subtitle.setColour (juce::Label::textColourId, PCColours::accent);
    addAndMakeVisible (subtitle);

    fileLabel.setFont (juce::Font (juce::FontOptions (10.0f)));
    fileLabel.setColour (juce::Label::textColourId, PCColours::textDim);
    addAndMakeVisible (fileLabel);

    countLabel.setFont (juce::Font (juce::FontOptions (9.5f)));
    countLabel.setColour (juce::Label::textColourId, PCColours::textDim);
    addAndMakeVisible (countLabel);

    rangeLabel.setText ("RANGE", juce::dontSendNotification);
    rangeLabel.setFont (juce::Font (juce::FontOptions (8.0f, juce::Font::bold)));
    rangeLabel.setColour (juce::Label::textColourId, PCColours::textDim);
    rangeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (rangeLabel);

    confidenceLabel.setText ("TARGET CONFIDENCE: 100%", juce::dontSendNotification);
    confidenceLabel.setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
    confidenceLabel.setColour (juce::Label::textColourId, PCColours::neon);
    confidenceLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (confidenceLabel);

    statusLabel.setText ("Ready. Space: preview | Esc: stop | R: random | G: regen | H/F1: help", juce::dontSendNotification);
    statusLabel.setFont (juce::Font (juce::FontOptions (9.0f)));
    statusLabel.setColour (juce::Label::textColourId, PCColours::textDim);
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (statusLabel);

    outputLabel.setText ("OUT -inf dBFS", juce::dontSendNotification);
    outputLabel.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    outputLabel.setColour (juce::Label::textColourId, PCColours::textDim);
    outputLabel.setJustificationType (juce::Justification::centredRight);
    outputLabel.setTooltip ("Peak output level after PreChorus processing. CLIP appears at or above 0 dBFS.");
    addAndMakeVisible (outputLabel);

    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);
    addAndMakeVisible (loadButton);
    addAndMakeVisible (playButton);
    addAndMakeVisible (exportButton);
    addAndMakeVisible (resetButton);
    addAndMakeVisible (randomButton);
    addAndMakeVisible (regenSeedButton);
    addAndMakeVisible (optionsButton);
    addAndMakeVisible (helpButton);
    addAndMakeVisible (abButton);
    addAndMakeVisible (abCopyButton);
    addAndMakeVisible (viewButton);
    viewButton.setButtonText (proc.simpleView ? "FULL VIEW" : "SIMPLE VIEW");
    viewButton.onClick = [this] {
        proc.simpleView = ! proc.simpleView;
        viewButton.setButtonText (proc.simpleView ? "FULL VIEW" : "SIMPLE VIEW");
        resized();
        repaint();
        setStatus (proc.simpleView ? "Simple view: the main build controls. Click FULL VIEW for every control."
                                   : "Full view: every control.");
    };
    abButton.onClick = [this] {
        proc.switchABSlot();
        setStatus (juce::String ("A/B: now editing ") + (proc.isSlotBActive() ? "B." : "A."));
    };
    abCopyButton.onClick = [this] {
        proc.copyCurrentToOtherAB();
        setStatus (juce::String ("Copied ") + (proc.isSlotBActive() ? "B to A." : "A to B."));
    };

    prevButton.onClick = [this] {
        proc.prevSample();
        waveform.rebuild();
        const auto file = proc.getCurrentFile();
        setStatus (file.existsAsFile() ? "Source: " + file.getFileName() : "No previous source is available.");
    };
    nextButton.onClick = [this] {
        proc.nextSample();
        waveform.rebuild();
        const auto file = proc.getCurrentFile();
        setStatus (file.existsAsFile() ? "Source: " + file.getFileName() : "No next source is available.");
    };
    loadButton.onClick = [this] {
        chooser = std::make_unique<juce::FileChooser> ("Load Vocal or Sample Stem", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc) {
                auto f = fc.getResult();
                if (f.existsAsFile())
                {
                    const bool ok = proc.loadSampleFile (f, true);
                    waveform.rebuild();
                    const bool cropped = ok && proc.getLastLoadFileSeconds() > proc.getLastLoadUsedSeconds();
                    setStatus (! ok ? "Could not load selected audio file."
                                    : cropped ? "Loaded: " + f.getFileName() + " (cropped to " + juce::String (proc.getLastLoadUsedSeconds())
                                                  + " s of " + juce::String (proc.getLastLoadFileSeconds()) + " s - raise Max source length in OPTIONS)"
                                              : "Loaded: " + f.getFileName());
                }
            });
    };
    playButton.onClick      = [this] { proc.triggerPreview(); setStatus ("Preview triggered."); };
    resetButton.onClick     = [this] { proc.resetEdits(); waveform.rebuild(); setStatus ("Sound-design edits reset."); };
    randomButton.onClick    = [this] { proc.randomizePreChorus(); waveform.rebuild(); setStatus ("Sound-design parameters randomized."); };
    regenSeedButton.onClick = [this] { proc.regenerateSeed(); setStatus ("Swarm seed regenerated: " + juce::String ((int) proc.param (IDs::seed)) + "."); };
    optionsButton.onClick   = [this] { showOptionsMenu(); };
    helpButton.onClick      = [this] { openHelp(); };

    exportButton.onClick = [this] { runExport(); };
    exportOptionsButton.onClick = [this] { showExportMenu(); };
    themeButton.onClick = [this] { showThemeMenu(); };
    retakeButton.onClick = [this] {
        proc.retakeCapture();
        setStatus ("Retake: armed. The new take replaces the previous one (unless that slot is locked).");
    };
    swarmViewButton.setClickingTogglesState (true);
    swarmViewButton.onClick = [this] {
        showSwarm = swarmViewButton.getToggleState();
        waveform.setVisible (! showSwarm);
        swarmView.setVisible (showSwarm);
        swarmViewButton.setButtonText (showSwarm ? "WAVEFORM" : "SWARM VIEW");
        setStatus (showSwarm ? "Swarm view: one row per voice (top = left pan). Click to preview." : "Waveform view.");
    };
    for (auto* b : { &exportOptionsButton, &themeButton, &retakeButton, &swarmViewButton, &presetButton, &presetPrevButton, &presetNextButton })
        addAndMakeVisible (b);
    addAndMakeVisible (levelBar);
    addAndMakeVisible (swarmView);
    swarmView.setVisible (false);
    addAndMakeVisible (stepEditor);

    // Combos
    sourceModeCombo.addItemList (juce::StringArray { "Live Capture", "Loaded Sample", "Hybrid Layer", "Slice Scatter" }, 1);
    addAndMakeVisible (sourceModeCombo);
    sourceModeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::sourceMode, sourceModeCombo);

    scaleCombo.addItemList (juce::StringArray { "Chromatic", "Major", "Minor", "Pentatonic", "Oct / 5ths" }, 1);
    addAndMakeVisible (scaleCombo);
    scaleComboAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::scaleLock, scaleCombo);

    dirCombo.addItemList (juce::StringArray { "Forward", "Reverse", "Alternating", "Random" }, 1);
    addAndMakeVisible (dirCombo);
    dirComboAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::voiceDirection, dirCombo);

    postReleaseCombo.addItemList (juce::StringArray { "Cut at Impact", "Sustain Chorus", "Scatter Out" }, 1);
    addAndMakeVisible (postReleaseCombo);
    postReleaseAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::postRelease, postReleaseCombo);

    charCombo.addItemList (juce::StringArray { "Clean Digital", "Analog Ensemble", "Bucket-Brigade", "Tape Choir", "Dimension", "String Ensemble", "Granular Cloud", "Lo-Fi Choral", "Vowel Choir", "Stadium Crowd", "Glitch Scatter" }, 1);
    addAndMakeVisible (charCombo);
    charAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::character, charCombo);

    seqCombo.addItemList (juce::StringArray { "Every Note", "Beat 1 Only", "Every 2 Bars", "Every 4 Bars" }, 1);
    addAndMakeVisible (seqCombo);
    seqAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::sequence, seqCombo);

    stutterGridCombo.addItemList (juce::StringArray { "Straight", "Triplet", "Dotted", "Custom" }, 1);
    addAndMakeVisible (stutterGridCombo);
    stutterGridAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::stutterGrid, stutterGridCombo);
    targetCombo.addItemList (juce::StringArray { "MIDI Note", "Next Bar Line", "Loop End", "Input Transient" }, 1);
    addAndMakeVisible (targetCombo);
    targetAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::tgtSource, targetCombo);
    detectorCombo.addItemList (juce::StringArray { "Detect: Main In", "Detect: Sidechain" }, 1);
    addAndMakeVisible (detectorCombo);
    detectorAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::scSource, detectorCombo);

    library = std::make_unique<PresetLibrary> (PreChorusProcessor::getUserPresetFolder(),
                                               PreChorusProcessor::getUserPresetFolder().getParentDirectory().getChildFile ("settings.xml"));
    presetButton.onClick = [this] { openPresetBrowser(); };
    presetPrevButton.onClick = [this] { stepPreset (-1); };
    presetNextButton.onClick = [this] { stepPreset (1); };

    // Live Capture Controls & History
    addAndMakeVisible (armButton);
    addAndMakeVisible (captureButton);
    addAndMakeVisible (lockButton);
    armButton.onClick = [this] {
        if (proc.isCaptureArmed() || proc.isCapturing()) { proc.stopCapture(); setStatus ("Capture stopped."); }
        else { proc.armCapture(); setStatus ("Capture armed: waiting for input (" + captureCombo.getText() + ")."); }
    };
    captureButton.onClick = [this] {
        const bool wasRecording = proc.isCapturing();
        proc.triggerManualCapture();
        setStatus (wasRecording ? "Capture stopped; take stored in the history." : "Recording live input (click again to stop, max 8 s).");
    };
    lockButton.onClick = [this] {
        proc.setCaptureLock (! proc.isCaptureLocked());
        setStatus (proc.isCaptureLocked() ? "Active take locked: new captures go to the next free slot." : "Capture slot unlocked.");
    };

    captureCombo.addItemList (juce::StringArray { "Threshold", "1/16 Note", "1/8 Note", "1/4 Beat", "1/2 Note", "1 Bar", "2 Bars", "Manual" }, 1);
    addAndMakeVisible (captureCombo);
    captureComboAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::captureMode, captureCombo);

    for (int i = 0; i < 8; ++i)
    {
        historySlotButtons[(size_t) i].setButtonText (juce::String (i + 1));
        historySlotButtons[(size_t) i].onClick = [this, i] {
            proc.selectCaptureSlot (i);
            setStatus ("Capture slot " + juce::String (i + 1) + (proc.isSlotFilled (i) ? " selected." : " selected (empty: Live Capture uses the loaded sample)."));
        };
        addAndMakeVisible (historySlotButtons[(size_t) i]);
    }

    // Toggles
    freezeAtt     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, IDs::freeze, freezeToggle);
    revConvergeAtt= std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, IDs::revConverge, revConvergeToggle);
    alignAtt      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, IDs::align, alignToggle);
    syncAtt       = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, IDs::sync,  syncToggle);
    keytrackAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, IDs::keytrack, keytrackToggle);
    addAndMakeVisible (keytrackToggle);

    addAndMakeVisible (freezeToggle);
    addAndMakeVisible (revConvergeToggle);
    addAndMakeVisible (alignToggle);
    addAndMakeVisible (syncToggle);
    addAndMakeVisible (reducedMotionToggle);
    reducedMotionToggle.onClick = [this] {
        visualizer.setReducedMotion (reducedMotionToggle.getToggleState());
        setStatus (reducedMotionToggle.getToggleState() ? "Reduced motion enabled." : "Reduced motion disabled.");
    };

    syncCombo.addItemList (juce::StringArray { "1/2 Bar", "1 Bar", "2 Bars", "4 Bars" }, 1);
    addAndMakeVisible (syncCombo);
    syncComboAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::syncLen, syncCombo);

    rangeCombo.addItemList (juce::StringArray { "1 Oct", "2 Oct", "4 Oct" }, 1);
    addAndMakeVisible (rangeCombo);
    rangeComboAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::pitchRange, rangeCombo);

    // Visualizers
    addAndMakeVisible (visualizer);
    addAndMakeVisible (waveform);
    addAndMakeVisible (dragPad);
    addAndMakeVisible (pitchTension);

    // Knobs - Swarm Engine
    kVoiceCount   = &makeKnob (IDs::voiceCount,   "VOICES");
    kVoiceDensity = &makeKnob (IDs::voiceDensity, "DENSITY");
    kVoiceAge     = &makeKnob (IDs::voiceAge,     "VOICE AGE");
    kProgReveal   = &makeKnob (IDs::progReveal,   "REVEAL");
    kHumanize     = &makeKnob (IDs::humanize,     "HUMANIZE");
    kGrainSize    = &makeKnob (IDs::grainSize,    "GRAIN MS");

    // Knobs - Convergence Engine & Macro
    kMacro         = &makeKnob (IDs::macro,         "MACRO");
    kTimeSpread    = &makeKnob (IDs::timeSpread,    "TIME SPREAD");
    kTimeConverge  = &makeKnob (IDs::timeConverge,  "TIME CONV");
    kPitchSpread   = &makeKnob (IDs::pitchSpread,   "PITCH SPREAD");
    kDetune        = &makeKnob (IDs::detune,        "DETUNE");
    kPitchConverge = &makeKnob (IDs::pitchConverge, "PITCH CONV");
    kPanSpread     = &makeKnob (IDs::panSpread,     "PAN SPREAD");
    kPanConverge   = &makeKnob (IDs::panConverge,   "WIDTH CONV");
    kToneConverge  = &makeKnob (IDs::toneConverge,  "TONE CONV");
    kFocus         = &makeKnob (IDs::focus,         "FOCUS");

    // Knobs - Physics & Spatial
    kAttraction = &makeKnob (IDs::attraction, "ATTRACT");
    kTurbulence = &makeKnob (IDs::turbulence, "TURBULENCE");
    kOvershoot  = &makeKnob (IDs::overshoot,  "OVERSHOOT");
    kOrbit      = &makeKnob (IDs::orbit,      "ORBIT");
    kDistance   = &makeKnob (IDs::distance,   "3D DISTANCE");

    // Knobs - Swell & Tone Shaping
    kTail       = &makeKnob (IDs::tail,       "LENGTH");
    kShape      = &makeKnob (IDs::shape,      "SHAPE");
    kTone       = &makeKnob (IDs::tone,       "TONE");
    kBass       = &makeKnob (IDs::basscut,    "BASS CUT");
    kResonance  = &makeKnob (IDs::resonance,  "RESONANCE");
    kTilt       = &makeKnob (IDs::tilt,       "TILT EQ");
    kPresence   = &makeKnob (IDs::presence,   "PRESENCE");
    kAir        = &makeKnob (IDs::air,        "AIR");
    kSpace      = &makeKnob (IDs::space,      "SPACE");
    kDrive      = &makeKnob (IDs::drive,      "DRIVE");
    kTransients = &makeKnob (IDs::transients, "TRANSIENTS");
    kFormant    = &makeKnob (IDs::formant,    "FORMANT");
    kMonoBass   = &makeKnob (IDs::monoBass,   "MONO BASS");

    // Knobs - Mix & Capture
    kDry        = &makeKnob (IDs::dry,        "HIT DRY");
    kWet        = &makeKnob (IDs::wet,        "SWARM WET");
    kDryReplace = &makeKnob (IDs::dryReplace, "REPLACE");
    kDucking    = &makeKnob (IDs::ducking,    "DUCKING");
    kThresh     = &makeKnob (IDs::thresh,     "THRESH");
    kStutter    = &makeKnob (IDs::stutter,    "STUTTER");

    // Knobs - Pitch & Volume
    kPitch      = &makeKnob (IDs::pitch,      "PITCH");
    kVolStart   = &makeKnob (IDs::volStart,   "START");
    kVolEnd     = &makeKnob (IDs::volEnd,     "END");
    kVolTension = &makeKnob (IDs::volTension, "TENSION");
    kTrimStart  = &makeKnob (IDs::trimStart,  "TRIM IN");
    kTrimEnd    = &makeKnob (IDs::trimEnd,    "TRIM OUT");
    kStutter->slider.setColour (juce::Slider::rotarySliderFillColourId, PCColours::hitCol);

    // Knobs - Stutter grid, layers, targeting & morph
    kStutterSwing = &makeKnob (IDs::stutterSwing, "SWING");
    kLayRiser     = &makeKnob (IDs::layRiser,     "RISER");
    kLayRev       = &makeKnob (IDs::layRev,       "REV CYMBAL");
    kLaySub       = &makeKnob (IDs::laySub,       "SUB DROP");
    kLayImpact    = &makeKnob (IDs::layImpact,    "IMPACT");
    kLayTone      = &makeKnob (IDs::layTone,      "LAYER TONE");
    kTgtThresh    = &makeKnob (IDs::tgtThresh,    "TRANSIENT");
    kMorph        = &makeKnob (IDs::morph,        "A/B MORPH");
    kStutterSwing->slider.setColour (juce::Slider::rotarySliderFillColourId, PCColours::hitCol);
    kMorph->slider.setColour (juce::Slider::rotarySliderFillColourId, PCColours::neon);

    // Knob tooltips describe the audible result (spec 7.1), not just the label.
    const std::pair<Knob*, const char*> knobTips[] = {
        { kVoiceCount, "Number of rendered ensemble voices (1-32). More voices = denser, wider swarm." },
        { kVoiceDensity, "When voices join the build: negative = most voices enter early, positive = an avalanche near the hit." },
        { kVoiceAge, "Darkens the earliest voices with tape-style roll-off so the swarm brightens as it approaches the hit." },
        { kProgReveal, "Early voices play short fragments; higher values reveal more of the phrase only in later voices." },
        { kHumanize, "Random micro-timing (up to 15 ms) and pitch drift per voice for a more organic ensemble." },
        { kGrainSize, "Grain length in milliseconds for the Granular Cloud character (10-200 ms)." },
        { kMacro, "Master convergence amount: scales time, pitch, width and tone convergence together." },
        { kTimeSpread, "How far before the hit (in seconds) the earliest voices begin." },
        { kTimeConverge, "How tightly voice start times pull together toward the hit." },
        { kPitchSpread, "Range of random pitch offsets (semitones) the voices start from." },
        { kDetune, "Fine detune spread in cents for chorus thickness." },
        { kPitchConverge, "How strongly the scattered pitches glide into unison at the hit." },
        { kPanSpread, "Initial stereo spread of the voices." },
        { kPanConverge, "Width motion: positive collapses to mono centre at the hit, negative blooms outward." },
        { kToneConverge, "Scattered bright/dark voice colours converge to the full target spectrum." },
        { kFocus, "Delays convergence then snaps it into a laser-focused point right before the hit." },
        { kAttraction, "Shape of the pull toward unison: higher values hold the scatter longer before converging." },
        { kTurbulence, "Adds fluttering pitch jitter to the voices as they move." },
        { kOvershoot, "Spring-like pitch overshoot past unison near the hit." },
        { kOrbit, "Voices circle around the stereo field during the build." },
        { kDistance, "Voices start far away (quieter, more diffuse) and rush upfront and dry at the hit." },
        { kTail, "Swell length in seconds when SYNC is off (0.1-8 s)." },
        { kShape, "Swell volume curve: negative = fast fade-in, positive = slow build that surges at the end." },
        { kTone, "Low-pass cutoff for the swarm (Hz)." },
        { kBass, "High-pass cutoff for the swarm (Hz) to clear low-end mud." },
        { kResonance, "Q of the Tone and Bass Cut filters: higher values add a resonant peak." },
        { kTilt, "One-knob spectral tilt around 1 kHz: left = darker/warmer, right = brighter." },
        { kPresence, "10 kHz shelf lift for vocal clarity." },
        { kAir, "Saturated high-frequency exciter (8.5 kHz+) on the swarm and the target hit." },
        { kSpace, "Diffuse stereo echo wash around the swarm." },
        { kDrive, "Soft-clip saturation for harmonics and grit." },
        { kTransients, "Negative softens attacks and sibilance; positive emphasizes transient punch." },
        { kFormant, "Shifts the swarm's vocal timbre up/down (semitones) for a smaller or larger sounding choir." },
        { kMonoBass, "Below this frequency the swarm is made mono for solid club low end (Hz)." },
        { kDry, "Level of the dry input and of the target hit after the swell." },
        { kWet, "Level of the swarm build-up." },
        { kDryReplace, "Fades out the original target hit so the swarm replaces it (1 = no hit)." },
        { kDucking, "Ducks the swarm whenever the live input is loud, keeping vocals/drums clear." },
        { kThresh, "Input level (dBFS) that starts and ends Threshold auto-capture." },
        { kStutter, "Build Stutter: rhythmic gate in the second half of the swell, accelerating 1/8 > 1/16 > 1/32 into the hit." },
        { kPitch, "Pitch sweep over the whole rendered result: right rises, left falls (range set by RANGE)." },
        { kVolStart, "Volume envelope level at the start (also draggable on the waveform)." },
        { kVolEnd, "Volume envelope level at the end (also draggable on the waveform)." },
        { kVolTension, "Curve of the volume envelope between start and end." },
        { kTrimStart, "Crops the start of the rendered result (fraction of full length)." },
        { kTrimEnd, "Crops the end of the rendered result (fraction of full length)." },
        { kStutterSwing, "Swing for the build stutter: delays every second step (0-60%)." },
        { kLayRiser, "Synthesised noise riser that sweeps up into the hit." },
        { kLayRev, "Reverse cymbal swell that ends exactly on the hit." },
        { kLaySub, "Sub-bass drop (62 Hz down to 30 Hz) that starts on the hit." },
        { kLayImpact, "Short noise + sub thump on the hit." },
        { kLayTone, "Brightness of the riser, cymbal and impact layers." },
        { kTgtThresh, "Input level (dBFS) that triggers the swarm when Target Source is Input Transient." },
        { kMorph, "Blend between the A and B snapshots (store A, press A>B, edit B). Re-renders when you stop moving it." } };
    for (auto& [k, tip] : knobTips)
        k->slider.setTooltip (juce::String (tip) + " Double-click to reset.");

    kDry->slider.setColour (juce::Slider::rotarySliderFillColourId, PCColours::hitCol);
    kMacro->slider.setColour (juce::Slider::rotarySliderFillColourId, PCColours::neon);
    kDistance->slider.setColour (juce::Slider::rotarySliderFillColourId, PCColours::neon);

    prevButton.setTooltip ("Load the previous supported audio file in the current folder.");
    nextButton.setTooltip ("Load the next supported audio file in the current folder.");
    loadButton.setTooltip ("Load a WAV, AIFF, FLAC, MP3, or OGG source. Loading switches Live Capture to Loaded Sample.");
    playButton.setTooltip ("Trigger the current rendered swarm preview. Shortcut: Space.");
    exportButton.setTooltip ("Export the current rendered result as a stereo 24-bit WAV.");
    resetButton.setTooltip ("Reset the main sound-design edits without replacing the loaded or captured source.");
    randomButton.setTooltip ("Randomize musical sound-design parameters while preserving the source and any locked groups. Shortcut: R.");
    regenSeedButton.setTooltip ("Generate a new deterministic swarm seed without changing the other controls. Shortcut: G.");
    optionsButton.setTooltip ("Interface options: tooltips and window size.");
    helpButton.setTooltip ("Open workflow and control help. Shortcut: H or F1.");
    armButton.setTooltip ("Arm live capture. Click again to cancel the armed state.");
    captureButton.setTooltip ("Start or stop manual live capture for the active history slot.");
    lockButton.setTooltip ("Protect the active capture slot from accidental overwrite.");
    sourceModeCombo.setTooltip ("Choose whether the swarm uses live capture, a loaded file, both, or scattered slices.");
    captureCombo.setTooltip ("Choose threshold, tempo-length, or manual live capture behavior.");
    scaleCombo.setTooltip ("Constrain scattered pitch offsets to the selected musical scale.");
    dirCombo.setTooltip ("Choose forward, reverse, alternating, or deterministic-random voice playback direction.");
    postReleaseCombo.setTooltip ("Choose what the swarm does after the target impact.");
    charCombo.setTooltip ("Choose the per-voice character/color model.");
    seqCombo.setTooltip ("Restrict automatic target triggering to the selected beat/bar cycle.");
    presetButton.setTooltip ("Open the preset browser: search, tags, favorites, user presets and banks. Presets never replace the source audio or capture history.");
    presetPrevButton.setTooltip ("Previous preset in the list.");
    presetNextButton.setTooltip ("Next preset in the list.");
    exportOptionsButton.setTooltip ("Export options: format (WAV 16/24/32f, FLAC), sample rate, normalise, trim silent tail.");
    themeButton.setTooltip ("Randomize themes (Subtle, Wide, Dark, Wild, Tight, Cinematic) and lock groups that randomize must leave alone.");
    retakeButton.setTooltip ("Record the last capture again into the same slot.");
    swarmViewButton.setTooltip ("Switch between the rendered waveform and the per-voice swarm timeline.");
    stutterGridCombo.setTooltip ("Build stutter rhythm: straight, triplet, dotted, or your own 16-step pattern.");
    detectorCombo.setTooltip ("Which input drives Ducking, Threshold capture and Input Transient triggering. Sidechain needs the host's sidechain routing to be enabled for this plug-in.");
    targetCombo.setTooltip ("What triggers the swarm: MIDI notes, the next bar line, the loop end, or an input transient. The automatic modes need the host to be playing.");
    levelBar.setTooltip ("Live input level. The red line is the capture threshold.");
    stepEditor.setTooltip ("16-step stutter pattern for the Custom grid. The right-most step is the one just before the hit.");
    freezeToggle.setTooltip ("Hold convergence at its current spread to create a sustained cloud.");
    revConvergeToggle.setTooltip ("Invert the motion so voices move from coherent toward scattered.");
    alignToggle.setTooltip ("Reports the swell length as latency and delays the dry input to match, so the target hit lands exactly on the MIDI note.");
    keytrackToggle.setTooltip ("MIDI notes transpose the swarm (C4 = original pitch) so it can be played like an instrument.");
    abButton.setTooltip ("Switch between two sound-design snapshots (A/B compare). Shortcut: B.");
    viewButton.setTooltip ("Switch between the SIMPLE view (the main build controls) and the FULL view (every control). Your settings are never changed by switching.");
    abCopyButton.setTooltip ("Copy the current A/B slot's settings into the other slot.");
    pitchTension.setTooltip ("Pitch sweep curve: drag up/down to bend, double-click to reset to linear.");
    waveform.setTooltip ("Rendered result (swarm + amber target hit). Drag the white volume points; click elsewhere to preview.");
    dragPad.setTooltip ("Drag onto a DAW track to drop the rendered result as a 24-bit WAV.");
    for (int i = 0; i < 8; ++i)
        historySlotButtons[(size_t) i].setTooltip ("Select capture history slot " + juce::String (i + 1) + ". Filled slots are tinted.");

    syncToggle.setTooltip ("Use host tempo divisions for the swell length.");
    syncCombo.setTooltip ("Select the tempo-synced swell duration.");
    rangeCombo.setTooltip ("Set the maximum pitch sweep range.");
    reducedMotionToggle.setTooltip ("Stops decorative orbit and transient-flash animation while retaining the audio meters and functional waveform updates.");

    applyAccessibility();
    sendLookAndFeelChange();   // children created before the look-and-feel was applied pick it up (slider value boxes)
    addChildComponent (help);
    setResizable (true, true);
    setResizeLimits (kBaseW * 6 / 10, kBaseH * 6 / 10, kBaseW * 2, kBaseH * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) kBaseW / (double) kBaseH);
    if (proc.uiWidth >= kBaseW * 6 / 10) setSize (proc.uiWidth, juce::roundToInt (proc.uiWidth * (double) kBaseH / kBaseW));
    else setSize (kBaseW * 85 / 100, kBaseH * 85 / 100);
    setWantsKeyboardFocus (true);
    startTimerHz (15);
    timerCallback();
    grabKeyboardFocus();
}

PreChorusEditor::~PreChorusEditor()
{
    if (browserBox != nullptr)
    {
        if (auto* content = browserBox->getChildComponent (0)) content->setLookAndFeel (nullptr);
        browserBox->setLookAndFeel (nullptr);
        browserBox->dismiss();
        browserBox = nullptr;
    }
    proc.cancelMidiLearn();
    setLookAndFeel (nullptr);
}

PreChorusEditor::Knob& PreChorusEditor::makeKnob (const juce::String& id, const juce::String& textName)
{
    auto k = std::make_unique<Knob>();
    auto& s = k->slider;
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 52, 13);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    s.setColour (juce::Slider::rotarySliderFillColourId, PCColours::accent);
    addAndMakeVisible (s);

    k->id = id;
    k->baseName = textName;
    k->label.setText (textName, juce::dontSendNotification);
    k->label.setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setColour (juce::Label::textColourId, PCColours::textDim);
    addAndMakeVisible (k->label);

    if (auto* parameter = proc.apvts.getParameter (id))
    {
        s.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));
        s.setTooltip ("Adjust " + parameter->getName (64)
                      + " in the current swarm sound. Double-click to return to its documented default.");
    }
    k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, s);
    knobs.push_back (std::move (k));
    auto* knobPtr = knobs.back().get();
    knobPtr->slider.onContextClick = [this, knobPtr] (const juce::MouseEvent&) { showKnobMenu (*knobPtr); };
    return *knobPtr;
}

void PreChorusEditor::timerCallback()
{
    auto f = proc.getCurrentFile();
    const int sMode = (int) proc.param (IDs::sourceMode);
    if (sMode == 0) fileLabel.setText ("[Live Input Capture]", juce::dontSendNotification);
    else if (sMode == 1) fileLabel.setText (f.existsAsFile() ? f.getFileName() : "[Loaded Sample]", juce::dontSendNotification);
    else if (sMode == 2) fileLabel.setText ("[Hybrid Layer: Live + Loaded]", juce::dontSendNotification);
    else fileLabel.setText ("[Slice Scatter Ensemble]", juce::dontSendNotification);

    const int n = proc.getSampleCount();
    countLabel.setText (n > 0 ? juce::String (proc.getSampleIndex() + 1) + " / " + juce::String (n) : "", juce::dontSendNotification);

    const bool sync = proc.param (IDs::sync) > 0.5f;
    kTail->slider.setEnabled (! sync);
    kTail->slider.setAlpha (sync ? 0.4f : 1.0f);
    syncCombo.setEnabled (sync);
    syncCombo.setAlpha (sync ? 1.0f : 0.5f);

    const float conf = proc.getTargetConfidence();
    const int confPct = juce::roundToInt (conf * 100.0f);
    confidenceLabel.setText ("TARGET CONFIDENCE: " + juce::String (confPct) + "%", juce::dontSendNotification);
    confidenceLabel.setColour (juce::Label::textColourId, confPct > 90 ? PCColours::neon : (confPct > 70 ? PCColours::hitCol : PCColours::recCol));

    // Post-processing stereo output peak (spec 7.4): CLIP at or above 0 dBFS.
    const float outputPeak = proc.getOutputLevel();
    if (outputPeak >= 1.0f)
    {
        outputLabel.setText ("OUT CLIP +" + juce::String (juce::Decibels::gainToDecibels (outputPeak), 1) + " dBFS", juce::dontSendNotification);
        outputLabel.setColour (juce::Label::textColourId, PCColours::recCol);
    }
    else
    {
        const float db = juce::Decibels::gainToDecibels (outputPeak, -100.0f);
        outputLabel.setText ("OUT " + (db <= -99.9f ? juce::String ("-inf") : juce::String (db, 1)) + " dBFS", juce::dontSendNotification);
        outputLabel.setColour (juce::Label::textColourId, db > -6.0f ? PCColours::hitCol : PCColours::textDim);
    }

    {
        const auto summary = proc.getLastCaptureSummary();
        if (summary != lastSummaryShown) { lastSummaryShown = summary; if (summary.isNotEmpty()) setStatus (summary); }
        const bool busy = proc.isCapturing() || proc.isCaptureArmed();
        retakeButton.setEnabled (summary.isNotEmpty() && ! busy);
        retakeButton.setAlpha (retakeButton.isEnabled() ? 1.0f : 0.45f);
        const bool customGrid = (int) proc.param (IDs::stutterGrid) == 3;
        stepEditor.setAlpha (customGrid ? 1.0f : 0.5f);
        const bool transientMode = (int) proc.param (IDs::tgtSource) == 3;
        kTgtThresh->slider.setEnabled (transientMode);
        kTgtThresh->slider.setAlpha (transientMode ? 1.0f : 0.4f);
        presetButton.setButtonText (currentPresetKey.isEmpty() ? juce::String ("PRESETS")
                                                               : (currentPresetKey.startsWith ("F:") ? currentPresetKey.substring (2)
                                                                                                     : currentPresetKey.substring (2).fromLastOccurrenceOf ("/", false, false).upToLastOccurrenceOf (".pcpreset", false, true)));
        updateLearnVisuals();
    }

    abButton.setButtonText (proc.isSlotBActive() ? "B" : "A");
    abCopyButton.setButtonText (proc.isSlotBActive() ? "B>A" : "A>B");
    playButton.setButtonText (proc.isPreviewPlaying() ? "PLAYING" : "PLAY");

    const int currentSlot = proc.getActiveCaptureSlot();
    for (int i = 0; i < 8; ++i)
    {
        const bool isCur = (i == currentSlot);
        const bool isFilled = proc.isSlotFilled (i);
        if (isCur) historySlotButtons[(size_t) i].setColour (juce::TextButton::buttonColourId, PCColours::accent);
        else if (isFilled) historySlotButtons[(size_t) i].setColour (juce::TextButton::buttonColourId, PCColours::neon.withAlpha (0.35f));
        else historySlotButtons[(size_t) i].setColour (juce::TextButton::buttonColourId, PCColours::panel2);
    }

    lockButton.setColour (juce::TextButton::buttonColourId, proc.isCaptureLocked() ? PCColours::hitCol : PCColours::panel2);

    const auto state = proc.getCaptureState();
    if (state == PreChorusProcessor::CaptureState::recording)
    {
        captureButton.setColour (juce::TextButton::buttonColourId, PCColours::recCol);
        captureButton.setButtonText ("RECORDING...");
    }
    else if (state == PreChorusProcessor::CaptureState::armed)
    {
        captureButton.setColour (juce::TextButton::buttonColourId, PCColours::hitCol);
        captureButton.setButtonText ("ARMED (WAITING)");
    }
    else
    {
        captureButton.setColour (juce::TextButton::buttonColourId, PCColours::panel2);
        captureButton.setButtonText ("LIVE CAPTURE");
    }

    const juce::Colour col = PCColours::swellColour (proc.param (IDs::tone), proc.param (IDs::basscut));
    for (auto* k : { kTone, kBass, kWet, kTail, kShape, kTimeSpread, kTimeConverge, kPitchConverge, kVoiceCount, kFocus })
    {
        if (k->slider.findColour (juce::Slider::rotarySliderFillColourId) != col)
        {
            k->slider.setColour (juce::Slider::rotarySliderFillColourId, col);
            k->slider.repaint();
        }
    }
}

void PreChorusEditor::applyTooltipSetting()
{
    if (tooltipsEnabled)
    {
        if (tooltipWindow == nullptr)
            tooltipWindow = std::make_unique<juce::TooltipWindow> (this, 650);
    }
    else
    {
        tooltipWindow.reset();
    }
}

void PreChorusEditor::showOptionsMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader ("Interface");
    menu.addItem (1, "Show tooltips", true, tooltipsEnabled);
    menu.addItem (2, "Reduced motion", true, reducedMotionToggle.getToggleState());
    menu.addItem (5, "High contrast", true, proc.highContrast);
    menu.addSectionHeader ("Edit");
    menu.addItem (10, "Undo  (Ctrl/Cmd+Z)", proc.canUndo());
    menu.addItem (11, "Redo  (Ctrl/Cmd+Shift+Z)", proc.canRedo());
    menu.addSectionHeader ("Source");
    juce::PopupMenu demos;
    const auto demoNames = PCDemo::names();
    for (int i = 0; i < demoNames.size(); ++i) demos.addItem (2000 + i, demoNames[i]);
    menu.addSubMenu ("Load demo source", demos);
    juce::PopupMenu lengths;
    for (int sec : { 12, 30, 60, 120 })
        lengths.addItem (3000 + sec, juce::String (sec) + " seconds", true, proc.getMaxSourceSeconds() == sec);
    menu.addSubMenu ("Max source length", lengths);
    menu.addItem (3, "Store audio inside project", true, proc.embedAudio);
    menu.addSectionHeader ("Capture");
    menu.addItem (6, "Smart trim: remove silence around takes", true, proc.smartTrimCapture);
    menu.addItem (7, "Normalise takes to -1 dBFS", true, proc.smartNormaliseCapture);
    menu.addSectionHeader ("MIDI");
    menu.addItem (8, "Clear all MIDI mappings");
    menu.addSectionHeader ("Support");
    menu.addItem (4, "Copy diagnostics to clipboard");
    juce::PopupMenu sizes;
    for (int pct : { 60, 75, 85, 100, 125, 150 })
        sizes.addItem (1000 + pct, juce::String (pct) + "%", true, std::abs (getWidth() - kBaseW * pct / 100) < 4);
    menu.addSubMenu ("Window size", sizes);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&optionsButton),
                        [this] (int result)
                        {
                            if (result == 1)
                            {
                                tooltipsEnabled = ! tooltipsEnabled;
                                applyTooltipSetting();
                                setStatus (tooltipsEnabled ? "Tooltips enabled." : "Tooltips disabled.");
                            }
                            else if (result == 2)
                            {
                                reducedMotionToggle.setToggleState (! reducedMotionToggle.getToggleState(), juce::sendNotification);
                            }
                            else if (result == 10 || result == 11)
                            {
                                doUndoRedo (result == 11);
                            }
                            else if (result == 5)
                            {
                                proc.highContrast = ! proc.highContrast;
                                PCColours::applyHighContrast (proc.highContrast);
                                lnf.refreshColours();
                                for (auto& k : knobs) k->label.setColour (juce::Label::textColourId, PCColours::textDim);
                                for (auto* l : { &fileLabel, &countLabel, &rangeLabel, &statusLabel }) l->setColour (juce::Label::textColourId, PCColours::textDim);
                                title.setColour (juce::Label::textColourId, PCColours::text);
                                subtitle.setColour (juce::Label::textColourId, PCColours::accent);
                                sendLookAndFeelChange();
                                repaint();
                                setStatus (proc.highContrast ? "High contrast on." : "High contrast off.");
                            }
                            else if (result == 6) { proc.smartTrimCapture = ! proc.smartTrimCapture; setStatus (proc.smartTrimCapture ? "Smart trim on: silence around new takes is removed." : "Smart trim off."); }
                            else if (result == 7) { proc.smartNormaliseCapture = ! proc.smartNormaliseCapture; setStatus (proc.smartNormaliseCapture ? "New takes are normalised to -1 dBFS." : "Take normalising off."); }
                            else if (result == 8)
                            {
                                for (auto& k : knobs) proc.clearMidiMapping (k->id);
                                for (auto id : { IDs::sourceMode, IDs::captureMode, IDs::captureSlot, IDs::captureLock, IDs::freeze, IDs::revConverge,
                                                 IDs::postRelease, IDs::character, IDs::scaleLock, IDs::voiceDirection, IDs::stutterGrid, IDs::tgtSource })
                                    proc.clearMidiMapping (id);
                                setStatus ("All MIDI mappings cleared.");
                            }
                            else if (result == 3)
                            {
                                proc.embedAudio = ! proc.embedAudio;
                                setStatus (proc.embedAudio ? "Source audio will be stored inside the project." : "Source audio will be referenced by file path only.");
                            }
                            else if (result == 4)
                            {
                                juce::SystemClipboard::copyTextToClipboard (proc.getDiagnosticsReport());
                                setStatus ("Diagnostics copied. Paste them into your bug report.");
                            }
                            else if (result >= 3000 && result <= 3120)
                            {
                                proc.setMaxSourceSeconds (result - 3000);
                                setStatus ("Max source length: " + juce::String (proc.getMaxSourceSeconds()) + " s (live capture buffer resized; applies to the next file load).");
                            }
                            else if (result >= 2000 && result < 2000 + PCDemo::numKinds)
                            {
                                proc.loadDemoSource (result - 2000);
                                waveform.rebuild();
                                setStatus ("Loaded demo source: " + PCDemo::names()[result - 2000]);
                            }
                            else if (result > 1000)
                            {
                                const int pct = result - 1000;
                                setSize (kBaseW * pct / 100, kBaseH * pct / 100);
                                setStatus ("Window size " + juce::String (pct) + "%.");
                            }
                        });
}

void PreChorusEditor::loadPresetEntry (const PresetLibrary::Entry& entry)
{
    juce::String err;
    if (entry.factoryIndex >= 0)
    {
        proc.loadFactoryPreset (entry.factoryIndex);
        currentPresetKey = entry.key;
        setStatus ("Preset: " + entry.name);
    }
    else if (proc.loadUserPreset (entry.file, err))
    {
        currentPresetKey = entry.key;
        setStatus ("User preset: " + entry.name);
    }
    else setStatus ("Preset load failed: " + err);
    library->markRecent (entry.key);
    waveform.rebuild();
}

void PreChorusEditor::stepPreset (int direction)
{
    const auto order = library->filter ("", {}, false, 0);
    if (order.empty()) return;
    int pos = -1;
    for (int i = 0; i < (int) order.size(); ++i)
        if (library->entries()[(size_t) order[(size_t) i]].key == currentPresetKey) { pos = i; break; }
    pos = pos < 0 ? (direction > 0 ? 0 : (int) order.size() - 1) : (pos + direction + (int) order.size()) % (int) order.size();
    loadPresetEntry (library->entries()[(size_t) order[(size_t) pos]]);
}

void PreChorusEditor::savePresetDialog()
{
    auto folder = PreChorusProcessor::getUserPresetFolder();
    folder.createDirectory();
    presetChooser = std::make_shared<juce::FileChooser> ("Save PreChorus Preset", folder, "*.pcpreset");
    presetChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
        [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f == juce::File()) return;
            juce::String err;
            f = f.withFileExtension ("pcpreset");
            const bool ok = proc.saveUserPreset (f, err);
            setStatus (ok ? "Saved preset: " + f.getFileNameWithoutExtension() : "Preset save failed: " + err);
            library->rescan();
            if (ok) currentPresetKey = "U:" + f.getRelativePathFrom (library->getFolder()).replaceCharacter ('\\', '/');
        });
}

void PreChorusEditor::openPresetBrowser()
{
    if (browserBox != nullptr) { browserBox->dismiss(); browserBox = nullptr; return; }
    library->rescan();
    PresetBrowser::Callbacks cb;
    cb.load = [this] (const PresetLibrary::Entry& entry) { loadPresetEntry (entry); };
    cb.save = [this] { savePresetDialog(); };
    cb.init = [this] { proc.loadInitPreset(); currentPresetKey.clear(); waveform.rebuild(); setStatus ("Init: all sound-design controls reset to their defaults."); };
    cb.status = [this] (const juce::String& s) { setStatus (s); };
    auto content = std::make_unique<PresetBrowser> (*library, cb);
    content->setLookAndFeel (&lnf);
    content->setCurrentKey (currentPresetKey);
    auto area = presetButton.getScreenBounds();
    browserBox = &juce::CallOutBox::launchAsynchronously (std::move (content), area, nullptr);
    browserBox->setLookAndFeel (&lnf);
}

void PreChorusEditor::showThemeMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader ("Randomize theme");
    const auto themes = PreChorusProcessor::getRandomThemeNames();
    m.addItem (7, "Classic  (R)");
    for (int i = 0; i < themes.size(); ++i) m.addItem (1 + i, themes[i]);
    m.addSeparator();
    m.addSectionHeader ("Lock groups (kept out of randomize)");
    const auto groups = PreChorusProcessor::getLockGroupNames();
    for (int i = 0; i < groups.size(); ++i) m.addItem (20 + i, "Lock: " + groups[i], true, (proc.lockedGroups & (1u << i)) != 0);
    m.addItem (30, "Unlock all");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&themeButton), [this, themes, groups] (int r)
    {
        if (r >= 1 && r <= 6) { proc.randomizeThemed (r - 1); waveform.rebuild(); setStatus ("Randomized: " + themes[r - 1] + " theme (same seed + theme = same sound)."); }
        else if (r == 7) { proc.randomizePreChorus(); waveform.rebuild(); setStatus ("Sound-design parameters randomized."); }
        else if (r >= 20 && r < 20 + groups.size())
        {
            proc.lockedGroups ^= (1u << (r - 20));
            setStatus (juce::String ((proc.lockedGroups & (1u << (r - 20))) ? "Locked: " : "Unlocked: ") + groups[r - 20]);
        }
        else if (r == 30) { proc.lockedGroups = 0; setStatus ("All groups unlocked."); }
    });
}

void PreChorusEditor::showExportMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader ("Format");
    const auto fmts = PreChorusProcessor::exportFormatNames();
    for (int i = 0; i < fmts.size(); ++i) m.addItem (10 + i, fmts[i], true, proc.exportSettings.format == i);
    m.addSectionHeader ("Sample rate");
    const auto rates = PreChorusProcessor::exportRateNames();
    for (int i = 0; i < rates.size(); ++i) m.addItem (20 + i, rates[i], true, proc.exportSettings.rate == i);
    m.addSectionHeader ("Processing");
    m.addItem (30, "Normalise to -1 dBFS", true, proc.exportSettings.normalise);
    m.addItem (31, "Trim silent tail", true, proc.exportSettings.trimTail);
    m.addSeparator();
    m.addItem (40, "Export stems (Swarm + Hit)...");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&exportOptionsButton), [this] (int r)
    {
        if (r >= 10 && r < 14) proc.exportSettings.format = r - 10;
        else if (r >= 20 && r < 24) proc.exportSettings.rate = r - 20;
        else if (r == 30) proc.exportSettings.normalise = ! proc.exportSettings.normalise;
        else if (r == 31) proc.exportSettings.trimTail = ! proc.exportSettings.trimTail;
        else if (r == 40)
        {
            const auto ext = PreChorusProcessor::exportExtension (proc.exportSettings);
            chooser = std::make_unique<juce::FileChooser> ("Export stems (base name)", juce::File(), "*." + ext);
            chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                [this, ext] (const juce::FileChooser& fc)
                {
                    auto f = fc.getResult();
                    if (f == juce::File()) return;
                    const auto base = f.getFileNameWithoutExtension();
                    juce::String err;
                    const bool ok1 = proc.exportWithSettings (f.getSiblingFile (base + "_Swarm." + ext), proc.exportSettings, &err, 1);
                    const bool ok2 = ok1 && proc.exportWithSettings (f.getSiblingFile (base + "_Hit." + ext), proc.exportSettings, &err, 2);
                    setStatus (ok2 ? "Exported stems: " + base + "_Swarm and " + base + "_Hit." : "Stem export failed: " + err);
                });
            return;
        }
        else return;
        setStatus ("Export: " + PreChorusProcessor::exportFormatNames()[proc.exportSettings.format] + ", "
                   + PreChorusProcessor::exportRateNames()[proc.exportSettings.rate]
                   + (proc.exportSettings.normalise ? ", normalised" : "") + (proc.exportSettings.trimTail ? ", tail trimmed" : "")
                   + ". DRAG TO DAW uses the same settings.");
    });
}

void PreChorusEditor::runExport()
{
    const auto ext = PreChorusProcessor::exportExtension (proc.exportSettings);
    chooser = std::make_unique<juce::FileChooser> ("Export PreChorus Swell", juce::File(), "*." + ext);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
        [this, ext] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f == juce::File()) return;
            const auto out = f.withFileExtension (ext);
            juce::String err;
            const bool ok = proc.exportAudio (out, &err);
            setStatus (ok ? "Exported: " + out.getFileName() : "Export failed: " + err);
        });
}

void PreChorusEditor::showKnobMenu (Knob& k)
{
    juce::PopupMenu m;
    const int mapped = proc.getMappedMidiSource (k.id);
    const bool learning = proc.getMidiLearnParam() == k.id;
    m.addItem (1, learning ? "Cancel MIDI learn" : "MIDI learn...  (then move a CC / mod wheel)");
    if (mapped >= 0) m.addItem (2, "Clear MIDI mapping (" + PreChorusProcessor::midiSourceName (mapped) + ")");
    m.addSeparator();
    m.addItem (3, "Reset to default");
    auto* kp = &k;
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&k.slider), [this, kp] (int r)
    {
        if (r == 1)
        {
            if (proc.getMidiLearnParam() == kp->id) { proc.cancelMidiLearn(); setStatus ("MIDI learn cancelled."); }
            else { proc.startMidiLearn (kp->id); setStatus ("MIDI learn: move a controller for " + kp->baseName + " (Esc cancels). Not every host forwards CC to effects."); }
        }
        else if (r == 2) { proc.clearMidiMapping (kp->id); setStatus ("MIDI mapping cleared for " + kp->baseName + "."); }
        else if (r == 3)
        {
            if (auto* p = proc.apvts.getParameter (kp->id)) { proc.beginGesture (kp->id); p->setValueNotifyingHost (p->getDefaultValue()); proc.endGesture (kp->id); }
        }
    });
}

void PreChorusEditor::updateLearnVisuals()
{
    const auto learning = proc.getMidiLearnParam();
    for (auto& k : knobs)
    {
        const bool isLearning = k->id == learning;
        const int src = proc.getMappedMidiSource (k->id);
        const auto wanted = isLearning ? juce::String ("LEARNING...") : (src >= 0 ? k->baseName + " [" + PreChorusProcessor::midiSourceName (src) + "]" : k->baseName);
        if (k->label.getText() != wanted)
        {
            k->label.setText (wanted, juce::dontSendNotification);
            k->label.setColour (juce::Label::textColourId, isLearning ? PCColours::hitCol : (src >= 0 ? PCColours::neon : PCColours::textDim));
        }
    }
}

void PreChorusEditor::applyAccessibility()
{
    for (auto& k : knobs)
    {
        k->slider.setTitle (k->baseName);
        k->slider.setDescription (k->slider.getTooltip());
        k->slider.setWantsKeyboardFocus (true);
        k->label.setInterceptsMouseClicks (false, false);
        k->label.setAccessible (false);   // the slider already carries the name
    }
    struct Named { juce::Component* c; const char* name; };
    for (auto n : { Named { &sourceModeCombo, "Source mode" }, { &scaleCombo, "Scale lock" }, { &dirCombo, "Voice direction" },
                    { &postReleaseCombo, "Post-target release" }, { &charCombo, "Voice character" }, { &seqCombo, "Target sequence" },
                    { &captureCombo, "Capture mode" }, { &syncCombo, "Sync length" }, { &rangeCombo, "Pitch range" },
                    { &stutterGridCombo, "Stutter grid" }, { &targetCombo, "Target source" }, { &detectorCombo, "Detector source" }, { &dragPad, "Drag to DAW" } })
    {
        n.c->setTitle (n.name);
        n.c->setWantsKeyboardFocus (true);
    }
    for (auto* t : { &freezeToggle, &revConvergeToggle, &alignToggle, &syncToggle, &reducedMotionToggle, &keytrackToggle })
        t->setWantsKeyboardFocus (true);
    statusLabel.setTitle ("Status");
    outputLabel.setTitle ("Output level");
    confidenceLabel.setTitle ("Target confidence");
}

void PreChorusEditor::openHelp()
{
    help.setVisible (true);
    help.toFront (true);
    help.grabKeyboardFocus();
    setStatus ("Help opened (Esc or click to close).");
}

void PreChorusEditor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale ((float) getWidth() / (float) kBaseW));
    juce::ColourGradient grad (PCColours::bg.brighter (0.05f), 0.0f, 0.0f, PCColours::bg, 0.0f, (float) kBaseH, false);
    g.setGradientFill (grad);
    g.fillAll();

    const juce::Colour col = PCColours::swellColour (proc.param (IDs::tone), proc.param (IDs::basscut));
    g.setColour (col.withAlpha (0.04f));
    g.fillEllipse (-120.0f, -140.0f, 480.0f, 360.0f);
    g.setColour (PCColours::hitCol.withAlpha (0.035f));
    g.fillEllipse ((float) kBaseW - 340.0f, (float) kBaseH - 280.0f, 480.0f, 340.0f);

    for (auto& gr : groups)
    {
        auto r = gr.bounds.toFloat();
        g.setColour (PCColours::panel.withAlpha (0.75f));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (PCColours::outline);
        g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);
        g.setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
        g.setColour (PCColours::textDim);
        g.drawText (gr.name, gr.bounds.withHeight (16).withTrimmedLeft (10), juce::Justification::centredLeft);
    }
}

void PreChorusEditor::layoutKnobs (juce::Rectangle<int> area, std::initializer_list<Knob*> ks)
{
    const int kw = area.getWidth() / (int) ks.size();
    for (auto* k : ks)
    {
        auto cell = area.removeFromLeft (kw);
        k->label.setBounds (cell.removeFromTop (12));
        k->slider.setBounds (cell);
    }
}

void PreChorusEditor::resized()
{
    // Lay out in fixed base coordinates, then scale every child uniformly (resizable UI).
    const float scale = (float) getWidth() / (float) kBaseW;
    proc.uiWidth = getWidth(); proc.uiHeight = getHeight();
    const juce::Rectangle<int> base (0, 0, kBaseW, kBaseH);
    help.setBounds (base);
    groups.clear();
    auto area = base.reduced (12);

    // 1. Header Row
    auto header = area.removeFromTop (38);
    auto titleArea = header.removeFromLeft (230);
    title.setBounds (titleArea.removeFromTop (22));
    subtitle.setBounds (titleArea);
    helpButton.setBounds (header.removeFromRight (28).reduced (0, 5));
    header.removeFromRight (4);
    optionsButton.setBounds (header.removeFromRight (70).reduced (0, 5));
    header.removeFromRight (6);
    abCopyButton.setBounds (header.removeFromRight (40).reduced (0, 5));
    header.removeFromRight (3);
    abButton.setBounds (header.removeFromRight (28).reduced (0, 5));
    header.removeFromRight (6);

    sourceModeCombo.setBounds (header.removeFromRight (110).reduced (0, 5));
    header.removeFromRight (6);

    charCombo.setBounds (header.removeFromRight (116).reduced (0, 5));
    header.removeFromRight (6);

    presetNextButton.setBounds (header.removeFromRight (20).reduced (0, 5));
    header.removeFromRight (2);
    presetButton.setBounds (header.removeFromRight (126).reduced (0, 5));
    header.removeFromRight (2);
    presetPrevButton.setBounds (header.removeFromRight (20).reduced (0, 5));
    header.removeFromRight (6);

    auto browser = header.withTrimmedLeft (12);
    loadButton.setBounds (browser.removeFromRight (64).reduced (0, 5));
    browser.removeFromRight (4);
    nextButton.setBounds (browser.removeFromRight (28).reduced (0, 5));
    browser.removeFromRight (3);
    prevButton.setBounds (browser.removeFromRight (28).reduced (0, 5));
    browser.removeFromRight (6);
    countLabel.setBounds (browser.removeFromRight (48));
    fileLabel.setBounds (browser.reduced (0, 5));

    // 2. Capture History & Trigger Strip
    area.removeFromTop (4);
    auto capStrip = area.removeFromTop (26);
    armButton.setBounds (capStrip.removeFromLeft (44));      capStrip.removeFromLeft (4);
    captureButton.setBounds (capStrip.removeFromLeft (110)); capStrip.removeFromLeft (5);
    captureCombo.setBounds (capStrip.removeFromLeft (90));   capStrip.removeFromLeft (10);

    for (int i = 0; i < 8; ++i)
    {
        historySlotButtons[(size_t) i].setBounds (capStrip.removeFromLeft (20));
        capStrip.removeFromLeft (2);
    }
    capStrip.removeFromLeft (4);
    lockButton.setBounds (capStrip.removeFromLeft (48));  capStrip.removeFromLeft (8);
    retakeButton.setBounds (capStrip.removeFromLeft (60)); capStrip.removeFromLeft (8);
    levelBar.setBounds (capStrip.removeFromLeft (110).reduced (0, 6));

    confidenceLabel.setBounds (capStrip.removeFromRight (170));
    capStrip.removeFromRight (6);
    viewButton.setBounds (capStrip.removeFromRight (86));

    area.removeFromTop (3);
    auto statusRow = area.removeFromTop (18);
    swarmViewButton.setBounds (statusRow.removeFromRight (94).reduced (0, 1));
    statusRow.removeFromRight (6);
    reducedMotionToggle.setBounds (statusRow.removeFromRight (118));
    outputLabel.setBounds (statusRow.removeFromRight (118));
    statusLabel.setBounds (statusRow);

    // 3. Visualizers Row (32-Voice Constellation + Interactive Waveform)
    area.removeFromTop (6);
    auto vis = area.removeFromTop (210);
    visualizer.setBounds (vis.removeFromLeft (210));
    vis.removeFromLeft (10);
    waveform.setBounds (vis);
    swarmView.setBounds (vis);

    // 4. Transport & Alignment Row
    area.removeFromTop (6);
    auto trans = area.removeFromTop (28);
    playButton.setBounds (trans.removeFromLeft (64));       trans.removeFromLeft (4);
    exportButton.setBounds (trans.removeFromLeft (84));     trans.removeFromLeft (1);
    exportOptionsButton.setBounds (trans.removeFromLeft (20)); trans.removeFromLeft (4);
    dragPad.setBounds (trans.removeFromLeft (100));         trans.removeFromLeft (4);
    resetButton.setBounds (trans.removeFromLeft (80));      trans.removeFromLeft (4);
    randomButton.setBounds (trans.removeFromLeft (64));     trans.removeFromLeft (1);
    themeButton.setBounds (trans.removeFromLeft (20));      trans.removeFromLeft (4);
    regenSeedButton.setBounds (trans.removeFromLeft (56));  trans.removeFromLeft (10);

    freezeToggle.setBounds (trans.removeFromLeft (70));     trans.removeFromLeft (4);
    revConvergeToggle.setBounds (trans.removeFromLeft (82));trans.removeFromLeft (4);
    keytrackToggle.setBounds (trans.removeFromLeft (84));   trans.removeFromLeft (8);

    syncCombo.setBounds (trans.removeFromRight (84));      trans.removeFromRight (4);
    syncToggle.setBounds (trans.removeFromRight (56));     trans.removeFromRight (4);
    alignToggle.setBounds (trans.removeFromRight (122));   trans.removeFromRight (6);
    seqCombo.setBounds (trans.removeFromRight (96));

    // 5. Knob Panels (Rows A & B)
    area.removeFromTop (8);
    const auto knobArea = area;
    auto rowC = area.removeFromBottom (124);
    area.removeFromBottom (8);
    const int rowH = (area.getHeight() - 8) / 2;
    auto rowA = area.removeFromTop (rowH);
    area.removeFromTop (8);
    auto rowB = area;

    auto group = [&] (juce::Rectangle<int>& src, int width, const juce::String& name)
    {
        auto r = src.removeFromLeft (width);
        src.removeFromLeft (6);
        groups.push_back ({ name, r });
        return r.reduced (5).withTrimmedTop (13);
    };

    // Row A: SWARM VOICES, CONVERGENCE & MACRO, PHYSICS & DISTANCE
    const int wA = rowA.getWidth();
    auto swarmGrp = group (rowA, (int) (wA * 0.35f), "SWARM ENGINE (1-32 VOICES)");
    {
        auto rightCombo = swarmGrp.removeFromRight (76);
        dirCombo.setBounds (rightCombo.withSizeKeepingCentre (74, 22));
        layoutKnobs (swarmGrp, { kVoiceCount, kVoiceDensity, kVoiceAge, kProgReveal, kHumanize, kGrainSize });
    }

    auto convGrp = group (rowA, (int) (wA * 0.43f), "CONVERGENCE & MACRO");
    {
        auto rightCombo = convGrp.removeFromRight (80);
        scaleCombo.setBounds (rightCombo.withSizeKeepingCentre (78, 22));
        layoutKnobs (convGrp, { kMacro, kTimeSpread, kTimeConverge, kPitchSpread, kDetune, kPitchConverge, kPanSpread, kPanConverge, kToneConverge, kFocus });
    }

    layoutKnobs (group (rowA, rowA.getWidth(), "PHYSICS & 3D DISTANCE"), { kAttraction, kTurbulence, kOvershoot, kOrbit, kDistance });

    // Row B: TONE SHAPING & COLOR, MIX & DUCK, PITCH & ENVELOPE
    const int wB = rowB.getWidth();
    layoutKnobs (group (rowB, (int) (wB * 0.49f), "TONE SHAPING, ACOUSTICS & COLOR"),
                 { kTail, kShape, kTone, kBass, kResonance, kTilt, kPresence, kAir, kSpace, kDrive, kTransients, kFormant, kMonoBass });

    auto mixGrp = group (rowB, (int) (wB * 0.255f), "MIX, CAPTURE, DUCK & STUTTER");
    {
        auto rightRelease = mixGrp.removeFromRight (94);
        postReleaseCombo.setBounds (rightRelease.withSizeKeepingCentre (90, 22));
        layoutKnobs (mixGrp, { kDry, kWet, kDryReplace, kDucking, kThresh, kStutter });
    }

    auto pitchArea = group (rowB, rowB.getWidth(), "PITCH, VOLUME & TRIM");
    {
        auto right = pitchArea.removeFromRight (60);
        rangeLabel.setBounds (right.removeFromTop (11));
        rangeCombo.setBounds (right.removeFromTop (20).reduced (2, 0));
        right.removeFromTop (2);
        pitchTension.setBounds (right.withSizeKeepingCentre (50, juce::jmin (50, right.getHeight())));
        layoutKnobs (pitchArea, { kPitch, kVolStart, kVolEnd, kVolTension, kTrimStart, kTrimEnd });
    }

    // Row C: STUTTER GRID, IMPACT LAYERS, TARGET & MORPH
    const int wC = rowC.getWidth();
    {
        auto g1 = group (rowC, (int) (wC * 0.38f), "BUILD STUTTER GRID");
        stutterGridCombo.setBounds (g1.removeFromLeft (92).withSizeKeepingCentre (88, 22));
        g1.removeFromLeft (4);
        layoutKnobs (g1.removeFromLeft (70), { kStutterSwing });
        g1.removeFromLeft (8);
        stepEditor.setBounds (g1.withSizeKeepingCentre (g1.getWidth(), 34));
    }
    layoutKnobs (group (rowC, (int) (wC * 0.33f), "IMPACT & RISER LAYERS"), { kLayRiser, kLayRev, kLaySub, kLayImpact, kLayTone });
    {
        auto g3 = group (rowC, rowC.getWidth(), "TARGET SOURCE & A/B MORPH");
        auto combos = g3.removeFromLeft (112);
        targetCombo.setBounds (combos.removeFromTop (combos.getHeight() / 2).withSizeKeepingCentre (108, 22));
        detectorCombo.setBounds (combos.withSizeKeepingCentre (108, 22));
        g3.removeFromLeft (4);
        layoutKnobs (g3, { kTgtThresh, kMorph });
    }

    // Simple view: only the main build controls, laid out large. (Pure layout switch: no parameters change.)
    const bool simple = proc.simpleView;
    const std::initializer_list<Knob*> simpleKnobs { kMacro, kVoiceCount, kTail, kPanSpread, kTilt, kSpace, kWet };
    for (auto& k : knobs)
    {
        const bool show = ! simple || std::find (simpleKnobs.begin(), simpleKnobs.end(), k.get()) != simpleKnobs.end();
        k->slider.setVisible (show);
        k->label.setVisible (show);
    }
    for (auto* c : std::initializer_list<juce::Component*> { &dirCombo, &scaleCombo, &postReleaseCombo, &rangeCombo, &rangeLabel, &pitchTension,
                                                              &stutterGridCombo, &targetCombo, &detectorCombo, &stepEditor })
        c->setVisible (! simple);
    if (simple)
    {
        groups.clear();
        groups.push_back ({ "ONE-KNOB BUILD   (FULL VIEW shows every control)", knobArea });
        auto simpleRow = knobArea.reduced (30, 22).withTrimmedTop (14);
        layoutKnobs (simpleRow.withSizeKeepingCentre (simpleRow.getWidth(), juce::jmin (simpleRow.getHeight(), 250)), simpleKnobs);
    }

    const auto t = juce::AffineTransform::scale (scale);
    for (auto* c : getChildren())
        if (c != resizableCorner.get() && dynamic_cast<juce::TooltipWindow*> (c) == nullptr)
            c->setTransform (t);
}

bool PreChorusEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg")) return true;
    return false;
}

void PreChorusEditor::filesDropped (const juce::StringArray& files, int, int)
{
    for (auto& f : files)
    {
        const juce::File file (f);
        if (proc.loadSampleFile (file, true))
        {
            waveform.rebuild();
            setStatus ("Loaded: " + file.getFileName());
            return;
        }
    }
    setStatus ("Dropped file could not be loaded.");
}

void PreChorusEditor::doUndoRedo (bool redo)
{
    if (redo ? ! proc.canRedo() : ! proc.canUndo()) { setStatus (redo ? "Nothing to redo." : "Nothing to undo."); return; }
    if (redo) proc.redo(); else proc.undo();
    waveform.rebuild();
    setStatus (redo ? "Redo." : "Undo.");
}

void PreChorusEditor::setStatus (const juce::String& text)
{
    statusLabel.setText (text, juce::dontSendNotification);
    proc.logStatus (text);
    if (auto* h = statusLabel.getAccessibilityHandler()) h->notifyAccessibilityEvent (juce::AccessibilityEvent::textChanged);
}

bool PreChorusEditor::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey)
    {
        proc.triggerPreview();
        setStatus ("Preview triggered.");
        return true;
    }

    if (key.getKeyCode() == juce::KeyPress::escapeKey && proc.getMidiLearnParam().isNotEmpty())
    {
        proc.cancelMidiLearn();
        setStatus ("MIDI learn cancelled.");
        return true;
    }
    if (key.getKeyCode() == juce::KeyPress::escapeKey && help.isVisible())
    {
        help.setVisible (false);
        return true;
    }
    if (key.getKeyCode() == juce::KeyPress::escapeKey)
    {
        proc.stopAll();
        setStatus ("Preview stopped.");
        return true;
    }

    // Undo / redo: Ctrl/Cmd+Z, Ctrl/Cmd+Shift+Z, Ctrl/Cmd+Y
    if (key.getModifiers().isCommandDown())
    {
        const int code = key.getKeyCode();
        const bool shift = key.getModifiers().isShiftDown();
        if ((code == 'Z' || code == 'z') && ! shift) { doUndoRedo (false); return true; }
        if (((code == 'Z' || code == 'z') && shift) || code == 'Y' || code == 'y') { doUndoRedo (true); return true; }
    }

    const auto ch = juce::CharacterFunctions::toLowerCase (key.getTextCharacter());
    if (ch == 'r')
    {
        proc.randomizePreChorus();
        waveform.rebuild();
        setStatus ("Sound-design parameters randomized.");
        return true;
    }
    if (ch == 'g')
    {
        proc.regenerateSeed();
        setStatus ("Swarm seed regenerated: " + juce::String ((int) proc.param (IDs::seed)) + ".");
        return true;
    }
    if (ch == 'b')
    {
        abButton.triggerClick();
        return true;
    }
    if (ch == 'h' || key.getKeyCode() == juce::KeyPress::F1Key)
    {
        openHelp();
        return true;
    }

    return false;
}
