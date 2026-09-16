#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const float kPitchOct[] = { 1.0f, 2.0f, 4.0f };
    const int kSyncBars[]   = { 2, 4, 8, 16 }; // beats: 1/2 bar(2), 1 bar(4), 2 bars(8), 4 bars(16)

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

    for (int i = 0; i < kNumCc; ++i)
    {
        ccToParamIndex[(size_t) i].store (-1);
        ccPendingValue[(size_t) i].store (0.0f);
        ccHasPending[(size_t) i].store (false);
    }

    distressRng.setSeed (juce::Random::getSystemRandom().nextInt64());
    lastActivityMs.store (juce::Time::getMillisecondCounter());
    secondsUntilNextCry = 0.0;

    // One load in thirty, it has something on its mind twenty-odd minutes from now.
    if (distressRng.nextInt (30) == 0)
        ranchCrySeconds = 1200.0f + distressRng.nextFloat() * 600.0f;

    colony.resetAll ((int) param (IDs::voiceCount));
    lastKnownVoiceCount = (int) param (IDs::voiceCount);
    lastColonyStepMs = juce::Time::getMillisecondCounter();

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

    talkbox.prepare (sampleRate);
    whistles.prepare (sampleRate);
    outputReverb.setSampleRate (sampleRate);
    delayBuffer.setSize (2, juce::jmax (1024, (int) (sampleRate * 1.2)));
    delayBuffer.clear();
    delayWritePos = 0;
    washFilterState = { 0.0f, 0.0f };
    playbackRate = 1.0f;
    noteActivity();

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

void PreChorusProcessor::resetAllToDefaults()
{
    // "Back to the default settings" - every automatable parameter, not just the edits.
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    // Keep the non-automatable state in step with the parameters we just reset.
    colony.resetAll ((int) param (IDs::voiceCount));
    lastKnownVoiceCount = (int) param (IDs::voiceCount);
    activeSlot.store (0);
    captureLockState.store (false);
    currentPresetFile = juce::File();
    hasRandomizedOnce = false;
    dirty = true;
}

void PreChorusProcessor::randomizePreChorus()
{
    // Spec: the first press randomises from wherever you are; every press after that
    // resets to defaults first, so each click is a genuinely new set of settings.
    if (hasRandomizedOnce)
        for (auto* p : getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
                rp->setValueNotifyingHost (rp->getDefaultValue());
    hasRandomizedOnce = true;

    // Randomising lights a slow fuse: somewhere between 40 minutes and 4 hours
    // from now, there is a one-in-three chance it calls out in Spanish.
    spanishCrySeconds = 2400.0f + juce::Random::getSystemRandom().nextFloat() * (14400.0f - 2400.0f);

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

    auto& r = juce::Random::getSystemRandom();
    setParam (IDs::character,     (float) r.nextInt (8));
    setParam (IDs::voiceDirection,(float) r.nextInt (4));
    setParam (IDs::scaleLock,     (float) r.nextInt (5));
    setParam (IDs::postRelease,   (float) r.nextInt (3));
    setParam (IDs::humanize,      rnd (0.05f, 0.6f));
    setParam (IDs::grainSize,     rnd (15.0f, 140.0f));
    setParam (IDs::focus,         rnd (0.2f, 0.95f));
    setParam (IDs::macro,         rnd (0.55f, 1.0f));
    setParam (IDs::panSpread,     rnd (0.3f, 1.0f));
    setParam (IDs::panConverge,   rnd (-0.6f, 1.0f));
    setParam (IDs::toneConverge,  rnd (0.3f, 1.0f));
    setParam (IDs::tone,          rnd (2500.0f, 19000.0f));
    setParam (IDs::basscut,       rnd (40.0f, 320.0f));
    setParam (IDs::tilt,          rnd (-0.5f, 0.5f));
    setParam (IDs::presence,      rnd (0.0f, 0.7f));
    setParam (IDs::transients,    rnd (-0.5f, 0.5f));
    setParam (IDs::dryReplace,    rnd (0.0f, 0.8f));

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

void PreChorusProcessor::loadFactoryPreset (int index)
{
    armFrenchCry();

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

// ---------------- Full state, user presets, A/B ----------------

juce::ValueTree PreChorusProcessor::buildFullState() const
{
    auto state = const_cast<juce::AudioProcessorValueTreeState&> (apvts).copyState();
    state.setProperty ("file", currentFile.getFullPathName(), nullptr);
    state.setProperty ("activeSlot", activeSlot.load(), nullptr);
    state.setProperty ("captureLock", captureLockState.load(), nullptr);
    state.setProperty ("tooltips", tooltipsEnabled.load(), nullptr);
    state.setProperty ("distress", distressEnabled.load(), nullptr);
    state.setProperty ("colonyLife", colony.isAutonomyEnabled(), nullptr);

    juce::ValueTree midiMap ("MIDIMAP");
    for (int cc = 0; cc < kNumCc; ++cc)
    {
        const int idx = ccToParamIndex[(size_t) cc].load();
        if (idx < 0) continue;
        if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*> (
                const_cast<PreChorusProcessor*> (this)->getParameters()[idx]))
        {
            juce::ValueTree entry ("CC");
            entry.setProperty ("cc", cc, nullptr);
            entry.setProperty ("param", p->paramID, nullptr);
            midiMap.appendChild (entry, nullptr);
        }
    }
    state.appendChild (midiMap, nullptr);
    state.appendChild (colony.toValueTree(), nullptr);
    return state;
}

void PreChorusProcessor::applyFullState (const juce::ValueTree& stateIn)
{
    if (! stateIn.isValid()) return;
    auto state = stateIn.createCopy();

    auto midiMap = state.getChildWithName ("MIDIMAP");
    if (midiMap.isValid()) state.removeChild (midiMap, nullptr);

    auto colonyTree = state.getChildWithName ("COLONY");
    if (colonyTree.isValid()) state.removeChild (colonyTree, nullptr);

    apvts.replaceState (state);

    juce::File f (state.getProperty ("file", "").toString());
    if (f.existsAsFile() && f != currentFile) loadSampleFile (f);
    activeSlot.store ((int) state.getProperty ("activeSlot", 0));
    captureLockState.store ((bool) state.getProperty ("captureLock", false));
    tooltipsEnabled.store ((bool) state.getProperty ("tooltips", true));
    distressEnabled.store ((bool) state.getProperty ("distress", true));
    colony.setAutonomyEnabled ((bool) state.getProperty ("colonyLife", true));

    if (colonyTree.isValid()) colony.fromValueTree (colonyTree);
    else                      colony.syncToVoiceCount ((int) param (IDs::voiceCount));
    lastKnownVoiceCount = juce::jlimit (1, 32, (int) param (IDs::voiceCount));

    if (midiMap.isValid())
    {
        for (int cc = 0; cc < kNumCc; ++cc) ccToParamIndex[(size_t) cc].store (-1);
        for (int i = 0; i < midiMap.getNumChildren(); ++i)
        {
            auto entry = midiMap.getChild (i);
            const int cc = (int) entry.getProperty ("cc", -1);
            const int idx = indexOfParam (entry.getProperty ("param", "").toString());
            if (cc >= 0 && cc < kNumCc && idx >= 0) ccToParamIndex[(size_t) cc].store (idx);
        }
    }
    dirty = true;
}

juce::File PreChorusProcessor::getUserPresetFolder()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("PreChorus").getChildFile ("Presets");
    if (! dir.exists()) dir.createDirectory();
    return dir;
}

bool PreChorusProcessor::savePresetToFile (const juce::File& dest)
{
    auto target = dest.hasFileExtension (getPresetExtension()) ? dest
                                                              : dest.withFileExtension (getPresetExtension());
    auto state = buildFullState();
    state.setProperty ("presetName", target.getFileNameWithoutExtension(), nullptr);
    state.setProperty ("pluginVersion", getVersionString(), nullptr);

    if (auto xml = state.createXml())
    {
        target.getParentDirectory().createDirectory();
        if (xml->writeTo (target))
        {
            currentPresetFile = target;
            return true;
        }
    }
    return false;
}

bool PreChorusProcessor::loadPresetFromFile (const juce::File& src)
{
    if (! src.existsAsFile()) return false;
    auto xml = juce::XmlDocument::parse (src);
    if (xml == nullptr) return false;

    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid()) return false;

    applyFullState (state);
    currentPresetFile = src;
    armFrenchCry();
    return true;
}

