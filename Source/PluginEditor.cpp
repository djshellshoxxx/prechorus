#include "PluginEditor.h"

namespace PCColours
{
    juce::Colour swellColour (float toneHz, float bassCutHz)
    {
        // Dynamic color shifting from warm magenta/violet to bright electric cyan/gold
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
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto center = bounds.getCentre();
    const float arcRadius = radius - 5.0f;
    const float currentAngle = startAngle + pos * (endAngle - startAngle);

    // Track background
    juce::Path bgArc;
    bgArc.addCentredArc (center.x, center.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (PCColours::outline);
    g.strokePath (bgArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value Arc
    if (pos > 0.001f)
    {
        juce::Path valArc;
        valArc.addCentredArc (center.x, center.y, arcRadius, arcRadius, 0.0f, startAngle, currentAngle, true);
        g.setColour (s.findColour (juce::Slider::rotarySliderFillColourId));
        g.strokePath (valArc, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Dial body
    const float dialRadius = arcRadius - 6.0f;
    g.setColour (PCColours::panel2);
    g.fillEllipse (center.x - dialRadius, center.y - dialRadius, dialRadius * 2.0f, dialRadius * 2.0f);
    g.setColour (PCColours::outline.brighter (0.1f));
    g.drawEllipse (center.x - dialRadius, center.y - dialRadius, dialRadius * 2.0f, dialRadius * 2.0f, 1.0f);

    // Pointer notch
    juce::Path p;
    const float pLen = dialRadius * 0.65f;
    p.startNewSubPath (center.x + std::sin (currentAngle) * (dialRadius * 0.2f),
                       center.y - std::cos (currentAngle) * (dialRadius * 0.2f));
    p.lineTo (center.x + std::sin (currentAngle) * pLen,
              center.y - std::cos (currentAngle) * pLen);
    g.setColour (PCColours::text);
    g.strokePath (p, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (b.getToggleState() ? PCColours::accent : PCColours::outline);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
}

void PCLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool isOver, bool)
{
    auto r = b.getLocalBounds().toFloat();
    const float boxSize = 16.0f;
    auto box = juce::Rectangle<float> (r.getX() + 2.0f, r.getCentreY() - boxSize * 0.5f, boxSize, boxSize);

    g.setColour (PCColours::panel2);
    g.fillRoundedRectangle (box, 3.0f);
    g.setColour (isOver ? PCColours::accent : PCColours::outline);
    g.drawRoundedRectangle (box, 3.0f, 1.0f);

    if (b.getToggleState())
    {
        g.setColour (PCColours::accent);
        g.fillRoundedRectangle (box.reduced (3.5f), 2.0f);
    }

    g.setColour (b.getToggleState() ? PCColours::text : PCColours::textDim);
    g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (boxSize + 8.0f), juce::Justification::centredLeft);
}

juce::Font PCLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return juce::Font (juce::FontOptions (11.0f, juce::Font::bold));
}

juce::Label* PCLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (juce::Font (juce::FontOptions (11.0f)));
    l->setColour (juce::Label::textColourId, PCColours::textDim);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    return l;
}

void PCLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool isDown, int, int, int, int, juce::ComboBox& b)
{
    auto r = juce::Rectangle<int> (0, 0, w, h).toFloat().reduced (0.5f);
    g.setColour (isDown ? PCColours::panel2.brighter (0.1f) : PCColours::panel2);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (PCColours::outline);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);

    // Arrow icon
    juce::Path arrow;
    const float ax = w - 16.0f;
    const float ay = h * 0.5f;
    arrow.startNewSubPath (ax - 4.0f, ay - 2.0f);
    arrow.lineTo (ax, ay + 2.0f);
    arrow.lineTo (ax + 4.0f, ay - 2.0f);
    g.setColour (PCColours::textDim);
    g.strokePath (arrow, juce::PathStrokeType (1.5f));
}

void PCLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (1, 1, box.getWidth() - 22, box.getHeight() - 2);
    label.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    label.setColour (juce::Label::textColourId, PCColours::text);
}

// ---------------- Voice Orbit Visualizer ----------------

