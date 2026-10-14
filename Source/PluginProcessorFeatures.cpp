// PreChorus™
// Copyright © 2026 Sheldon Davidson. All rights reserved.
// Proprietary source code. See LICENSE and COPYRIGHT-TRADEMARK.md.
// SPDX-License-Identifier: LicenseRef-Proprietary

// Feature layer of the processor: smart capture, export options, MIDI learn, themed randomize + locks,
// A<->B morph and the auto target sources. Kept apart from PluginProcessor.cpp (engine + audio callback).

#include "PluginProcessor.h"
#include "Dsp.h"

// ---------------- Factory preset tags (preset browser) ----------------

juce::StringArray PreChorusProcessor::getFactoryPresetTags (int index)
{
    static const std::vector<juce::StringArray> tags = {
        { "Vocal", "Pop", "Subtle" },               // Pop Vocal Double
        { "EDM", "Extreme" },                       // EDM Riser Swarm
        { "EDM", "Pop", "FX" },                     // Future Bass Shimmer
        { "Dubstep", "Extreme" },                   // Dubstep Chaos Impact
        { "Vocal", "Subtle" },                      // Intimate Whisper Build
        { "Vocal", "FX", "Extreme" },               // Cinematic Choir Pad
        { "Vocal", "Subtle" },                      // Lo-Fi Bedroom Vocal
        { "FX", "Extreme" },                        // Ambient Drone Freeze
        { "Dubstep", "EDM", "Extreme" },            // Aggressive Distortion Drop
        { "Vocal", "Dubstep", "FX" },               // Trap Vocal Stutter
        { "Vocal", "Pop" },                         // Vowel Choir Lift
        { "EDM", "Pop" },                           // Stadium Anthem Build
        { "FX", "Dubstep" },                        // Glitch Fill
        { "EDM", "FX", "Extreme" },                 // Riser & Reverse Cymbal Classic
        { "Dubstep", "FX" },                        // Sub Drop Impact
        { "Vocal", "Pop" },                         // Wide Pop Chorus
        { "FX", "Extreme" },                        // Dark Cinema Rumble
        { "Vocal", "Dubstep" },                     // Triplet Trap Build
        { "Dubstep", "EDM" },                       // Dotted Dub Build
        { "Vocal", "Subtle" },                      // Tight Rap Double
        { "Vocal", "FX", "Subtle" },                // Whisper Pad Bloom
        { "EDM", "Pop" },                           // Festival Vocal Riser
        { "FX", "Extreme" },                        // Scatter Storm
        { "Vocal", "Pop", "Subtle" }                // Gentle Harmony Stack
    };
    return index >= 0 && index < (int) tags.size() ? tags[(size_t) index] : juce::StringArray();
}

// ---------------- Randomize themes & lock groups ----------------

juce::StringArray PreChorusProcessor::getRandomThemeNames()
{
    return { "Subtle", "Wide", "Dark", "Wild", "Tight", "Cinematic" };
}

juce::StringArray PreChorusProcessor::getLockGroupNames()
{
    return { "Swarm", "Convergence", "Physics & 3D", "Tone & Colour", "Mix & Stutter", "Character & Layers" };
}

int PreChorusProcessor::lockGroupOf (const juce::String& id)
{
    static const std::map<juce::String, int> groups = [] {
        std::map<juce::String, int> m;
        for (auto* s : { &IDs::voiceCount, &IDs::voiceDensity, &IDs::voiceAge, &IDs::progReveal, &IDs::voiceDirection,
                         &IDs::humanize, &IDs::grainSize, &IDs::seed })
            m[*s] = 0;
        for (auto* s : { &IDs::macro, &IDs::freeze, &IDs::revConverge, &IDs::postRelease, &IDs::timeSpread, &IDs::timeConverge,
                         &IDs::pitchSpread, &IDs::detune, &IDs::pitchConverge, &IDs::scaleLock, &IDs::panSpread,
                         &IDs::panConverge, &IDs::toneConverge, &IDs::focus })
            m[*s] = 1;
        for (auto* s : { &IDs::attraction, &IDs::turbulence, &IDs::overshoot, &IDs::orbit, &IDs::distance })
            m[*s] = 2;
        for (auto* s : { &IDs::tail, &IDs::shape, &IDs::tone, &IDs::basscut, &IDs::resonance, &IDs::tilt, &IDs::presence,
                         &IDs::air, &IDs::space, &IDs::drive, &IDs::transients, &IDs::formant, &IDs::monoBass })
            m[*s] = 3;
        for (auto* s : { &IDs::dryReplace, &IDs::ducking, &IDs::stutter, &IDs::stutterGrid, &IDs::stutterSwing, &IDs::stutterPattern })
            m[*s] = 4;
        for (auto* s : { &IDs::character, &IDs::layRiser, &IDs::layRev, &IDs::laySub, &IDs::layImpact, &IDs::layTone })
            m[*s] = 5;
        return m;
    }();
    auto it = groups.find (id);
    return it == groups.end() ? -1 : it->second;
}