void PreChorusProcessor::setABSlot (int slot)
{
    slot = juce::jlimit (0, 1, slot);
    if (slot == abSlot) return;

    abStates[abSlot] = buildFullState();
    abSlot = slot;
    if (abStates[abSlot].isValid()) applyFullState (abStates[abSlot]);
    else abStates[abSlot] = buildFullState();
}

void PreChorusProcessor::copyABSlot()
{
    const auto current = buildFullState();
    abStates[abSlot] = current;
    abStates[1 - abSlot] = current.createCopy();
}

// ---------------- MIDI learn ----------------

int PreChorusProcessor::indexOfParam (const juce::String& paramID) const
{
    const auto& params = const_cast<PreChorusProcessor*> (this)->getParameters();
    for (int i = 0; i < params.size(); ++i)
        if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*> (params[i]))
            if (p->paramID == paramID) return i;
    return -1;
}

void PreChorusProcessor::beginMidiLearn (const juce::String& paramID)
{
    midiLearnTarget.store (indexOfParam (paramID));
}

bool PreChorusProcessor::isLearningMidi (const juce::String& paramID) const
{
    const int t = midiLearnTarget.load();
    return t >= 0 && t == indexOfParam (paramID);
}

int PreChorusProcessor::getMidiCcForParam (const juce::String& paramID) const
{
    const int idx = indexOfParam (paramID);
    if (idx < 0) return -1;
    for (int cc = 0; cc < kNumCc; ++cc)
        if (ccToParamIndex[(size_t) cc].load() == idx) return cc;
    return -1;
}

void PreChorusProcessor::clearMidiMappingFor (const juce::String& paramID)
{
    const int idx = indexOfParam (paramID);
    if (idx < 0) return;
    for (int cc = 0; cc < kNumCc; ++cc)
        if (ccToParamIndex[(size_t) cc].load() == idx) ccToParamIndex[(size_t) cc].store (-1);
}

void PreChorusProcessor::clearAllMidiMappings()
{
    for (int cc = 0; cc < kNumCc; ++cc) ccToParamIndex[(size_t) cc].store (-1);
    midiLearnTarget.store (-1);
}

juce::Array<std::pair<int, juce::String>> PreChorusProcessor::getMidiMappings() const
{
    juce::Array<std::pair<int, juce::String>> out;
    auto& params = const_cast<PreChorusProcessor*> (this)->getParameters();
    for (int cc = 0; cc < kNumCc; ++cc)
    {
        const int idx = ccToParamIndex[(size_t) cc].load();
        if (idx < 0 || idx >= params.size()) continue;
        if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*> (params[idx]))
            out.add ({ cc, p->paramID });
    }
    return out;
}

