// PreChorus™
// Copyright © 2026 Sheldon Davidson. All rights reserved.
// Proprietary source code. See LICENSE and COPYRIGHT-TRADEMARK.md.
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const float kPitchOct[] = { 1.0f, 2.0f, 4.0f };
    const int kSyncBars[]   = { 2, 4, 8, 16 }; // beats: 1/2 bar(2), 1 bar(4), 2 bars(8), 4 bars(16)

    juce::ValueTree soundParameterState (juce::AudioProcessorValueTreeState& state)
    {
        auto copy = state.copyState().createCopy();
        for (const auto& key : { "file", "activeSlot", "captureLock", "compareA", "compareB", "currentCompareSlot" })
            copy.removeProperty (key, nullptr);
        return copy;
    }

    float quantizeToScale (float semi, int scaleMode)
    {
        if (scaleMode == 0) return semi; // Chromatic
        int baseOct = (int) std::floor (semi / 12.0f);
        float noteInOct = semi - baseOct * 12.0f;
        int noteRounded = juce::roundToInt (noteInOct);

        const int majorScale[] = { 0, 2, 4, 5, 7, 9, 11 };
        const int minorScale[] = { 0, 2, 3, 5, 7, 8, 10 };
        const int pentatonic[] = { 0, 2, 4, 7, 9 };
        const int fifthOct[]   = { 0, 7, 12 };

        const int* scale = majorScale;
        int scaleSize = 7;
        if (scaleMode == 2) { scale = minorScale; scaleSize = 7; }
        else if (scaleMode == 3) { scale = pentatonic; scaleSize = 5; }
        else if (scaleMode == 4) { scale = fifthOct; scaleSize = 3; }

        int closest = scale[0];
        int minDist = 999;
        for (int i = 0; i < scaleSize; ++i)
        {
            int d = std::abs (noteRounded - scale[i]);
            if (d < minDist) { minDist = d; closest = scale[i]; }
        }
        return (float) (baseOct * 12 + closest);
    }
}

PreChorusProcessor::PreChorusProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    formatManager.registerBasicFormats();
    for (auto* p : getParameters())
        apvts.addParameterListener (static_cast<juce::AudioProcessorParameterWithID*> (p)->paramID, this);

    dryParam = apvts.getRawParameterValue (IDs::dry);
    wetParam = apvts.getRawParameterValue (IDs::wet);

    loadedSR = 44100.0;
    loadedBuffer.setSize (2, (int) (loadedSR * 1.5));
    loadedBuffer.clear();
    for (int i = 0; i < loadedBuffer.getNumSamples(); ++i)
    {
        const double t = (double) i / loadedSR;
        const float env = (float) (std::exp (-t * 3.2));
        const float s1 = std::sin ((float) (2.0 * juce::MathConstants<double>::pi * 261.63 * t)); // C4
        const float s2 = std::sin ((float) (2.0 * juce::MathConstants<double>::pi * 329.63 * t)); // E4
        const float s3 = std::sin ((float) (2.0 * juce::MathConstants<double>::pi * 392.00 * t)); // G4
        const float s4 = std::sin ((float) (2.0 * juce::MathConstants<double>::pi * 523.25 * t)); // C5
        const float val = (s1 * 0.4f + s2 * 0.3f + s3 * 0.3f + s4 * 0.2f) * env * 0.75f;
        loadedBuffer.setSample (0, i, val);
        loadedBuffer.setSample (1, i, val);
    }

    captureSlots[0].makeCopyOf (loadedBuffer);
    captureSlotSRs[0] = loadedSR;
    captureSlotFilled[0] = true;

    compareStates[0] = soundParameterState (apvts);
    compareStates[1] = soundParameterState (apvts);

    startTimerHz (30);
    dirty = true;
}

PreChorusProcessor::~PreChorusProcessor()
{
    stopTimer();
    for (auto* p : getParameters())
        apvts.removeParameterListener (static_cast<juce::AudioProcessorParameterWithID*> (p)->paramID, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout PreChorusProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    // Source & Capture
    p.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::sourceMode, "Source Mode", juce::StringArray { "Live Capture", "Loaded Sample", "Hybrid Layer", "Slice Scatter" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::captureMode, "Capture Mode", juce::StringArray { "Threshold", "1/16 Note", "1/8 Note", "1/4 Beat", "1/2 Note", "1 Bar", "2 Bars", "Manual" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::thresh, "Threshold", juce::NormalisableRange<float> (-48.0f, 0.0f, 0.5f), -24.0f));
    p.push_back (std::make_unique<juce::AudioParameterInt> (IDs::captureSlot, "History Slot", 0, 7, 0));
    p.push_back (std::make_unique<juce::AudioParameterBool> (IDs::captureLock, "Capture Lock", false));

    // Voice Swarm Engine (Up to 32 Voices)
    p.push_back (std::make_unique<juce::AudioParameterInt> (IDs::voiceCount, "Voice Count", 1, 32, 12));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::voiceDensity, "Density Curve", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.35f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::voiceAge, "Voice Age", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.25f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::progReveal, "Prog Reveal", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.50f));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::voiceDirection, "Direction", juce::StringArray { "Forward", "Reverse", "Alternating", "Random" }, 2));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::character, "Character", juce::StringArray { "Clean Digital", "Analog Ensemble", "Bucket-Brigade", "Tape Choir", "Dimension", "String Ensemble", "Granular Cloud", "Lo-Fi Choral" }, 1));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::humanize, "Humanize", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.25f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::grainSize, "Grain Size", juce::NormalisableRange<float> (10.0f, 200.0f, 1.0f), 45.0f));

    // Convergence Engine & Macros
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::macro, "Convergence Macro", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));
    p.push_back (std::make_unique<juce::AudioParameterBool> (IDs::freeze, "Freeze Ensemble", false));
    p.push_back (std::make_unique<juce::AudioParameterBool> (IDs::revConverge, "Reverse Convergence", false));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::postRelease, "Post-Target Release", juce::StringArray { "Cut at Impact", "Sustain Chorus", "Scatter Out" }, 1));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::timeSpread, "Time Spread", juce::NormalisableRange<float> (0.1f, 4.0f, 0.01f, 0.6f), 1.2f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::timeConverge, "Time Converge", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.85f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::pitchSpread, "Pitch Spread", juce::NormalisableRange<float> (0.0f, 24.0f, 0.5f), 7.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::detune, "Detune", juce::NormalisableRange<float> (0.0f, 50.0f, 0.5f), 18.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::pitchConverge, "Pitch Converge", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.90f));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::scaleLock, "Scale Lock", juce::StringArray { "Chromatic", "Major", "Minor", "Pentatonic", "Oct / 5ths" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::panSpread, "Pan Spread", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.90f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::panConverge, "Pan Converge", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.70f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::toneConverge, "Tone Converge", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.75f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::focus, "Focus", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.70f));

    // Physics & 3D Distance
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::attraction, "Attraction", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.60f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::turbulence, "Turbulence", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.15f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::overshoot, "Overshoot", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.20f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::orbit, "Orbit", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.25f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::distance, "3D Distance", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.50f));
    p.push_back (std::make_unique<juce::AudioParameterInt> (IDs::seed, "Random Seed", 1, 9999, 4242));

    // Swell & Tone Shaping
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::tail, "Length", juce::NormalisableRange<float> (0.1f, 8.0f, 0.01f, 0.5f), 2.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::shape, "Shape", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.40f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::tone, "Tone", juce::NormalisableRange<float> (200.0f, 20000.0f, 1.0f, 0.25f), 14000.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::basscut, "Bass Cut", juce::NormalisableRange<float> (20.0f, 1000.0f, 1.0f, 0.35f), 80.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::resonance, "Resonance", juce::NormalisableRange<float> (0.1f, 4.0f, 0.05f), 0.707f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::tilt, "Tilt EQ", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::presence, "Presence", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.30f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::air, "Air Sheen", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.35f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::space, "Space", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.35f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::drive, "Drive", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.20f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::transients, "Transients", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::formant, "Formant Shift", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.5f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::monoBass, "Mono Bass Hz", juce::NormalisableRange<float> (20.0f, 300.0f, 1.0f), 120.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::ducking, "Ducking", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    // Mix & PDC & Sequence
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::dry, "Dry", juce::NormalisableRange<float> (0.0f, 1.5f, 0.01f), 1.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::wet, "Wet", juce::NormalisableRange<float> (0.0f, 1.5f, 0.01f), 1.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::dryReplace, "Dry Replace", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterBool> (IDs::align, "PDC Align", true));
    p.push_back (std::make_unique<juce::AudioParameterBool> (IDs::sync, "Sync", true));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::syncLen, "Sync Length", juce::StringArray { "1/2 Bar", "1 Bar", "2 Bars", "4 Bars" }, 1));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::sequence, "Target Sequence", juce::StringArray { "Every Note", "Beat 1 Only", "Every 2 Bars", "Every 4 Bars" }, 0));

    // Envelopes & Trim
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::pitch, "Pitch Sweep", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::pitchRange, "Pitch Range", juce::StringArray { "1 Oct", "2 Oct", "4 Oct" }, 1));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::pitchTension, "Pitch Tension", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::volStart, "Vol Start", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::volEnd, "Vol End", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::volTension, "Vol Tension", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.3f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::trimStart, "Trim Start", juce::NormalisableRange<float> (0.0f, 0.95f, 0.001f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::trimEnd, "Trim End", juce::NormalisableRange<float> (0.05f, 1.0f, 0.001f), 1.0f));

    return { p.begin(), p.end() };
}