namespace
{
    struct Range { const juce::String* id; float lo, hi; };
    struct Theme { std::vector<Range> ranges; std::vector<int> characters; int postRelease = -1; };

    const Theme& themeFor (int theme)
    {
        using namespace IDs;
        static const std::vector<Theme> themes = {
            // 0 Subtle
            { { { &voiceCount, 3, 8 }, { &voiceDensity, -0.2f, 0.3f }, { &progReveal, 0.1f, 0.4f }, { &voiceAge, 0.05f, 0.3f },
                { &timeSpread, 0.3f, 0.9f }, { &timeConverge, 0.7f, 1.0f }, { &pitchSpread, 1, 5 }, { &detune, 6, 16 },
                { &pitchConverge, 0.8f, 1.0f }, { &attraction, 0.3f, 0.7f }, { &turbulence, 0, 0.15f }, { &overshoot, 0, 0.2f },
                { &orbit, 0, 0.2f }, { &distance, 0.1f, 0.4f }, { &air, 0.1f, 0.35f }, { &drive, 0, 0.2f }, { &space, 0.1f, 0.35f },
                { &shape, 0, 0.5f }, { &panSpread, 0.2f, 0.6f }, { &tilt, -0.1f, 0.2f } }, { 0, 1, 4 }, -1 },
            // 1 Wide
            { { { &voiceCount, 14, 28 }, { &voiceDensity, 0.0f, 0.6f }, { &progReveal, 0.3f, 0.8f }, { &voiceAge, 0.1f, 0.5f },
                { &timeSpread, 1.0f, 3.0f }, { &timeConverge, 0.6f, 0.95f }, { &pitchSpread, 6, 14 }, { &detune, 15, 35 },
                { &pitchConverge, 0.6f, 0.95f }, { &panSpread, 0.8f, 1.0f }, { &panConverge, -0.5f, 0.8f }, { &orbit, 0.3f, 0.8f },
                { &space, 0.4f, 0.8f }, { &distance, 0.3f, 0.7f }, { &air, 0.3f, 0.6f }, { &presence, 0.3f, 0.6f } }, { 4, 1, 5 }, -1 },
            // 2 Dark
            { { { &voiceCount, 8, 22 }, { &voiceDensity, -0.3f, 0.4f }, { &voiceAge, 0.4f, 0.8f }, { &tilt, -0.6f, -0.15f },
                { &tone, 1800, 8000 }, { &air, 0, 0.15f }, { &presence, 0, 0.2f }, { &drive, 0.2f, 0.5f }, { &timeSpread, 0.8f, 2.5f },
                { &pitchSpread, 3, 12 }, { &detune, 10, 30 }, { &space, 0.3f, 0.7f }, { &monoBass, 100, 180 } }, { 2, 3, 7 }, -1 },
            // 3 Wild
            { { { &voiceCount, 12, 32 }, { &voiceDensity, -0.8f, 0.8f }, { &progReveal, 0.0f, 1.0f }, { &voiceAge, 0, 0.8f },
                { &timeSpread, 0.4f, 4.0f }, { &timeConverge, 0.3f, 1.0f }, { &pitchSpread, 10, 24 }, { &detune, 20, 50 },
                { &pitchConverge, 0.3f, 1.0f }, { &turbulence, 0.3f, 0.9f }, { &overshoot, 0.3f, 0.9f }, { &orbit, 0.2f, 1.0f },
                { &drive, 0.3f, 0.8f }, { &stutter, 0, 0.8f }, { &attraction, 0.1f, 1.0f }, { &focus, 0.2f, 1.0f },
                { &stutterSwing, 0, 0.5f } }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, -1 },
            // 4 Tight
            { { { &voiceCount, 4, 10 }, { &timeSpread, 0.2f, 0.7f }, { &timeConverge, 0.9f, 1.0f }, { &pitchSpread, 0, 4 },
                { &detune, 3, 12 }, { &panSpread, 0.1f, 0.5f }, { &focus, 0.6f, 1.0f }, { &attraction, 0.6f, 0.9f },
                { &turbulence, 0, 0.05f }, { &space, 0, 0.2f }, { &voiceAge, 0, 0.2f }, { &progReveal, 0.1f, 0.5f },
                { &drive, 0, 0.25f } }, { 0, 1, 4 }, 0 },
            // 5 Cinematic
            { { { &voiceCount, 20, 32 }, { &timeSpread, 2.0f, 4.0f }, { &progReveal, 0.5f, 0.9f }, { &voiceDensity, 0.1f, 0.6f },
                { &space, 0.5f, 0.9f }, { &air, 0.3f, 0.7f }, { &presence, 0.3f, 0.6f }, { &panSpread, 0.7f, 1.0f },
                { &laySub, 0.3f, 0.8f }, { &layRev, 0.2f, 0.6f }, { &layRiser, 0.2f, 0.6f }, { &layImpact, 0.2f, 0.6f },
                { &pitchSpread, 5, 12 }, { &detune, 12, 28 }, { &distance, 0.4f, 0.8f } }, { 3, 5, 8 }, 1 }
        };
        return themes[(size_t) juce::jlimit (0, (int) themes.size() - 1, theme)];
    }
}

