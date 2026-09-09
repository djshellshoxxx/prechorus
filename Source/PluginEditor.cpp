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
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (3.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto center = bounds.getCentre();
    const float arcRadius = radius - 4.5f;
    const float currentAngle = startAngle + pos * (endAngle - startAngle);

    // Track background
    juce::Path bgArc;
    bgArc.addCentredArc (center.x, center.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (PCColours::outline);
    g.strokePath (bgArc, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value Arc
    if (pos > 0.001f)
    {
        juce::Path valArc;
        valArc.addCentredArc (center.x, center.y, arcRadius, arcRadius, 0.0f, startAngle, currentAngle, true);
        g.setColour (s.findColour (juce::Slider::rotarySliderFillColourId));
        g.strokePath (valArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Dial body
    const float dialRadius = arcRadius - 5.5f;
    g.setColour (PCColours::panel2);
    g.fillEllipse (center.x - dialRadius, center.y - dialRadius, dialRadius * 2.0f, dialRadius * 2.0f);
    g.setColour (PCColours::outline.brighter (0.08f));
    g.drawEllipse (center.x - dialRadius, center.y - dialRadius, dialRadius * 2.0f, dialRadius * 2.0f, 1.0f);

    // Pointer notch
    juce::Path p;
    const float pLen = dialRadius * 0.65f;
    p.startNewSubPath (center.x + std::sin (currentAngle) * (dialRadius * 0.15f),
                       center.y - std::cos (currentAngle) * (dialRadius * 0.15f));
    p.lineTo (center.x + std::sin (currentAngle) * pLen,
              center.y - std::cos (currentAngle) * pLen);
    g.setColour (PCColours::text);
    g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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
    const float boxSize = 15.0f;
    auto box = juce::Rectangle<float> (r.getX() + 2.0f, r.getCentreY() - boxSize * 0.5f, boxSize, boxSize);

    g.setColour (PCColours::panel2);
    g.fillRoundedRectangle (box, 3.0f);
    g.setColour (isOver ? PCColours::accent : PCColours::outline);
    g.drawRoundedRectangle (box, 3.0f, 1.0f);

    if (b.getToggleState())
    {
        g.setColour (PCColours::accent);
        g.fillRoundedRectangle (box.reduced (3.0f), 2.0f);
    }

    g.setColour (b.getToggleState() ? PCColours::text : PCColours::textDim);
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (boxSize + 6.0f), juce::Justification::centredLeft);
}

juce::Font PCLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return juce::Font (juce::FontOptions (10.5f, juce::Font::bold));
}

juce::Label* PCLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (juce::Font (juce::FontOptions (10.5f)));
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
    const float ax = w - 14.0f;
    const float ay = h * 0.5f;
    arrow.startNewSubPath (ax - 3.5f, ay - 2.0f);
    arrow.lineTo (ax, ay + 2.0f);
    arrow.lineTo (ax + 3.5f, ay - 2.0f);
    g.setColour (PCColours::textDim);
    g.strokePath (arrow, juce::PathStrokeType (1.5f));
}

void PCLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (1, 1, box.getWidth() - 20, box.getHeight() - 2);
    label.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    label.setColour (juce::Label::textColourId, PCColours::text);
}

