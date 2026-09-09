#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const float kPitchOct[] = { 1.0f, 2.0f, 4.0f };
    const int kSyncBars[]   = { 2, 4, 8, 16 }; // beats: 1/2 bar(2), 1 bar(4), 2 bars(8), 4 bars(16)
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

    // Initial default test sound (a warm vocal transient synth chord)
    // so PreChorus sounds incredible even before loading a file or recording!
    sourceSR = 44100.0;
    sourceBuffer.setSize (2, (int) (sourceSR * 1.5));
    sourceBuffer.clear();
    for (int i = 0; i < sourceBuffer.getNumSamples(); ++i)
    {
        const double t = (double) i / sourceSR;
        const float env = (float) (std::exp (-t * 3.5));
        const float s1 = std::sin ((float) (2.0 * juce::MathConstants<double>::pi * 261.63 * t)); // C4
        const float s2 = std::sin ((float) (2.0 * juce::MathConstants<double>::pi * 329.63 * t)); // E4
        const float s3 = std::sin ((float) (2.0 * juce::MathConstants<double>::pi * 392.00 * t)); // G4
        const float s4 = std::sin ((float) (2.0 * juce::MathConstants<double>::pi * 523.25 * t)); // C5
        const float val = (s1 * 0.4f + s2 * 0.3f + s3 * 0.3f + s4 * 0.2f) * env * 0.7f;
        sourceBuffer.setSample (0, i, val);
        sourceBuffer.setSample (1, i, val);
    }

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

    // Voice Engine
    p.push_back (std::make_unique<juce::AudioParameterInt> (IDs::voiceCount, "Voices", 2, 12, 6));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::spread, "Spread", juce::NormalisableRange<float> (0.0f, 1.5f, 0.01f), 0.35f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::detune, "Detune", juce::NormalisableRange<float> (0.0f, 50.0f, 0.5f), 18.0f));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::harmony, "Harmony", juce::StringArray { "Unison", "Octaves", "Power 5ths", "Choral Stack" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::panSpread, "Pan Spread", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.85f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::space, "Space", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.40f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::reverse, "Reverse", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.50f));

    // Swell & Tone
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::tail, "Length", juce::NormalisableRange<float> (0.1f, 8.0f, 0.01f, 0.5f), 2.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::shape, "Shape", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.4f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::tone, "Tone", juce::NormalisableRange<float> (200.0f, 20000.0f, 1.0f, 0.25f), 14000.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::basscut, "Bass Cut", juce::NormalisableRange<float> (20.0f, 1000.0f, 1.0f, 0.35f), 80.0f));

    // Live Capture
    p.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::captureMode, "Capture Mode", juce::StringArray { "Threshold", "1 Beat", "2 Beats", "1 Bar", "2 Bars", "Manual" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::thresh, "Threshold", juce::NormalisableRange<float> (-48.0f, 0.0f, 0.5f), -24.0f));

    // Mix
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::dry, "Dry", juce::NormalisableRange<float> (0.0f, 1.5f, 0.01f), 1.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::wet, "Wet", juce::NormalisableRange<float> (0.0f, 1.5f, 0.01f), 1.0f));

    // Sync & Alignment
    p.push_back (std::make_unique<juce::AudioParameterBool> (IDs::align, "PDC Align", true));
    p.push_back (std::make_unique<juce::AudioParameterBool> (IDs::sync, "Sync", true));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::syncLen, "Sync Length", juce::StringArray { "1/2 Bar", "1 Bar", "2 Bars", "4 Bars" }, 1));

    // Pitch sweep
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::pitch, "Pitch Sweep", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::pitchRange, "Pitch Range", juce::StringArray { "1 Oct", "2 Oct", "4 Oct" }, 1));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::pitchTension, "Pitch Tension", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));

    // Volume Envelope
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::volStart, "Vol Start", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::volEnd, "Vol End", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::volTension, "Vol Tension", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.3f));

    // Trim
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

    // Allocate 10 seconds max buffer for Live Capture
    const int maxCaptureSamples = (int) (sampleRate * 10.0);
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
}