void PreChorusProcessor::randomizeThemed (int theme)
{
    auto locked = [this] (const juce::String& id)
    {
        const int g = lockGroupOf (id);
        return g >= 0 && (lockedGroups & (1u << g)) != 0;
    };

    if (theme < 0)
    {
        // classic randomize: the original sound-design ranges with the system RNG
        auto rnd = [] (float a, float b) { return a + (b - a) * juce::Random::getSystemRandom().nextFloat(); };
        auto set = [&] (const juce::String& id, float v) { if (! locked (id)) setParam (id, v); };
        set (IDs::voiceCount, (float) juce::Random::getSystemRandom().nextInt (juce::Range<int> (6, 28)));
        set (IDs::voiceDensity, rnd (-0.4f, 0.6f));   set (IDs::progReveal, rnd (0.2f, 0.8f));
        set (IDs::voiceAge, rnd (0.1f, 0.6f));        set (IDs::timeSpread, rnd (0.4f, 2.5f));
        set (IDs::timeConverge, rnd (0.6f, 1.0f));    set (IDs::pitchSpread, rnd (3.0f, 16.0f));
        set (IDs::detune, rnd (10.0f, 35.0f));        set (IDs::pitchConverge, rnd (0.5f, 1.0f));
        set (IDs::attraction, rnd (0.3f, 0.9f));      set (IDs::turbulence, rnd (0.05f, 0.4f));
        set (IDs::overshoot, rnd (0.0f, 0.5f));       set (IDs::orbit, rnd (0.0f, 0.6f));
        set (IDs::distance, rnd (0.2f, 0.8f));        set (IDs::air, rnd (0.1f, 0.6f));
        set (IDs::drive, rnd (0.1f, 0.5f));           set (IDs::space, rnd (0.2f, 0.6f));
        set (IDs::shape, rnd (-0.2f, 0.7f));
        if (! locked (IDs::seed)) regenerateSeed();
        return;
    }

    const auto& t = themeFor (theme);
    // Pure function of (seed, theme, locks): the same inputs always give the same sound.
    juce::Random rng ((juce::int64) param (IDs::seed) * 2654435761LL + (juce::int64) theme * 40503LL + 17);
    for (auto& r : t.ranges)
    {
        const float v = r.lo + (r.hi - r.lo) * rng.nextFloat();   // always draw so locks do not shift later values
        if (! locked (*r.id)) setParam (*r.id, v);
    }
    const int chIdx = rng.nextInt ((int) t.characters.size());
    if (! locked (IDs::character)) setParam (IDs::character, (float) t.characters[(size_t) chIdx]);
    if (t.postRelease >= 0 && ! locked (IDs::postRelease)) setParam (IDs::postRelease, (float) t.postRelease);
    if (theme == 3 && ! locked (IDs::stutterGrid)) setParam (IDs::stutterGrid, (float) rng.nextInt (4));
    if (theme == 3 && ! locked (IDs::stutterPattern)) setParam (IDs::stutterPattern, (float) rng.nextInt (65536));
    if (! locked (IDs::seed)) setParam (IDs::seed, (float) (1 + (int) (rng.nextInt() & 0x7fffffff) % 9999));
}