double PreChorusProcessor::getTailLengthSeconds() const
{
    return reportedTailSeconds.load();
}

bool PreChorusProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo()) return false;
    const auto& in = layouts.getMainInputChannelSet();
    return in.isDisabled() || in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono();
}

void PreChorusProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    hostSampleRate = sampleRate;

    const int maxCaptureSamples = (int) (sampleRate * 12.0);
    captureRingBuffer.setSize (2, maxCaptureSamples);
    captureRingBuffer.clear();
    captureWritePos = 0;
    lastPlayheadSample = -1;
    duckEnv = 0.0f;

    dirty = true;
}

void PreChorusProcessor::setParam (const juce::String& id, float value)
{
    if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void PreChorusProcessor::resetEdits()
{
    setParam (IDs::trimStart, 0.0f); setParam (IDs::trimEnd, 1.0f);
    setParam (IDs::volStart, 0.0f);   setParam (IDs::volEnd, 1.0f); setParam (IDs::volTension, 0.3f);
    setParam (IDs::pitch, 0.0f);      setParam (IDs::pitchTension, 0.0f);
    setParam (IDs::pitchConverge, 0.9f);
    setParam (IDs::timeConverge, 0.85f);
    setParam (IDs::macro, 1.0f);
    setParam (IDs::freeze, 0.0f);
    setParam (IDs::revConverge, 0.0f);
    setParam (IDs::tilt, 0.0f);
    setParam (IDs::air, 0.35f);
    setParam (IDs::drive, 0.2f);
    setParam (IDs::distance, 0.5f);
}

void PreChorusProcessor::randomizePreChorus()
{
    auto rnd = [] (float a, float b) { return a + (b - a) * juce::Random::getSystemRandom().nextFloat(); };
    setParam (IDs::voiceCount, (float) juce::Random::getSystemRandom().nextInt (juce::Range<int> (6, 28)));
    setParam (IDs::voiceDensity, rnd (-0.4f, 0.6f));
    setParam (IDs::progReveal, rnd (0.2f, 0.8f));
    setParam (IDs::voiceAge, rnd (0.1f, 0.6f));
    setParam (IDs::timeSpread, rnd (0.4f, 2.5f));
    setParam (IDs::timeConverge, rnd (0.6f, 1.0f));
    setParam (IDs::pitchSpread, rnd (3.0f, 16.0f));
    setParam (IDs::detune, rnd (10.0f, 35.0f));
    setParam (IDs::pitchConverge, rnd (0.5f, 1.0f));
    setParam (IDs::attraction, rnd (0.3f, 0.9f));
    setParam (IDs::turbulence, rnd (0.05f, 0.4f));
    setParam (IDs::overshoot, rnd (0.0f, 0.5f));
    setParam (IDs::orbit, rnd (0.0f, 0.6f));
    setParam (IDs::distance, rnd (0.2f, 0.8f));
    setParam (IDs::air, rnd (0.1f, 0.6f));
    setParam (IDs::drive, rnd (0.1f, 0.5f));
    setParam (IDs::space, rnd (0.2f, 0.6f));
    setParam (IDs::shape, rnd (-0.2f, 0.7f));
    regenerateSeed();
}

void PreChorusProcessor::regenerateSeed()
{
    setParam (IDs::seed, (float) juce::Random::getSystemRandom().nextInt (juce::Range<int> (1, 9999)));
}

juce::StringArray PreChorusProcessor::getFactoryPresetNames()
{
    return { "Pop Vocal Double", "EDM Riser Swarm", "Future Bass Shimmer", "Dubstep Chaos Impact",
             "Intimate Whisper Build", "Cinematic Choir Pad", "Lo-Fi Bedroom Vocal",
             "Ambient Drone Freeze", "Aggressive Distortion Drop", "Trap Vocal Stutter" };
}

juce::StringArray PreChorusProcessor::getMatchingFactoryPresetNames (const juce::String& filter)
{
    const auto all = getFactoryPresetNames();
    if (filter.trim().isEmpty()) return all;

    juce::StringArray matches;
    const auto needle = filter.trim().toLowerCase();
    for (const auto& name : all)
        if (name.toLowerCase().contains (needle))
            matches.add (name);
    return matches;
}

void PreChorusProcessor::loadFactoryPreset (int index)
{
    // Shapes the swarm/tone/convergence character only; leaves source, capture, and per-take
    // envelope trims untouched since those depend on the loaded/captured audio itself.
    switch (index)
    {
        case 0: // Pop Vocal Double
            setParam (IDs::voiceCount, 5.0f);      setParam (IDs::character, 1.0f);
            setParam (IDs::voiceDensity, 0.1f);    setParam (IDs::progReveal, 0.3f);
            setParam (IDs::timeSpread, 0.6f);      setParam (IDs::timeConverge, 0.9f);
            setParam (IDs::pitchSpread, 4.0f);     setParam (IDs::detune, 12.0f);
            setParam (IDs::pitchConverge, 0.95f);  setParam (IDs::panSpread, 0.4f);
            setParam (IDs::focus, 0.3f);           setParam (IDs::tilt, 0.1f);
            setParam (IDs::presence, 0.35f);       setParam (IDs::air, 0.2f);
            setParam (IDs::drive, 0.1f);           setParam (IDs::space, 0.2f);
            setParam (IDs::postRelease, 0.0f);     setParam (IDs::freeze, 0.0f);
            break;
        case 1: // EDM Riser Swarm
            setParam (IDs::voiceCount, 26.0f);     setParam (IDs::character, 4.0f);
            setParam (IDs::voiceDensity, 0.5f);    setParam (IDs::progReveal, 0.7f);
            setParam (IDs::timeSpread, 2.2f);      setParam (IDs::timeConverge, 0.95f);
            setParam (IDs::pitchSpread, 14.0f);    setParam (IDs::detune, 28.0f);
            setParam (IDs::pitchConverge, 0.9f);   setParam (IDs::panSpread, 0.9f);
            setParam (IDs::panConverge, 1.0f);     setParam (IDs::distance, 0.7f);
            setParam (IDs::focus, 0.8f);           setParam (IDs::tilt, 0.3f);
            setParam (IDs::presence, 0.6f);        setParam (IDs::air, 0.55f);
            setParam (IDs::drive, 0.4f);           setParam (IDs::space, 0.5f);
            setParam (IDs::postRelease, 0.0f);     setParam (IDs::dryReplace, 0.6f);
            break;
        case 2: // Future Bass Shimmer
            setParam (IDs::voiceCount, 18.0f);     setParam (IDs::character, 5.0f);
            setParam (IDs::voiceDensity, 0.2f);    setParam (IDs::progReveal, 0.5f);
            setParam (IDs::timeSpread, 1.4f);      setParam (IDs::timeConverge, 0.8f);
            setParam (IDs::pitchSpread, 9.0f);     setParam (IDs::detune, 22.0f);
            setParam (IDs::pitchConverge, 0.75f);  setParam (IDs::scaleLock, 3.0f);
            setParam (IDs::panSpread, 0.8f);       setParam (IDs::orbit, 0.3f);
            setParam (IDs::tilt, 0.45f);           setParam (IDs::presence, 0.55f);
            setParam (IDs::air, 0.65f);            setParam (IDs::space, 0.45f);
            setParam (IDs::drive, 0.15f);          setParam (IDs::postRelease, 1.0f);
            break;
        case 3: // Dubstep Chaos Impact
            setParam (IDs::voiceCount, 22.0f);     setParam (IDs::character, 2.0f);
            setParam (IDs::voiceDensity, -0.3f);   setParam (IDs::progReveal, 0.2f);
            setParam (IDs::revConverge, 1.0f);     setParam (IDs::timeConverge, 1.0f);
            setParam (IDs::turbulence, 0.35f);     setParam (IDs::overshoot, 0.5f);
            setParam (IDs::pitchSpread, 12.0f);    setParam (IDs::detune, 30.0f);
            setParam (IDs::tilt, -0.2f);           setParam (IDs::drive, 0.55f);
            setParam (IDs::transients, 1.0f);      setParam (IDs::dryReplace, 0.85f);
            setParam (IDs::monoBass, 120.0f);      setParam (IDs::postRelease, 0.0f);
            break;
        case 4: // Intimate Whisper Build
            setParam (IDs::voiceCount, 4.0f);      setParam (IDs::character, 0.0f);
            setParam (IDs::voiceDensity, -0.2f);   setParam (IDs::progReveal, 0.15f);
            setParam (IDs::timeSpread, 0.4f);      setParam (IDs::timeConverge, 0.7f);
            setParam (IDs::pitchSpread, 2.0f);     setParam (IDs::detune, 6.0f);
            setParam (IDs::panSpread, 0.2f);       setParam (IDs::focus, 0.1f);
            setParam (IDs::tilt, 0.0f);            setParam (IDs::drive, 0.0f);
            setParam (IDs::air, 0.15f);            setParam (IDs::space, 0.15f);
            setParam (IDs::ducking, 0.3f);         setParam (IDs::freeze, 0.0f);
            break;
        case 5: // Cinematic Choir Pad
            setParam (IDs::voiceCount, 32.0f);     setParam (IDs::character, 3.0f);
            setParam (IDs::voiceDensity, 0.3f);    setParam (IDs::progReveal, 0.6f);
            setParam (IDs::timeSpread, 2.5f);      setParam (IDs::timeConverge, 0.6f);
            setParam (IDs::pitchSpread, 7.0f);     setParam (IDs::detune, 18.0f);
            setParam (IDs::panSpread, 1.0f);       setParam (IDs::tail, 0.85f);
            setParam (IDs::tilt, 0.15f);           setParam (IDs::presence, 0.4f);
            setParam (IDs::air, 0.35f);            setParam (IDs::space, 0.75f);
            setParam (IDs::postRelease, 1.0f);     setParam (IDs::freeze, 0.0f);
            break;
        case 6: // Lo-Fi Bedroom Vocal
            setParam (IDs::voiceCount, 7.0f);      setParam (IDs::character, 7.0f);
            setParam (IDs::voiceDensity, 0.0f);    setParam (IDs::progReveal, 0.3f);
            setParam (IDs::timeSpread, 0.8f);      setParam (IDs::timeConverge, 0.85f);
            setParam (IDs::pitchSpread, 5.0f);     setParam (IDs::detune, 15.0f);
            setParam (IDs::tilt, -0.35f);          setParam (IDs::presence, 0.15f);
            setParam (IDs::air, 0.05f);            setParam (IDs::drive, 0.25f);
            setParam (IDs::monoBass, 90.0f);       setParam (IDs::space, 0.25f);
            break;
        case 7: // Ambient Drone Freeze
            setParam (IDs::voiceCount, 30.0f);     setParam (IDs::character, 6.0f);
            setParam (IDs::voiceDensity, -0.1f);   setParam (IDs::progReveal, 0.1f);
            setParam (IDs::freeze, 1.0f);          setParam (IDs::timeSpread, 3.0f);
            setParam (IDs::pitchSpread, 10.0f);    setParam (IDs::detune, 25.0f);
            setParam (IDs::panSpread, 1.0f);       setParam (IDs::orbit, 0.6f);
            setParam (IDs::tail, 1.0f);            setParam (IDs::space, 0.8f);
            setParam (IDs::air, 0.3f);             setParam (IDs::drive, 0.05f);
            break;
        case 8: // Aggressive Distortion Drop
            setParam (IDs::voiceCount, 20.0f);     setParam (IDs::character, 2.0f);
            setParam (IDs::voiceDensity, 0.4f);    setParam (IDs::progReveal, 0.4f);
            setParam (IDs::timeConverge, 1.0f);    setParam (IDs::pitchConverge, 1.0f);
            setParam (IDs::focus, 1.0f);           setParam (IDs::tilt, -0.4f);
            setParam (IDs::drive, 0.8f);           setParam (IDs::transients, 1.0f);
            setParam (IDs::dryReplace, 1.0f);      setParam (IDs::monoBass, 150.0f);
            setParam (IDs::postRelease, 0.0f);     setParam (IDs::distance, 0.8f);
            break;
        case 9: // Trap Vocal Stutter
        default:
            setParam (IDs::voiceCount, 12.0f);     setParam (IDs::character, 0.0f);
            setParam (IDs::voiceDensity, 0.6f);    setParam (IDs::progReveal, 0.15f);
            setParam (IDs::timeSpread, 0.5f);      setParam (IDs::timeConverge, 1.0f);
            setParam (IDs::pitchSpread, 3.0f);     setParam (IDs::detune, 10.0f);
            setParam (IDs::panSpread, 0.5f);       setParam (IDs::focus, 0.6f);
            setParam (IDs::tilt, 0.2f);            setParam (IDs::drive, 0.3f);
            setParam (IDs::air, 0.3f);             setParam (IDs::dryReplace, 0.7f);
            break;
    }
    regenerateSeed();
}

bool PreChorusProcessor::storeCompareState (int slot)
{
    if (slot < 0 || slot > 1) return false;
    compareStates[(size_t) slot] = soundParameterState (apvts);
    currentCompareSlot.store (slot);
    return compareStates[(size_t) slot].isValid();
}

bool PreChorusProcessor::recallCompareState (int slot)
{
    if (slot < 0 || slot > 1) return false;
    auto state = compareStates[(size_t) slot];
    if (! state.isValid()) return false;
    apvts.replaceState (state.createCopy());
    currentCompareSlot.store (slot);
    dirty = true;
    return true;
}

bool PreChorusProcessor::saveUserPreset (const juce::File& file)
{
    if (file == juce::File()) return false;

    juce::ValueTree wrapper ("PRECHORUS_PRESET");
    wrapper.setProperty ("schemaVersion", 1, nullptr);
    wrapper.setProperty ("product", "PreChorus", nullptr);
    wrapper.addChild (soundParameterState (apvts), -1, nullptr);

    auto xml = wrapper.createXml();
    if (xml == nullptr) return false;
    return file.replaceWithText (xml->toString());
}

bool PreChorusProcessor::loadUserPreset (const juce::File& file)
{
    if (! file.existsAsFile()) return false;

    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr) return false;

    auto wrapper = juce::ValueTree::fromXml (*xml);
    if (! wrapper.isValid() || wrapper.getType().toString() != "PRECHORUS_PRESET")
        return false;

    const int version = (int) wrapper.getProperty ("schemaVersion", 0);
    const auto product = wrapper.getProperty ("product", "").toString();
    if (version != 1 || product != "PreChorus" || wrapper.getNumChildren() != 1)
        return false;

    auto state = wrapper.getChild (0);
    if (! state.isValid() || state.getType() != apvts.state.getType())
        return false;

    apvts.replaceState (state.createCopy());
    dirty = true;
    return true;
}

