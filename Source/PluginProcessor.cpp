#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const float kPitchOct[] = { 1.0f, 2.0f, 4.0f };
    const int kSyncBars[]   = { 2, 4, 8, 16 }; // beats: 1/2 bar(2), 1 bar(4), 2 bars(8), 4 bars(16)

    // Musical Scale Quantizer
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
    for (auto* p : apvts.getParameterTree().getParameters())
        apvts.addParameterListener (p->getParameterID(), this);

    dryParam = apvts.getRawParameterValue (IDs::dry);
    wetParam = apvts.getRawParameterValue (IDs::wet);

    // Initial default vocal harmonic chord in slot 0 & loadedBuffer
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

    startTimerHz (30);
    dirty = true;
}

PreChorusProcessor::~PreChorusProcessor()
{
    stopTimer();
    for (auto* p : apvts.getParameterTree().getParameters())
        apvts.removeParameterListener (p->getParameterID(), this);
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

    // Convergence Engine
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::timeSpread, "Time Spread", juce::NormalisableRange<float> (0.1f, 4.0f, 0.01f, 0.6f), 1.2f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::timeConverge, "Time Converge", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.85f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::pitchSpread, "Pitch Spread", juce::NormalisableRange<float> (0.0f, 24.0f, 0.5f), 7.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::detune, "Detune", juce::NormalisableRange<float> (0.0f, 50.0f, 0.5f), 18.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::pitchConverge, "Pitch Converge", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.90f));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::scaleLock, "Scale Lock", juce::StringArray { "Chromatic", "Major", "Minor", "Pentatonic", "Oct / 5ths" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::panSpread, "Pan Spread", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.90f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::panConverge, "Pan Converge", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.70f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::toneConverge, "Tone Converge", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.75f));

    // Physics & Modulation
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::attraction, "Attraction", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.60f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::turbulence, "Turbulence", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.15f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::overshoot, "Overshoot", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.20f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::orbit, "Orbit", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.25f));
    p.push_back (std::make_unique<juce::AudioParameterInt> (IDs::seed, "Random Seed", 1, 9999, 4242));

    // Swell & Tone
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::tail, "Length", juce::NormalisableRange<float> (0.1f, 8.0f, 0.01f, 0.5f), 2.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::shape, "Shape", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.40f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::tone, "Tone", juce::NormalisableRange<float> (200.0f, 20000.0f, 1.0f, 0.25f), 14000.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::basscut, "Bass Cut", juce::NormalisableRange<float> (20.0f, 1000.0f, 1.0f, 0.35f), 80.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::space, "Space", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.35f));

    // Mix & PDC
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::dry, "Dry", juce::NormalisableRange<float> (0.0f, 1.5f, 0.01f), 1.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::wet, "Wet", juce::NormalisableRange<float> (0.0f, 1.5f, 0.01f), 1.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::dryReplace, "Dry Replace", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterBool> (IDs::align, "PDC Align", true));
    p.push_back (std::make_unique<juce::AudioParameterBool> (IDs::sync, "Sync", true));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::syncLen, "Sync Length", juce::StringArray { "1/2 Bar", "1 Bar", "2 Bars", "4 Bars" }, 1));

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
    setParam (IDs::space, rnd (0.2f, 0.6f));
    setParam (IDs::shape, rnd (-0.2f, 0.7f));
    regenerateSeed();
}