// ---------------- 32-Voice Convergence & Constellation Visualizer ----------------

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
    const float pitchConverge = proc.param (IDs::pitchConverge);
    const float timeConverge = proc.param (IDs::timeConverge);
    const float turb = proc.param (IDs::turbulence);

    // Orbital background rings
    for (int r = 1; r <= 3; ++r)
    {
        const float rad = maxRadius * (r / 3.0f);
        g.setColour (PCColours::outline.withAlpha (0.35f));
        g.drawEllipse (center.x - rad, center.y - rad, rad * 2.0f, rad * 2.0f, 1.0f);
    }

    if (space > 0.05f)
    {
        g.setColour (PCColours::accent.withAlpha (0.05f * space));
        g.fillEllipse (center.x - maxRadius, center.y - maxRadius, maxRadius * 2.0f, maxRadius * 2.0f);
    }

    const juce::Colour col = PCColours::swellColour (proc.param (IDs::tone), proc.param (IDs::basscut));

    // Draw active voices as orbital nodes converging toward the center
    for (int v = 0; v < voices; ++v)
    {
        const float vNorm = (voices > 1) ? (float) v / (float) (voices - 1) : 0.5f;
        const float orbitAngle = vNorm * juce::MathConstants<float>::twoPi + phase * (1.0f + orbitAmt * 1.5f);

        // Distance from center reflects dispersion & convergence
        const float distNorm = 0.35f + 0.65f * (1.0f - timeConverge * 0.5f);
        const float radX = maxRadius * distNorm * (0.6f + 0.4f * std::abs ((vNorm * 2.0f - 1.0f) * panSpread));
        const float radY = maxRadius * distNorm * 0.75f;

        const float jitter = (turb > 0.01f) ? std::sin (phase * 6.0f + (float) v) * turb * 6.0f : 0.0f;
        const float x = center.x + std::cos (orbitAngle) * radX + jitter;
        const float y = center.y + std::sin (orbitAngle) * radY;

        // Convergence attraction lines toward center
        g.setColour (col.withAlpha (0.15f + pitchConverge * 0.2f));
        g.drawLine (x, y, center.x, center.y, 1.0f);

        // Voice node dot
        const float nodeSize = 4.5f + 3.5f * liveMeter;
        g.setColour (col.interpolatedWith (PCColours::neon, vNorm));
        g.fillEllipse (x - nodeSize * 0.5f, y - nodeSize * 0.5f, nodeSize, nodeSize);
    }

    // Center focal hit core
    const float coreSize = 10.0f + 16.0f * liveMeter;
    g.setColour (PCColours::hitCol.withAlpha (0.25f));
    g.fillEllipse (center.x - coreSize * 0.5f, center.y - coreSize * 0.5f, coreSize, coreSize);
    g.setColour (PCColours::hitCol);
    g.fillEllipse (center.x - 3.5f, center.y - 3.5f, 7.0f, 7.0f);

    g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    g.setColour (PCColours::textDim);
    g.drawText ("32-VOICE CONSTELLATION", b.withTrimmedTop (5), juce::Justification::centredTop);
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
        g.setColour (juce::Colours::white);
        g.drawVerticalLine ((int) phX, p.getY(), p.getBottom());
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

    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
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
    body.setFont (juce::Font (juce::FontOptions (12.5f)));
    body.setText (
        "PRECHORUS - 32-Voice Swarm & Convergence Engine\n\n"
        "PreChorus transforms live vocals, synth hits, or loaded samples into massive multi-voice swarms "
        "that start dispersed and converge in pitch, time, stereo width, and tone directly into the chorus drop.\n\n"
        "1. SOURCE & CAPTURE ENGINE:\n"
        "• SOURCE MODES: Live Capture, Loaded Sample, Hybrid (layers both), Slice Scatter (voices play different transient slices).\n"
        "• CAPTURE HISTORY: 8 performance memory slots (1-8). Click LOCK to preserve chosen performances.\n"
        "• TIMED CAPTURE: Isolate hits via Threshold or capture musical lengths from 1/16 to 2 bars.\n\n"
        "2. SWARM CONVERGENCE:\n"
        "• VOICES (1 to 32): Scalable from natural vocal doubling to dense cinematic swarms.\n"
        "• TIMING CONVERGE: Voices start at staggered offsets and tighten toward the downbeat.\n"
        "• PITCH CONVERGE: Divergent pitches glide into unison at the climax. Scale lock ensures musicality.\n"
        "• WIDTH CONVERGE: Collapse wide voices to center focus, or bloom outward.\n"
        "• PROGRESSIVE REVEAL: Early voices play short fragment chops; later voices reveal the full phrase.\n"
        "• PHYSICS: Attraction pulls voices in; Turbulence adds flutter; Overshoot bounces past unison; Orbit circulates voices.\n\n"
        "3. WORKFLOW:\n"
        "• PDC ALIGN: Delays output so the climax lands dead-on the MIDI note.\n"
        "• REGEN: Seeds deterministic randomness for identical renders until regenerated.\n"
        "• DRAG TO DAW: Drag the button to drop the rendered swarm WAV directly into your arrangement!\n"
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
    title.setFont (juce::Font (juce::FontOptions (19.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, PCColours::text);
    addAndMakeVisible (title);

    subtitle.setText ("SWARM & CONVERGENCE ENGINE", juce::dontSendNotification);
    subtitle.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    subtitle.setColour (juce::Label::textColourId, PCColours::accent);
    addAndMakeVisible (subtitle);

    fileLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
    fileLabel.setColour (juce::Label::textColourId, PCColours::textDim);
    addAndMakeVisible (fileLabel);

    countLabel.setFont (juce::Font (juce::FontOptions (10.5f)));
    countLabel.setColour (juce::Label::textColourId, PCColours::textDim);
    addAndMakeVisible (countLabel);

    rangeLabel.setText ("RANGE", juce::dontSendNotification);
    rangeLabel.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    rangeLabel.setColour (juce::Label::textColourId, PCColours::textDim);
    rangeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (rangeLabel);

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
        chooser->launchOpenMode ([this] (const juce::FileChooser& fc) {
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
        chooser->launchSaveMode ([this] (const juce::FileChooser& fc) {
            auto f = fc.getResult();
            if (f != juce::File()) proc.exportWav (f.withFileExtension ("wav"));
        });
    };

    // Source Mode Combo
    sourceModeCombo.addItemList (juce::StringArray { "Live Capture", "Loaded Sample", "Hybrid Layer", "Slice Scatter" }, 1);
    addAndMakeVisible (sourceModeCombo);
    sourceModeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::sourceMode, sourceModeCombo);

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

    // 8 History Slot Buttons
    for (int i = 0; i < 8; ++i)
    {
        historySlotButtons[(size_t) i].setButtonText (juce::String (i + 1));
        historySlotButtons[(size_t) i].onClick = [this, i] {
            proc.selectCaptureSlot (i);
            waveform.rebuild();
        };
        addAndMakeVisible (historySlotButtons[(size_t) i]);
    }

    // Scale Lock & Direction Combos
    scaleCombo.addItemList (juce::StringArray { "Chromatic", "Major", "Minor", "Pentatonic", "Oct / 5ths" }, 1);
    addAndMakeVisible (scaleCombo);
    scaleComboAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::scaleLock, scaleCombo);

    dirCombo.addItemList (juce::StringArray { "Forward", "Reverse", "Alternating", "Random" }, 1);
    addAndMakeVisible (dirCombo);
    dirComboAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::voiceDirection, dirCombo);

    // Sync & Alignment
    alignAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, IDs::align, alignToggle);
    syncAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, IDs::sync,  syncToggle);
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

    // Knobs - Convergence Engine
    kTimeSpread    = &makeKnob (IDs::timeSpread,    "TIME SPREAD");
    kTimeConverge  = &makeKnob (IDs::timeConverge,  "TIME CONV");
    kPitchSpread   = &makeKnob (IDs::pitchSpread,   "PITCH SPREAD");
    kDetune        = &makeKnob (IDs::detune,        "DETUNE");
    kPitchConverge = &makeKnob (IDs::pitchConverge, "PITCH CONV");
    kPanSpread     = &makeKnob (IDs::panSpread,     "PAN SPREAD");
    kPanConverge   = &makeKnob (IDs::panConverge,   "WIDTH CONV");
    kToneConverge  = &makeKnob (IDs::toneConverge,  "TONE CONV");

    // Knobs - Physics
    kAttraction = &makeKnob (IDs::attraction, "ATTRACT");
    kTurbulence = &makeKnob (IDs::turbulence, "TURBULENCE");
    kOvershoot  = &makeKnob (IDs::overshoot,  "OVERSHOOT");
    kOrbit      = &makeKnob (IDs::orbit,      "ORBIT");

    // Knobs - Swell & Filter
    kTail  = &makeKnob (IDs::tail,    "LENGTH");
    kShape = &makeKnob (IDs::shape,   "SHAPE");
    kTone  = &makeKnob (IDs::tone,    "TONE");
    kBass  = &makeKnob (IDs::basscut, "BASS CUT");
    kSpace = &makeKnob (IDs::space,   "SPACE");

    // Knobs - Mix & Capture
    kDry        = &makeKnob (IDs::dry,        "HIT DRY");
    kWet        = &makeKnob (IDs::wet,        "SWARM WET");
    kDryReplace = &makeKnob (IDs::dryReplace, "REPLACE");
    kThresh     = &makeKnob (IDs::thresh,     "THRESH");

    // Knobs - Pitch & Volume
    kPitch      = &makeKnob (IDs::pitch,      "PITCH");
    kVolStart   = &makeKnob (IDs::volStart,   "START");
    kVolEnd     = &makeKnob (IDs::volEnd,     "END");
    kVolTension = &makeKnob (IDs::volTension, "TENSION");

    kDry->slider.setColour (juce::Slider::rotarySliderFillColourId, PCColours::hitCol);

    addChildComponent (help);
    setSize (1140, 780);
    startTimerHz (15);
    timerCallback();
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
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 68, 14);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    s.setColour (juce::Slider::rotarySliderFillColourId, PCColours::accent);
    addAndMakeVisible (s);

    k->label.setText (textName, juce::dontSendNotification);
    k->label.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
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
    if (sMode == 0)
        fileLabel.setText ("[Live Input Capture]", juce::dontSendNotification);
    else if (sMode == 1)
        fileLabel.setText (f.existsAsFile() ? f.getFileName() : "[Loaded Sample]", juce::dontSendNotification);
    else if (sMode == 2)
        fileLabel.setText ("[Hybrid Layer: Live + Loaded]", juce::dontSendNotification);
    else
        fileLabel.setText ("[Slice Scatter Ensemble]", juce::dontSendNotification);

    const int n = proc.getSampleCount();
    countLabel.setText (n > 0 ? juce::String (proc.getSampleIndex() + 1) + " / " + juce::String (n) : "", juce::dontSendNotification);

    const bool sync = proc.param (IDs::sync) > 0.5f;
    kTail->slider.setEnabled (! sync);
    kTail->slider.setAlpha (sync ? 0.4f : 1.0f);
    syncCombo.setEnabled (sync);
    syncCombo.setAlpha (sync ? 1.0f : 0.5f);

    // Update History Slot buttons
    const int currentSlot = proc.getActiveCaptureSlot();
    for (int i = 0; i < 8; ++i)
    {
        const bool isCur = (i == currentSlot);
        const bool isFilled = proc.isSlotFilled (i);
        if (isCur)
            historySlotButtons[(size_t) i].setColour (juce::TextButton::buttonColourId, PCColours::accent);
        else if (isFilled)
            historySlotButtons[(size_t) i].setColour (juce::TextButton::buttonColourId, PCColours::neon.withAlpha (0.35f));
        else
            historySlotButtons[(size_t) i].setColour (juce::TextButton::buttonColourId, PCColours::panel2);
    }

    lockButton.setColour (juce::TextButton::buttonColourId, proc.isCaptureLocked() ? PCColours::hitCol : PCColours::panel2);

    // Live Capture state button indicators
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
    for (auto* k : { kTone, kBass, kWet, kTail, kShape, kTimeSpread, kTimeConverge, kPitchConverge, kVoiceCount })
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
        g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
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
        k->label.setBounds (cell.removeFromTop (13));
        k->slider.setBounds (cell);
    }
}

