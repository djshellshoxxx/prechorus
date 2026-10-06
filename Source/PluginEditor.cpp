// PreChorus™
// Copyright © 2026 Sheldon Davidson. All rights reserved.
// Proprietary source code. See LICENSE and COPYRIGHT-TRADEMARK.md.
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "PluginEditor.h"

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

PCLookAndFeel::PCLookAndFeel()
{
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

void PCLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool isDown, int, int, int, int, juce::ComboBox& b)
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
    phase += 0.025f;
    const float lvl = proc.getOutputLevel();
    if (lvl - smoothedOut > 0.12f) flashRing = 1.0f;
    smoothedOut += (lvl - smoothedOut) * 0.35f;
    flashRing *= 0.88f;
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
    const int ph = proc.getPlayheadPosition();
    const float t = proc.param (IDs::tone);
    const float b = proc.param (IDs::basscut);
    const float v0 = proc.param (IDs::volStart);
    const float v1 = proc.param (IDs::volEnd);
    const float vt = proc.param (IDs::volTension);

    if (ph != lastPlayhead || t != lastTone || b != lastBass || v0 != lastV0 || v1 != lastV1 || vt != lastVT)
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
    for (int i = 1; i <= 64; ++i)
    {
        const float norm = (float) i / 64.0f;
        const float lvl = v0 + (v1 - v0) * tensionCurve (norm, vt);
        volPath.lineTo (p.getX() + norm * p.getWidth(), volY (lvl));
    }
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.strokePath (volPath, juce::PathStrokeType (1.8f));

    g.setColour (PCColours::neon);
    g.fillEllipse (p.getX() - 4.0f, volY (v0) - 4.0f, 8.0f, 8.0f);
    g.fillEllipse (p.getRight() - 4.0f, volY (v1) - 4.0f, 8.0f, 8.0f);

    const float midLvl = v0 + (v1 - v0) * tensionCurve (0.5f, vt);
    g.setColour (PCColours::accent);
    g.fillEllipse (p.getCentreX() - 3.5f, volY (midLvl) - 3.5f, 7.0f, 7.0f);

    const float tStart = proc.param (IDs::trimStart);
    const float tEnd   = proc.param (IDs::trimEnd);
    if (tStart > 0.001f)
    {
        const float tx = p.getX() + tStart * p.getWidth();
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRect (p.getX(), p.getY(), tx - p.getX(), p.getHeight());
        g.setColour (PCColours::textDim);
        g.drawVerticalLine ((int) tx, p.getY(), p.getBottom());
    }
    if (tEnd < 0.999f)
    {
        const float tx = p.getX() + tEnd * p.getWidth();
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRect (tx, p.getY(), p.getRight() - tx, p.getHeight());
        g.setColour (PCColours::textDim);
        g.drawVerticalLine ((int) tx, p.getY(), p.getBottom());
    }

    const int ph = proc.getPlayheadPosition();
    if (ph >= 0 && total > 0)
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

void WaveformDisplay::mouseDown (const juce::MouseEvent& e)
{
    auto p = plot();
    downPos = e.position;
    moved = false;

    const float v0 = proc.param (IDs::volStart);
    const float v1 = proc.param (IDs::volEnd);
    const float vt = proc.param (IDs::volTension);
    const float midLvl = v0 + (v1 - v0) * tensionCurve (0.5f, vt);

    if (e.position.getDistanceFrom ({ p.getX(), volY (v0) }) < 12.0f) drag = Drag::volStart;
    else if (e.position.getDistanceFrom ({ p.getRight(), volY (v1) }) < 12.0f) drag = Drag::volEnd;
    else if (e.position.getDistanceFrom ({ p.getCentreX(), volY (midLvl) }) < 12.0f) { drag = Drag::volTension; downA = vt; }
    else { drag = Drag::none; proc.triggerPreview(); }
}

void WaveformDisplay::mouseDrag (const juce::MouseEvent& e)
{
    moved = true;
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

void WaveformDisplay::mouseUp (const juce::MouseEvent&) { drag = Drag::none; }
void WaveformDisplay::mouseMove (const juce::MouseEvent&) {}

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
}