void PreChorusProcessor::selectCaptureSlot (int slotIdx)
{
    if (slotIdx >= 0 && slotIdx < kNumHistorySlots)
    {
        activeSlot.store (slotIdx);
        setParam (IDs::captureSlot, (float) slotIdx);
        dirty = true;
    }
}

bool PreChorusProcessor::isSlotFilled (int slotIdx) const
{
    if (slotIdx >= 0 && slotIdx < kNumHistorySlots)
        return captureSlotFilled[(size_t) slotIdx];
    return false;
}

void PreChorusProcessor::setCaptureLock (bool locked)
{
    captureLockState.store (locked);
    setParam (IDs::captureLock, locked ? 1.0f : 0.0f);
}

std::shared_ptr<const RenderedSample> PreChorusProcessor::getRendered() const
{
    const juce::ScopedLock l (renderedOwnerLock);
    return rendered;
}

void PreChorusProcessor::timerCallback()
{
    if (captureState.load() == CaptureState::done)
        finalizePendingCapture();

    if (realtimeReaders.load (std::memory_order_acquire) == 0 && ! retiredRendered.empty())
        retiredRendered.clear();

    if (param (IDs::sync) > 0.5f)
    {
        const double bpm = hostBpm.load();
        if (std::abs (bpm - lastRenderBpm) > 0.05)
        {
            lastRenderBpm = bpm;
            dirty = true;
        }
    }
    if (dirty.exchange (false))
    {
        render();
        if (previewAfterRender.exchange (false)) triggerPreview();
    }
}

// ---------------- Live Capture Engine ----------------

void PreChorusProcessor::armCapture()
{
    if (captureState.load() == CaptureState::done)
        finalizePendingCapture();

    captureState.store (CaptureState::armed);
    captureWritePos = 0;
    captureSilenceCounter = 0;
}

void PreChorusProcessor::triggerManualCapture()
{
    if (captureState.load() == CaptureState::recording)
    {
        stopCapture();
    }
    else
    {
        if (captureState.load() == CaptureState::done)
            finalizePendingCapture();

        captureWritePos = 0;
        captureSilenceCounter = 0;
        captureTargetSamples = (int) (hostSampleRate * 8.0);
        captureState.store (CaptureState::recording);
    }
}

void PreChorusProcessor::requestCaptureFinalizeRealtime()
{
    if (captureState.load() != CaptureState::recording)
        return;

    const int capturedLen = captureWritePos;
    pendingCaptureSamples.store (capturedLen > (int) (hostSampleRate * 0.05) ? capturedLen : 0,
                                 std::memory_order_release);
    captureState.store (CaptureState::done);
    captureWritePos = 0;
}