void PreChorusEditor::resized()
{
    help.setBounds (getLocalBounds());
    groups.clear();
    auto area = getLocalBounds().reduced (14);

    // 1. Header Row
    auto header = area.removeFromTop (42);
    auto titleArea = header.removeFromLeft (250);
    title.setBounds (titleArea.removeFromTop (26));
    subtitle.setBounds (titleArea);
    helpButton.setBounds (header.removeFromRight (32).reduced (0, 6));
    header.removeFromRight (8);

    sourceModeCombo.setBounds (header.removeFromRight (120).reduced (0, 6));
    header.removeFromRight (8);

    auto browser = header.withTrimmedLeft (16);
    loadButton.setBounds (browser.removeFromRight (70).reduced (0, 6));
    browser.removeFromRight (5);
    nextButton.setBounds (browser.removeFromRight (32).reduced (0, 6));
    browser.removeFromRight (4);
    prevButton.setBounds (browser.removeFromRight (32).reduced (0, 6));
    browser.removeFromRight (6);
    countLabel.setBounds (browser.removeFromRight (52));
    fileLabel.setBounds (browser.reduced (0, 6));

    // 2. Capture History & Trigger Strip
    area.removeFromTop (6);
    auto capStrip = area.removeFromTop (28);
    armButton.setBounds (capStrip.removeFromLeft (48));      capStrip.removeFromLeft (5);
    captureButton.setBounds (capStrip.removeFromLeft (116)); capStrip.removeFromLeft (6);
    captureCombo.setBounds (capStrip.removeFromLeft (96));   capStrip.removeFromLeft (12);

    for (int i = 0; i < 8; ++i)
    {
        historySlotButtons[(size_t) i].setBounds (capStrip.removeFromLeft (24));
        capStrip.removeFromLeft (3);
    }
    capStrip.removeFromLeft (4);
    lockButton.setBounds (capStrip.removeFromLeft (52));

    // 3. Visualizers Row (32-Voice Constellation + Interactive Waveform)
    area.removeFromTop (8);
    auto vis = area.removeFromTop (215);
    visualizer.setBounds (vis.removeFromLeft (210));
    vis.removeFromLeft (10);
    waveform.setBounds (vis);

    // 4. Transport & Alignment Row
    area.removeFromTop (8);
    auto trans = area.removeFromTop (32);
    playButton.setBounds (trans.removeFromLeft (70));       trans.removeFromLeft (5);
    exportButton.setBounds (trans.removeFromLeft (90));     trans.removeFromLeft (5);
    dragPad.setBounds (trans.removeFromLeft (108));         trans.removeFromLeft (5);
    resetButton.setBounds (trans.removeFromLeft (88));      trans.removeFromLeft (5);
    randomButton.setBounds (trans.removeFromLeft (72));     trans.removeFromLeft (5);
    regenSeedButton.setBounds (trans.removeFromLeft (64));  trans.removeFromLeft (14);

    syncCombo.setBounds (trans.removeFromRight (90));      trans.removeFromRight (5);
    syncToggle.setBounds (trans.removeFromRight (62));     trans.removeFromRight (5);
    alignToggle.setBounds (trans.removeFromRight (134));

    // 5. Knob Panels (Rows A & B)
    area.removeFromTop (10);
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

    // Row A: SWARM VOICES, CONVERGENCE ENGINE, SWELL & FILTER
    const int wA = rowA.getWidth();
    auto swarmGrp = group (rowA, (int) (wA * 0.32f), "SWARM ENGINE (1-32 VOICES)");
    {
        auto rightCombo = swarmGrp.removeFromRight (86);
        dirCombo.setBounds (rightCombo.withSizeKeepingCentre (82, 24));
        layoutKnobs (swarmGrp, { kVoiceCount, kVoiceDensity, kVoiceAge, kProgReveal });
    }

    auto convGrp = group (rowA, (int) (wA * 0.44f), "CONVERGENCE ENGINE");
    {
        auto rightCombo = convGrp.removeFromRight (86);
        scaleCombo.setBounds (rightCombo.withSizeKeepingCentre (82, 24));
        layoutKnobs (convGrp, { kTimeSpread, kTimeConverge, kPitchSpread, kDetune, kPitchConverge, kPanSpread, kPanConverge, kToneConverge });
    }

    layoutKnobs (group (rowA, rowA.getWidth(), "SWELL & FILTER"), { kTail, kShape, kTone, kBass, kSpace });

    // Row B: PHYSICS & MOTION, MIX & REPLACE, PITCH SWEEP, VOLUME TENSION
    const int wB = rowB.getWidth();
    layoutKnobs (group (rowB, (int) (wB * 0.28f), "PHYSICS & MODULATION"), { kAttraction, kTurbulence, kOvershoot, kOrbit });
    layoutKnobs (group (rowB, (int) (wB * 0.24f), "MIX & CAPTURE"), { kDry, kWet, kDryReplace, kThresh });

    auto pitchArea = group (rowB, (int) (wB * 0.20f), "PITCH SWEEP");
    {
        auto right = pitchArea.removeFromRight (68);
        rangeLabel.setBounds (right.removeFromTop (13));
        rangeCombo.setBounds (right.removeFromTop (24).reduced (2, 0));
        right.removeFromTop (3);
        pitchTension.setBounds (right.withSizeKeepingCentre (58, juce::jmin (58, right.getHeight())));
        layoutKnobs (pitchArea, { kPitch });
    }

    layoutKnobs (group (rowB, rowB.getWidth(), "VOLUME ENVELOPE (FL Tension)"), { kVolStart, kVolEnd, kVolTension });
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
