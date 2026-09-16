#include "PluginEditor.h"
#include "Tooltips.h"

namespace PCColours
{
    juce::Colour swellColour (float toneHz, float bassCutHz)
    {
        const float toneNorm = juce::jlimit (0.0f, 1.0f, (std::log10 (juce::jmax (200.0f, toneHz)) - 2.3f) / 2.0f);
        const float bassNorm = juce::jlimit (0.0f, 1.0f, (std::log10 (juce::jmax (20.0f, bassCutHz)) - 1.3f) / 1.7f);

        // Dark tone reads as the secondary teal, bright tone as the primary accent,
        // and a higher bass cut warms it towards the warning amber.
        auto blend = neon.interpolatedWith (accent, toneNorm);
        return blend.interpolatedWith (hitCol, bassNorm * 0.35f);
    }
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

juce::Point<float> VoiceOrbitVisualizer::toPixels (float nx, float ny) const
{
    return { centre.x + nx * maxRadius, centre.y + ny * maxRadius * 0.82f };
}

juce::Point<float> VoiceOrbitVisualizer::toNormalised (juce::Point<float> px) const
{
    if (maxRadius <= 0.0f) return { 0.0f, 0.0f };
    return { (px.x - centre.x) / maxRadius,
             (px.y - centre.y) / (maxRadius * 0.82f) };
}

void VoiceOrbitVisualizer::mouseDown (const juce::MouseEvent& e)
{
    // Three right-clicks breed a red orb, four left-clicks split three orbs,
    // two middle-clicks leave one of them carrying a clutch of eggs.
    if (e.mods.isPopupMenu() || e.mods.isRightButtonDown()) proc.colonyRightClick();
    else if (e.mods.isMiddleButtonDown())                   proc.colonyMiddleClick();
    else
    {
        proc.colonyLeftClick();
        const auto n = toNormalised (e.position);
        proc.colonyClickAt (n.x, n.y);      // a yellow orb lights its fuse
        proc.colonyBeginDrag (n.x, n.y);
        dragging = proc.getColony().isDragging();
    }

    repaint();
}

void VoiceOrbitVisualizer::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging) return;
    const auto n = toNormalised (e.position);
    proc.colonyDragTo (n.x, n.y);
    repaint();
}

void VoiceOrbitVisualizer::mouseUp (const juce::MouseEvent&)
{
    if (! dragging) return;
    dragging = false;
    proc.colonyEndDrag();
    repaint();
}

void VoiceOrbitVisualizer::drawFractal (juce::Graphics& g, juce::Point<float> from, float angle,
                                        float len, int depth, juce::Colour col, float alpha) const
{
    if (depth <= 0 || len < 1.5f) return;

    const juce::Point<float> to { from.x + std::cos (angle) * len,
                                  from.y + std::sin (angle) * len };
    g.setColour (col.withAlpha (alpha));
    g.drawLine (from.x, from.y, to.x, to.y, juce::jmax (0.6f, len * 0.05f));

    const float spread = 0.55f + 0.12f * (float) depth;
    drawFractal (g, to, angle - spread, len * 0.62f, depth - 1, col, alpha * 0.72f);
    drawFractal (g, to, angle + spread, len * 0.62f, depth - 1, col, alpha * 0.72f);
}