void PreChorusProcessor::finalizePendingCapture()
{
    const int capturedLen = pendingCaptureSamples.exchange (0, std::memory_order_acq_rel);
    if (capturedLen > 0)
    {
        juce::AudioBuffer<float> newBuf (2, capturedLen);
        for (int ch = 0; ch < 2; ++ch)
            newBuf.copyFrom (ch, 0, captureRingBuffer, ch, 0, capturedLen);

        int slot = activeSlot.load();
        if (captureLockState.load() && captureSlotFilled[(size_t) slot])
        {
            for (int i = 0; i < kNumHistorySlots; ++i)
            {
                const int nextCandidate = (slot + i + 1) % kNumHistorySlots;
                if (! captureSlotFilled[(size_t) nextCandidate])
                {
                    slot = nextCandidate;
                    break;
                }
            }
        }

        {
            const juce::ScopedLock sl (sourceLock);
            captureSlots[(size_t) slot] = std::move (newBuf);
            captureSlotSRs[(size_t) slot] = hostSampleRate;
            captureSlotFilled[(size_t) slot] = true;
            activeSlot.store (slot);
        }
        dirty = true;
    }

    captureState.store (CaptureState::idle);
}

void PreChorusProcessor::stopCapture()
{
    const auto state = captureState.load();
    if (state == CaptureState::recording)
        requestCaptureFinalizeRealtime();

    if (captureState.load() == CaptureState::done)
        finalizePendingCapture();
    else
        captureState.store (CaptureState::idle);

    captureWritePos = 0;
}

// ---------------- Swarm, Shaping & Spatial Render Engine ----------------