void VoiceOrbitVisualizer::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (4.0f);
    g.setColour (PCColours::panel);
    g.fillRoundedRectangle (b, 8.0f);
    g.setColour (PCColours::outline);
    g.drawRoundedRectangle (b, 8.0f, 1.0f);

    const auto center = b.getCentre();
    const float maxRadius = juce::jmin (b.getWidth(), b.getHeight()) * 0.42f;

    const int voices = juce::jlimit (2, 12, (int) proc.param (IDs::voiceCount));
    const float spread = proc.param (IDs::panSpread);
    const float revAmt = proc.param (IDs::reverse);
    const float space = proc.param (IDs::space);
    const float liveMeter = proc.getLiveInputMeter();

    // Orbital background rings
    for (int r = 1; r <= 3; ++r)
    {
        const float rad = maxRadius * (r / 3.0f);
        g.setColour (PCColours::outline.withAlpha (0.4f));
        g.drawEllipse (center.x - rad, center.y - rad, rad * 2.0f, rad * 2.0f, 1.0f);
    }

    // Ambient space glow
    if (space > 0.05f)
    {
        g.setColour (PCColours::accent.withAlpha (0.06f * space));
        g.fillEllipse (center.x - maxRadius, center.y - maxRadius, maxRadius * 2.0f, maxRadius * 2.0f);
    }

    // Voice orbital nodes
    const juce::Colour col = PCColours::swellColour (proc.param (IDs::tone), proc.param (IDs::basscut));
    for (int v = 0; v < voices; ++v)
    {
        const float angleNorm = (float) v / (float) voices;
        const float rot = angleNorm * juce::MathConstants<float>::twoPi + phase * (revAmt > 0.5f ? -1.0f : 1.0f);
        const float panOffset = ((float) v / (float) juce::jmax (1, voices - 1) * 2.0f - 1.0f) * spread;

        const float radX = maxRadius * (0.55f + 0.45f * std::abs (panOffset));
        const float radY = maxRadius * 0.7f;

        const float x = center.x + std::cos (rot) * radX;
        const float y = center.y + std::sin (rot) * radY;

        // Draw connecting vector from core
        g.setColour (col.withAlpha (0.25f));
        g.drawLine (center.x, center.y, x, y, 1.0f);

        // Voice node dot
        const float nodeSize = 6.0f + 5.0f * liveMeter;
        g.setColour (col.interpolatedWith (PCColours::neon, angleNorm));
        g.fillEllipse (x - nodeSize * 0.5f, y - nodeSize * 0.5f, nodeSize, nodeSize);
    }

    // Center focal core
    const float coreSize = 12.0f + 16.0f * liveMeter;
    g.setColour (PCColours::hitCol.withAlpha (0.3f));
    g.fillEllipse (center.x - coreSize * 0.5f, center.y - coreSize * 0.5f, coreSize, coreSize);
    g.setColour (PCColours::hitCol);
    g.fillEllipse (center.x - 4.0f, center.y - 4.0f, 8.0f, 8.0f);

    // Title label inside visualizer
    g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    g.setColour (PCColours::textDim);
    g.drawText ("VOICE CONSTELLATION", b.withTrimmedTop (6), juce::Justification::centredTop);
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

    // Build swell wave path
    swellPath.startNewSubPath (p.getX(), midY);
    for (float px = 0; px < w; ++px)
    {
        const int s0 = (int) ((px / w) * numSamples);
        const int s1 = (int) (((px + 1.0f) / w) * numSamples);
        if (s0 >= split) break;

        float maxV = 0.0f;
        for (int i = s0; i < juce::jmin (s1, split); ++i)
        {
            maxV = juce::jmax (maxV, std::abs (l[i]), std::abs (r[i]));
        }
        swellPath.lineTo (p.getX() + px, midY - maxV * halfH);
    }
    for (float px = w - 1.0f; px >= 0.0f; --px)
    {
        const int s0 = (int) ((px / w) * numSamples);
        const int s1 = (int) (((px + 1.0f) / w) * numSamples);
        if (s0 >= split) continue;

        float maxV = 0.0f;
        for (int i = s0; i < juce::jmin (s1, split); ++i)
        {
            maxV = juce::jmax (maxV, std::abs (l[i]), std::abs (r[i]));
        }
        swellPath.lineTo (p.getX() + px, midY + maxV * halfH);
    }
    swellPath.closeSubPath();

    // Build hit wave path (if climax hit is present)
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

    // Beat grid lines
    if (cached != nullptr && cached->beats > 0)
    {
        g.setColour (PCColours::outline.withAlpha (0.45f));
        for (int i = 1; i < cached->beats; ++i)
        {
            const float x = p.getX() + (float) i / (float) cached->beats * p.getWidth();
            g.drawVerticalLine ((int) x, p.getY(), p.getBottom());
        }
    }

    // Midline
    g.setColour (PCColours::outline.withAlpha (0.3f));
    g.drawHorizontalLine ((int) p.getCentreY(), p.getX(), p.getRight());

    // Swell Waveform
    const juce::Colour col = PCColours::swellColour (proc.param (IDs::tone), proc.param (IDs::basscut));
    g.setColour (col.withAlpha (0.85f));
    g.fillPath (swellPath);

    // Hit Waveform
    if (! hitPath.isEmpty())
    {
        g.setColour (PCColours::hitCol.withAlpha (0.9f));
        g.fillPath (hitPath);
    }

    // Volume Tension Envelope Line
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

    // Envelope points
    g.setColour (PCColours::neon);
    g.fillEllipse (p.getX() - 4.0f, volY (v0) - 4.0f, 8.0f, 8.0f);
    g.fillEllipse (p.getRight() - 4.0f, volY (v1) - 4.0f, 8.0f, 8.0f);

    // Tension midpoint
    const float midLvl = v0 + (v1 - v0) * tensionCurve (0.5f, vt);
    g.setColour (PCColours::accent);
    g.fillEllipse (p.getCentreX() - 3.5f, volY (midLvl) - 3.5f, 7.0f, 7.0f);

    // Trim markers
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

    // Playhead line
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

    // Check interaction with envelope dots
    const float v0 = proc.param (IDs::volStart);
    const float v1 = proc.param (IDs::volEnd);
    const float vt = proc.param (IDs::volTension);
    const float midLvl = v0 + (v1 - v0) * tensionCurve (0.5f, vt);

    if (e.position.getDistanceFrom ({ p.getX(), volY (v0) }) < 12.0f)
    {
        drag = Drag::volStart;
    }
    else if (e.position.getDistanceFrom ({ p.getRight(), volY (v1) }) < 12.0f)
    {
        drag = Drag::volEnd;
    }
    else if (e.position.getDistanceFrom ({ p.getCentreX(), volY (midLvl) }) < 12.0f)
    {
        drag = Drag::volTension;
        downA = vt;
    }
    else
    {
        drag = Drag::none;
        proc.triggerPreview();
    }
}