void VoiceOrbitVisualizer::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (PCTheme::panel);
    g.fillRoundedRectangle (b, PCTheme::Metrics::radiusPanel);
    g.setColour (PCTheme::edge);
    g.drawRoundedRectangle (b.reduced (0.5f), PCTheme::Metrics::radiusPanel, 1.0f);

    auto field = b.reduced (6.0f).withTrimmedTop (14.0f).withTrimmedBottom (14.0f);
    centre = field.getCentre();
    maxRadius = juce::jmin (field.getWidth(), field.getHeight()) * 0.46f;

    const auto& colony = proc.getColony();
    const float outLvl = juce::jlimit (0.0f, 1.0f, smoothedOut * 1.8f);
    const float liveMeter = proc.getLiveInputMeter();
    const float space = proc.param (IDs::space);
    const float ghost = proc.getGhostAmount();
    const bool isFrozen = proc.param (IDs::freeze) > 0.5f;

    const juce::Colour baseCol = isFrozen
        ? PCColours::freezeCol
        : PCColours::swellColour (proc.param (IDs::tone), proc.param (IDs::basscut));

    // Orbital guide rings
    for (int r = 1; r <= 3; ++r)
    {
        const float rad = maxRadius * ((float) r / 3.0f);
        g.setColour (PCTheme::edge.interpolatedWith (PCColours::neon, outLvl * 0.5f)
                                  .withAlpha (0.32f + outLvl * 0.25f));
        g.drawEllipse (centre.x - rad, centre.y - rad * 0.82f, rad * 2.0f, rad * 1.64f, 1.0f);
    }

    if (space > 0.05f)
    {
        g.setColour (PCColours::accent.withAlpha (0.05f * space));
        g.fillEllipse (centre.x - maxRadius, centre.y - maxRadius * 0.82f, maxRadius * 2.0f, maxRadius * 1.64f);
    }

    // --- Eggs ---------------------------------------------------------------
    for (const auto& egg : colony.getEggs())
    {
        if (! egg.alive) continue;
        const auto p = toPixels (egg.x, egg.y);
        const float imminent = juce::jlimit (0.0f, 1.0f, 1.0f - egg.hatchIn / 6.0f);
        const float w = 5.0f + imminent * 2.5f;
        const float h = w * 1.35f;
        const auto hue = juce::Colour::fromHSV (egg.hue, 0.45f, 0.92f, 1.0f);

        g.setColour (hue.withAlpha (0.16f + imminent * 0.3f));
        g.fillEllipse (p.x - w, p.y - h, w * 2.0f, h * 2.0f);
        g.setColour (hue.withAlpha (0.55f + imminent * 0.45f));
        g.drawEllipse (p.x - w * 0.5f, p.y - h * 0.5f, w, h, 1.0f);
    }

    // --- Orbs ---------------------------------------------------------------
    const auto& orbs = colony.getOrbs();
    int drawn = 0;
    for (int i = 0; i < Colony::kMaxOrbs; ++i)
    {
        const auto& o = orbs[(size_t) i];
        if (! o.alive) continue;

        const auto p = toPixels (o.x, o.y);
        const float vNorm = (float) (drawn % 16) / 15.0f;
        ++drawn;

        juce::Colour col;
        if (o.kind == Colony::Kind::red)        col = PCTheme::clipRed;
        else if (o.kind == Colony::Kind::green) col = PCTheme::signalOn;
        else if (o.hatchling)                   col = juce::Colour::fromHSV (o.hue, 0.72f, 1.0f, 1.0f);
        else col = baseCol.interpolatedWith (PCColours::neon, vNorm)
                          .interpolatedWith (PCColours::hitCol, outLvl * 0.6f);

        const float alpha = juce::jlimit (0.0f, 1.0f, o.fade) * juce::jlimit (0.2f, 1.0f, o.birth);

        // Ghost tracers, left behind while the distress echo lasts
        if (ghost > 0.01f && o.trailCount > 0)
        {
            for (int t = 0; t < o.trailCount; ++t)
            {
                const int idx = (o.trailWrite - 1 - t + Colony::Orb::kTrail * 2) % Colony::Orb::kTrail;
                const auto tp = toPixels (o.trail[(size_t) idx].x, o.trail[(size_t) idx].y);
                const float k = 1.0f - (float) t / (float) Colony::Orb::kTrail;
                const float r = (2.6f + 2.4f * o.size) * k;
                g.setColour (col.withAlpha (alpha * ghost * k * 0.34f));
                g.fillEllipse (tp.x - r, tp.y - r, r * 2.0f, r * 2.0f);
            }
        }

        // Tether back to the target
        g.setColour (col.withAlpha (alpha * (0.12f + outLvl * 0.28f)));
        g.drawLine (p.x, p.y, centre.x, centre.y, 1.0f + outLvl * 1.1f);

        const float nodeSize = (3.2f + 2.6f * liveMeter + 4.6f * outLvl) * o.size;

        if (o.kind == Colony::Kind::green)
        {
            g.setColour (col.withAlpha (alpha * 0.28f));
            g.fillEllipse (p.x - nodeSize * 1.9f, p.y - nodeSize * 1.9f, nodeSize * 3.8f, nodeSize * 3.8f);
        }

        g.setColour (col.withAlpha (alpha));
        g.fillEllipse (p.x - nodeSize * 0.5f, p.y - nodeSize * 0.5f, nodeSize, nodeSize);

        // The orb in hand gets a halo that reddens as you haul it faster
        if (i == colony.getDraggedOrb())
        {
            const float heat = juce::jlimit (0.0f, 1.0f, colony.getDragSpeed() / Colony::kFastDragSpeed);
            const float ringR = nodeSize * 2.2f;
            g.setColour (PCTheme::accent.interpolatedWith (PCTheme::clipRed, heat).withAlpha (0.85f));
            g.drawEllipse (p.x - ringR, p.y - ringR, ringR * 2.0f, ringR * 2.0f, 1.4f);
        }

        // A carrying orb wears a ring until the clutch is laid
        if (o.pregnant)
        {
            const float ringR = nodeSize * 1.5f;
            g.setColour (PCTheme::warning.withAlpha (alpha * 0.85f));
            g.drawEllipse (p.x - ringR, p.y - ringR, ringR * 2.0f, ringR * 2.0f, 1.2f);
        }
    }

    // --- Fractals, thrown off by a slow drag --------------------------------
    const float fractal = colony.getFractalAmount();
    if (fractal > 0.01f)
    {
        const int dragged = colony.getDraggedOrb();
        for (int i = 0; i < Colony::kMaxOrbs; ++i)
        {
            const auto& o = orbs[(size_t) i];
            if (! o.alive) continue;
            if (dragged >= 0 && i != dragged && (i % 5) != 0) continue;

            const auto p = toPixels (o.x, o.y);
            const float len = maxRadius * 0.24f * fractal;
            const juce::Colour fc = (i == dragged) ? PCTheme::accent : PCColours::neon;
            for (int arm = 0; arm < 3; ++arm)
            {
                const float a = o.angle + (float) arm * juce::MathConstants<float>::twoPi / 3.0f;
                drawFractal (g, p, a, len, 4, fc, fractal * 0.65f);
            }
        }
    }

    // --- Detonation shockwave -----------------------------------------------
    const float boom = colony.getExplosionFlash();
    if (boom > 0.02f)
    {
        const auto c = toPixels (colony.getExplosionCentre().x, colony.getExplosionCentre().y);
        const float ringRad = maxRadius * (0.15f + (1.0f - boom) * 1.1f);
        g.setColour (PCTheme::clipRed.withAlpha (boom * 0.8f));
        g.drawEllipse (c.x - ringRad, c.y - ringRad, ringRad * 2.0f, ringRad * 2.0f, 1.5f + boom * 4.0f);
        g.setColour (PCTheme::warning.withAlpha (boom * 0.35f));
        g.fillEllipse (c.x - ringRad * 0.45f, c.y - ringRad * 0.45f, ringRad * 0.9f, ringRad * 0.9f);
    }

    // Transient flash ring from playback
    if (flashRing > 0.02f)
    {
        const float ringRad = maxRadius * (0.25f + flashRing * 0.65f);
        g.setColour (PCColours::hitCol.withAlpha (flashRing * 0.5f));
        g.drawEllipse (centre.x - ringRad, centre.y - ringRad, ringRad * 2.0f, ringRad * 2.0f, 1.5f + flashRing * 3.0f);
    }

    // --- Core ---------------------------------------------------------------
    const float coreSize = 8.0f + 12.0f * liveMeter + 18.0f * outLvl;
    const juce::Colour coreCol = PCColours::accent.interpolatedWith (PCColours::hitCol, outLvl);
    g.setColour (coreCol.withAlpha (0.25f + outLvl * 0.25f));
    g.fillEllipse (centre.x - coreSize * 0.5f, centre.y - coreSize * 0.5f, coreSize, coreSize);
    g.setColour (coreCol);
    g.fillEllipse (centre.x - 3.0f, centre.y - 3.0f, 6.0f, 6.0f);

    // --- Captions -----------------------------------------------------------
    juce::String caption = isFrozen ? "SWARM FROZEN" : "COLONY";
    if (proc.isDistressCallActive())   caption = "CALLING FOR HELP";
    else if (colony.isReversing())     caption = "TIME TURNING";
    else if (colony.isReversed())      caption = "REVERSED";
    else if (fractal > 0.01f)          caption = "FRACTALISING";
    else if (ghost > 0.01f)            caption = "ECHO";

    g.setColour (proc.isDistressCallActive() ? PCTheme::warning
                 : (colony.isReversing() || colony.isReversed()) ? PCColours::neon
                 : (isFrozen ? PCColours::freezeCol : PCTheme::textMuted));
    PCTheme::drawTracked (g, caption, b.withTrimmedTop (4.0f).withHeight (12.0f),
                          juce::Justification::centred, PCTheme::labelFont (9.0f));

    // Click-progress pips, so the gestures are discoverable
    auto pipRow = b.withTrimmedBottom (3.0f).removeFromBottom (11.0f).reduced (8.0f, 0.0f);
    auto drawPips = [&g, &pipRow] (const juce::String& tag, int done, int total, juce::Colour c)
    {
        auto cell = pipRow.removeFromLeft (pipRow.getWidth() / 3.0f);
        g.setColour (PCTheme::textMuted);
        PCTheme::drawTracked (g, tag, cell.removeFromLeft (14.0f), juce::Justification::centredLeft,
                              PCTheme::labelFont (8.0f));
        for (int i = 0; i < total; ++i)
        {
            const float d = 4.0f;
            const float x = cell.getX() + (float) i * (d + 3.0f);
            const float y = cell.getCentreY() - d * 0.5f;
            if (i < done) { g.setColour (c); g.fillEllipse (x, y, d, d); }
            else          { g.setColour (PCTheme::edge); g.drawEllipse (x, y, d, d, 1.0f); }
        }
    };
    drawPips ("L", colony.getLeftClickProgress(),   Colony::kLeftClicksToSplit,        PCTheme::accent);
    drawPips ("R", colony.getRightClickProgress(),  Colony::kRightClicksToSpawn,       PCTheme::clipRed);
    drawPips ("M", colony.getMiddleClickProgress(), Colony::kMiddleClicksToImpregnate, PCTheme::warning);
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
    auto b = getLocalBounds().toFloat();
    g.setColour (PCTheme::panel);
    g.fillRoundedRectangle (b, PCTheme::Metrics::radiusPanel);
    g.setColour (PCTheme::edge);
    g.drawRoundedRectangle (b.reduced (0.5f), PCTheme::Metrics::radiusPanel, 1.0f);

    auto p = plot();

    if (cached != nullptr && cached->beats > 0)
    {
        g.setColour (PCTheme::edge.withAlpha (0.45f));
        for (int i = 1; i < cached->beats; ++i)
        {
            const float x = p.getX() + (float) i / (float) cached->beats * p.getWidth();
            g.drawVerticalLine ((int) x, p.getY(), p.getBottom());
        }
    }

    g.setColour (PCTheme::edge.withAlpha (0.35f));
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
    g.setColour (PCTheme::textPrimary.withAlpha (0.8f));
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
        g.setColour (PCTheme::textMuted);
        g.drawVerticalLine ((int) tx, p.getY(), p.getBottom());
    }
    if (tEnd < 0.999f)
    {
        const float tx = p.getX() + tEnd * p.getWidth();
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRect (tx, p.getY(), p.getRight() - tx, p.getHeight());
        g.setColour (PCTheme::textMuted);
        g.drawVerticalLine ((int) tx, p.getY(), p.getBottom());
    }

    const int ph = proc.getPlayheadPosition();
    if (ph >= 0 && total > 0)
    {
        const float phX = p.getX() + ((float) ph / (float) total) * p.getWidth();
        const float lvl = juce::jlimit (0.0f, 1.0f, proc.getOutputLevel() * 1.8f);
        g.setColour (PCTheme::textPrimary.interpolatedWith (PCColours::hitCol, lvl));
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
    else { drag = Drag::none; if (! e.mods.isPopupMenu()) proc.triggerPreview(); }

    if (e.mods.isPopupMenu())
    {
        const juce::String id = drag == Drag::volStart ? IDs::volStart
                              : drag == Drag::volEnd   ? IDs::volEnd
                              : drag == Drag::volTension ? IDs::volTension : juce::String();
        drag = Drag::none;
        if (id.isNotEmpty()) showParamContextMenu (*this, proc, id);
    }
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
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (over ? PCTheme::panel.brighter (0.08f) : PCTheme::panel);
    g.fillRoundedRectangle (r, PCTheme::Metrics::radiusPanel);
    g.setColour (over ? PCColours::accent : PCTheme::edge);
    g.drawRoundedRectangle (r, PCTheme::Metrics::radiusPanel, 1.0f);

    g.setColour (over ? PCColours::accent : PCTheme::textMuted);
    PCTheme::drawTracked (g, "DRAG TO DAW", r, juce::Justification::centred, PCTheme::headerFont (10.0f));
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

void TensionBox::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        menuOpen = true;
        showParamContextMenu (*this, proc, paramId);
        return;
    }
    menuOpen = false;
    downT = proc.param (paramId);
    downY = e.y;
}