void PreChorusProcessor::render()
{
    auto out = std::make_shared<RenderedSample>();
    const double sr = hostSampleRate > 1000.0 ? hostSampleRate : 44100.0;
    out->sampleRate = sr;

    // 1. Source Audio
    const int sMode = (int) param (IDs::sourceMode);
    juce::AudioBuffer<float> source;
    double rawSR = 44100.0;

    {
        const juce::ScopedLock sl (sourceLock);
        int slot = activeSlot.load();
        if (sMode == 0)
        {
            if (captureSlotFilled[(size_t) slot])
            {
                source.makeCopyOf (captureSlots[(size_t) slot]);
                rawSR = captureSlotSRs[(size_t) slot];
            }
            else
            {
                source.makeCopyOf (loadedBuffer);
                rawSR = loadedSR;
            }
        }
        else if (sMode == 1)
        {
            source.makeCopyOf (loadedBuffer);
            rawSR = loadedSR;
        }
        else if (sMode == 2)
        {
            int len = juce::jmax (loadedBuffer.getNumSamples(), captureSlots[(size_t) slot].getNumSamples());
            if (len > 0)
            {
                source.setSize (2, len);
                source.clear();
                for (int ch = 0; ch < 2; ++ch)
                {
                    if (loadedBuffer.getNumSamples() > 0)
                        source.addFrom (ch, 0, loadedBuffer, ch, 0, loadedBuffer.getNumSamples(), 0.5f);
                    if (captureSlotFilled[(size_t) slot] && captureSlots[(size_t) slot].getNumSamples() > 0)
                        source.addFrom (ch, 0, captureSlots[(size_t) slot], ch, 0, captureSlots[(size_t) slot].getNumSamples(), 0.5f);
                }
                rawSR = loadedSR;
            }
        }
        else
        {
            source.makeCopyOf (loadedBuffer);
            rawSR = loadedSR;
        }
    }

    if (source.getNumSamples() == 0) return;

    // Resample if necessary
    if (std::abs (rawSR - sr) > 1.0)
    {
        const double ratio = rawSR / sr;
        const int newLen = (int) (source.getNumSamples() / ratio);
        juce::AudioBuffer<float> resampled (2, newLen);
        for (int ch = 0; ch < 2; ++ch)
        {
            const float* srcPtr = source.getReadPointer (ch);
            float* dstPtr = resampled.getWritePointer (ch);
            for (int i = 0; i < newLen; ++i)
            {
                const double p = i * ratio;
                const int i0 = (int) p;
                const float frac = (float) (p - i0);
                const float s0 = srcPtr[juce::jmin (i0, source.getNumSamples() - 1)];
                const float s1 = srcPtr[juce::jmin (i0 + 1, source.getNumSamples() - 1)];
                dstPtr[i] = s0 + (s1 - s0) * frac;
            }
        }
        source = std::move (resampled);
    }

    const int srcLen = source.getNumSamples();

    // 2. Swell Duration
    int beats = 0;
    double swellSeconds = param (IDs::tail);
    if (param (IDs::sync) > 0.5f)
    {
        const double bpm = juce::jlimit (30.0, 300.0, hostBpm.load());
        const double secPerBeat = 60.0 / bpm;
        const int sIdx = juce::jlimit (0, 3, (int) param (IDs::syncLen));
        beats = kSyncBars[sIdx];
        swellSeconds = beats * secPerBeat;
    }
    swellSeconds = juce::jlimit (0.1, 16.0, swellSeconds);
    const int swellLen = (int) (swellSeconds * sr);

    // 3. Parameters
    const int numVoices = juce::jlimit (1, 32, (int) param (IDs::voiceCount));
    const float macroParam = param (IDs::macro);
    const bool isFrozen = param (IDs::freeze) > 0.5f;
    const bool isRevConverge = param (IDs::revConverge) > 0.5f;
    const int postReleaseMode = (int) param (IDs::postRelease);
    const int charMode = (int) param (IDs::character);
    const float humanizeAmt = param (IDs::humanize);
    const float focusAmt = param (IDs::focus);
    const float distanceAmt = param (IDs::distance);
    const float grainSizeMs = param (IDs::grainSize);

    const float timeSpreadSec = param (IDs::timeSpread);
    const float timeConvergeAmt = param (IDs::timeConverge) * macroParam;
    const float densityCurveParam = param (IDs::voiceDensity);
    const float pitchSpreadSemi = param (IDs::pitchSpread);
    const float detuneCents = param (IDs::detune);
    const float pitchConvergeAmt = param (IDs::pitchConverge) * macroParam;
    const int scaleMode = (int) param (IDs::scaleLock);
    const float panSpreadAmt = param (IDs::panSpread);
    const float panConvergeAmt = param (IDs::panConverge) * macroParam;
    const float toneConvergeAmt = param (IDs::toneConverge) * macroParam;
    const float attractionAmt = param (IDs::attraction);
    const float turbulenceAmt = param (IDs::turbulence);
    const float overshootAmt = param (IDs::overshoot);
    const float orbitAmt = param (IDs::orbit);
    const float progRevealAmt = param (IDs::progReveal);
    const float voiceAgeAmt = param (IDs::voiceAge);
    const int voiceDir = (int) param (IDs::voiceDirection);
    const float shapeVal = param (IDs::shape);
    const float formantShift = param (IDs::formant);

    juce::Random rnd ((juce::int64) param (IDs::seed));

    int postReleaseExtraSamples = 0;
    if (postReleaseMode == 1 || postReleaseMode == 2)
        postReleaseExtraSamples = (int) (sr * 1.5);

    const int totalSwarmLen = swellLen + postReleaseExtraSamples;
    juce::AudioBuffer<float> swarmBuffer (2, totalSwarmLen);
    swarmBuffer.clear();

    const int numSlices = 8;
    const int sliceLen = srcLen / numSlices;
    const int grainSamples = (int) (sr * (grainSizeMs * 0.001f));

    // Formant pitch multiplier
    const double formantRatio = std::pow (2.0, (double) formantShift / 12.0);

    // Render Voices
    for (int v = 0; v < numVoices; ++v)
    {
        const float vNorm = (numVoices > 1) ? (float) v / (float) (numVoices - 1) : 0.5f;

        const float hTime = (rnd.nextFloat() * 2.0f - 1.0f) * humanizeAmt * 0.015f;
        const float hPitch = (rnd.nextFloat() * 2.0f - 1.0f) * humanizeAmt * 0.12f;

        const float densityNorm = tensionCurve (vNorm, densityCurveParam);
        const float rawOffsetSec = (1.0f - densityNorm) * timeSpreadSec + hTime;
        const float offsetSec = juce::jmax (0.01f, rawOffsetSec * (1.0f - timeConvergeAmt * 0.7f));
        const int voiceStartSample = juce::jlimit (0, swellLen - 1, (int) ((swellSeconds - offsetSec) * sr));

        const float randPitch = (rnd.nextFloat() * 2.0f - 1.0f) * pitchSpreadSemi;
        const float microDetune = (vNorm - 0.5f) * 2.0f * (detuneCents / 100.0f) + hPitch;
        const float initPitchSemi = quantizeToScale (randPitch, scaleMode) + microDetune;

        bool isRev = false;
        if (voiceDir == 1) isRev = true;
        else if (voiceDir == 2) isRev = (v % 2 == 1);
        else if (voiceDir == 3) isRev = (rnd.nextFloat() > 0.5f);

        const float revealFrac = juce::jlimit (0.08f, 1.0f, vNorm + (1.0f - progRevealAmt) * 0.92f);
        const int playableSrcLen = juce::jmax (64, (int) (srcLen * revealFrac));

        int srcOffset = 0;
        if (sMode == 3 && sliceLen > 64) srcOffset = (v % numSlices) * sliceLen;

        const float initPan = (vNorm * 2.0f - 1.0f) * panSpreadAmt;
        float ageFilterL = 0.0f, ageFilterR = 0.0f;
        const float ageCoeff = juce::jlimit (0.0f, 0.85f, (1.0f - vNorm) * voiceAgeAmt);

        // Tone Convergence: each voice starts with a scattered random brightness bias and
        // converges toward the full (unfiltered) target spectrum as toneConvergeAmt * progress rises.
        float toneFilterL = 0.0f, toneFilterR = 0.0f;
        const float voiceToneRand = rnd.nextFloat();

        // Character mode state
        float bbdFilterL = 0.0f, bbdFilterR = 0.0f;
        float bbdNoisePhase = rnd.nextFloat() * 100.0f;

        double readPos = 0.0;
        const int voiceActiveSamples = totalSwarmLen - voiceStartSample;
        if (voiceActiveSamples <= 0) continue;

        for (int i = voiceStartSample; i < totalSwarmLen; ++i)
        {
            float progress = 0.0f;
            if (i < swellLen) progress = (float) (i - voiceStartSample) / (float) juce::jmax (1, swellLen - voiceStartSample);
            else { if (postReleaseMode == 0) break; progress = 1.0f; }

            if (isFrozen) progress = 0.5f;
            const float effectiveProgress = isRevConverge ? (1.0f - progress) : progress;

            // Pitch trajectory with Focus acceleration
            const float focusCurve = std::pow (effectiveProgress, 1.0f + focusAmt * 2.0f);
            const float convergeCurve = tensionCurve (focusCurve, attractionAmt);
            float curPitchSemi = initPitchSemi * (1.0f - pitchConvergeAmt * convergeCurve);

            if (overshootAmt > 0.01f && effectiveProgress > 0.6f)
            {
                const float spring = std::sin ((effectiveProgress - 0.6f) * 16.0f) * std::exp (-(effectiveProgress - 0.6f) * 6.0f);
                curPitchSemi += initPitchSemi * overshootAmt * spring;
            }

            if (turbulenceAmt > 0.01f)
            {
                const float turb = std::sin (effectiveProgress * 42.0f + (float) v * 3.7f) * turbulenceAmt * 0.8f;
                curPitchSemi += turb;
            }

            // String Ensemble dual-LFO chorusing (Solina emulation)
            if (charMode == 5)
            {
                const float lfo1 = std::sin ((float) i / sr * juce::MathConstants<float>::twoPi * 0.6f + (float) v * 1.2f);
                const float lfo2 = std::sin ((float) i / sr * juce::MathConstants<float>::twoPi * 6.0f + (float) v * 1.8f);
                curPitchSemi += (lfo1 * 0.18f + lfo2 * 0.08f);
            }
            // Tape Choir flutter & wow
            else if (charMode == 3)
            {
                const float wow = std::sin ((float) i / sr * juce::MathConstants<float>::twoPi * 1.5f + (float) v) * 0.12f;
                const float flutter = std::sin ((float) i / sr * juce::MathConstants<float>::twoPi * 14.0f) * 0.06f;
                curPitchSemi += (wow + flutter);
            }

            if (i >= swellLen && postReleaseMode == 2)
            {
                const float postFrac = (float) (i - swellLen) / (float) postReleaseExtraSamples;
                curPitchSemi += initPitchSemi * postFrac * 0.75f;
            }

            const double pitchSpeed = std::pow (2.0, (curPitchSemi + (float) (formantRatio - 1.0) * 4.0f) / 12.0);

            // Pan & Width trajectory
            float curPan = initPan;
            if (panConvergeAmt > 0.0f) curPan = initPan * (1.0f - panConvergeAmt * effectiveProgress);
            else if (panConvergeAmt < 0.0f) curPan = initPan * (1.0f + (-panConvergeAmt) * effectiveProgress * 1.5f);

            if (orbitAmt > 0.01f)
            {
                const float orbitAngle = effectiveProgress * juce::MathConstants<float>::twoPi * 2.0f + vNorm * juce::MathConstants<float>::twoPi;
                curPan = juce::jlimit (-1.0f, 1.0f, curPan + std::sin (orbitAngle) * orbitAmt * 0.7f);
            }

            if (i >= swellLen && postReleaseMode == 2)
            {
                const float postFrac = (float) (i - swellLen) / (float) postReleaseExtraSamples;
                curPan = juce::jlimit (-1.0f, 1.0f, curPan * (1.0f + postFrac * 2.0f));
            }

            float panL = std::cos ((curPan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
            float panR = std::sin ((curPan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);

            // Dimension Mode (Dimension D cross-coupling preserving mono center)
            if (charMode == 4)
            {
                panL = (panL * 0.85f - panR * 0.25f);
                panR = (panR * 0.85f - panL * 0.25f);
            }

            // Sample read with Granular Cloud or direct slice
            int sIdx = (int) readPos;
            float grainEnv = 1.0f;
            if (charMode == 6 && grainSamples > 16) // Granular Cloud
            {
                const int gPos = sIdx % grainSamples;
                grainEnv = 0.5f * (1.0f - std::cos ((float) gPos / (float) grainSamples * juce::MathConstants<float>::twoPi));
            }

            if (sMode == 3) sIdx = srcOffset + (sIdx % juce::jmax (1, sliceLen));
            else sIdx = sIdx % playableSrcLen;

            if (isRev) sIdx = (srcOffset + playableSrcLen - 1) - (sIdx % playableSrcLen);
            sIdx = juce::jlimit (0, srcLen - 2, sIdx);

            const float frac = (float) (readPos - (int) readPos);
            float sampL = source.getSample (0, sIdx) + (source.getSample (0, sIdx + 1) - source.getSample (0, sIdx)) * frac;
            float sampR = source.getSample (1, sIdx) + (source.getSample (1, sIdx + 1) - source.getSample (1, sIdx)) * frac;
            sampL *= grainEnv;
            sampR *= grainEnv;

            // Character Sound Shaping
            if (charMode == 1 || charMode == 3) // Analog / Tape saturation
            {
                sampL = std::tanh (sampL * 1.35f);
                sampR = std::tanh (sampR * 1.35f);
            }
            else if (charMode == 2) // Bucket-Brigade (clock filtering & subtle BBD noise)
            {
                bbdFilterL = bbdFilterL * 0.65f + sampL * 0.35f;
                bbdFilterR = bbdFilterR * 0.65f + sampR * 0.35f;
                bbdNoisePhase += 0.05f;
                const float bbdNoise = std::sin (bbdNoisePhase * 17.0f) * 0.003f;
                sampL = std::tanh (bbdFilterL) + bbdNoise;
                sampR = std::tanh (bbdFilterR) + bbdNoise;
            }
            else if (charMode == 7) // Lo-Fi bit reduction
            {
                sampL = std::floor (sampL * 16.0f) / 16.0f;
                sampR = std::floor (sampR * 16.0f) / 16.0f;
            }

            // Voice Age darkening
            if (ageCoeff > 0.01f)
            {
                ageFilterL = ageFilterL * ageCoeff + sampL * (1.0f - ageCoeff);
                ageFilterR = ageFilterR * ageCoeff + sampR * (1.0f - ageCoeff);
                sampL = ageFilterL;
                sampR = ageFilterR;
            }

            // Tone Convergence: scattered bright/dark voices progressively match the target spectrum
            const float toneFilterAmt = (1.0f - juce::jlimit (0.0f, 1.0f, toneConvergeAmt * effectiveProgress)) * voiceToneRand;
            if (toneFilterAmt > 0.01f)
            {
                const float toneCoeff = 1.0f - toneFilterAmt * 0.85f;
                toneFilterL += toneCoeff * (sampL - toneFilterL);
                toneFilterR += toneCoeff * (sampR - toneFilterR);
                sampL = toneFilterL;
                sampR = toneFilterR;
            }

            // Click Protection & Reverse Turnaround Fades (5ms cosine ramp)
            const int fadeLen = (int) (sr * 0.005);
            if (i - voiceStartSample < fadeLen)
            {
                const float inRamp = 0.5f * (1.0f - std::cos ((float) (i - voiceStartSample) / (float) fadeLen * juce::MathConstants<float>::pi));
                sampL *= inRamp;
                sampR *= inRamp;
            }

            // 3D Distance Simulation: approaches from far cavern to upfront dry
            float distFactor = 1.0f;
            if (distanceAmt > 0.01f && i < swellLen)
            {
                const float distNorm = (1.0f - effectiveProgress) * distanceAmt;
                distFactor = 1.0f - distNorm * 0.45f;
            }

            // Swell envelope
            float swellGain = 1.0f;
            if (i < swellLen)
            {
                const float globalSwellNorm = (float) i / (float) juce::jmax (1, swellLen - 1);
                swellGain = tensionCurve (globalSwellNorm, shapeVal);
            }
            else
            {
                const float postFrac = (float) (i - swellLen) / (float) postReleaseExtraSamples;
                swellGain = std::cos (postFrac * juce::MathConstants<float>::halfPi);
            }

            const float voiceAmp = 1.0f / std::sqrt ((float) numVoices);
            swarmBuffer.addSample (0, i, sampL * swellGain * panL * voiceAmp * distFactor);
            swarmBuffer.addSample (1, i, sampR * swellGain * panR * voiceAmp * distFactor);

            readPos += pitchSpeed;
        }
    }

    const float swarmMag = swarmBuffer.getMagnitude (0, totalSwarmLen);
    if (swarmMag > 0.001f) swarmBuffer.applyGain (0.90f / swarmMag);

    // 4. Drive & Saturation
    const float driveAmt = param (IDs::drive);
    if (driveAmt > 0.01f)
    {
        const float driveGain = 1.0f + driveAmt * 3.0f;
        for (int ch = 0; ch < 2; ++ch)
        {
            float* ptr = swarmBuffer.getWritePointer (ch);
            for (int i = 0; i < totalSwarmLen; ++i)
                ptr[i] = std::tanh (ptr[i] * driveGain);
        }
    }

    // 4b. Transient Softening / Preservation (differential fast/slow envelope shaper)
    const float transAmt = param (IDs::transients);
    if (std::abs (transAmt) > 0.01f)
    {
        float fastEnvL = 0.0f, fastEnvR = 0.0f, slowEnvL = 0.0f, slowEnvR = 0.0f;
        const float fastCoeff = 1.0f - std::exp (-1.0f / (0.001f * (float) sr));
        const float slowCoeff = 1.0f - std::exp (-1.0f / (0.050f * (float) sr));
        float* ptrL = swarmBuffer.getWritePointer (0);
        float* ptrR = swarmBuffer.getWritePointer (1);
        for (int i = 0; i < totalSwarmLen; ++i)
        {
            const float absL = std::abs (ptrL[i]), absR = std::abs (ptrR[i]);
            fastEnvL += fastCoeff * (absL - fastEnvL);
            fastEnvR += fastCoeff * (absR - fastEnvR);
            slowEnvL += slowCoeff * (absL - slowEnvL);
            slowEnvR += slowCoeff * (absR - slowEnvR);
            const float transL = juce::jlimit (0.0f, 1.0f, (fastEnvL - slowEnvL) * 6.0f);
            const float transR = juce::jlimit (0.0f, 1.0f, (fastEnvR - slowEnvR) * 6.0f);
            ptrL[i] *= juce::jmax (0.0f, 1.0f + transAmt * transL * 0.9f);
            ptrR[i] *= juce::jmax (0.0f, 1.0f + transAmt * transR * 0.9f);
        }
    }

    // 5. Space & 3D Distance Diffusion
    const float spaceAmt = param (IDs::space);
    if (spaceAmt > 0.01f || distanceAmt > 0.01f)
    {
        const float totalSpace = juce::jlimit (0.0f, 1.0f, spaceAmt + distanceAmt * 0.35f);
        const int delayL = (int) (sr * 0.031), delayR = (int) (sr * 0.043);
        std::vector<float> bufL ((size_t) delayL, 0.0f), bufR ((size_t) delayR, 0.0f);
        int idxL = 0, idxR = 0;
        float* ptrL = swarmBuffer.getWritePointer (0);
        float* ptrR = swarmBuffer.getWritePointer (1);

        for (int i = 0; i < totalSwarmLen; ++i)
        {
            const float inL = ptrL[i], inR = ptrR[i];
            const float dL = bufL[(size_t) idxL], dR = bufR[(size_t) idxR];
            bufL[(size_t) idxL] = inL + dL * 0.62f * totalSpace;
            bufR[(size_t) idxR] = inR + dR * 0.62f * totalSpace;
            idxL = (idxL + 1) % delayL;
            idxR = (idxR + 1) % delayR;
            ptrL[i] = inL * (1.0f - totalSpace * 0.35f) + dL * (totalSpace * 0.65f);
            ptrR[i] = inR * (1.0f - totalSpace * 0.35f) + dR * (totalSpace * 0.65f);
        }
    }

    // 6. Tone, Bass Cut, Resonance & Tilt EQ
    auto applyIIR = [&] (const juce::IIRCoefficients& coeffs)
    {
        juce::IIRFilter fL, fR;
        fL.setCoefficients (coeffs); fR.setCoefficients (coeffs);
        fL.processSamples (swarmBuffer.getWritePointer (0), totalSwarmLen);
        fR.processSamples (swarmBuffer.getWritePointer (1), totalSwarmLen);
    };

    const float hp = param (IDs::basscut);
    const float lp = param (IDs::tone);
    const float q = param (IDs::resonance);

    if (hp > 21.0f) applyIIR (juce::IIRCoefficients::makeHighPass (sr, hp, q));
    if (lp < 19900.0f) applyIIR (juce::IIRCoefficients::makeLowPass (sr, lp, q));

    // Tilt EQ & Presence
    const float tiltAmt = param (IDs::tilt);
    if (std::abs (tiltAmt) > 0.01f)
    {
        const float tiltGain = std::pow (10.0f, tiltAmt * 0.3f);
        applyIIR (juce::IIRCoefficients::makeLowShelf (sr, 1000.0, 0.707, 1.0f / tiltGain));
        applyIIR (juce::IIRCoefficients::makeHighShelf (sr, 1000.0, 0.707, tiltGain));
    }

    const float presenceAmt = param (IDs::presence);
    if (presenceAmt > 0.01f)
    {
        const float presGain = 1.0f + presenceAmt * 1.5f;
        applyIIR (juce::IIRCoefficients::makeHighShelf (sr, 10000.0, 0.707, presGain));
    }

    // High-frequency Air Exciter (filtered high-frequency excitation on ensemble and target)
    const float airAmt = param (IDs::air);
    if (airAmt > 0.01f)
    {
        juce::IIRFilter airHpL, airHpR;
        airHpL.setCoefficients (juce::IIRCoefficients::makeHighPass (sr, 8500.0));
        airHpR.setCoefficients (juce::IIRCoefficients::makeHighPass (sr, 8500.0));
        float* ptrL = swarmBuffer.getWritePointer (0);
        float* ptrR = swarmBuffer.getWritePointer (1);
        for (int i = 0; i < totalSwarmLen; ++i)
        {
            const float hfL = airHpL.processSingleSampleRaw (ptrL[i]);
            const float hfR = airHpR.processSingleSampleRaw (ptrR[i]);
            ptrL[i] += std::tanh (hfL * (1.0f + airAmt * 2.5f)) * airAmt * 0.45f;
            ptrR[i] += std::tanh (hfR * (1.0f + airAmt * 2.5f)) * airAmt * 0.45f;
        }

        if (source.getNumSamples() > 0)
        {
            juce::IIRFilter targetAirL, targetAirR;
            targetAirL.setCoefficients (juce::IIRCoefficients::makeHighPass (sr, 8500.0));
            targetAirR.setCoefficients (juce::IIRCoefficients::makeHighPass (sr, 8500.0));
            float* sL = source.getWritePointer (0);
            float* sR = source.getWritePointer (1);
            for (int i = 0; i < source.getNumSamples(); ++i)
            {
                const float hfL = targetAirL.processSingleSampleRaw (sL[i]);
                const float hfR = targetAirR.processSingleSampleRaw (sR[i]);
                sL[i] += std::tanh (hfL * (1.0f + airAmt * 2.5f)) * airAmt * 0.45f;
                sR[i] += std::tanh (hfR * (1.0f + airAmt * 2.5f)) * airAmt * 0.45f;
            }
        }
    }

    // 7. Mono Bass (Mono-maker below monoBass cutoff)
    const float monoCutoff = param (IDs::monoBass);
    if (monoCutoff > 21.0f)
    {
        // Extract Side channel and highpass it
        juce::IIRFilter sideHp;
        sideHp.setCoefficients (juce::IIRCoefficients::makeHighPass (sr, monoCutoff));
        float* ptrL = swarmBuffer.getWritePointer (0);
        float* ptrR = swarmBuffer.getWritePointer (1);
        for (int i = 0; i < totalSwarmLen; ++i)
        {
            const float mid = (ptrL[i] + ptrR[i]) * 0.5f;
            float side = (ptrL[i] - ptrR[i]) * 0.5f;
            side = sideHp.processSingleSampleRaw (side);
            ptrL[i] = mid + side;
            ptrR[i] = mid - side;
        }
    }

    // 8. Combine: Swell + Climax Hit (or Dry Replacement)
    const float dryReplaceAmt = param (IDs::dryReplace);
    const int hitLen = juce::jmin (srcLen, (int) (sr * 2.0));
    const int fullLen = totalSwarmLen + hitLen;
    juce::AudioBuffer<float> full (2, fullLen);
    full.clear();

    for (int ch = 0; ch < 2; ++ch)
    {
        full.copyFrom (ch, 0, swarmBuffer, ch, 0, totalSwarmLen);
        if (dryReplaceAmt < 0.99f)
        {
            full.copyFrom (ch, swellLen, source, ch, 0, hitLen);
            full.applyGain (ch, swellLen, hitLen, 1.0f - dryReplaceAmt);
        }
    }
    out->fullLengthSec = fullLen / sr;
    out->beats = beats;

    // 9. Trim
    int tStart = (int) (param (IDs::trimStart) * fullLen);
    int tEnd   = (int) (param (IDs::trimEnd) * fullLen);
    tStart = juce::jlimit (0, fullLen - 1, tStart);
    tEnd   = juce::jlimit (tStart + (int) (sr * 0.02), fullLen, tEnd);
    const int trimLen = tEnd - tStart;
    out->trimStartSec = tStart / sr;
    out->trimEndSec   = tEnd / sr;
    int hitIdx = swellLen - tStart;
    if (hitIdx < 0 || hitIdx >= trimLen) hitIdx = -1;

    // 10. Pitch sweep
    const float pitchAmt = param (IDs::pitch);
    const float octaves = kPitchOct[juce::jlimit (0, 2, (int) param (IDs::pitchRange))];
    const float pitchT = param (IDs::pitchTension);
    juce::AudioBuffer<float> outBuf;
    std::vector<float> semiPerSample;
    int hitOut = -1;

    if (std::abs (pitchAmt) > 0.001f)
    {
        std::vector<float> l, r;
        l.reserve ((size_t) trimLen * 2); r.reserve ((size_t) trimLen * 2);
        const float* fl = full.getReadPointer (0) + tStart;
        const float* fr = full.getReadPointer (1) + tStart;
        double p = 0.0;
        while (p < trimLen - 1 && l.size() < (size_t) (sr * 60.0))
        {
            const int i0 = (int) p; const float frac = (float) (p - i0);
            l.push_back (fl[i0] + (fl[i0 + 1] - fl[i0]) * frac);
            r.push_back (fr[i0] + (fr[i0 + 1] - fr[i0]) * frac);
            const float semis = pitchAmt * octaves * 12.0f * tensionCurve ((float) p / (float) trimLen, pitchT);
            semiPerSample.push_back (semis);
            if (hitIdx >= 0 && hitOut < 0 && p >= hitIdx) hitOut = (int) l.size() - 1;
            p += std::pow (2.0, semis / 12.0);
        }
        outBuf.setSize (2, (int) l.size());
        for (size_t i = 0; i < l.size(); ++i) { outBuf.setSample (0, (int) i, l[i]); outBuf.setSample (1, (int) i, r[i]); }
    }
    else
    {
        outBuf.setSize (2, trimLen);
        for (int ch = 0; ch < 2; ++ch) outBuf.copyFrom (ch, 0, full, ch, tStart, trimLen);
        semiPerSample.assign ((size_t) trimLen, 0.0f);
        hitOut = hitIdx;
    }

    // 11. Volume envelope
    const int n = outBuf.getNumSamples();
    const float v0 = param (IDs::volStart), v1 = param (IDs::volEnd), vt = param (IDs::volTension);
    const bool flatVol = std::abs (v0 - 1.0f) < 0.001f && std::abs (v1 - 1.0f) < 0.001f;
    for (int i = 0; i < n; ++i)
    {
        float g = 1.0f;
        if (! flatVol)
        {
            const float lvl = v0 + (v1 - v0) * tensionCurve ((float) i / (float) juce::jmax (1, n - 1), vt);
            g = lvl * lvl;
            for (int ch = 0; ch < 2; ++ch) outBuf.getWritePointer (ch)[i] *= g;
        }
        if (i % RenderedSample::envStep == 0) { out->gainLin.push_back (g); out->pitchSemi.push_back (semiPerSample[(size_t) i]); }
    }

    out->audio = std::move (outBuf);
    out->hitIndex = hitOut;

    const int latency = out->hitIndex > 0 ? out->hitIndex : 0;

    std::shared_ptr<RenderedSample> previous;
    {
        const juce::ScopedLock l (renderedOwnerLock);
        previous = std::move (rendered);
        rendered = out;
    }

    if (previous != nullptr)
        retiredRendered.push_back (std::move (previous));

    realtimeRendered.store (out.get(), std::memory_order_release);
    reportedTailSeconds.store (out->sampleRate > 0.0
                                   ? (double) out->audio.getNumSamples() / out->sampleRate
                                   : 0.0);

    if (realtimeReaders.load (std::memory_order_acquire) == 0)
        retiredRendered.clear();

    setLatencySamples (param (IDs::align) > 0.5f ? latency : 0);
}

// ---------------- Realtime Process Block with Ducking ----------------

void PreChorusProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    // Transport Recovery & Follow Tempo
    float conf = 1.0f;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (pos->getBpm())
            {
                double curBpm = *pos->getBpm();
                if (std::abs (curBpm - lastKnownBpm) > 0.05)
                {
                    lastKnownBpm = curBpm;
                    hostBpm.store (curBpm);
                    dirty = true;
                }
            }

            if (pos->getTimeInSamples())
            {
                juce::int64 curSample = *pos->getTimeInSamples();
                if (lastPlayheadSample >= 0 && std::abs (curSample - (lastPlayheadSample + numSamples)) > 2048)
                {
                    for (auto& v : voices) v.active = false;
                    playhead.store (-1);
                }
                lastPlayheadSample = curSample;
            }
        }
    }

    if (param (IDs::align) < 0.5f) conf *= 0.8f;
    if (param (IDs::sync) < 0.5f) conf *= 0.85f;
    targetConfidence.store (conf);

    // Live Input Meter & Sidechain Ducking Follower
    float inPeak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        inPeak = juce::jmax (inPeak, buffer.getMagnitude (ch, 0, numSamples));
    inputMeter.store (inPeak);

    const float duckAmt = param (IDs::ducking);
    if (duckAmt > 0.01f)
    {
        // Fast attack (10ms), smooth release (150ms)
        const float targetDuck = juce::jlimit (0.0f, 1.0f, inPeak * duckAmt * 2.5f);
        duckEnv = duckEnv * 0.9f + targetDuck * 0.1f;
    }
    else
    {
        duckEnv = 0.0f;
    }
    const float duckGain = 1.0f - duckEnv;

    const auto state = captureState.load();
    if (state == CaptureState::armed)
    {
        const int mode = (int) param (IDs::captureMode);
        const float threshLin = juce::Decibels::decibelsToGain (param (IDs::thresh));
        if (mode == 7 || inPeak >= threshLin)
        {
            captureState.store (CaptureState::recording);
            captureWritePos = 0;
            captureSilenceCounter = 0;

            if (mode >= 1 && mode <= 6)
            {
                const double beatFractions[] = { 0.25, 0.5, 1.0, 2.0, 4.0, 8.0 };
                const double secPerBeat = 60.0 / juce::jlimit (30.0, 300.0, hostBpm.load());
                captureTargetSamples = (int) (beatFractions[mode - 1] * secPerBeat * hostSampleRate);
            }
            else
            {
                captureTargetSamples = (int) (hostSampleRate * 6.0);
            }
        }
    }
    else if (state == CaptureState::recording)
    {
        const int spaceLeft = captureRingBuffer.getNumSamples() - captureWritePos;
        const int toCopy = juce::jmin (numSamples, spaceLeft);
        if (toCopy > 0)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                const int inCh = juce::jmin (ch, buffer.getNumChannels() - 1);
                captureRingBuffer.copyFrom (ch, captureWritePos, buffer, inCh, 0, toCopy);
            }
            captureWritePos += toCopy;
        }

        const int mode = (int) param (IDs::captureMode);
        if (mode == 0)
        {
            const float threshLin = juce::Decibels::decibelsToGain (param (IDs::thresh));
            if (inPeak < threshLin * 0.5f) captureSilenceCounter += numSamples;
            else captureSilenceCounter = 0;

            if ((captureWritePos > (int) (hostSampleRate * 0.3) && captureSilenceCounter > (int) (hostSampleRate * 0.35))
                || captureWritePos >= captureTargetSamples)
            {
                requestCaptureFinalizeRealtime();
            }
        }
        else if (captureWritePos >= captureTargetSamples)
        {
            requestCaptureFinalizeRealtime();
        }
    }

    if (stopRequest.exchange (0))
    {
        for (auto& v : voices) v.active = false;
        playhead.store (-1);
    }
    if (triggerRequest.exchange (0)) startVoice (1.0f);

    const int seqMode = (int) param (IDs::sequence);
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            bool allowTrigger = true;
            if (seqMode > 0)
            {
                if (auto* ph = getPlayHead())
                {
                    if (auto pos = ph->getPosition())
                    {
                        if (pos->getPpqPosition())
                        {
                            const double ppq = *pos->getPpqPosition();
                            const double barFrac = std::fmod (ppq, 4.0);
                            if (seqMode == 1 && barFrac > 0.4) allowTrigger = false;
                            else if (seqMode == 2 && std::fmod (std::floor (ppq / 4.0), 2.0) != 0.0) allowTrigger = false;
                            else if (seqMode == 3 && std::fmod (std::floor (ppq / 4.0), 4.0) != 0.0) allowTrigger = false;
                        }
                    }
                }
            }
            if (allowTrigger) startVoice (m.getFloatVelocity());
        }
        else if (m.isAllNotesOff()) for (auto& v : voices) v.active = false;
    }

    realtimeReaders.fetch_add (1, std::memory_order_acq_rel);
    const auto* r = realtimeRendered.load (std::memory_order_acquire);
    if (r == nullptr || r->audio.getNumSamples() == 0)
    {
        realtimeReaders.fetch_sub (1, std::memory_order_release);
        return;
    }

    const float dryLvl = dryParam != nullptr ? dryParam->load() : 1.0f;
    const float wetLvl = wetParam != nullptr ? wetParam->load() : 1.0f;

    if (dryLvl > 0.0f && buffer.getNumChannels() > 0) buffer.applyGain (dryLvl);
    else buffer.clear();

    int activePos = -1;
    for (auto& v : voices)
    {
        if (! v.active) continue;
        renderRange (buffer, *r, v.pos, numSamples, dryLvl, wetLvl * v.gain, duckGain);
        v.pos += numSamples;
        activePos = v.pos;
        if (v.pos >= r->audio.getNumSamples()) v.active = false;
    }
    playhead.store (activePos);
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peak = juce::jmax (peak, buffer.getMagnitude (ch, 0, numSamples));
    outputMeter.store (peak);
    realtimeReaders.fetch_sub (1, std::memory_order_release);
}