void WaveformDisplay::mouseDrag (const juce::MouseEvent& e)
{
    moved = true;
    auto p = plot();
    const float lvl = juce::jlimit (0.0f, 1.0f, (p.getBottom() - e.position.y) / p.getHeight());

    if (drag == Drag::volStart)
    {
        proc.setParam (IDs::volStart, lvl);
        repaint();
    }
    else if (drag == Drag::volEnd)
    {
        proc.setParam (IDs::volEnd, lvl);
        repaint();
    }
    else if (drag == Drag::volTension)
    {
        const float diff = (downPos.y - e.position.y) / 50.0f;
        proc.setParam (IDs::volTension, juce::jlimit (-1.0f, 1.0f, downA + diff));
        repaint();
    }
}

void WaveformDisplay::mouseUp (const juce::MouseEvent&)
{
    drag = Drag::none;
}

void WaveformDisplay::mouseMove (const juce::MouseEvent&)
{
}

// ---------------- Drag Out Pad ----------------

void DragOutPad::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (over ? PCColours::panel2.brighter (0.15f) : PCColours::panel2);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (over ? PCColours::neon : PCColours::outline);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);

    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.setColour (over ? PCColours::neon : PCColours::text);
    g.drawText ("DRAG TO DAW", getLocalBounds(), juce::Justification::centred);
}