void TensionBox::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);

    g.setColour (PCTheme::panel);
    g.fillRoundedRectangle (r, PCTheme::Metrics::radiusPanel);
    g.setColour (PCTheme::edge);
    g.drawRoundedRectangle (r, PCTheme::Metrics::radiusPanel, 1.0f);

    // Diagonal reference: the linear, untensioned curve
    g.setColour (PCTheme::edge.withAlpha (0.6f));
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

    g.setColour (PCTheme::textMuted);
    PCTheme::drawTracked (g, "BEND", r.removeFromBottom (11.0f), juce::Justification::centred, PCTheme::labelFont (8.0f));
}

// ---------------- Gear (settings) button ----------------

void GearButton::paintButton (juce::Graphics& g, bool isOver, bool isDown)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const auto c = r.getCentre();
    const float outer = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;

    juce::Colour col = isDown ? PCTheme::accent : (isOver ? PCTheme::textPrimary : PCTheme::textMuted);
    g.setColour (col);

    juce::Path teeth;
    for (int i = 0; i < 8; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 8.0f;
        const float tw = 0.30f;
        juce::Path tooth;
        tooth.addRectangle (-outer * 0.16f, -outer, outer * 0.32f, outer * tw + outer * 0.55f);
        teeth.addPath (tooth, juce::AffineTransform::rotation (a).translated (c.x, c.y));
    }
    g.fillPath (teeth);
    g.fillEllipse (c.x - outer * 0.62f, c.y - outer * 0.62f, outer * 1.24f, outer * 1.24f);

    g.setColour (PCTheme::bgBase);
    g.fillEllipse (c.x - outer * 0.26f, c.y - outer * 0.26f, outer * 0.52f, outer * 0.52f);
}

// ---------------- PreChorus Editor ----------------