void PreChorusProcessor::startVoice (float gain)
{
    Voice* best = nullptr;
    juce::uint32 oldestId = 0xffffffff;
    for (auto& v : voices)
    {
        if (! v.active) { best = &v; break; }
        if (v.id < oldestId) { oldestId = v.id; best = &v; }
    }
    if (best != nullptr)
    {
        best->active = true;
        best->pos = 0;
        best->gain = gain;
        best->id = ++voiceCounter;
    }
}

void PreChorusProcessor::renderRange (juce::AudioBuffer<float>& out, const RenderedSample& r, int start, int num, float dry, float wet, float duckGain)
{
    const int total = r.audio.getNumSamples();
    if (start >= total) return;
    const int available = juce::jmin (num, total - start);
    const int hitAt = r.hitIndex >= 0 ? r.hitIndex : total;

    for (int ch = 0; ch < juce::jmin (out.getNumChannels(), 2); ++ch)
    {
        float* dst = out.getWritePointer (ch);
        const float* src = r.audio.getReadPointer (ch) + start;
        for (int i = 0; i < available; ++i)
        {
            const int sampleIdx = start + i;
            const float g = (sampleIdx < hitAt) ? (wet * duckGain) : dry;
            dst[i] += src[i] * g;
        }
    }
}

// ---------------- Sample File Management ----------------