void DragOutPad::mouseDrag (const juce::MouseEvent&)
{
    if (dragging) return;
    dragging = true;
    auto tempFile = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("PreChorus_Swarm.wav");
    if (proc.exportWav (tempFile))
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
        "PRECHORUS - Complete 32-Voice Swarm & Convergence Engine\n\n"
        "The Signature Sound: A cloud of related voices becoming progressively more recognizable "
        "and coherent until they meet the original drop or event.\n\n"
        "FACTORY PRESETS: Pick a starting point from the header dropdown (Pop Vocal Double, EDM "
        "Riser Swarm, Future Bass Shimmer, Dubstep Chaos Impact, Intimate Whisper Build, Cinematic "
        "Choir Pad, Lo-Fi Bedroom Vocal, Ambient Drone Freeze, Aggressive Distortion Drop, Trap "
        "Vocal Stutter). Presets only touch swarm/tone/convergence knobs, never your loaded audio.\n\n"
        "CHARACTER MODES:\n"
        "• Clean Digital: Pristine transparent sinc/linear interpolation.\n"
        "• Analog Ensemble: Warm saturation, gentle drift, bandwidth contouring.\n"
        "• Bucket-Brigade (BBD): Darker repeats, analog BBD clock roll-off, companding, and clock noise.\n"
        "• Tape Choir: Wow & flutter pitch modulation and tape head saturation.\n"
        "• Dimension: Ultra-wide cross-coupled chorusing designed to preserve solid mono center.\n"
        "• String Ensemble: Solina-style multi-rate dual-LFO modulation.\n"
        "• Granular Cloud: Micro-grain cloud with variable grain size and Hann windowing.\n"
        "• Lo-Fi Choral: Vintage bit-depth and sample-rate reduction.\n\n"
        "GLOBAL SHAPING & MOTION:\n"
        "• 3D Distance: Doppler distance staging (far cavern wash -> upfront dry impact).\n"
        "• Focus: Accelerates convergence near the drop into laser focus.\n"
        "• Tilt EQ, Presence & Air: Spectral balance pivot, presence lift, and filtered HF air excitation.\n"
        "• Mono Bass: High-passes side channel below cutoff frequency (pure mono sub-bass).\n"
        "• Ducking: Sidechains and ducks the swell when live input vocals/hits strike.\n"
    );
    addAndMakeVisible (body);
    addAndMakeVisible (closeButton);
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
    closeButton.setBounds (r.removeFromBottom (32).withSizeKeepingCentre (120, 30));
    r.removeFromBottom (10);
    body.setBounds (r);
}

// ---------------- PreChorus Editor ----------------