void PreChorusProcessor::regenerateSeed()
{
    setParam (IDs::seed, (float) juce::Random::getSystemRandom().nextInt (juce::Range<int> (1, 9999)));
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
    if (param (IDs::sync) > 0.5f)
    {
        const double bpm = hostBpm.load();
        if (std::abs (bpm - lastRenderBpm) > 0.05) { lastRenderBpm = bpm; dirty = true; }
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
        // If locked and filled, find next unlocked slot
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

// ---------------- Swarm & Convergence Render Engine ----------------

void PreChorusProcessor::render()
{
    auto out = std::make_shared<RenderedSample>();
    const double sr = hostSampleRate > 1000.0 ? hostSampleRate : 44100.0;
    out->sampleRate = sr;

    // 1. Determine Source Audio based on Source Mode
    const int sMode = (int) param (IDs::sourceMode);
    juce::AudioBuffer<float> source;
    double rawSR = 44100.0;

    {
        const juce::ScopedLock sl (sourceLock);
        int slot = activeSlot.load();
        if (sMode == 0 /* Live Capture */)
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
        else if (sMode == 1 /* Loaded Sample */)
        {
            source.makeCopyOf (loadedBuffer);
            rawSR = loadedSR;
        }
        else if (sMode == 2 /* Hybrid */)
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
        else /* Sliced */
        {
            source.makeCopyOf (loadedBuffer);
            rawSR = loadedSR;
        }
    }

    if (source.getNumSamples() == 0) return;

    // Resample source to host SR if different
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

    // 2. Swell Length calculation
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

    // 3. Swarm Parameters
    const int numVoices = juce::jlimit (1, 32, (int) param (IDs::voiceCount));
    const float timeSpreadSec = param (IDs::timeSpread);
    const float timeConvergeAmt = param (IDs::timeConverge);
    const float densityCurveParam = param (IDs::voiceDensity);
    const float pitchSpreadSemi = param (IDs::pitchSpread);
    const float detuneCents = param (IDs::detune);
    const float pitchConvergeAmt = param (IDs::pitchConverge);
    const int scaleMode = (int) param (IDs::scaleLock);
    const float panSpreadAmt = param (IDs::panSpread);
    const float panConvergeAmt = param (IDs::panConverge);
    const float toneConvergeAmt = param (IDs::toneConverge);
    const float attractionAmt = param (IDs::attraction);
    const float turbulenceAmt = param (IDs::turbulence);
    const float overshootAmt = param (IDs::overshoot);
    const float orbitAmt = param (IDs::orbit);
    const float progRevealAmt = param (IDs::progReveal);
    const float voiceAgeAmt = param (IDs::voiceAge);
    const int voiceDir = (int) param (IDs::voiceDirection);
    const float shapeVal = param (IDs::shape);

    // Deterministic PRNG
    juce::Random rnd ((juce::int64) param (IDs::seed));

    juce::AudioBuffer<float> swarmBuffer (2, swellLen);
    swarmBuffer.clear();

    // Slicing pre-division for slice scatter mode
    const int numSlices = 8;
    const int sliceLen = srcLen / numSlices;

    // Render each voice in the swarm
    for (int v = 0; v < numVoices; ++v)
    {
        const float vNorm = (numVoices > 1) ? (float) v / (float) (numVoices - 1) : 0.5f;

        // Entry Timing: early voices start further back, density curve controls arrival grouping
        const float densityNorm = tensionCurve (vNorm, densityCurveParam);
        const float rawOffsetSec = (1.0f - densityNorm) * timeSpreadSec;
        const float offsetSec = rawOffsetSec * (1.0f - timeConvergeAmt * 0.7f);
        const int voiceStartSample = juce::jlimit (0, swellLen - 1, (int) ((swellSeconds - offsetSec) * sr));

        // Pitch generation & scale lock
        const float randPitch = (rnd.nextFloat() * 2.0f - 1.0f) * pitchSpreadSemi;
        const float microDetune = (vNorm - 0.5f) * 2.0f * (detuneCents / 100.0f);
        const float initPitchSemi = quantizeToScale (randPitch, scaleMode) + microDetune;

        // Direction: Forward, Reverse, Alternating, or Random
        bool isRev = false;
        if (voiceDir == 1) isRev = true;
        else if (voiceDir == 2) isRev = (v % 2 == 1);
        else if (voiceDir == 3) isRev = (rnd.nextFloat() > 0.5f);

        // Progressive Reveal: early voices use shorter sliced windows
        const float revealFrac = juce::jlimit (0.08f, 1.0f, vNorm + (1.0f - progRevealAmt) * 0.92f);
        const int playableSrcLen = juce::jmax (64, (int) (srcLen * revealFrac));

        // Sliced mode offset
        int srcOffset = 0;
        if (sMode == 3 /* Sliced */ && sliceLen > 64)
        {
            srcOffset = (v % numSlices) * sliceLen;
        }

        // Voice panning start
        const float initPan = (vNorm * 2.0f - 1.0f) * panSpreadAmt;

        // Voice Age: earlier voices have low-pass darkening filter state
        float ageFilterL = 0.0f, ageFilterR = 0.0f;
        const float ageCoeff = juce::jlimit (0.0f, 0.85f, (1.0f - vNorm) * voiceAgeAmt);

        // Synthesis loop for voice 'v'
        double readPos = 0.0;
        const int voiceActiveSamples = swellLen - voiceStartSample;
        if (voiceActiveSamples <= 0) continue;

        for (int i = voiceStartSample; i < swellLen; ++i)
        {
            const float progress = (float) (i - voiceStartSample) / (float) juce::jmax (1, voiceActiveSamples - 1);
            const float globalSwellNorm = (float) i / (float) juce::jmax (1, swellLen - 1);

            // Convergence Trajectories:
            // 1. Pitch Convergence with attraction, overshoot & turbulence
            const float convergeCurve = tensionCurve (progress, attractionAmt);
            float curPitchSemi = initPitchSemi * (1.0f - pitchConvergeAmt * convergeCurve);

            if (overshootAmt > 0.01f && progress > 0.6f)
            {
                // Damped spring oscillation past unison
                const float spring = std::sin ((progress - 0.6f) * 16.0f) * std::exp (-(progress - 0.6f) * 6.0f);
                curPitchSemi += initPitchSemi * overshootAmt * spring;
            }

            if (turbulenceAmt > 0.01f)
            {
                const float turb = std::sin (progress * 42.0f + (float) v * 3.7f) * turbulenceAmt * 0.8f;
                curPitchSemi += turb;
            }

            const double pitchSpeed = std::pow (2.0, curPitchSemi / 12.0);

            // 2. Pan & Width Convergence + Orbit
            float curPan = initPan;
            if (panConvergeAmt > 0.0f)
            {
                // Collapse to center
                curPan = initPan * (1.0f - panConvergeAmt * progress);
            }
            else if (panConvergeAmt < 0.0f)
            {
                // Bloom outward
                curPan = initPan * (1.0f + (-panConvergeAmt) * progress * 1.5f);
            }

            if (orbitAmt > 0.01f)
            {
                const float orbitAngle = progress * juce::MathConstants<float>::twoPi * 2.0f + vNorm * juce::MathConstants<float>::twoPi;
                curPan = juce::jlimit (-1.0f, 1.0f, curPan + std::sin (orbitAngle) * orbitAmt * 0.7f);
            }

            const float panL = std::cos ((curPan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
            const float panR = std::sin ((curPan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);

            // Read source with interpolation
            int sIdx = (int) readPos;
            if (sMode == 3) sIdx = srcOffset + (sIdx % juce::jmax (1, sliceLen));
            else sIdx = sIdx % playableSrcLen;

            if (isRev) sIdx = (srcOffset + playableSrcLen - 1) - (sIdx % playableSrcLen);
            sIdx = juce::jlimit (0, srcLen - 2, sIdx);

            const float frac = (float) (readPos - (int) readPos);
            float sampL = source.getSample (0, sIdx) + (source.getSample (0, sIdx + 1) - source.getSample (0, sIdx)) * frac;
            float sampR = source.getSample (1, sIdx) + (source.getSample (1, sIdx + 1) - source.getSample (1, sIdx)) * frac;

            // Apply Voice Age filtering
            if (ageCoeff > 0.01f)
            {
                ageFilterL = ageFilterL * ageCoeff + sampL * (1.0f - ageCoeff);
                ageFilterR = ageFilterR * ageCoeff + sampR * (1.0f - ageCoeff);
                sampL = ageFilterL;
                sampR = ageFilterR;
            }

            // Swell envelope rising toward climax
            const float swellGain = tensionCurve (globalSwellNorm, shapeVal);
            const float voiceAmp = 1.0f / std::sqrt ((float) numVoices);

            swarmBuffer.addSample (0, i, sampL * swellGain * panL * voiceAmp);
            swarmBuffer.addSample (1, i, sampR * swellGain * panR * voiceAmp);

            readPos += pitchSpeed;
        }
    }

    // Normalize swarm
    const float swarmMag = swarmBuffer.getMagnitude (0, swellLen);
    if (swarmMag > 0.001f) swarmBuffer.applyGain (0.90f / swarmMag);

    // 4. Space / Diffusion wash
    const float spaceAmt = param (IDs::space);
    if (spaceAmt > 0.01f)
    {
        const int delayL = (int) (sr * 0.031);
        const int delayR = (int) (sr * 0.043);
        std::vector<float> bufL ((size_t) delayL, 0.0f);
        std::vector<float> bufR ((size_t) delayR, 0.0f);
        int idxL = 0, idxR = 0;
        float* ptrL = swarmBuffer.getWritePointer (0);
        float* ptrR = swarmBuffer.getWritePointer (1);

        for (int i = 0; i < swellLen; ++i)
        {
            const float inL = ptrL[i], inR = ptrR[i];
            const float dL = bufL[(size_t) idxL], dR = bufR[(size_t) idxR];
            bufL[(size_t) idxL] = inL + dL * 0.62f * spaceAmt;
            bufR[(size_t) idxR] = inR + dR * 0.62f * spaceAmt;
            idxL = (idxL + 1) % delayL;
            idxR = (idxR + 1) % delayR;
            ptrL[i] = inL * (1.0f - spaceAmt * 0.35f) + dL * (spaceAmt * 0.65f);
            ptrR[i] = inR * (1.0f - spaceAmt * 0.35f) + dR * (spaceAmt * 0.65f);
        }
    }

    // 5. Tone & Bass Cut Filters
    auto applyIIR = [&] (const juce::IIRCoefficients& coeffs)
    {
        juce::IIRFilter fL, fR;
        fL.setCoefficients (coeffs); fR.setCoefficients (coeffs);
        fL.processSamples (swarmBuffer.getWritePointer (0), swellLen);
        fR.processSamples (swarmBuffer.getWritePointer (1), swellLen);
    };

    const float hp = param (IDs::basscut);
    if (hp > 21.0f) applyIIR (juce::IIRCoefficients::makeHighPass (sr, hp));
    const float lp = param (IDs::tone);
    if (lp < 19900.0f) applyIIR (juce::IIRCoefficients::makeLowPass (sr, lp));

    // 6. Combine: Swell + Climax Hit (or Dry Replacement)
    const float dryReplaceAmt = param (IDs::dryReplace);
    const int hitLen = juce::jmin (srcLen, (int) (sr * 2.0));
    const int fullLen = swellLen + hitLen;
    juce::AudioBuffer<float> full (2, fullLen);
    full.clear();

    for (int ch = 0; ch < 2; ++ch)
    {
        full.copyFrom (ch, 0, swarmBuffer, ch, 0, swellLen);
        // If dry replacement is 1.0, the dry hit is replaced by the peak converged swarm!
        if (dryReplaceAmt < 0.99f)
        {
            full.copyFrom (ch, swellLen, source, ch, 0, hitLen);
            full.applyGain (ch, swellLen, hitLen, 1.0f - dryReplaceAmt);
        }
    }
    out->fullLengthSec = fullLen / sr;
    out->beats = beats;

    // 7. Trim
    int tStart = (int) (param (IDs::trimStart) * fullLen);
    int tEnd   = (int) (param (IDs::trimEnd) * fullLen);
    tStart = juce::jlimit (0, fullLen - 1, tStart);
    tEnd   = juce::jlimit (tStart + (int) (sr * 0.02), fullLen, tEnd);
    const int trimLen = tEnd - tStart;
    out->trimStartSec = tStart / sr;
    out->trimEndSec   = tEnd / sr;
    int hitIdx = swellLen - tStart;
    if (hitIdx < 0 || hitIdx >= trimLen) hitIdx = -1;

    // 8. Pitch sweep
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

    // 9. Volume envelope
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
    }
    setLatencySamples (param (IDs::align) > 0.5f ? latency : 0);
}

// ---------------- Realtime Process Block ----------------

void PreChorusProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    // Track host BPM
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (pos->getBpm()) hostBpm.store (*pos->getBpm());
        }
    }

    // Live Input Meter & Capture State Machine
    const float inPeak = buffer.getMagnitude (0, numSamples);
    inputMeter.store (inPeak);

    const auto state = captureState.load();
    if (state == CaptureState::armed)
    {
        const int mode = (int) param (IDs::captureMode);
        const float threshLin = juce::Decibels::decibelsToGain (param (IDs::thresh));
        if (mode == 7 /* Manual */ || inPeak >= threshLin)
        {
            captureState.store (CaptureState::recording);
            captureWritePos = 0;
            captureSilenceCounter = 0;

            if (mode >= 1 && mode <= 6)
            {
                // Musical divisions: 1/16, 1/8, 1/4(1 beat), 1/2(2 beats), 1 bar(4 beats), 2 bars(8 beats)
                const double beatFractions[] = { 0.25, 0.5, 1.0, 2.0, 4.0, 8.0 };
                const double secPerBeat = 60.0 / juce::jlimit (30.0, 300.0, hostBpm.load());
                captureTargetSamples = (int) (beatFractions[mode - 1] * secPerBeat * hostSampleRate);
            }
            else
            {
                captureTargetSamples = (int) (hostSampleRate * 6.0); // max 6s threshold capture
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
        if (mode == 0 /* Threshold */)
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

    // MIDI & Triggers
    if (stopRequest.exchange (0))
    {
        for (auto& v : voices) v.active = false;
        playhead.store (-1);
    }
    if (triggerRequest.exchange (0)) startVoice (1.0f);

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn()) startVoice (m.getFloatVelocity());
        else if (m.isAllNotesOff()) for (auto& v : voices) v.active = false;
    }

    // Realtime playback voices
    auto r = getRendered();
    if (r == nullptr || r->audio.getNumSamples() == 0) return;

    const float dryLvl = dryParam != nullptr ? dryParam->load() : 1.0f;
    const float wetLvl = wetParam != nullptr ? wetParam->load() : 1.0f;

    if (dryLvl > 0.0f && buffer.getNumChannels() > 0) buffer.applyGain (dryLvl);
    else buffer.clear();

    int activePos = -1;
    for (auto& v : voices)
    {
        if (! v.active) continue;
        renderRange (buffer, *r, v.pos, numSamples, dryLvl, wetLvl * v.gain);
        v.pos += numSamples;
        activePos = v.pos;
        if (v.pos >= r->audio.getNumSamples()) v.active = false;
    }
    playhead.store (activePos);
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

void PreChorusProcessor::renderRange (juce::AudioBuffer<float>& out, const RenderedSample& r, int start, int num, float dry, float wet)
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
            const float g = (sampleIdx < hitAt) ? wet : dry;
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
    juce::AudioBuffer<float> buf ((int) reader->numChannels, len);
    reader->read (&buf, 0, len, 0, true, true);
    {
        const juce::ScopedLock sl (sourceLock);
        loadedBuffer = std::move (buf);
        loadedSR = reader->sampleRate;
    }
    currentFile = f;
    refreshFolderList (f);
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
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void PreChorusProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (! state.isValid()) return;
        apvts.replaceState (state);
        juce::File f (state.getProperty ("file", "").toString());
        if (f.existsAsFile()) loadSampleFile (f);
        activeSlot.store (state.getProperty ("activeSlot", 0));
        captureLockState.store (state.getProperty ("captureLock", false));
        dirty = true;
    }
}

juce::AudioProcessorEditor* PreChorusProcessor::createEditor() { return new PreChorusEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PreChorusProcessor(); }