// ---------------- A<->B morph ----------------

void PreChorusProcessor::applyMorph()
{
    const int cur = abSlotB ? 1 : 0, other = 1 - cur;
    if (! morphActive)
    {
        // First use: the live sound becomes the current slot; an unused other slot starts as a copy.
        abSnapshots[cur] = soundDesignSnapshot();
        if (! abSnapshots[other].isValid()) abSnapshots[other] = abSnapshots[cur].createCopy();
        morphActive = true;
    }
    const float m = juce::jlimit (0.0f, 1.0f, param (IDs::morph));
    const auto& a = abSnapshots[0];
    const auto& b = abSnapshots[1];
    for (auto& id : getSoundDesignIds())
    {
        const juce::Identifier key (id);
        if (! a.hasProperty (key) || ! b.hasProperty (key)) continue;
        auto* p = apvts.getParameter (id);
        if (p == nullptr) continue;
        const double va = (double) a.getProperty (key), vb = (double) b.getProperty (key);
        const bool stepped = dynamic_cast<juce::AudioParameterChoice*> (p) != nullptr || dynamic_cast<juce::AudioParameterBool*> (p) != nullptr
                             || id == IDs::seed || id == IDs::stutterPattern;
        const double v = stepped ? (m < 0.5f ? va : vb) : va + (vb - va) * (double) m;
        const auto range = p->getNormalisableRange();
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (range.start, range.end, (float) v)));
    }
    lastMorphApplyMs = juce::Time::getMillisecondCounterHiRes();
}

// ---------------- Smart capture ----------------

void PreChorusProcessor::postProcessCapture (juce::AudioBuffer<float>& b)
{
    const int before = b.getNumSamples();
    if (smartTrimCapture || smartNormaliseCapture)
        PCDsp::smartTrim (b, hostSampleRate, -50.0f, 0.05, 10.0, smartNormaliseCapture, -1.0f, smartTrimCapture);
    const double secs = (double) b.getNumSamples() / juce::jmax (1.0, hostSampleRate);
    const double trimmed = (double) (before - b.getNumSamples()) / juce::jmax (1.0, hostSampleRate);
    lastCaptureSummary = "Captured " + juce::String (secs, 2) + " s"
                         + (trimmed > 0.005 ? " (trimmed " + juce::String (trimmed, 2) + " s of silence)" : juce::String())
                         + ". RETAKE re-records into the same slot.";
}

void PreChorusProcessor::retakeCapture()
{
    if (lastCaptureSlot >= 0 && lastCaptureSlot < kNumHistorySlots && ! captureLockState.load())
    {
        activeSlot.store (lastCaptureSlot);
        setParam (IDs::captureSlot, (float) lastCaptureSlot);
    }
    armCapture();
}

// ---------------- MIDI learn ----------------

void PreChorusProcessor::startMidiLearn (const juce::String& paramId)
{
    const int idx = allParamIds.indexOf (paramId);
    learnParamIndex.store (idx);
}