void DragOutPad::mouseDrag (const juce::MouseEvent&)
{
    if (dragging) return;
    dragging = true;
    auto tempFile = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("PreChorus_Export.wav");
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
    body.setFont (juce::Font (juce::FontOptions (13.0f)));
    body.setText (
        "PRECHORUS - Multi-Voice Anticipation & Swell Engine\n\n"
        "PreChorus transforms incoming audio (vocals, synths, one-shots, words) into lush, swelling "
        "multi-voice choral risers that build up energy right before the chorus drop.\n\n"
        "KEY FEATURES:\n"
        "• LIVE CAPTURE: Click ARM to capture incoming audio. In Threshold mode, it auto-detects when you sing or hit a note. "
        "Or use Beat Sync (1 Beat, 2 Beats, 1 Bar, 2 Bars) to capture timed phrases.\n"
        "• VOICE ENGINE: Generates 2 to 12 sample voices with customizable timing spread, micro-detune, "
        "stereo fanning, and musical harmony stacks (Octaves, 5ths, Choral chords).\n"
        "• REVERSE & SWELL: Blend between forward choral bloom and reversed vocal swell into the drop.\n"
        "• PDC ALIGN: Plug-in delay compensation ensures the dry hit lands exactly on the MIDI note/downbeat.\n"
        "• TENSION ENVELOPES: FL-style volume and pitch tension curves overlaid on the interactive waveform.\n"
        "• DRAG TO DAW: Grab the DRAG TO DAW button to export and drop the generated swell directly into your arrangement!\n"
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
    auto r = getLocalBounds().reduced (65);
    closeButton.setBounds (r.removeFromBottom (34).withSizeKeepingCentre (120, 32));
    r.removeFromBottom (12);
    body.setBounds (r);
}

// ---------------- PreChorus Editor ----------------

PreChorusEditor::PreChorusEditor (PreChorusProcessor& p)
    : AudioProcessorEditor (&p), proc (p), waveform (p), visualizer (p), dragPad (p),
      pitchTension (p, IDs::pitchTension)
{
    setLookAndFeel (&lnf);

    title.setText ("PRECHORUS", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, PCColours::text);
    addAndMakeVisible (title);

    subtitle.setText ("MULTI-VOICE SWELL & CAPTURE ENGINE", juce::dontSendNotification);
    subtitle.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    subtitle.setColour (juce::Label::textColourId, PCColours::accent);
    addAndMakeVisible (subtitle);

    fileLabel.setFont (juce::Font (juce::FontOptions (11.5f)));
    fileLabel.setColour (juce::Label::textColourId, PCColours::textDim);
    addAndMakeVisible (fileLabel);

    countLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
    countLabel.setColour (juce::Label::textColourId, PCColours::textDim);
    addAndMakeVisible (countLabel);

    rangeLabel.setText ("RANGE", juce::dontSendNotification);
    rangeLabel.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
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
    addAndMakeVisible (helpButton);

    prevButton.onClick = [this] { proc.prevSample(); waveform.rebuild(); };
    nextButton.onClick = [this] { proc.nextSample(); waveform.rebuild(); };
    loadButton.onClick = [this] {
        chooser = std::make_unique<juce::FileChooser> ("Load Sample / Vocal Slice", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        chooser->launchOpenMode ([this] (const juce::FileChooser& fc) {
            auto f = fc.getResult();
            if (f.existsAsFile()) { proc.loadSampleFile (f, true); waveform.rebuild(); }
        });
    };
    playButton.onClick   = [this] { proc.triggerPreview(); };
    resetButton.onClick  = [this] { proc.resetEdits(); waveform.rebuild(); };
    randomButton.onClick = [this] { proc.randomizePreChorus(); waveform.rebuild(); };
    helpButton.onClick   = [this] { help.setVisible (true); };

    exportButton.onClick = [this] {
        chooser = std::make_unique<juce::FileChooser> ("Export PreChorus Swell WAV", juce::File(), "*.wav");
        chooser->launchSaveMode ([this] (const juce::FileChooser& fc) {
            auto f = fc.getResult();
            if (f != juce::File()) proc.exportWav (f.withFileExtension ("wav"));
        });
    };

    // Live Capture Controls
    addAndMakeVisible (armButton);
    addAndMakeVisible (captureButton);
    armButton.onClick = [this] {
        if (proc.isCaptureArmed()) proc.stopCapture();
        else proc.armCapture();
    };
    captureButton.onClick = [this] {
        proc.triggerManualCapture();
        waveform.rebuild();
    };

    captureCombo.addItemList (juce::StringArray { "Threshold", "1 Beat", "2 Beats", "1 Bar", "2 Bars", "Manual" }, 1);
    addAndMakeVisible (captureCombo);
    captureComboAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::captureMode, captureCombo);

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

    harmonyCombo.addItemList (juce::StringArray { "Unison", "Octaves", "Power 5ths", "Choral Stack" }, 1);
    addAndMakeVisible (harmonyCombo);
    harmonyComboAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, IDs::harmony, harmonyCombo);

    // Visualizers
    addAndMakeVisible (visualizer);
    addAndMakeVisible (waveform);
    addAndMakeVisible (dragPad);
    addAndMakeVisible (pitchTension);

    // Knobs
    kVoices    = &makeKnob (IDs::voiceCount, "VOICES");
    kSpread    = &makeKnob (IDs::spread,     "SPREAD");
    kDetune    = &makeKnob (IDs::detune,     "DETUNE");
    kPanSpread = &makeKnob (IDs::panSpread,  "STEREO PAN");
    kSpace     = &makeKnob (IDs::space,      "SPACE");
    kReverse   = &makeKnob (IDs::reverse,    "REVERSE");

    kTail      = &makeKnob (IDs::tail,       "LENGTH");
    kShape     = &makeKnob (IDs::shape,      "SHAPE");
    kTone      = &makeKnob (IDs::tone,       "TONE");
    kBass      = &makeKnob (IDs::basscut,    "BASS CUT");

    kThresh    = &makeKnob (IDs::thresh,     "THRESH");
    kDry       = &makeKnob (IDs::dry,        "HIT / DRY");
    kWet       = &makeKnob (IDs::wet,        "SWELL / WET");

    kPitch      = &makeKnob (IDs::pitch,      "PITCH");
    kVolStart   = &makeKnob (IDs::volStart,   "START");
    kVolEnd     = &makeKnob (IDs::volEnd,     "END");
    kVolTension = &makeKnob (IDs::volTension, "TENSION");

    kDry->slider.setColour (juce::Slider::rotarySliderFillColourId, PCColours::hitCol);

    addChildComponent (help);
    setSize (1080, 730);
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
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 15);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    s.setColour (juce::Slider::rotarySliderFillColourId, PCColours::accent);
    addAndMakeVisible (s);

    k->label.setText (textName, juce::dontSendNotification);
    k->label.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
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
    if (f.existsAsFile()) fileLabel.setText (f.getFileName(), juce::dontSendNotification);
    else fileLabel.setText ("[Live Input Captured / Default]", juce::dontSendNotification);

    const int n = proc.getSampleCount();
    countLabel.setText (n > 0 ? juce::String (proc.getSampleIndex() + 1) + " / " + juce::String (n) : "", juce::dontSendNotification);

    const bool sync = proc.param (IDs::sync) > 0.5f;
    kTail->slider.setEnabled (! sync);
    kTail->slider.setAlpha (sync ? 0.4f : 1.0f);
    syncCombo.setEnabled (sync);
    syncCombo.setAlpha (sync ? 1.0f : 0.5f);

    // Update Live Capture button indicators
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
    for (auto* k : { kTone, kBass, kWet, kTail, kShape, kSpread, kDetune, kVoices })
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
    juce::ColourGradient grad (PCColours::bg.brighter (0.06f), 0.0f, 0.0f, PCColours::bg, 0.0f, (float) getHeight(), false);
    g.setGradientFill (grad);
    g.fillAll();

    const juce::Colour col = PCColours::swellColour (proc.param (IDs::tone), proc.param (IDs::basscut));
    g.setColour (col.withAlpha (0.05f));
    g.fillEllipse (-120.0f, -150.0f, 480.0f, 360.0f);
    g.setColour (PCColours::hitCol.withAlpha (0.04f));
    g.fillEllipse ((float) getWidth() - 340.0f, (float) getHeight() - 280.0f, 480.0f, 340.0f);

    for (auto& gr : groups)
    {
        auto r = gr.bounds.toFloat();
        g.setColour (PCColours::panel.withAlpha (0.75f));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (PCColours::outline);
        g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.setColour (PCColours::textDim);
        g.drawText (gr.name, gr.bounds.withHeight (18).withTrimmedLeft (12), juce::Justification::centredLeft);
    }
}