void PreChorusProcessor::refreshFolderList (const juce::File& f)
{
    auto dir = f.getParentDirectory();
    if (folderFiles.isEmpty() || folderFiles[0].getParentDirectory() != dir)
    {
        folderFiles = dir.findChildFiles (juce::File::findFiles, false, "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        folderFiles.sort();
    }
    currentIndex = folderFiles.indexOf (f);
}

bool PreChorusProcessor::loadSampleFile (const juce::File& f, bool previewAfter)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (f));
    if (reader == nullptr || reader->lengthInSamples <= 0) return false;
    const int len = (int) juce::jmin<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 12.0));

    // Always store stereo: the render engine unconditionally reads both channels of the
    // source buffer, so a mono file loaded as a 1-channel buffer would read past the end
    // of its channel pointer array later on. Duplicate mono sources to both channels.
    juce::AudioBuffer<float> buf (2, len);
    if (reader->numChannels >= 2)
    {
        reader->read (&buf, 0, len, 0, true, true);
    }
    else
    {
        juce::AudioBuffer<float> mono (1, len);
        reader->read (&mono, 0, len, 0, true, true);
        buf.copyFrom (0, 0, mono, 0, 0, len);
        buf.copyFrom (1, 0, mono, 0, 0, len);
    }
    {
        const juce::ScopedLock sl (sourceLock);
        loadedBuffer = std::move (buf);
        loadedSR = reader->sampleRate;
    }
    currentFile = f;
    refreshFolderList (f);
    // Loading a file only has an audible effect in "Loaded Sample" (and later, "Hybrid"/
    // "Slice Scatter") modes; "Live Capture" ignores loadedBuffer entirely. Switch out of
    // Live Capture automatically so Load/drag-and-drop always does what the user expects.
    if ((int) param (IDs::sourceMode) == 0) setParam (IDs::sourceMode, 1.0f);
    if (previewAfter) previewAfterRender = true;
    dirty = true;
    return true;
}