PreChorusEditor::PreChorusEditor (PreChorusProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      outputMeter ([&p] { return p.getOutputLevel(); }, "OUT"),
      outputLed ([&p] { return p.getOutputLevel(); }),
      waveform (p), visualizer (p), dragPad (p),
      pitchTension (p, IDs::pitchTension),
      options (p)
{
    setLookAndFeel (&lnf);

    // ---- Header strip ----
    title.setText ("PreChorus", juce::dontSendNotification);
    title.setFont (PCTheme::titleFont (14.0f));
    title.setColour (juce::Label::textColourId, PCTheme::textPrimary);
    addAndMakeVisible (title);

    subtitle.setText ("SWARM & CONVERGENCE", juce::dontSendNotification);
    subtitle.setFont (PCTheme::labelFont (8.5f));
    subtitle.setColour (juce::Label::textColourId, PCTheme::textMuted);
    addAndMakeVisible (subtitle);

    menuButton.setTooltip (PCTips::menuButton());
    menuButton.onClick = [this] { showFileMenu(); };
    addAndMakeVisible (menuButton);

    helpButton.setTooltip (PCTips::helpButton());
    helpButton.onClick = [this] { help.setVisible (true); };
    addAndMakeVisible (helpButton);

    gearButton.setTooltip (PCTips::gearButton());
    gearButton.onClick = [this] { options.setVisible (true); };
    addAndMakeVisible (gearButton);

    for (auto* b : { &aButton, &bButton })
    {
        b->setTooltip (PCTips::abButton());
        b->setClickingTogglesState (false);
        addAndMakeVisible (*b);
    }
    aButton.onClick = [this] { proc.setABSlot (0); };
    bButton.onClick = [this] { proc.setABSlot (1); };
    copyABButton.setTooltip (PCTips::abButton());
    copyABButton.onClick = [this] { proc.copyABSlot(); };
    addAndMakeVisible (copyABButton);

    outputMeter.setTooltip (PCTips::meter());
    addAndMakeVisible (outputMeter);

    outputLed.setTooltip (PCTips::outputLed());
    addAndMakeVisible (outputLed);

    presetCombo.setTextWhenNothingSelected ("FACTORY PRESETS");
    presetCombo.addItemList (PreChorusProcessor::getFactoryPresetNames(), 1);
    presetCombo.setTooltip (PCTips::presetCombo());
    presetCombo.onChange = [this]
    {
        const int idx = presetCombo.getSelectedItemIndex();
        if (idx >= 0) { proc.loadFactoryPreset (idx); waveform.rebuild(); }
    };
    addAndMakeVisible (presetCombo);

    // ---- Source row ----
    fileLabel.setFont (PCTheme::valueFont (11.0f));
    fileLabel.setColour (juce::Label::textColourId, PCTheme::textMuted);
    addAndMakeVisible (fileLabel);

    countLabel.setFont (PCTheme::monoFont (10.0f));
    countLabel.setColour (juce::Label::textColourId, PCTheme::textMuted);
    countLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (countLabel);

    rangeLabel.setText ("RANGE", juce::dontSendNotification);
    rangeLabel.setFont (PCTheme::labelFont (8.0f));
    rangeLabel.setColour (juce::Label::textColourId, PCTheme::textMuted);
    rangeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (rangeLabel);

    confidenceLabel.setFont (PCTheme::monoFont (10.0f));
    confidenceLabel.setColour (juce::Label::textColourId, PCColours::neon);
    confidenceLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (confidenceLabel);

    prevButton.setTooltip (PCTips::prevButton());
    nextButton.setTooltip (PCTips::nextButton());
    loadButton.setTooltip (PCTips::loadButton());
    playButton.setTooltip (PCTips::playButton());
    exportButton.setTooltip (PCTips::exportButton());
    resetButton.setTooltip (PCTips::resetButton());
    randomButton.setTooltip (PCTips::randomButton());
    regenSeedButton.setTooltip (PCTips::regenButton());
    dragPad.setTooltip (PCTips::dragPad());
    waveform.setTooltip (PCTips::waveform());
    visualizer.setTooltip (PCTips::visualizer());

    for (auto* b : { &prevButton, &nextButton, &loadButton, &playButton, &exportButton,
                     &resetButton, &randomButton, &regenSeedButton })
        addAndMakeVisible (*b);

    prevButton.onClick = [this] { proc.prevSample(); waveform.rebuild(); };
    nextButton.onClick = [this] { proc.nextSample(); waveform.rebuild(); };
    loadButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Load Vocal or Sample Stem", juce::File(),
                                                       "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f.existsAsFile()) { proc.loadSampleFile (f, true); waveform.rebuild(); }
            });
    };
    playButton.onClick      = [this] { proc.triggerPreview(); };
    resetButton.onClick     = [this] { proc.resetAllToDefaults(); presetCombo.setSelectedId (0, juce::dontSendNotification); waveform.rebuild(); };
    randomButton.onClick    = [this] { proc.randomizePreChorus(); waveform.rebuild(); };
    regenSeedButton.onClick = [this] { proc.regenerateSeed(); waveform.rebuild(); };
    exportButton.onClick    = [this] { doExportWav(); };

    // ---- Combo boxes ----
    sourceModeCombo  = makeCombo (IDs::sourceMode,     { "Live Capture", "Loaded Sample", "Hybrid Layer", "Slice Scatter" }, sourceModeAtt);
    charCombo        = makeCombo (IDs::character,      { "Clean Digital", "Analog Ensemble", "Bucket-Brigade", "Tape Choir",
                                                         "Dimension", "String Ensemble", "Granular Cloud", "Lo-Fi Choral" }, charAtt);
    captureCombo     = makeCombo (IDs::captureMode,    { "Threshold", "1/16 Note", "1/8 Note", "1/4 Beat", "1/2 Note",
                                                         "1 Bar", "2 Bars", "Manual" }, captureComboAtt);
    scaleCombo       = makeCombo (IDs::scaleLock,      { "Chromatic", "Major", "Minor", "Pentatonic", "Oct / 5ths" }, scaleComboAtt);
    dirCombo         = makeCombo (IDs::voiceDirection, { "Forward", "Reverse", "Alternating", "Random" }, dirComboAtt);
    postReleaseCombo = makeCombo (IDs::postRelease,    { "Cut at Impact", "Sustain Chorus", "Scatter Out" }, postReleaseAtt);
    seqCombo         = makeCombo (IDs::sequence,       { "Every Note", "Beat 1 Only", "Every 2 Bars", "Every 4 Bars" }, seqAtt);
    syncCombo        = makeCombo (IDs::syncLen,        { "1/2 Bar", "1 Bar", "2 Bars", "4 Bars" }, syncComboAtt);
    rangeCombo       = makeCombo (IDs::pitchRange,     { "1 Oct", "2 Oct", "4 Oct" }, rangeComboAtt);

    // ---- Toggles ----
    freezeToggle      = makeToggle (IDs::freeze,      "Freeze",   freezeAtt);
    revConvergeToggle = makeToggle (IDs::revConverge, "Rev Conv", revConvergeAtt);
    alignToggle       = makeToggle (IDs::align,       "Hit on note (PDC)", alignAtt);
    syncToggle        = makeToggle (IDs::sync,        "Sync",     syncAtt);

    // ---- Live capture controls & history ----
    armButton.setTooltip (PCTips::armButton());
    captureButton.setTooltip (PCTips::captureButton());
    lockButton.setTooltip (PCTips::lockButton());
    addAndMakeVisible (armButton);
    addAndMakeVisible (captureButton);
    addAndMakeVisible (lockButton);

    armButton.onClick     = [this] { proc.isCaptureArmed() ? proc.stopCapture() : proc.armCapture(); };
    captureButton.onClick = [this] { proc.triggerManualCapture(); waveform.rebuild(); };
    lockButton.onClick    = [this] { proc.setCaptureLock (! proc.isCaptureLocked()); };

    for (int i = 0; i < 8; ++i)
    {
        auto b = std::make_unique<PCTextButton> (juce::String (i + 1));
        b->setTooltip (PCTips::historySlot());
        b->onClick = [this, i] { proc.selectCaptureSlot (i); waveform.rebuild(); };
        addAndMakeVisible (*b);
        historySlotButtons[(size_t) i] = std::move (b);
    }

    addChildComponent (help);
    addChildComponent (options);

    // ---- Knobs ----
    kVoiceCount   = &makeKnob (IDs::voiceCount,   "VOICES");
    kVoiceDensity = &makeKnob (IDs::voiceDensity, "DENSITY");
    kVoiceAge     = &makeKnob (IDs::voiceAge,     "VOICE AGE");
    kProgReveal   = &makeKnob (IDs::progReveal,   "REVEAL");
    kHumanize     = &makeKnob (IDs::humanize,     "HUMANIZE");
    kGrainSize    = &makeKnob (IDs::grainSize,    "GRAIN MS");

    kMacro         = &makeKnob (IDs::macro,         "MACRO");
    kTimeSpread    = &makeKnob (IDs::timeSpread,    "TIME SPREAD");
    kTimeConverge  = &makeKnob (IDs::timeConverge,  "TIME CONV");
    kPitchSpread   = &makeKnob (IDs::pitchSpread,   "PITCH SPRD");
    kDetune        = &makeKnob (IDs::detune,        "DETUNE");
    kPitchConverge = &makeKnob (IDs::pitchConverge, "PITCH CONV");
    kPanSpread     = &makeKnob (IDs::panSpread,     "PAN SPREAD");
    kPanConverge   = &makeKnob (IDs::panConverge,   "WIDTH CONV");
    kToneConverge  = &makeKnob (IDs::toneConverge,  "TONE CONV");
    kFocus         = &makeKnob (IDs::focus,         "FOCUS");

    kAttraction = &makeKnob (IDs::attraction, "ATTRACT");
    kTurbulence = &makeKnob (IDs::turbulence, "TURBULENCE");
    kOvershoot  = &makeKnob (IDs::overshoot,  "OVERSHOOT");
    kOrbit      = &makeKnob (IDs::orbit,      "ORBIT");
    kDistance   = &makeKnob (IDs::distance,   "3D DISTANCE");

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

    kDry        = &makeKnob (IDs::dry,        "HIT DRY");
    kWet        = &makeKnob (IDs::wet,        "SWARM WET");
    kDryReplace = &makeKnob (IDs::dryReplace, "REPLACE");
    kDucking    = &makeKnob (IDs::ducking,    "DUCKING");
    kThresh     = &makeKnob (IDs::thresh,     "THRESH");

    kPitch      = &makeKnob (IDs::pitch,      "PITCH");
    kVolStart   = &makeKnob (IDs::volStart,   "START");
    kVolEnd     = &makeKnob (IDs::volEnd,     "END");
    kVolTension = &makeKnob (IDs::volTension, "TENSION");

    kDry->slider->setColour (juce::Slider::rotarySliderFillColourId, PCColours::hitCol);
    kMacro->slider->setColour (juce::Slider::rotarySliderFillColourId, PCColours::neon);
    kDistance->slider->setColour (juce::Slider::rotarySliderFillColourId, PCColours::neon);

    // ---- Colony controls ----
    gravityAddButton.setTooltip (PCTips::gravityAdd());
    gravityReleaseButton.setTooltip (PCTips::gravityRelease());
    enzymeButton.setTooltip (PCTips::enzyme());
    gammaButton.setTooltip (PCTips::gamma());

    gravityAddButton.setAccentColour (PCTheme::accent2);
    gravityReleaseButton.setAccentColour (PCTheme::accent2);
    enzymeButton.setAccentColour (PCTheme::signalOn);
    gammaButton.setAccentColour (PCTheme::warning);
    waterButton.setAccentColour (PCTheme::accent2);
    waterButton.setTooltip (PCTips::water());

    gravityAddButton.onClick     = [this] { proc.colonyAddGravity(); };
    gravityReleaseButton.onClick = [this] { proc.colonyReleaseGravity(); };
    enzymeButton.onClick         = [this] { proc.colonyAddEnzyme(); };
    gammaButton.onClick          = [this] { proc.colonyAddGamma(); };
    waterButton.onClick          = [this] { proc.colonyAddWater(); };

    for (auto* b : { &gravityAddButton, &gravityReleaseButton, &enzymeButton, &gammaButton, &waterButton })
        addAndMakeVisible (*b);

    scoreLabel.setFont (PCTheme::monoFont (13.0f));
    scoreLabel.setColour (juce::Label::textColourId, PCTheme::accent);
    scoreLabel.setJustificationType (juce::Justification::centredLeft);
    scoreLabel.setTooltip (PCTips::score());
    addAndMakeVisible (scoreLabel);

    colonyStatus.setFont (PCTheme::monoFont (10.0f));
    colonyStatus.setColour (juce::Label::textColourId, PCTheme::textMuted);
    colonyStatus.setJustificationType (juce::Justification::centredRight);
    colonyStatus.setTooltip (PCTips::colonyStatus());
    addAndMakeVisible (colonyStatus);

    addAndMakeVisible (visualizer);
    addAndMakeVisible (waveform);
    addAndMakeVisible (dragPad);
    addAndMakeVisible (pitchTension);
    pitchTension.setTooltip (PCTips::forParam (IDs::pitchTension));

    refreshTooltipMode();
    setSize (1240, 856);
    setWantsKeyboardFocus (true);
    startTimerHz (15);
    timerCallback();
    grabKeyboardFocus();
}