juce::String PreChorusProcessor::getMidiLearnParam() const
{
    const int idx = learnParamIndex.load();
    return idx >= 0 && idx < allParamIds.size() ? allParamIds[idx] : juce::String();
}

void PreChorusProcessor::clearMidiMapping (const juce::String& paramId)
{
    const int idx = allParamIds.indexOf (paramId);
    if (idx < 0) return;
    for (auto& m : midiMap) if (m.load() == idx) m.store (-1);
}

int PreChorusProcessor::getMappedMidiSource (const juce::String& paramId) const
{
    const int idx = allParamIds.indexOf (paramId);
    if (idx < 0) return -1;
    for (int s = 0; s < kNumMidiSources; ++s) if (midiMap[(size_t) s].load() == idx) return s;
    return -1;
}

juce::String PreChorusProcessor::midiSourceName (int s)
{
    if (s < 0) return {};
    if (s < 128) return "CC " + juce::String (s);
    return s == 128 ? "Aftertouch" : "Pitch bend";
}

void PreChorusProcessor::applyMidiMappings()
{
    // Another source was just learned for a parameter: keep one source per parameter (the newest wins).
    const int ls = learnedSource.exchange (-1);
    if (ls >= 0 && ls < kNumMidiSources)
    {
        const int idx = midiMap[(size_t) ls].load();
        for (int s = 0; s < kNumMidiSources; ++s)
            if (s != ls && midiMap[(size_t) s].load() == idx) midiMap[(size_t) s].store (-1);
    }
    for (int s = 0; s < kNumMidiSources; ++s)
    {
        const int idx = midiMap[(size_t) s].load();
        if (idx < 0) continue;
        const float v = midiValue[(size_t) s].exchange (-1.0f);
        if (v < 0.0f) continue;
        if (auto* p = apvts.getParameter (allParamIds[idx]))
            if (std::abs (p->getValue() - v) > 1.0e-4f) p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, v));
    }
}

// ---------------- Export ----------------

bool PreChorusProcessor::exportWithSettings (const juce::File& dest, const ExportSettings& s, juce::String* error)
{
    auto fail = [&] (const juce::String& why) { if (error != nullptr) *error = why; return false; };
    if (dirty.exchange (false) || renderRequested.load() || renderInFlight.load()) render();   // never export a stale render
    auto r = getRendered();
    if (r == nullptr || r->audio.getNumSamples() == 0) return fail ("Nothing has been rendered yet.");

    const int n = r->audio.getNumSamples();
    const int hitAt = r->hitIndex >= 0 ? r->hitIndex : n;
    juce::AudioBuffer<float> mix;
    mix.makeCopyOf (r->audio);
    for (int ch = 0; ch < 2; ++ch)
    {
        mix.applyGain (ch, 0, hitAt, wetParam->load());
        mix.applyGain (ch, hitAt, n - hitAt, dryParam->load());
    }

    if (s.trimTail)
    {
        int last = mix.getNumSamples() - 1;
        const float thr = juce::Decibels::decibelsToGain (-70.0f);
        while (last > hitAt && std::abs (mix.getSample (0, last)) < thr && std::abs (mix.getSample (1, last)) < thr) --last;
        const int keep = juce::jmin (mix.getNumSamples(), last + 1 + (int) (r->sampleRate * 0.02));
        if (keep < mix.getNumSamples())
        {
            juce::AudioBuffer<float> t (2, keep);
            for (int ch = 0; ch < 2; ++ch) t.copyFrom (ch, 0, mix, ch, 0, keep);
            mix = std::move (t);
        }
    }

    double outRate = r->sampleRate;
    if (s.rate == 1) outRate = 44100.0; else if (s.rate == 2) outRate = 48000.0; else if (s.rate == 3) outRate = 96000.0;
    PCDsp::resampleBuffer (mix, r->sampleRate, outRate);

    if (s.normalise)
    {
        const float mag = mix.getMagnitude (0, mix.getNumSamples());
        if (mag > 1.0e-5f) mix.applyGain (juce::Decibels::decibelsToGain (-1.0f) / mag);
    }
    if (s.format == 0) PCDsp::ditherTPDF (mix, 16);

    if (! dest.getParentDirectory().createDirectory()) return fail ("Could not create the destination folder.");
    dest.deleteFile();
    std::unique_ptr<juce::FileOutputStream> os (dest.createOutputStream());
    if (os == nullptr || ! os->openedOk()) return fail ("Could not open the file for writing (is it in use or read-only?).");

    std::unique_ptr<juce::AudioFormatWriter> w;
    if (s.format == 3)
    {
        juce::FlacAudioFormat flac;
        w.reset (flac.createWriterFor (os.get(), outRate, 2, 24, {}, 5));
    }
    else
    {
        juce::WavAudioFormat wav;
        const int bits = s.format == 0 ? 16 : s.format == 2 ? 32 : 24;
        w.reset (wav.createWriterFor (os.get(), outRate, 2, bits, {}, 0));
    }
    if (w == nullptr) return fail ("The selected file format is not available.");
    os.release();
    const bool ok = w->writeFromAudioSampleBuffer (mix, 0, mix.getNumSamples());
    w.reset();
    return ok ? true : fail ("Writing the audio data failed (disk full?).");
}