void PreChorusEditor::layoutKnobs (juce::Rectangle<int> area, std::initializer_list<Knob*> ks)
{
    const int kw = area.getWidth() / (int) ks.size();
    for (auto* k : ks)
    {
        auto cell = area.removeFromLeft (kw);
        k->label.setBounds (cell.removeFromTop (14));
        k->slider.setBounds (cell);
    }
}

void PreChorusEditor::resized()
{
    help.setBounds (getLocalBounds());
    groups.clear();
    auto area = getLocalBounds().reduced (16);

    // 1. Header Row
    auto header = area.removeFromTop (46);
    auto titleArea = header.removeFromLeft (260);
    title.setBounds (titleArea.removeFromTop (28));
    subtitle.setBounds (titleArea);
    helpButton.setBounds (header.removeFromRight (34).reduced (0, 7));
    header.removeFromRight (10);

    auto browser = header.withTrimmedLeft (20);
    loadButton.setBounds (browser.removeFromRight (74).reduced (0, 7));
    browser.removeFromRight (6);
    nextButton.setBounds (browser.removeFromRight (36).reduced (0, 7));
    browser.removeFromRight (4);
    prevButton.setBounds (browser.removeFromRight (36).reduced (0, 7));
    browser.removeFromRight (8);
    countLabel.setBounds (browser.removeFromRight (56));
    fileLabel.setBounds (browser.reduced (0, 7));

    // 2. Visualizers Row (Constellation Orbit + Waveform)
    area.removeFromTop (10);
    auto vis = area.removeFromTop (230);
    visualizer.setBounds (vis.removeFromLeft (220));
    vis.removeFromLeft (10);
    waveform.setBounds (vis);

    // 3. Transport & Live Capture Row
    area.removeFromTop (10);
    auto row = area.removeFromTop (34);
    playButton.setBounds (row.removeFromLeft (76));       row.removeFromLeft (6);
    exportButton.setBounds (row.removeFromLeft (96));     row.removeFromLeft (6);
    dragPad.setBounds (row.removeFromLeft (116));        row.removeFromLeft (6);
    resetButton.setBounds (row.removeFromLeft (96));     row.removeFromLeft (6);
    randomButton.setBounds (row.removeFromLeft (76));    row.removeFromLeft (14);

    // Live Capture triggers on transport row
    armButton.setBounds (row.removeFromLeft (56));       row.removeFromLeft (6);
    captureButton.setBounds (row.removeFromLeft (130));  row.removeFromLeft (6);
    captureCombo.setBounds (row.removeFromLeft (100));   row.removeFromLeft (10);

    syncCombo.setBounds (row.removeFromRight (94));      row.removeFromRight (6);
    syncToggle.setBounds (row.removeFromRight (64));     row.removeFromRight (6);
    alignToggle.setBounds (row.removeFromRight (140));

    // 4. Knob Rows
    area.removeFromTop (12);
    const int rowH = (area.getHeight() - 10) / 2;
    auto rowA = area.removeFromTop (rowH);
    area.removeFromTop (10);
    auto rowB = area;

    auto group = [&] (juce::Rectangle<int>& src, int width, const juce::String& name)
    {
        auto r = src.removeFromLeft (width);
        src.removeFromLeft (8);
        groups.push_back ({ name, r });
        return r.reduced (6).withTrimmedTop (14);
    };

    // Row A: VOICE ENGINE & SWELL
    const int wA = rowA.getWidth();
    const int wVoice = (int) (wA * 0.65f);
    auto vEngine = group (rowA, wVoice, "VOICE ENGINE");
    {
        auto harmArea = vEngine.removeFromRight (100);
        harmonyCombo.setBounds (harmArea.withSizeKeepingCentre (94, 26));
        layoutKnobs (vEngine, { kVoices, kSpread, kDetune, kPanSpread, kSpace, kReverse });
    }
    layoutKnobs (group (rowA, rowA.getWidth(), "SWELL & FILTER"), { kTail, kShape, kTone, kBass });

    // Row B: MIX, CAPTURE THRESH, PITCH, VOLUME
    const int totalB = rowB.getWidth() - 8 * 3;
    const int unitB = totalB / 11;
    layoutKnobs (group (rowB, unitB * 2, "MIX"), { kDry, kWet });
    layoutKnobs (group (rowB, unitB * 2, "CAPTURE SENSE"), { kThresh });

    auto pitchArea = group (rowB, unitB * 3, "PITCH SWEEP");
    {
        auto right = pitchArea.removeFromRight (74);
        rangeLabel.setBounds (right.removeFromTop (14));
        rangeCombo.setBounds (right.removeFromTop (26).reduced (2, 0));
        right.removeFromTop (4);
        pitchTension.setBounds (right.withSizeKeepingCentre (64, juce::jmin (64, right.getHeight())));
        layoutKnobs (pitchArea, { kPitch });
    }

    layoutKnobs (group (rowB, rowB.getWidth(), "VOLUME ENVELOPE (FL-Style Tension)"), { kVolStart, kVolEnd, kVolTension });
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