PreChorusEditor::~PreChorusEditor()
{
    setLookAndFeel (nullptr);
}

std::unique_ptr<PCComboBox> PreChorusEditor::makeCombo (const juce::String& id, const juce::StringArray& items,
                                                        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& att)
{
    auto box = std::make_unique<PCComboBox> (proc, id);
    box->addItemList (items, 1);
    box->setTooltip (PCTips::forParam (id));
    addAndMakeVisible (*box);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, *box);
    return box;
}

std::unique_ptr<PCToggleButton> PreChorusEditor::makeToggle (const juce::String& id, const juce::String& text,
                                                             std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& att)
{
    auto b = std::make_unique<PCToggleButton> (proc, id, text);
    b->setTooltip (PCTips::forParam (id));
    addAndMakeVisible (*b);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, *b);
    return b;
}

PreChorusEditor::Knob& PreChorusEditor::makeKnob (const juce::String& id, const juce::String& textName)
{
    auto k = std::make_unique<Knob>();
    k->slider = std::make_unique<PCSlider> (proc, id);
    k->slider->setTooltip (PCTips::forParam (id));
    addAndMakeVisible (*k->slider);

    k->label.setText (textName, juce::dontSendNotification);
    k->label.setFont (PCTheme::labelFont (PCTheme::Metrics::labelSize));
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setMinimumHorizontalScale (0.65f);   // squeeze long names rather than truncate them
    k->label.setColour (juce::Label::textColourId, PCTheme::textMuted);
    k->label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (k->label);

    k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, *k->slider);
    knobs.push_back (std::move (k));
    return *knobs.back();
}

// ---------------- File menu ----------------