// ---------------- Auto target sources ----------------

void PreChorusProcessor::processAutoTargets (double blockPpq, double bpm, int beatsPerBar, bool playing, bool looping, double loopEnd,
                                             int numSamples, const float* trigA, const float* trigB)
{
    const int mode = (int) param (IDs::tgtSource);
    if (blockPpq >= 0.0 && lastBlockPpq >= 0.0 && blockPpq < lastBlockPpq - 1.0e-6) { lastAutoBar = -1; loopFiredThisPass = false; }
    lastBlockPpq = blockPpq;
    if (mode == 0) return;

    if (mode == 3)
    {
        // Input transient: the swarm starts on the first sample that crosses the threshold (with a 250 ms hold-off)
        const float thr = juce::Decibels::decibelsToGain (param (IDs::tgtThresh));
        const int hold = (int) (hostSampleRate * 0.25);
        for (int i = 0; i < numSamples; ++i)
        {
            if (transientHoldoff > 0) { --transientHoldoff; continue; }
            if (std::abs (trigA[i]) >= thr || std::abs (trigB[i]) >= thr)
            {
                startVoice (1.0f, 1.0, i);
                transientHoldoff = hold;
            }
        }
        return;
    }

    auto r = getRendered();
    const int hit = r != nullptr ? r->hitIndex : -1;
    if (hit <= 0 || ! playing || blockPpq < 0.0 || bpm <= 0.0) return;
    const double spp = hostSampleRate * 60.0 / bpm;                  // samples per beat
    const int seqMode = (int) param (IDs::sequence);

    if (mode == 1)
    {
        // Next bar line: schedule the swarm `hit` samples ahead of the bar so the impact lands exactly on it
        const double barLen = (double) juce::jmax (1, beatsPerBar);
        const double endPpq = blockPpq + ((double) numSamples + (double) hit) / spp;
        for (juce::int64 n = (juce::int64) std::ceil (blockPpq / barLen - 1.0e-9); (double) n * barLen <= endPpq; ++n)
        {
            if (n <= lastAutoBar) continue;
            const bool allowed = seqMode <= 1 || (seqMode == 2 && n % 2 == 0) || (seqMode == 3 && n % 4 == 0);
            const double H = ((double) n * barLen - blockPpq) * spp;   // samples from block start to the impact
            const double S = H - (double) hit;                         // ... to the swarm start (may be before this block)
            if (S >= (double) numSamples) break;
            if (H <= 0.0) { lastAutoBar = n; continue; }   // the impact is not in the future
            if (allowed) startVoice (1.0f, 1.0, (int) std::floor (S));
            lastAutoBar = n;
        }
    }
    else if (mode == 2 && looping && loopEnd > blockPpq && ! loopFiredThisPass)
    {
        const double H = (loopEnd - blockPpq) * spp;
        const double S = H - (double) hit;
        if (S < (double) numSamples)
        {
            startVoice (1.0f, 1.0, (int) std::floor (S));
            loopFiredThisPass = true;
        }
    }
}