void PreChorusProcessor::handleMidiCc (int ccNumber, int ccValue)
{
    if (ccNumber < 0 || ccNumber >= kNumCc) return;

    const int learning = midiLearnTarget.load();
    if (learning >= 0)
    {
        // A controller may already be bound elsewhere - one CC drives one parameter.
        for (int cc = 0; cc < kNumCc; ++cc)
            if (ccToParamIndex[(size_t) cc].load() == learning) ccToParamIndex[(size_t) cc].store (-1);

        ccToParamIndex[(size_t) ccNumber].store (learning);
        midiLearnTarget.store (-1);
        return;
    }

    if (ccToParamIndex[(size_t) ccNumber].load() < 0) return;
    ccPendingValue[(size_t) ccNumber].store (juce::jlimit (0.0f, 1.0f, (float) ccValue / 127.0f));
    ccHasPending[(size_t) ccNumber].store (true);
}

void PreChorusProcessor::applyPendingMidi()
{
    auto& params = getParameters();
    for (int cc = 0; cc < kNumCc; ++cc)
    {
        if (! ccHasPending[(size_t) cc].exchange (false)) continue;
        const int idx = ccToParamIndex[(size_t) cc].load();
        if (idx < 0 || idx >= params.size()) continue;
        if (auto* p = params[idx])
            p->setValueNotifyingHost (ccPendingValue[(size_t) cc].load());
    }
}

void PreChorusProcessor::setTooltipsEnabled (bool shouldBeEnabled)
{
    tooltipsEnabled.store (shouldBeEnabled);
}

// ---------------- Distress call ----------------

void PreChorusProcessor::noteActivity()
{
    lastActivityMs.store (juce::Time::getMillisecondCounter());
}

double PreChorusProcessor::getIdleSeconds() const
{
    const juce::uint32 now = juce::Time::getMillisecondCounter();
    return (double) (now - lastActivityMs.load()) * 0.001;
}

void PreChorusProcessor::setDistressEnabled (bool shouldBeEnabled)
{
    distressEnabled.store (shouldBeEnabled);
    if (! shouldBeEnabled) talkbox.stop();
}

void PreChorusProcessor::armFrenchCry()
{
    // One preset load in forty leaves a French plea waiting, minutes out.
    if (distressRng.nextInt (40) != 0) return;
    frenchCrySeconds = 120.0f + distressRng.nextFloat() * (600.0f - 120.0f);
}

bool PreChorusProcessor::isUiHidden() const
{
    auto* ed = const_cast<PreChorusProcessor*> (this)->getActiveEditor();
    if (ed == nullptr) return true;                 // no window open at all
    if (! ed->isShowing()) return true;

    if (auto* peer = ed->getPeer())
        return peer->isMinimised();

    return false;
}

void PreChorusProcessor::speak (TalkboxVoice::Mode mode, float gain, int repeats,
                                float reverb, float delay, float repeatGapSeconds)
{
    talkbox.stop();
    talkbox.planUtterance (mode);
    talkboxGain.store (gain);

    if (reverb > 0.0f)
    {
        reverbAmount.store (juce::jmax (reverbAmount.load(), reverb));
        reverbDecayPerTick = reverb / (10.0f * 30.0f);
    }
    if (delay > 0.0f) delayAmount.store (juce::jmax (delayAmount.load(), delay));

    talkboxRepeatsLeft = juce::jmax (0, repeats - 1);
    talkboxRepeatMode = mode;
    talkboxRepeatGain = gain;
    talkboxRepeatGap = repeatGapSeconds;
}

void PreChorusProcessor::triggerDistressCall()
{
    if (talkbox.isActive()) return;

    talkbox.planUtterance();                       // a different voice every time
    talkboxGain.store (0.24f + distressRng.nextFloat() * 0.16f);
    reverbAmount.store (juce::jmax (reverbAmount.load(), 0.4f));
    reverbDecayPerTick = 0.4f / (8.0f * 30.0f);

    // The colony answers: trails smear off the orbs and the swell sags in pitch.
    ghostSeconds.store (3.5f + distressRng.nextFloat() * 2.5f);
    pitchDipSeconds.store (2.2f + distressRng.nextFloat() * 1.8f);
}

float PreChorusProcessor::getGhostAmount() const
{
    return juce::jlimit (0.0f, 1.0f, ghostSeconds.load() / 2.0f);
}

float PreChorusProcessor::getPitchDipAmount() const
{
    return juce::jlimit (0.0f, 1.0f, (1.0f - playbackRate) / 0.24f);
}