void PreChorusEditor::showFileMenu()
{
    enum { mOpen = 1, mSave, mSaveAs, mExport, mReset, mFolder, mOptions, mManual };

    juce::PopupMenu m;
    m.setLookAndFeel (&lnf);
    m.addSectionHeader ("PreChorus " + PreChorusProcessor::getVersionString());
    m.addItem (mOpen, "Open Preset...");
    m.addItem (mSave, "Save Preset");
    m.addItem (mSaveAs, "Save Preset As...");
    m.addSeparator();
    m.addItem (mExport, "Export Audio to WAV...");
    m.addSeparator();
    m.addItem (mReset, "Reset to Defaults");
    m.addSeparator();
    m.addItem (mFolder, "Open Preset Folder");
    m.addItem (mOptions, "Options...");
    m.addItem (mManual, "Manual / Help");

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menuButton),
        [this] (int r)
        {
            switch (r)
            {
                case mOpen:    doOpenPreset(); break;
                case mSave:    doSavePreset (false); break;
                case mSaveAs:  doSavePreset (true); break;
                case mExport:  doExportWav(); break;
                case mReset:   proc.resetAllToDefaults();
                               presetCombo.setSelectedId (0, juce::dontSendNotification);
                               waveform.rebuild(); break;
                case mFolder:  PreChorusProcessor::getUserPresetFolder().revealToUser(); break;
                case mOptions: options.setVisible (true); break;
                case mManual:  help.setVisible (true); break;
                default: break;
            }
        });
}

void PreChorusEditor::doOpenPreset()
{
    chooser = std::make_unique<juce::FileChooser> ("Open PreChorus Preset",
                                                   PreChorusProcessor::getUserPresetFolder(),
                                                   "*" + PreChorusProcessor::getPresetExtension());
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (! f.existsAsFile()) return;
            if (proc.loadPresetFromFile (f))
            {
                presetCombo.setSelectedId (0, juce::dontSendNotification);
                waveform.rebuild();
            }
            else
            {
                juce::NativeMessageBox::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                    "Open Preset", "That file could not be read as a PreChorus preset.");
            }
        });
}

void PreChorusEditor::doSavePreset (bool forceSaveAs)
{
    const auto existing = proc.getCurrentPresetFile();
    if (! forceSaveAs && existing.getFullPathName().isNotEmpty())
    {
        if (proc.savePresetToFile (existing)) return;
    }

    auto startIn = existing.getFullPathName().isNotEmpty() ? existing
                                                           : PreChorusProcessor::getUserPresetFolder()
                                                                 .getChildFile ("My Preset" + PreChorusProcessor::getPresetExtension());

    chooser = std::make_unique<juce::FileChooser> ("Save PreChorus Preset", startIn,
                                                   "*" + PreChorusProcessor::getPresetExtension());
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
        [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f == juce::File()) return;
            if (! proc.savePresetToFile (f))
                juce::NativeMessageBox::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                    "Save Preset", "The preset could not be written to that location.");
        });
}

void PreChorusEditor::doExportWav()
{
    chooser = std::make_unique<juce::FileChooser> ("Export PreChorus Swell WAV", juce::File(), "*.wav");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
        [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f != juce::File()) proc.exportWav (f.withFileExtension ("wav"));
        });
}

void PreChorusEditor::refreshTooltipMode()
{
    const bool wanted = proc.areTooltipsEnabled();
    if (wanted && tooltips == nullptr)
        tooltips = std::make_unique<juce::TooltipWindow> (this, PCTheme::Metrics::tooltipMs);
    else if (! wanted && tooltips != nullptr)
        tooltips.reset();
}

// ---------------- Timer ----------------

void PreChorusEditor::timerCallback()
{
    refreshTooltipMode();

    auto f = proc.getCurrentFile();
    const int sMode = (int) proc.param (IDs::sourceMode);
    if (sMode == 0)      fileLabel.setText ("[Live Input Capture]", juce::dontSendNotification);
    else if (sMode == 1) fileLabel.setText (f.existsAsFile() ? f.getFileName() : "[Loaded Sample]", juce::dontSendNotification);
    else if (sMode == 2) fileLabel.setText ("[Hybrid Layer: Live + Loaded]", juce::dontSendNotification);
    else                 fileLabel.setText ("[Slice Scatter Ensemble]", juce::dontSendNotification);

    const int n = proc.getSampleCount();
    countLabel.setText (n > 0 ? juce::String (proc.getSampleIndex() + 1) + " / " + juce::String (n) : "",
                        juce::dontSendNotification);

    const bool sync = proc.param (IDs::sync) > 0.5f;
    kTail->slider->setEnabled (! sync);
    kTail->slider->setAlpha (sync ? 0.4f : 1.0f);
    kTail->label.setAlpha (sync ? 0.4f : 1.0f);
    if (syncCombo != nullptr)
    {
        syncCombo->setEnabled (sync);
        syncCombo->setAlpha (sync ? 1.0f : 0.5f);
    }

    const float conf = proc.getTargetConfidence();
    const int confPct = juce::roundToInt (conf * 100.0f);
    confidenceLabel.setText ("TARGET CONFIDENCE " + juce::String (confPct) + "%", juce::dontSendNotification);
    confidenceLabel.setColour (juce::Label::textColourId,
                               confPct > 90 ? PCTheme::signalOn : (confPct > 70 ? PCTheme::warning : PCTheme::clipRed));

    const int currentSlot = proc.getActiveCaptureSlot();
    for (int i = 0; i < 8; ++i)
    {
        auto& b = *historySlotButtons[(size_t) i];
        const bool isCur = (i == currentSlot);
        b.setToggleState (isCur || proc.isSlotFilled (i), juce::dontSendNotification);
        b.setAccentColour (isCur ? PCTheme::accent : PCTheme::accent2);
    }

    lockButton.setToggleState (proc.isCaptureLocked(), juce::dontSendNotification);
    lockButton.setAccentColour (PCTheme::warning);

    const auto state = proc.getCaptureState();
    if (state == PreChorusProcessor::CaptureState::recording)
    {
        captureButton.setAccentColour (PCTheme::clipRed);
        captureButton.setToggleState (true, juce::dontSendNotification);
        captureButton.setButtonText ("Recording...");
    }
    else if (state == PreChorusProcessor::CaptureState::armed)
    {
        captureButton.setAccentColour (PCTheme::warning);
        captureButton.setToggleState (true, juce::dontSendNotification);
        captureButton.setButtonText ("Armed (waiting)");
    }
    else
    {
        captureButton.setAccentColour (PCTheme::accent);
        captureButton.setToggleState (false, juce::dontSendNotification);
        captureButton.setButtonText ("Live Capture");
    }
    armButton.setToggleState (proc.isCaptureArmed(), juce::dontSendNotification);

    {
        const int water = proc.getColony().getWaterCount();
        const juce::String wantedText = water > 0
            ? "Water " + juce::String (juce::jmin (water, Colony::kWaterToDrown))
                       + "/" + juce::String (Colony::kWaterToDrown)
            : juce::String ("Add Water");
        if (waterButton.getButtonText() != wantedText) waterButton.setButtonText (wantedText);
        waterButton.setToggleState (water > 0, juce::dontSendNotification);
        waterButton.setAccentColour (water >= Colony::kWaterToDrown ? PCTheme::clipRed : PCTheme::accent2);
    }

    {
        const double sc = proc.getColony().getScore();
        // Fractional once the colony has been wrecked too fast - that is the point.
        const bool fractional = std::abs (sc - std::floor (sc)) > 0.001;
        scoreLabel.setText ("SCORE " + juce::String (sc, fractional ? 2 : 0), juce::dontSendNotification);
        scoreLabel.setColour (juce::Label::textColourId,
                              sc < 0.0 ? PCTheme::clipRed
                                       : (fractional ? PCTheme::warning : PCTheme::accent));
    }

    colonyStatus.setText (proc.getColony().getStatusLine(), juce::dontSendNotification);
    colonyStatus.setColour (juce::Label::textColourId,
                            proc.isDistressCallActive() ? PCTheme::warning : PCTheme::textMuted);
    gravityReleaseButton.setEnabled (true);

    aButton.setToggleState (proc.getABSlot() == 0, juce::dontSendNotification);
    bButton.setToggleState (proc.getABSlot() == 1, juce::dontSendNotification);

    // MIDI learn is armed somewhere: flag it on the gear so the user can find Options
    gearButton.setToggleState (proc.isLearningAnything(), juce::dontSendNotification);

    const juce::Colour col = PCColours::swellColour (proc.param (IDs::tone), proc.param (IDs::basscut));
    for (auto* k : { kTone, kBass, kWet, kTail, kShape, kTimeSpread, kTimeConverge, kPitchConverge, kVoiceCount, kFocus })
    {
        if (k->slider->findColour (juce::Slider::rotarySliderFillColourId) != col)
        {
            k->slider->setColour (juce::Slider::rotarySliderFillColourId, col);
            k->slider->repaint();
        }
    }
}