void PreChorusProcessor::nextSample()
{
    if (folderFiles.isEmpty()) return;
    for (int tries = 0; tries < folderFiles.size(); ++tries)
    {
        currentIndex = (currentIndex + 1) % folderFiles.size();
        if (loadSampleFile (folderFiles[currentIndex], true)) return;
    }
}

void PreChorusProcessor::prevSample()
{
    if (folderFiles.isEmpty()) return;
    for (int tries = 0; tries < folderFiles.size(); ++tries)
    {
        currentIndex = (currentIndex - 1 + folderFiles.size()) % folderFiles.size();
        if (loadSampleFile (folderFiles[currentIndex], true)) return;
    }
}

bool PreChorusProcessor::exportWav (const juce::File& dest)
{
    if (dirty.exchange (false)) render();
    auto r = getRendered();
    if (r == nullptr || r->audio.getNumSamples() == 0) return false;
    const int n = r->audio.getNumSamples();
    const int hitAt = r->hitIndex >= 0 ? r->hitIndex : n;
    juce::AudioBuffer<float> mix;
    mix.makeCopyOf (r->audio);
    for (int ch = 0; ch < 2; ++ch)
    {
        mix.applyGain (ch, 0, hitAt, wetParam->load());
        mix.applyGain (ch, hitAt, n - hitAt, dryParam->load());
    }
    dest.deleteFile();
    std::unique_ptr<juce::FileOutputStream> os (dest.createOutputStream());
    if (os == nullptr || ! os->openedOk()) return false;
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), r->sampleRate, 2, 24, {}, 0));
    if (w == nullptr) return false;
    os.release();
    w->writeFromAudioSampleBuffer (mix, 0, n);
    return true;
}

void PreChorusProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("file", currentFile.getFullPathName(), nullptr);
    state.setProperty ("activeSlot", activeSlot.load(), nullptr);
    state.setProperty ("captureLock", captureLockState.load(), nullptr);
    state.setProperty ("currentCompareSlot", currentCompareSlot.load(), nullptr);

    for (int i = 0; i < 2; ++i)
    {
        if (compareStates[(size_t) i].isValid())
            if (auto xml = compareStates[(size_t) i].createXml())
                state.setProperty (i == 0 ? "compareA" : "compareB", xml->toString(), nullptr);
    }

    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void PreChorusProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (! state.isValid()) return;
        apvts.replaceState (state);
        const int restoredSourceMode = (int) param (IDs::sourceMode);
        juce::File f (state.getProperty ("file", "").toString());
        if (f.existsAsFile())
        {
            loadSampleFile (f);
            setParam (IDs::sourceMode, (float) restoredSourceMode);
        }
        activeSlot.store (state.getProperty ("activeSlot", 0));
        captureLockState.store (state.getProperty ("captureLock", false));
        currentCompareSlot.store (juce::jlimit (0, 1, (int) state.getProperty ("currentCompareSlot", 0)));

        for (int i = 0; i < 2; ++i)
        {
            const auto text = state.getProperty (i == 0 ? "compareA" : "compareB", "").toString();
            if (text.isNotEmpty())
            {
                if (auto compareXml = juce::XmlDocument::parse (text))
                {
                    auto compareState = juce::ValueTree::fromXml (*compareXml);
                    if (compareState.isValid() && compareState.getType() == apvts.state.getType())
                        compareStates[(size_t) i] = compareState;
                }
            }
        }
        dirty = true;
    }
}

juce::AudioProcessorEditor* PreChorusProcessor::createEditor() { return new PreChorusEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PreChorusProcessor(); }