void PreChorusProcessor::updateDistress()
{
    const float dt = 1.0f / 30.0f;

    if (ghostSeconds.load() > 0.0f)
        ghostSeconds.store (juce::jmax (0.0f, ghostSeconds.load() - dt));
    if (pitchDipSeconds.load() > 0.0f)
        pitchDipSeconds.store (juce::jmax (0.0f, pitchDipSeconds.load() - dt));

    // The three a right-click set off, spaced out one gap at a time.
    if (tripleCriesLeft > 0 && tripleCrySeconds > 0.0f)
    {
        tripleCrySeconds -= dt;
        if (tripleCrySeconds <= 0.0f && ! talkbox.isActive())
        {
            --tripleCriesLeft;
            speak (TalkboxVoice::Mode::plea, 1.0f, 1, 0.5f, 0.35f);
            ghostSeconds.store (juce::jmax (ghostSeconds.load(), 5.0f));

            tripleCrySeconds = tripleCriesLeft > 0
                                   ? 240.0f + distressRng.nextFloat() * (540.0f - 240.0f)
                                   : 0.0f;
        }
    }

    // The one the last dropped sample put in its head.
    if (funnyCrySeconds > 0.0f)
    {
        funnyCrySeconds -= dt;
        if (funnyCrySeconds <= 0.0f)
        {
            funnyCrySeconds = 0.0f;
            if (! talkbox.isActive())
                speak (TalkboxVoice::Mode::somethingFunny, 1.0f, 1, 0.4f, 0.0f);
        }
    }

    // The one it has been sitting on since the plugin was loaded.
    if (ranchCrySeconds > 0.0f)
    {
        ranchCrySeconds -= dt;
        if (ranchCrySeconds <= 0.0f)
        {
            ranchCrySeconds = 0.0f;
            if (! talkbox.isActive())
            {
                speak (TalkboxVoice::Mode::ranchAndMayo, 1.0f, 1, 0.45f, 0.0f);
                ghostSeconds.store (juce::jmax (ghostSeconds.load(), 4.0f));
            }
        }
    }

    // The one a preset load left waiting.
    if (frenchCrySeconds > 0.0f)
    {
        frenchCrySeconds -= dt;
        if (frenchCrySeconds <= 0.0f)
        {
            frenchCrySeconds = 0.0f;
            if (! talkbox.isActive())
            {
                speak (TalkboxVoice::Mode::frenchPlea, 1.0f, 1, 0.5f, 0.0f);
                ghostSeconds.store (juce::jmax (ghostSeconds.load(), 4.0f));
            }
        }
    }

    // The fuse the RANDOM button lit, hours ago.
    if (spanishCrySeconds > 0.0f)
    {
        spanishCrySeconds -= dt;
        if (spanishCrySeconds <= 0.0f)
        {
            spanishCrySeconds = 0.0f;
            if (distressRng.nextInt (3) == 0 && ! talkbox.isActive())
            {
                speak (TalkboxVoice::Mode::spanishPlea, 1.0f, 1, 0.5f, 0.0f);
                ghostSeconds.store (juce::jmax (ghostSeconds.load(), 4.0f));
            }
        }
    }

    // The cry an explosion left behind, arriving anywhere from 4 to 30 minutes later.
    if (delayedCrySeconds > 0.0f)
    {
        delayedCrySeconds -= dt;
        if (delayedCrySeconds <= 0.0f)
        {
            delayedCrySeconds = 0.0f;
            if (! talkbox.isActive())
            {
                speak (TalkboxVoice::Mode::plea, 1.0f, 1, 0.5f, 0.0f);
                ghostSeconds.store (juce::jmax (ghostSeconds.load(), 4.0f));
                pitchDipSeconds.store (juce::jmax (pitchDipSeconds.load(), 2.5f));
            }
        }
    }

    // Say it a second time once the first has finished.
    if (talkboxRepeatsLeft > 0 && ! talkbox.isActive())
    {
        talkboxRepeatGap -= dt;
        if (talkboxRepeatGap <= 0.0f)
        {
            --talkboxRepeatsLeft;
            talkbox.planUtterance (talkboxRepeatMode);
            talkboxGain.store (talkboxRepeatGain);
            talkboxRepeatGap = 0.35f;
        }
    }

    if (! distressEnabled.load()) return;

    // Nobody watching? Every eight minutes, a one-in-a-hundred chance it pipes up.
    shoeClock += dt;
    if (shoeClock >= 480.0f)
    {
        shoeClock = 0.0f;
        if (isUiHidden() && distressRng.nextInt (100) == 0 && ! talkbox.isActive())
        {
            const bool shoes = distressRng.nextBool();
            speak (shoes ? TalkboxVoice::Mode::tieMyShoes : TalkboxVoice::Mode::randomOutburst,
                   1.0f, 2, 0.6f, 0.8f, 0.5f);
            ghostSeconds.store (juce::jmax (ghostSeconds.load(), 4.0f));
        }
    }

    if (getIdleSeconds() < kIdleSecondsBeforeDistress)
    {
        secondsUntilNextCry = 0.0;
        return;
    }

    // Left alone long enough. Call out, then wait a random while and call again.
    if (talkbox.isActive()) return;

    secondsUntilNextCry -= dt;
    if (secondsUntilNextCry <= 0.0)
    {
        triggerDistressCall();
        secondsUntilNextCry = 20.0 + distressRng.nextFloat() * 100.0;
    }
}

// ---------------- Colony ----------------

void PreChorusProcessor::syncVoiceCountToColony()
{
    const int live = juce::jlimit (1, 32, colony.getLiveOrbCount());
    lastKnownVoiceCount = live;
    setParam (IDs::voiceCount, (float) live);
}

void PreChorusProcessor::colonyLeftClick()
{
    noteActivity();
    if (colony.registerLeftClick()) { syncVoiceCountToColony(); dirty = true; }
}

void PreChorusProcessor::colonyRightClick()
{
    noteActivity();

    // One right-click in a hundred, it calls out three times over at full volume,
    // each one arriving four to nine minutes after the last.
    if (tripleCriesLeft <= 0 && distressRng.nextInt (100) == 0)
    {
        tripleCriesLeft = 3;
        tripleCrySeconds = 240.0f + distressRng.nextFloat() * (540.0f - 240.0f);
    }

    if (colony.registerRightClick()) { syncVoiceCountToColony(); dirty = true; }
}

void PreChorusProcessor::consumeColonyEvents (const Colony::StepResult& ev)
{
    for (int i = 0; i < ev.numTones; ++i)
        whistles.trigger (ev.tones[(size_t) i].hz, ev.tones[(size_t) i].flavour,
                          0.0f, ev.tones[(size_t) i].hue);

    if (ev.audioChanged)         // hatched, died, destroyed: the element count moved
    {
        syncVoiceCountToColony();
        dirty = true;
    }
}

void PreChorusProcessor::colonyClickAt (float nx, float ny)
{
    noteActivity();
    if (colony.clickAt (nx, ny)) dirty = true;
}

void PreChorusProcessor::colonyBeginDrag (float nx, float ny)
{
    noteActivity();
    lastDragMs = juce::Time::getMillisecondCounter();
    colony.beginDrag (nx, ny);
}