// ---------------- Painting ----------------

void PreChorusEditor::paint (juce::Graphics& g)
{
    g.fillAll (PCTheme::bgBase);

    // Header strip
    g.setColour (PCTheme::panel);
    g.fillRect (headerStrip);
    PCTheme::drawSeparator (g, headerStrip.getX(), headerStrip.getRight(), headerStrip.getBottom());

    // Section headers and 1px separators - never boxes inside boxes
    for (auto& gr : groups)
    {
        PCTheme::drawSectionHeader (g, gr.bounds.withHeight (16), gr.name, PCTheme::accent);
        if (gr.separatorRight)
        {
            g.setColour (PCTheme::edge);
            g.drawVerticalLine (gr.bounds.getRight() + PCTheme::Metrics::grid,
                                (float) gr.bounds.getY(), (float) gr.bounds.getBottom());
        }
    }

    if (! rowSplit.isEmpty())
        PCTheme::drawSeparator (g, rowSplit.getX(), rowSplit.getRight(), rowSplit.getY());
}

void PreChorusEditor::paintOverChildren (juce::Graphics& g)
{
    // Main window corner radius + the signature identity marks
    g.setColour (PCTheme::edge);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), PCTheme::Metrics::radiusWindow, 1.0f);
    PCTheme::drawSignatureNotch (g, getLocalBounds(), PCTheme::accent);
    PCTheme::drawVersionStamp (g, getLocalBounds(), "v" + PreChorusProcessor::getVersionString());
}

// ---------------- Layout ----------------

void PreChorusEditor::layoutKnobs (juce::Rectangle<int> area, std::initializer_list<Knob*> ks)
{
    // Keep knobs on the 8px grid rather than stretching them to fill the section.
    const int wanted = 13 + 56 + 14;  // readout + knob + label
    if (area.getHeight() > wanted) area = area.withSizeKeepingCentre (area.getWidth(), wanted);

    const int kw = area.getWidth() / (int) ks.size();
    for (auto* k : ks)
    {
        auto cell = area.removeFromLeft (kw);
        k->label.setBounds (cell.removeFromBottom (14));
        k->slider->setBounds (cell.reduced (2, 0));
    }
}