void PreChorusProcessor::randomizePreChorus()
{
    auto rnd = [] (float a, float b) { return a + (b - a) * juce::Random::getSystemRandom().nextFloat(); };
    setParam (IDs::voiceCount, (float) juce::Random::getSystemRandom().nextInt (juce::Range<int> (4, 11)));
    setParam (IDs::spread, rnd (0.15f, 0.85f));
    setParam (IDs::detune, rnd (8.0f, 32.0f));
    setParam (IDs::panSpread, rnd (0.5f, 1.0f));
    setParam (IDs::space, rnd (0.2f, 0.7f));
    setParam (IDs::reverse, rnd (0.2f, 0.8f));
    setParam (IDs::shape, rnd (-0.3f, 0.7f));
    setParam (IDs::tone, rnd (6000.0f, 18000.0f));
    setParam (IDs::basscut, rnd (40.0f, 180.0f));
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

        {
            const juce::ScopedLock sl (sourceLock);
            sourceBuffer = std::move (newBuf);
            sourceSR = hostSampleRate;
            currentFile = juce::File(); // represents live capture
        }
        dirty = true;
    }
    captureState.store (CaptureState::idle);
    captureWritePos = 0;
}

// ---------------- Multi-Voice Render Engine ----------------

void PreChorusProcessor::render()
{
    auto out = std::make_shared<RenderedSample>();
    const double sr = hostSampleRate > 1000.0 ? hostSampleRate : 44100.0;
    out->sampleRate = sr;

    juce::AudioBuffer<float> rawSource;
    double rawSR = 44100.0;
    {
        const juce::ScopedLock sl (sourceLock);
        rawSource.makeCopyOf (sourceBuffer);
        rawSR = sourceSR;
    }
    if (rawSource.getNumSamples() == 0) return;

    // Resample source to host sample rate if different
    juce::AudioBuffer<float> source;
    if (std::abs (rawSR - sr) > 1.0)
    {
        const double ratio = rawSR / sr;
        const int newLen = (int) (rawSource.getNumSamples() / ratio);
        source.setSize (2, newLen);
        for (int ch = 0; ch < 2; ++ch)
        {
            const float* srcPtr = rawSource.getReadPointer (juce::jmin (ch, rawSource.getNumChannels() - 1));
            float* dstPtr = source.getWritePointer (ch);
            for (int i = 0; i < newLen; ++i)
            {
                const double p = i * ratio;
                const int i0 = (int) p;
                const float frac = (float) (p - i0);
                const float s0 = srcPtr[juce::jmin (i0, rawSource.getNumSamples() - 1)];
                const float s1 = srcPtr[juce::jmin (i0 + 1, rawSource.getNumSamples() - 1)];
                dstPtr[i] = s0 + (s1 - s0) * frac;
            }
        }
    }
    else
    {
        source.makeCopyOf (rawSource);
    }

    const int srcLen = source.getNumSamples();

    // 1. Determine swell duration
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

    // 2. Multi-Voice generation
    const int numVoices = juce::jlimit (2, 12, (int) param (IDs::voiceCount));
    const float spreadSec = param (IDs::spread);
    const float detuneCents = param (IDs::detune);
    const int harmonyMode = (int) param (IDs::harmony);
    const float panSpread = param (IDs::panSpread);
    const float reverseAmt = param (IDs::reverse);
    const float shapeVal = param (IDs::shape);

    juce::AudioBuffer<float> choralSwell (2, swellLen);
    choralSwell.clear();

    // Prepare forward and reversed source buffers
    juce::AudioBuffer<float> revSource (2, srcLen);
    for (int ch = 0; ch < 2; ++ch)
    {
        const float* fwd = source.getReadPointer (ch);
        float* r = revSource.getWritePointer (ch);
        for (int i = 0; i < srcLen; ++i) r[i] = fwd[srcLen - 1 - i];
    }

    // Stack voices into the choral swell
    for (int v = 0; v < numVoices; ++v)
    {
        // Stereo panning
        const float pan = (numVoices > 1) ? ((float) v / (float) (numVoices - 1) * 2.0f - 1.0f) * panSpread : 0.0f;
        const float panL = std::cos ((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
        const float panR = std::sin ((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);

        // Detune and harmony shift
        const float voiceDetune = (numVoices > 1) ? (((float) v / (float) (numVoices - 1)) - 0.5f) * 2.0f * detuneCents : 0.0f;
        float harmonySemi = 0.0f;
        if (harmonyMode == 1) // Octaves
        {
            const float octs[] = { -12.0f, 0.0f, 12.0f, 0.0f };
            harmonySemi = octs[v % 4];
        }
        else if (harmonyMode == 2) // Power 5ths
        {
            const float fifths[] = { 0.0f, 7.0f, -5.0f, 12.0f };
            harmonySemi = fifths[v % 4];
        }
        else if (harmonyMode == 3) // Choral Stack (3rd, 5th, 7th, 8ve)
        {
            const float stack[] = { 0.0f, -12.0f, 4.0f, 7.0f, 11.0f, 12.0f, 16.0f, 19.0f };
            harmonySemi = stack[v % 8];
        }

        const float totalSemi = harmonySemi + voiceDetune / 100.0f;
        const double pitchSpeed = std::pow (2.0, totalSemi / 12.0);

        // Timing stagger before the drop
        const int staggerSamples = (int) (((float) v / (float) juce::jmax (1, numVoices - 1)) * spreadSec * sr * 0.75f);
        const int voiceStart = juce::jmax (0, swellLen - (int) (srcLen / pitchSpeed) - staggerSamples);

        // Render voice samples into choralSwell
        for (int i = 0; i < swellLen; ++i)
        {
            if (i < voiceStart) continue;
            const double srcPos = (i - voiceStart) * pitchSpeed;
            if (srcPos >= srcLen - 1) continue;

            const int s0 = (int) srcPos;
            const float frac = (float) (srcPos - s0);

            // Interpolate forward & reverse
            const float fL = source.getSample (0, s0) + (source.getSample (0, s0 + 1) - source.getSample (0, s0)) * frac;
            const float fR = source.getSample (1, s0) + (source.getSample (1, s0 + 1) - source.getSample (1, s0)) * frac;
            const float rL = revSource.getSample (0, s0) + (revSource.getSample (0, s0 + 1) - revSource.getSample (0, s0)) * frac;
            const float rR = revSource.getSample (1, s0) + (revSource.getSample (1, s0 + 1) - revSource.getSample (1, s0)) * frac;

            const float vL = fL * (1.0f - reverseAmt) + rL * reverseAmt;
            const float vR = fR * (1.0f - reverseAmt) + rR * reverseAmt;

            // Swell envelope rising toward the hit
            const float swellNorm = (float) i / (float) juce::jmax (1, swellLen - 1);
            const float swellGain = tensionCurve (swellNorm, shapeVal);

            choralSwell.addSample (0, i, vL * swellGain * panL);
            choralSwell.addSample (1, i, vR * swellGain * panR);
        }
    }

    // Normalize swell
    const float swellMag = choralSwell.getMagnitude (0, swellLen);
    if (swellMag > 0.001f) choralSwell.applyGain (0.85f / swellMag);

    // 3. Space / Diffusion wash
    const float spaceAmt = param (IDs::space);
    if (spaceAmt > 0.01f)
    {
        // Simple stereo comb/allpass ambient wash
        const int delayL = (int) (sr * 0.029);
        const int delayR = (int) (sr * 0.037);
        std::vector<float> bufL ((size_t) delayL, 0.0f);
        std::vector<float> bufR ((size_t) delayR, 0.0f);
        int idxL = 0, idxR = 0;
        float* ptrL = choralSwell.getWritePointer (0);
        float* ptrR = choralSwell.getWritePointer (1);

        for (int i = 0; i < swellLen; ++i)
        {
            const float inL = ptrL[i];
            const float inR = ptrR[i];
            const float dL = bufL[(size_t) idxL];
            const float dR = bufR[(size_t) idxR];
            bufL[(size_t) idxL] = inL + dL * 0.65f * spaceAmt;
            bufR[(size_t) idxR] = inR + dR * 0.65f * spaceAmt;
            idxL = (idxL + 1) % delayL;
            idxR = (idxR + 1) % delayR;
            ptrL[i] = inL * (1.0f - spaceAmt * 0.4f) + dL * (spaceAmt * 0.7f);
            ptrR[i] = inR * (1.0f - spaceAmt * 0.4f) + dR * (spaceAmt * 0.7f);
        }
    }

    // 4. Tone (Low Pass) & Bass Cut (High Pass)
    auto applyIIR = [&] (const juce::IIRCoefficients& coeffs)
    {
        juce::IIRFilter fL, fR;
        fL.setCoefficients (coeffs); fR.setCoefficients (coeffs);
        fL.processSamples (choralSwell.getWritePointer (0), swellLen);
        fR.processSamples (choralSwell.getWritePointer (1), swellLen);
    };

    const float hp = param (IDs::basscut);
    if (hp > 21.0f) applyIIR (juce::IIRCoefficients::makeHighPass (sr, hp));
    const float lp = param (IDs::tone);
    if (lp < 19900.0f) applyIIR (juce::IIRCoefficients::makeLowPass (sr, lp));

    // 5. Combine: Swell + Climax Hit (source audio)
    const int hitLen = juce::jmin (srcLen, (int) (sr * 2.0));
    const int fullLen = swellLen + hitLen;
    juce::AudioBuffer<float> full (2, fullLen);
    full.clear();
    for (int ch = 0; ch < 2; ++ch)
    {
        full.copyFrom (ch, 0, choralSwell, ch, 0, swellLen);
        full.copyFrom (ch, swellLen, source, ch, 0, hitLen);
    }
    out->fullLengthSec = fullLen / sr;
    out->beats = beats;

    // 6. Trim
    int tStart = (int) (param (IDs::trimStart) * fullLen);
    int tEnd   = (int) (param (IDs::trimEnd) * fullLen);
    tStart = juce::jlimit (0, fullLen - 1, tStart);
    tEnd   = juce::jlimit (tStart + (int) (sr * 0.02), fullLen, tEnd);
    const int trimLen = tEnd - tStart;
    out->trimStartSec = tStart / sr;
    out->trimEndSec   = tEnd / sr;
    int hitIdx = swellLen - tStart;
    if (hitIdx < 0 || hitIdx >= trimLen) hitIdx = -1;

    // 7. Pitch sweep (varispeed)
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

    // 8. Volume envelope
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
        if (mode == 5 /* Manual */ || inPeak >= threshLin)
        {
            captureState.store (CaptureState::recording);
            captureWritePos = 0;
            captureSilenceCounter = 0;

            if (mode >= 1 && mode <= 4)
            {
                // Sync length in beats
                const int beatCounts[] = { 1, 2, 4, 8 };
                const double secPerBeat = 60.0 / juce::jlimit (30.0, 300.0, hostBpm.load());
                captureTargetSamples = (int) (beatCounts[mode - 1] * secPerBeat * hostSampleRate);
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

            // Stop if silence detected after at least 0.3s of audio, or reached target
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

    // Buffer passthrough: mix dry live audio if enabled, or clear
    if (dryLvl > 0.0f && buffer.getNumChannels() > 0)
    {
        buffer.applyGain (dryLvl);
    }
    else
    {
        buffer.clear();
    }

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
        sourceBuffer = std::move (buf);
        sourceSR = reader->sampleRate;
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
        dirty = true;
    }
}

juce::AudioProcessorEditor* PreChorusProcessor::createEditor() { return new PreChorusEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PreChorusProcessor(); }