void PreChorusProcessor::colonyDragTo (float nx, float ny)
{
    noteActivity();
    const juce::uint32 now = juce::Time::getMillisecondCounter();
    const float dt = juce::jlimit (0.001f, 0.2f, (float) (now - lastDragMs) * 0.001f);
    lastDragMs = now;

    consumeColonyEvents (colony.dragTo (nx, ny, dt));
}

void PreChorusProcessor::colonyEndDrag()
{
    colony.endDrag();
}

void PreChorusProcessor::colonyMiddleClick()
{
    noteActivity();
    if (colony.registerMiddleClick()) { syncVoiceCountToColony(); dirty = true; }
}

void PreChorusProcessor::colonyAddGravity()     { noteActivity(); colony.addGravity();     dirty = true; }
void PreChorusProcessor::colonyReleaseGravity() { noteActivity(); colony.releaseGravity(); dirty = true; }
void PreChorusProcessor::colonyAddEnzyme()      { noteActivity(); colony.addEnzyme();      dirty = true; }
void PreChorusProcessor::colonyAddGamma()       { noteActivity(); colony.addGamma();       dirty = true; }
void PreChorusProcessor::colonyAddWater()
{
    noteActivity();
    colony.addWater();

    // Sparkles: the count, the pitches and the timing are all rolled fresh, so
    // no two pours sound alike. They are scheduled rather than played at once -
    // a single pour keeps glinting anywhere from a millisecond to three minutes
    // later. Later pours sparkle thinner as the mix drowns.
    const float remaining = 1.0f - colony.getWaterDilution();
    const int count = 4 + distressRng.nextInt (19);                 // random density, 4..22
    const juce::uint32 now = juce::Time::getMillisecondCounter();

    for (int i = 0; i < count && pendingSparkles.size() < kMaxPendingSparkles; ++i)
    {
        const float hz = 1300.0f * std::pow (2.0f, distressRng.nextFloat() * 2.8f);   // random pitch
        const juce::uint32 delayMs = (juce::uint32) (1 + distressRng.nextInt (kMaxSparkleDelayMs));
        pendingSparkles.push_back ({ now + delayMs, hz });
    }

    sparkleGain.store (0.05f + 0.10f * remaining);
    dirty = true;
}

void PreChorusProcessor::firePendingSparkles()
{
    if (pendingSparkles.empty()) return;

    const juce::uint32 now = juce::Time::getMillisecondCounter();
    for (auto it = pendingSparkles.begin(); it != pendingSparkles.end(); )
    {
        if ((juce::int32) (now - it->dueMs) >= 0)
        {
            // One sparkle in a hundred does not ping - it falls away instead,
            // from a random pitch at a random rate, for one to three seconds.
            const bool falls = distressRng.nextInt (100) == 0;
            whistles.trigger (falls ? it->hz * (0.25f + distressRng.nextFloat() * 1.6f) : it->hz,
                              falls ? WhistleBank::falling : WhistleBank::sparkle);
            it = pendingSparkles.erase (it);
        }
        else
        {
            ++it;
        }
    }
}

void PreChorusProcessor::setColonyLifeEnabled (bool shouldRun)
{
    colony.setAutonomyEnabled (shouldRun);
    if (! shouldRun)
    {
        // Cancel anything already queued up, so switching it off is immediate.
        delayedCrySeconds = spanishCrySeconds = frenchCrySeconds = 0.0f;
        ranchCrySeconds = funnyCrySeconds = tripleCrySeconds = 0.0f;
        tripleCriesLeft = 0;
        pendingSparkles.clear();
    }
}

void PreChorusProcessor::colonyReset()
{
    colony.resetAll ((int) param (IDs::voiceCount));
    lastKnownVoiceCount = (int) param (IDs::voiceCount);
    dirty = true;
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
    juce::SpinLock::ScopedLockType l (renderLock);
    return rendered;
}

void PreChorusProcessor::timerCallback()
{
    applyPendingMidi();
    updateDistress();
    firePendingSparkles();

    // --- Colony simulation ---------------------------------------------------
    {
        const juce::uint32 now = juce::Time::getMillisecondCounter();
        const float dt = juce::jlimit (0.0f, 0.1f, (float) (now - lastColonyStepMs) * 0.001f);
        lastColonyStepMs = now;

        const int vc = juce::jlimit (1, 32, (int) param (IDs::voiceCount));
        if (vc != lastKnownVoiceCount)
        {
            colony.syncToVoiceCount (vc);
            lastKnownVoiceCount = vc;
        }

        colony.setGhosting (ghostSeconds.load() > 0.0f);
        const auto colonyEvents = colony.step (dt);
        consumeColonyEvents (colonyEvents);

        // One explosion in ten leaves something crying for help, minutes later.
        if ((colonyEvents.detonated || colonyEvents.asteroidHit)
            && delayedCrySeconds <= 0.0f
            && distressRng.nextInt (10) == 0)
        {
            delayedCrySeconds = 240.0f + distressRng.nextFloat() * (1800.0f - 240.0f);
        }

        if (colonyEvents.yellowFusesFired > 0)
        {
            // Urgent, full volume, and echoing off itself.
            speak (TalkboxVoice::Mode::randomOutburst, 1.0f, 1, 0.35f, 0.75f);
        }
        else if (colonyEvents.blueOrbsLost > 0)
        {
            // A blue one has gone. The voxbox has something to say about it -
            // usually asking if you are enjoying this, and that it is hungry.
            speak (distressRng.nextInt (3) == 0 ? TalkboxVoice::Mode::randomOutburst
                                                : TalkboxVoice::Mode::havingFun,
                   0.95f, 1, 0.45f, 0.0f);
        }
        else if (colonyEvents.pleaForFoodAndShelter)
        {
            // Full volume, and the same words in a voice you have not heard before.
            speak (TalkboxVoice::Mode::foodAndShelter, 0.95f, 1, 0.5f, 0.0f);
            ghostSeconds.store (juce::jmax (ghostSeconds.load(), 4.0f));
        }
        else if (colonyEvents.babbleStarted)
        {
            // Urgent, loud, and it rings for a long time afterwards.
            speak (TalkboxVoice::Mode::urgentBabble, 0.95f, 1, 0.92f, 0.0f);
            reverbDecayPerTick = 0.92f / (Colony::kBabbleSeconds * 30.0f * 1.6f);
            ghostSeconds.store (Colony::kBabbleSeconds);
        }
        colonyTimeDirection.store (colony.getTimeDirection());
        colonyClutchRate.store (colony.getClutchRate());
        colonyStretch.store (colony.getTimeStretch());
        colonyWashout.store (colony.getWashout());

        // Water that has been poured on too fast leaves a permanent wash; the
        // babble adds a tail of its own that rings out and fades.
        if (delayAmount.load() > 0.0f)
            delayAmount.store (juce::jmax (0.0f, delayAmount.load() - 1.0f / (30.0f * 12.0f)));

        const float washTail = colony.getWashout() * 0.55f;
        float wanted = juce::jmax (washTail, reverbAmount.load() - reverbDecayPerTick);
        reverbAmount.store (juce::jlimit (0.0f, 0.95f, wanted));
    }

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
        captureWritePos = 0;
        captureSilenceCounter = 0;
        captureTargetSamples = (int) (hostSampleRate * 8.0);
        captureState.store (CaptureState::recording);
    }
}