void PreChorusEditor::resized()
{
    help.setBounds (getLocalBounds());
    options.setBounds (getLocalBounds());
    groups.clear();

    const int g8 = PCTheme::Metrics::grid;
    const int pad = PCTheme::Metrics::windowPad;
    const int bh = PCTheme::Metrics::buttonHeight;

    auto all = getLocalBounds();

    // ---- 1. Header strip (32px) ----
    headerStrip = all.removeFromTop (PCTheme::Metrics::headerHeight);
    {
        auto h = headerStrip.reduced (pad, 2);

        outputLed.setBounds (h.removeFromLeft (12).withSizeKeepingCentre (9, 9));
        h.removeFromLeft (8);

        auto left = h.removeFromLeft (150);
        title.setBounds (left.removeFromTop (16));
        subtitle.setBounds (left);

        h.removeFromLeft (g8);
        menuButton.setBounds (h.removeFromLeft (52).withSizeKeepingCentre (52, 22));
        h.removeFromLeft (4);
        helpButton.setBounds (h.removeFromLeft (24).withSizeKeepingCentre (24, 22));

        gearButton.setBounds (h.removeFromRight (24).withSizeKeepingCentre (22, 22));
        h.removeFromRight (g8);
        presetCombo.setBounds (h.removeFromRight (150).withSizeKeepingCentre (150, 22));
        h.removeFromRight (g8 * 2);
        copyABButton.setBounds (h.removeFromRight (48).withSizeKeepingCentre (48, 22));
        h.removeFromRight (4);
        bButton.setBounds (h.removeFromRight (26).withSizeKeepingCentre (26, 22));
        h.removeFromRight (2);
        aButton.setBounds (h.removeFromRight (26).withSizeKeepingCentre (26, 22));
        h.removeFromRight (g8 * 2);
        outputMeter.setBounds (h.removeFromRight (160).withSizeKeepingCentre (160, 12));
    }

    auto area = all.reduced (pad);
    area.removeFromTop (g8);

    // ---- 2. Source row ----
    auto sourceRow = area.removeFromTop (bh);
    if (sourceModeCombo != nullptr) { sourceModeCombo->setBounds (sourceRow.removeFromLeft (120)); sourceRow.removeFromLeft (g8); }
    if (charCombo != nullptr)       { charCombo->setBounds (sourceRow.removeFromLeft (132));       sourceRow.removeFromLeft (g8 * 2); }

    loadButton.setBounds (sourceRow.removeFromLeft (64));   sourceRow.removeFromLeft (4);
    prevButton.setBounds (sourceRow.removeFromLeft (28));   sourceRow.removeFromLeft (2);
    nextButton.setBounds (sourceRow.removeFromLeft (28));   sourceRow.removeFromLeft (g8);
    countLabel.setBounds (sourceRow.removeFromRight (64));
    fileLabel.setBounds (sourceRow);

    area.removeFromTop (g8);

    // ---- 3. Capture row ----
    auto capRow = area.removeFromTop (bh);
    armButton.setBounds (capRow.removeFromLeft (48));           capRow.removeFromLeft (4);
    captureButton.setBounds (capRow.removeFromLeft (124));      capRow.removeFromLeft (4);
    if (captureCombo != nullptr) { captureCombo->setBounds (capRow.removeFromLeft (100)); capRow.removeFromLeft (g8 * 2); }

    for (int i = 0; i < 8; ++i)
    {
        historySlotButtons[(size_t) i]->setBounds (capRow.removeFromLeft (26));
        capRow.removeFromLeft (2);
    }
    capRow.removeFromLeft (g8);
    lockButton.setBounds (capRow.removeFromLeft (56));
    confidenceLabel.setBounds (capRow.removeFromRight (190));

    area.removeFromTop (g8);

    // ---- 4. Visualiser row ----
    auto vis = area.removeFromTop (220);
    visualizer.setBounds (vis.removeFromLeft (232));
    vis.removeFromLeft (g8);
    waveform.setBounds (vis);

    area.removeFromTop (g8);

    // ---- 4b. Colony row ----
    auto colonyRow = area.removeFromTop (bh);
    gravityAddButton.setBounds (colonyRow.removeFromLeft (104));      colonyRow.removeFromLeft (4);
    gravityReleaseButton.setBounds (colonyRow.removeFromLeft (124));  colonyRow.removeFromLeft (4);
    enzymeButton.setBounds (colonyRow.removeFromLeft (104));          colonyRow.removeFromLeft (4);
    gammaButton.setBounds (colonyRow.removeFromLeft (100));           colonyRow.removeFromLeft (4);
    waterButton.setBounds (colonyRow.removeFromLeft (100));           colonyRow.removeFromLeft (g8 * 2);
    scoreLabel.setBounds (colonyRow.removeFromLeft (150));
    colonyStatus.setBounds (colonyRow);

    area.removeFromTop (g8);

    // ---- 5. Transport row ----
    auto trans = area.removeFromTop (bh);
    playButton.setBounds (trans.removeFromLeft (64));        trans.removeFromLeft (4);
    exportButton.setBounds (trans.removeFromLeft (96));      trans.removeFromLeft (4);
    dragPad.setBounds (trans.removeFromLeft (104));          trans.removeFromLeft (4);
    resetButton.setBounds (trans.removeFromLeft (64));       trans.removeFromLeft (4);
    randomButton.setBounds (trans.removeFromLeft (72));      trans.removeFromLeft (4);
    regenSeedButton.setBounds (trans.removeFromLeft (60));   trans.removeFromLeft (g8 * 2);

    if (freezeToggle != nullptr)      { freezeToggle->setBounds (trans.removeFromLeft (74));      trans.removeFromLeft (4); }
    if (revConvergeToggle != nullptr) { revConvergeToggle->setBounds (trans.removeFromLeft (86)); }

    if (syncCombo != nullptr)   { syncCombo->setBounds (trans.removeFromRight (88));   trans.removeFromRight (4); }
    if (syncToggle != nullptr)  { syncToggle->setBounds (trans.removeFromRight (58));  trans.removeFromRight (g8); }
    if (alignToggle != nullptr) { alignToggle->setBounds (trans.removeFromRight (128)); trans.removeFromRight (g8); }
    if (seqCombo != nullptr)    { seqCombo->setBounds (trans.removeFromRight (104)); }

    area.removeFromTop (pad);

    // ---- 6. Knob sections ----
    const int rowH = (area.getHeight() - pad) / 2;
    auto rowA = area.removeFromTop (rowH);
    auto gapRow = area.removeFromTop (pad);
    rowSplit = { gapRow.getX(), gapRow.getCentreY(), gapRow.getWidth(), 1 };
    auto rowB = area;

    auto group = [&] (juce::Rectangle<int>& src, int width, const juce::String& name, bool last)
    {
        auto r = src.removeFromLeft (width);
        if (! last) src.removeFromLeft (g8 * 2);
        groups.push_back ({ name, r, ! last });
        auto content = r;
        content.removeFromTop (16 + g8);
        return content;
    };

    // Row A
    const int wA = rowA.getWidth();
    {
        auto swarmGrp = group (rowA, (int) (wA * 0.34f), "Swarm Engine (1-32 Voices)", false);
        auto rightCombo = swarmGrp.removeFromRight (84);
        if (dirCombo != nullptr) dirCombo->setBounds (rightCombo.withSizeKeepingCentre (80, 24));
        layoutKnobs (swarmGrp, { kVoiceCount, kVoiceDensity, kVoiceAge, kProgReveal, kHumanize, kGrainSize });
    }
    {
        auto convGrp = group (rowA, (int) (wA * 0.44f), "Convergence & Macro", false);
        auto rightCombo = convGrp.removeFromRight (88);
        if (scaleCombo != nullptr) scaleCombo->setBounds (rightCombo.withSizeKeepingCentre (84, 24));
        layoutKnobs (convGrp, { kMacro, kTimeSpread, kTimeConverge, kPitchSpread, kDetune,
                                kPitchConverge, kPanSpread, kPanConverge, kToneConverge, kFocus });
    }
    layoutKnobs (group (rowA, rowA.getWidth(), "Physics & 3D Distance", true),
                 { kAttraction, kTurbulence, kOvershoot, kOrbit, kDistance });

    // Row B
    const int wB = rowB.getWidth();
    layoutKnobs (group (rowB, (int) (wB * 0.53f), "Tone Shaping, Acoustics & Colour", false),
                 { kTail, kShape, kTone, kBass, kResonance, kTilt, kPresence, kAir, kSpace,
                   kDrive, kTransients, kFormant, kMonoBass });
    {
        auto mixGrp = group (rowB, (int) (wB * 0.26f), "Mix, Capture & Duck", false);
        auto rightRelease = mixGrp.removeFromRight (102);
        if (postReleaseCombo != nullptr) postReleaseCombo->setBounds (rightRelease.withSizeKeepingCentre (98, 24));
        layoutKnobs (mixGrp, { kDry, kWet, kDryReplace, kDucking, kThresh });
    }
    {
        auto pitchArea = group (rowB, rowB.getWidth(), "Pitch & Volume", true);
        auto right = pitchArea.removeFromRight (64);
        right = right.withSizeKeepingCentre (64, juce::jmin (84, right.getHeight()));
        rangeLabel.setBounds (right.removeFromTop (12));
        if (rangeCombo != nullptr) rangeCombo->setBounds (right.removeFromTop (24).reduced (2, 0));
        right.removeFromTop (4);
        pitchTension.setBounds (right.withSizeKeepingCentre (54, juce::jmin (44, right.getHeight())));
        layoutKnobs (pitchArea, { kPitch, kVolStart, kVolEnd, kVolTension });
    }
}

// ---------------- Input ----------------

bool PreChorusEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg")) return true;
    return false;
}

void PreChorusEditor::filesDropped (const juce::StringArray& files, int, int)
{
    for (auto& f : files)
        if (proc.loadSampleFile (juce::File (f), true)) { waveform.rebuild(); return; }
}

bool PreChorusEditor::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey) { proc.triggerPreview(); return true; }
    if (key == juce::KeyPress::escapeKey && proc.isLearningAnything()) { proc.cancelMidiLearn(); return true; }
    return false;
}