PreChorusEditor::PreChorusEditor (PreChorusProcessor& p)
    : AudioProcessorEditor (&p), proc (p), waveform (p), visualizer (p), dragPad (p),
      pitchTension (p, IDs::pitchTension)
{
    setLookAndFeel (&lnf);

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

    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);
    addAndMakeVisible (loadButton);
    addAndMakeVisible (playButton);
    addAndMakeVisible (exportButton);
    addAndMakeVisible (resetButton);
    addAndMakeVisible (randomButton);
    addAndMakeVisible (regenSeedButton);
    addAndMakeVisible (helpButton);

    prevButton.onClick = [this] { proc.prevSample(); waveform.rebuild(); };
    nextButton.onClick = [this] { proc.nextSample(); waveform.rebuild(); };
    loadButton.onClick = [this] {
        chooser = std::make_unique<juce::FileChooser> ("Load Vocal or Sample Stem", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc) {
                auto f = fc.getResult();
                if (f.existsAsFile()) { proc.loadSampleFile (f, true); waveform.rebuild(); }
            });
    };
    playButton.onClick      = [this] { proc.triggerPreview(); };
    resetButton.onClick     = [this] { proc.resetEdits(); waveform.rebuild(); };
    randomButton.onClick    = [this] { proc.randomizePreChorus(); waveform.rebuild(); };
    regenSeedButton.onClick = [this] { proc.regenerateSeed(); waveform.rebuild(); };
    helpButton.onClick      = [this] { help.setVisible (true); };

    exportButton.onClick = [this] {
        chooser = std::make_unique<juce::FileChooser> ("Export PreChorus Swell WAV", juce::File(), "*.wav");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
            [this] (const juce::FileChooser& fc) {
                auto f = fc.getResult();
                if (f != juce::File()) proc.exportWav (f.withFileExtension ("wav"));
            });
    };

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

    charCombo.addItemList (juce::StringArray { "Clean Digital", "Analog Ensemble", "Bucket-Brigade", "Tape Choir", "Dimension", "String Ensemble", "Granular Cloud", "Lo-Fi Choral" }, 1);
    addAndMakeVisible (charCombo);
    charAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::character, charCombo);

    seqCombo.addItemList (juce::StringArray { "Every Note", "Beat 1 Only", "Every 2 Bars", "Every 4 Bars" }, 1);
    addAndMakeVisible (seqCombo);
    seqAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::sequence, seqCombo);

    presetCombo.setTextWhenNothingSelected ("FACTORY PRESETS");
    presetCombo.addItemList (PreChorusProcessor::getFactoryPresetNames(), 1);
    addAndMakeVisible (presetCombo);
    presetCombo.onChange = [this] {
        const int idx = presetCombo.getSelectedItemIndex();
        if (idx >= 0) { proc.loadFactoryPreset (idx); waveform.rebuild(); }
    };

    // Live Capture Controls & History
    addAndMakeVisible (armButton);
    addAndMakeVisible (captureButton);
    addAndMakeVisible (lockButton);
    armButton.onClick = [this] {
        if (proc.isCaptureArmed()) proc.stopCapture();
        else proc.armCapture();
    };
    captureButton.onClick = [this] {
        proc.triggerManualCapture();
        waveform.rebuild();
    };
    lockButton.onClick = [this] {
        proc.setCaptureLock (! proc.isCaptureLocked());
    };

    captureCombo.addItemList (juce::StringArray { "Threshold", "1/16 Note", "1/8 Note", "1/4 Beat", "1/2 Note", "1 Bar", "2 Bars", "Manual" }, 1);
    addAndMakeVisible (captureCombo);
    captureComboAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::captureMode, captureCombo);

    for (int i = 0; i < 8; ++i)
    {
        historySlotButtons[(size_t) i].setButtonText (juce::String (i + 1));
        historySlotButtons[(size_t) i].onClick = [this, i] {
            proc.selectCaptureSlot (i);
            waveform.rebuild();
        };
        addAndMakeVisible (historySlotButtons[(size_t) i]);
    }

    // Toggles
    freezeAtt     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, IDs::freeze, freezeToggle);
    revConvergeAtt= std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, IDs::revConverge, revConvergeToggle);
    alignAtt      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, IDs::align, alignToggle);
    syncAtt       = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, IDs::sync,  syncToggle);

    addAndMakeVisible (freezeToggle);
    addAndMakeVisible (revConvergeToggle);
    addAndMakeVisible (alignToggle);
    addAndMakeVisible (syncToggle);

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

    // Knobs - Pitch & Volume
    kPitch      = &makeKnob (IDs::pitch,      "PITCH");
    kVolStart   = &makeKnob (IDs::volStart,   "START");
    kVolEnd     = &makeKnob (IDs::volEnd,     "END");
    kVolTension = &makeKnob (IDs::volTension, "TENSION");

    kDry->slider.setColour (juce::Slider::rotarySliderFillColourId, PCColours::hitCol);
    kMacro->slider.setColour (juce::Slider::rotarySliderFillColourId, PCColours::neon);
    kDistance->slider.setColour (juce::Slider::rotarySliderFillColourId, PCColours::neon);

    addChildComponent (help);
    setSize (1240, 840);
    setWantsKeyboardFocus (true);
    startTimerHz (15);
    timerCallback();
    grabKeyboardFocus();
}

PreChorusEditor::~PreChorusEditor()
{
    setLookAndFeel (nullptr);
}

PreChorusEditor::Knob& PreChorusEditor::makeKnob (const juce::String& id, const juce::String& textName)
{
    auto k = std::make_unique<Knob>();
    auto& s = k->slider;
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 13);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    s.setColour (juce::Slider::rotarySliderFillColourId, PCColours::accent);
    addAndMakeVisible (s);

    k->label.setText (textName, juce::dontSendNotification);
    k->label.setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setColour (juce::Label::textColourId, PCColours::textDim);
    addAndMakeVisible (k->label);

    k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, s);
    knobs.push_back (std::move (k));
    return *knobs.back();
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

void PreChorusEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient grad (PCColours::bg.brighter (0.05f), 0.0f, 0.0f, PCColours::bg, 0.0f, (float) getHeight(), false);
    g.setGradientFill (grad);
    g.fillAll();

    const juce::Colour col = PCColours::swellColour (proc.param (IDs::tone), proc.param (IDs::basscut));
    g.setColour (col.withAlpha (0.04f));
    g.fillEllipse (-120.0f, -140.0f, 480.0f, 360.0f);
    g.setColour (PCColours::hitCol.withAlpha (0.035f));
    g.fillEllipse ((float) getWidth() - 340.0f, (float) getHeight() - 280.0f, 480.0f, 340.0f);

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
    help.setBounds (getLocalBounds());
    groups.clear();
    auto area = getLocalBounds().reduced (12);

    // 1. Header Row
    auto header = area.removeFromTop (38);
    auto titleArea = header.removeFromLeft (230);
    title.setBounds (titleArea.removeFromTop (22));
    subtitle.setBounds (titleArea);
    helpButton.setBounds (header.removeFromRight (28).reduced (0, 5));
    header.removeFromRight (6);

    sourceModeCombo.setBounds (header.removeFromRight (110).reduced (0, 5));
    header.removeFromRight (6);

    charCombo.setBounds (header.removeFromRight (116).reduced (0, 5));
    header.removeFromRight (6);

    presetCombo.setBounds (header.removeFromRight (128).reduced (0, 5));
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
    lockButton.setBounds (capStrip.removeFromLeft (48));

    confidenceLabel.setBounds (capStrip.removeFromRight (170));

    // 3. Visualizers Row (32-Voice Constellation + Interactive Waveform)
    area.removeFromTop (6);
    auto vis = area.removeFromTop (210);
    visualizer.setBounds (vis.removeFromLeft (210));
    vis.removeFromLeft (10);
    waveform.setBounds (vis);

    // 4. Transport & Alignment Row
    area.removeFromTop (6);
    auto trans = area.removeFromTop (28);
    playButton.setBounds (trans.removeFromLeft (64));       trans.removeFromLeft (4);
    exportButton.setBounds (trans.removeFromLeft (84));     trans.removeFromLeft (4);
    dragPad.setBounds (trans.removeFromLeft (100));         trans.removeFromLeft (4);
    resetButton.setBounds (trans.removeFromLeft (80));      trans.removeFromLeft (4);
    randomButton.setBounds (trans.removeFromLeft (64));     trans.removeFromLeft (4);
    regenSeedButton.setBounds (trans.removeFromLeft (56));  trans.removeFromLeft (10);

    freezeToggle.setBounds (trans.removeFromLeft (70));     trans.removeFromLeft (4);
    revConvergeToggle.setBounds (trans.removeFromLeft (82));trans.removeFromLeft (8);

    syncCombo.setBounds (trans.removeFromRight (84));      trans.removeFromRight (4);
    syncToggle.setBounds (trans.removeFromRight (56));     trans.removeFromRight (4);
    alignToggle.setBounds (trans.removeFromRight (122));   trans.removeFromRight (6);
    seqCombo.setBounds (trans.removeFromRight (96));

    // 5. Knob Panels (Rows A & B)
    area.removeFromTop (8);
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
    layoutKnobs (group (rowB, (int) (wB * 0.54f), "TONE SHAPING, ACOUSTICS & COLOR"),
                 { kTail, kShape, kTone, kBass, kResonance, kTilt, kPresence, kAir, kSpace, kDrive, kTransients, kFormant, kMonoBass });

    auto mixGrp = group (rowB, (int) (wB * 0.26f), "MIX, CAPTURE & DUCK");
    {
        auto rightRelease = mixGrp.removeFromRight (94);
        postReleaseCombo.setBounds (rightRelease.withSizeKeepingCentre (90, 22));
        layoutKnobs (mixGrp, { kDry, kWet, kDryReplace, kDucking, kThresh });
    }

    auto pitchArea = group (rowB, rowB.getWidth(), "PITCH & VOLUME");
    {
        auto right = pitchArea.removeFromRight (60);
        rangeLabel.setBounds (right.removeFromTop (11));
        rangeCombo.setBounds (right.removeFromTop (20).reduced (2, 0));
        right.removeFromTop (2);
        pitchTension.setBounds (right.withSizeKeepingCentre (50, juce::jmin (50, right.getHeight())));
        layoutKnobs (pitchArea, { kPitch, kVolStart, kVolEnd, kVolTension });
    }
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
        if (proc.loadSampleFile (juce::File (f), true)) return;
}

bool PreChorusEditor::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey)
    {
        proc.triggerPreview();
        return true;
    }
    return false;
}