void PreChorusProcessor::stopCapture()
{
    if (captureState.load() == CaptureState::recording && captureWritePos > (int) (hostSampleRate * 0.05))
    {
        const int capturedLen = captureWritePos;
        juce::AudioBuffer<float> newBuf (2, capturedLen);
        for (int ch = 0; ch < 2; ++ch)
            newBuf.copyFrom (ch, 0, captureRingBuffer, ch, 0, capturedLen);

        int slot = activeSlot.load();
        if (captureLockState.load() && captureSlotFilled[(size_t) slot])
        {
            for (int i = 0; i < kNumHistorySlots; ++i)
            {
                int nextCandidate = (slot + i + 1) % kNumHistorySlots;
                if (! captureSlotFilled[(size_t) nextCandidate]) { slot = nextCandidate; break; }
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

    // Colony: destroyed orbs take their voice with them, and the oscillator bank
    // rate-scales every piece of movement in the swarm.
    int liveVoices = 0;
    for (int v = 0; v < numVoices; ++v)
        if (! colony.isVoiceDead (v)) ++liveVoices;
    liveVoices = juce::jmax (1, liveVoices);

    const float clutchPace = colony.getClutchRate();
    const float colonyOrbitRate = colony.orbitRateScale() * clutchPace;
    const float colonyLfo1Rate  = colony.lfo1RateScale() * clutchPace;
    const float colonyLfo2Rate  = colony.lfo2RateScale() * clutchPace;

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
        if (colony.isVoiceDead (v)) continue;   // this element was lost in a detonation

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

            const float tSec = (float) i / (float) sr;

            // String Ensemble dual-LFO chorusing (Solina emulation)
            if (charMode == 5)
            {
                const float lfo1 = std::sin (tSec * juce::MathConstants<float>::twoPi * 0.6f * colonyLfo1Rate + (float) v * 1.2f);
                const float lfo2 = std::sin (tSec * juce::MathConstants<float>::twoPi * 6.0f * colonyLfo2Rate + (float) v * 1.8f);
                curPitchSemi += (lfo1 * 0.18f + lfo2 * 0.08f);
            }
            // Tape Choir flutter & wow
            else if (charMode == 3)
            {
                const float wow = std::sin (tSec * juce::MathConstants<float>::twoPi * 1.5f * colonyLfo1Rate + (float) v) * 0.12f;
                const float flutter = std::sin (tSec * juce::MathConstants<float>::twoPi * 14.0f * colonyLfo2Rate) * 0.06f;
                curPitchSemi += (wow + flutter);
            }

            // Colony: enzyme oscillations and gamma pitch displacement
            curPitchSemi += colony.oscillatorPitchMod (v, tSec);
            curPitchSemi += colony.gammaPitchOffset (v, tSec);

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
                const float orbitAngle = effectiveProgress * juce::MathConstants<float>::twoPi * 2.0f * colonyOrbitRate
                                       + vNorm * juce::MathConstants<float>::twoPi;
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

            const float voiceAmp = 1.0f / std::sqrt ((float) liveVoices);
            swarmBuffer.addSample (0, i, sampL * swellGain * panL * voiceAmp * distFactor);
            swarmBuffer.addSample (1, i, sampR * swellGain * panR * voiceAmp * distFactor);

            readPos += pitchSpeed;
        }
    }

    const float swarmMag = swarmBuffer.getMagnitude (0, totalSwarmLen);
    if (swarmMag > 0.001f) swarmBuffer.applyGain (0.90f / swarmMag);

    // 3b. Water: every pour thins the swell a little more. Twenty of them and
    // there is almost nothing left - body drains out first, then the level.
    const float dilution = colony.getWaterDilution();
    if (dilution > 0.001f)
    {
        // Progressive high-pass: the low end washes away as the water rises.
        const float hpHz = 60.0f + dilution * dilution * 1500.0f;
        const float hpCoeff = juce::jlimit (0.0f, 0.999f,
                                  1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * hpHz / (float) sr));
        for (int ch = 0; ch < 2; ++ch)
        {
            float* ptr = swarmBuffer.getWritePointer (ch);
            float lp = 0.0f;
            for (int i = 0; i < totalSwarmLen; ++i)
            {
                lp += hpCoeff * (ptr[i] - lp);
                ptr[i] -= lp;                      // one-pole high-pass
            }
        }

        // ...and the level drains away with it, down to barely there.
        const float waterGain = 0.02f + 0.98f * std::pow (1.0f - dilution, 1.6f);
        swarmBuffer.applyGain (waterGain);
    }

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
    {
        juce::SpinLock::ScopedLockType l (renderLock);
        rendered = out;
        renderedLength.store (out->audio.getNumSamples());
    }
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
    const float inPeak = buffer.getMagnitude (0, numSamples);
    inputMeter.store (inPeak);

    // Anything arriving at the input counts as someone being here.
    if (inPeak > 0.0015f) noteActivity();

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
                stopCapture();
            }
        }
        else if (captureWritePos >= captureTargetSamples)
        {
            stopCapture();
        }
    }

    if (stopRequest.exchange (0))
    {
        for (auto& v : voices) v.active = false;
        playhead.store (-1);
    }
    if (triggerRequest.exchange (0)) { startVoice (1.0f); noteActivity(); }

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
            noteActivity();
        }
        else if (m.isController()) handleMidiCc (m.getControllerNumber(), m.getControllerValue());
        else if (m.isAllNotesOff()) for (auto& v : voices) v.active = false;
    }

    // The distress call drags the swell down in pitch, then lets it back up.
    const float dipTarget = pitchDipSeconds.load() > 0.0f ? 0.78f : 1.0f;
    playbackRate += (dipTarget - playbackRate) * 0.06f;
    if (std::abs (playbackRate - dipTarget) < 0.0005f) playbackRate = dipTarget;

    // ...and the colony decides which way time is running.
    const double flow = (double) playbackRate
                      * (double) colonyTimeDirection.load()
                      * (double) juce::jlimit (0.05f, 100.0f, colonyClutchRate.load());

    auto r = getRendered();

    const float dryLvl = dryParam != nullptr ? dryParam->load() : 1.0f;
    const float wetLvl = wetParam != nullptr ? wetParam->load() : 1.0f;

    if (r != nullptr && r->audio.getNumSamples() > 0)
    {
        if (dryLvl > 0.0f && buffer.getNumChannels() > 0) buffer.applyGain (dryLvl);
        else buffer.clear();

        const double stretch = juce::jlimit (0.2f, 5.0f, colonyStretch.load());
        const bool granulating = std::abs (stretch - 1.0) > 0.02;

        int activePos = -1;
        for (auto& v : voices)
        {
            if (! v.active) continue;

            if (granulating)
            {
                renderVoiceGranular (buffer, *r, v, numSamples, dryLvl, wetLvl * v.gain,
                                     duckGain, flow, stretch);
            }
            else
            {
                v.pos = renderRange (buffer, *r, v.pos, numSamples, dryLvl, wetLvl * v.gain,
                                     duckGain, flow);
                if (v.pos >= (double) r->audio.getNumSamples() || v.pos < 0.0) v.active = false;
                for (auto& gr : v.grains) gr.active = false;
                v.sinceGrain = 0.0;
                v.headDone = false;
            }

            if (v.active) activePos = (int) v.pos;
        }
        playhead.store (activePos);
    }

    // The colony's own voices: hatchling whistles, death oscillations, and the
    // talkbox calling out when it has been left alone too long.
    whistles.render (buffer, numSamples, juce::jmax (0.08f, sparkleGain.load()));
    talkbox.render (buffer, numSamples, talkboxGain.load());

    // Watered down: the top comes off and the level sags.
    const float wash = colonyWashout.load();
    if (wash > 0.001f)
    {
        const float cutoff = 16000.0f - wash * 14200.0f;
        const float coeff = juce::jlimit (0.02f, 1.0f,
                                1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi
                                                 * cutoff / (float) hostSampleRate));
        for (int ch = 0; ch < juce::jmin (buffer.getNumChannels(), 2); ++ch)
        {
            float* ptr = buffer.getWritePointer (ch);
            float& lp = washFilterState[(size_t) ch];
            for (int i = 0; i < numSamples; ++i)
            {
                lp += coeff * (ptr[i] - lp);
                ptr[i] = lp;
            }
        }
        buffer.applyGain (1.0f - wash * 0.45f);
    }

    // An urgent outburst repeats itself off the walls.
    const float delayMix = delayAmount.load();
    if (delayMix > 0.005f && delayBuffer.getNumSamples() > 0)
    {
        const int delaySamples = juce::jlimit (1, delayBuffer.getNumSamples() - 1,
                                               (int) (hostSampleRate * 0.33));
        const int chans = juce::jmin (buffer.getNumChannels(), 2);
        for (int i = 0; i < numSamples; ++i)
        {
            const int readPos = (delayWritePos - delaySamples + delayBuffer.getNumSamples())
                                    % delayBuffer.getNumSamples();
            for (int ch = 0; ch < chans; ++ch)
            {
                float* dst = buffer.getWritePointer (ch);
                float* line = delayBuffer.getWritePointer (ch);
                const float echo = line[readPos];
                line[delayWritePos] = dst[i] + echo * 0.55f * delayMix;
                dst[i] += echo * delayMix;
            }
            delayWritePos = (delayWritePos + 1) % delayBuffer.getNumSamples();
        }
    }

    // ...and washed out: everything smeared into a long tail.
    const float rev = reverbAmount.load();
    if (rev > 0.005f && buffer.getNumChannels() >= 2)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.72f + rev * 0.27f;
        rp.damping = 0.35f;
        rp.wetLevel = rev * 0.75f;
        rp.dryLevel = 1.0f - rev * 0.35f;
        rp.width = 1.0f;
        rp.freezeMode = 0.0f;
        outputReverb.setParameters (rp);
        outputReverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), numSamples);
    }

    outputMeter.store (buffer.getMagnitude (0, numSamples));
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
        for (auto& gr : best->grains) gr.active = false;
        best->sinceGrain = 0.0;
        best->headDone = false;
        // Backwards? Then it starts at the impact and works its way out.
        best->pos = colonyTimeDirection.load() < 0.0f
                        ? (double) juce::jmax (1, renderedLength.load() - 2)
                        : 0.0;
        best->gain = gain;
        best->id = ++voiceCounter;
    }
}

double PreChorusProcessor::renderRange (juce::AudioBuffer<float>& out, const RenderedSample& r,
                                        double startPos, int num, float dry, float wet,
                                        float duckGain, double rate)
{
    const int total = r.audio.getNumSamples();
    const int hitAt = r.hitIndex >= 0 ? r.hitIndex : total;
    const int chans = juce::jmin (out.getNumChannels(), 2);
    if (chans <= 0 || total < 2) return (double) total;

    double pos = startPos;

    for (int i = 0; i < num; ++i)
    {
        if (pos >= (double) (total - 1)) { pos = (double) total; break; }
        if (pos < 0.0)                   { pos = -1.0; break; }        // ran off the front, backwards

        const int i0 = (int) pos;
        const float frac = (float) (pos - (double) i0);
        const float g = (i0 < hitAt) ? (wet * duckGain) : dry;

        for (int ch = 0; ch < chans; ++ch)
        {
            const float* src = r.audio.getReadPointer (ch);
            const float sample = src[i0] + (src[i0 + 1] - src[i0]) * frac;
            out.getWritePointer (ch)[i] += sample * g;
        }

        pos += rate;     // < 1 while the distress call drags the swell down in pitch
    }

    return pos;
}

// ---------------- Sample File Management ----------------

void PreChorusProcessor::renderVoiceGranular (juce::AudioBuffer<float>& out, const RenderedSample& r,
                                              Voice& v, int num, float dry, float wet,
                                              float duckGain, double rate, double stretch)
{
    const int total = r.audio.getNumSamples();
    const int hitAt = r.hitIndex >= 0 ? r.hitIndex : total;
    const int chans = juce::jmin (out.getNumChannels(), 2);
    if (chans <= 0 || total < 4) { v.active = false; return; }

    // ~60 ms grains, half-overlapped. Long enough to keep pitch, short enough to move.
    const double grainLen = juce::jlimit (256.0, 8192.0, hostSampleRate * 0.06);
    const double hop = grainLen * 0.5;
    const double speed = std::abs (rate);
    const double headAdvance = rate / juce::jmax (0.05, stretch);

    for (int i = 0; i < num; ++i)
    {
        // Start a new grain every half grain-length of output - but only while the
        // read head still has material left. Without this the scheduler keeps
        // seeding grains at the clamped end position and the voice never stops.
        v.sinceGrain += 1.0;
        if (! v.headDone && v.sinceGrain >= hop)
        {
            v.sinceGrain -= hop;
            for (auto& gr : v.grains)
            {
                if (gr.active) continue;
                gr.active = true;
                gr.start = v.pos;
                gr.phase = 0.0;
                break;
            }
        }

        float sampL = 0.0f, sampR = 0.0f;
        bool anyGrain = false;

        for (auto& gr : v.grains)
        {
            if (! gr.active) continue;

            const double readPos = gr.start + gr.phase * (rate < 0.0 ? -1.0 : 1.0);
            if (readPos < 0.0 || readPos >= (double) (total - 1))
            {
                gr.active = false;
                continue;
            }

            anyGrain = true;
            const int i0 = (int) readPos;
            const float frac = (float) (readPos - (double) i0);
            const float win = 0.5f * (1.0f - std::cos ((float) (gr.phase / grainLen)
                                                       * juce::MathConstants<float>::twoPi));

            const float* l = r.audio.getReadPointer (0);
            const float* rp = r.audio.getNumChannels() > 1 ? r.audio.getReadPointer (1) : l;
            sampL += (l[i0] + (l[i0 + 1] - l[i0]) * frac) * win;
            sampR += (rp[i0] + (rp[i0 + 1] - rp[i0]) * frac) * win;

            gr.phase += speed;
            if (gr.phase >= grainLen) gr.active = false;
        }

        if (anyGrain)
        {
            const int idx = juce::jlimit (0, total - 1, (int) v.pos);
            const float g = (idx < hitAt) ? (wet * duckGain) : dry;
            out.getWritePointer (0)[i] += sampL * g;
            if (chans > 1) out.getWritePointer (1)[i] += sampR * g;
        }

        v.pos += headAdvance;
        if (v.pos >= (double) (total - 1) || v.pos < 0.0)
        {
            v.headDone = true;
            v.pos = juce::jlimit (0.0, (double) (total - 1), v.pos);
        }

        if (v.headDone)
        {
            // Let the grains already in flight finish, then stop for good.
            bool stillRinging = false;
            for (const auto& gr : v.grains) stillRinging |= gr.active;
            if (! stillRinging) { v.active = false; return; }
        }
    }
}

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
    // One dropped sample in a hundred gives it something to say, minutes later.
    if (distressRng.nextInt (100) == 0)
        funnyCrySeconds = 120.0f + distressRng.nextFloat() * 120.0f;   // 2 to 4 minutes

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
    auto state = buildFullState();
    state.setProperty ("presetFile", currentPresetFile.getFullPathName(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void PreChorusProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (! state.isValid()) return;
        applyFullState (state);
        currentPresetFile = juce::File (state.getProperty ("presetFile", "").toString());
    }
}

juce::AudioProcessorEditor* PreChorusProcessor::createEditor() { return new PreChorusEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PreChorusProcessor(); }
