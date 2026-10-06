// PreChorus™
// Copyright © 2026 Sheldon Davidson. All rights reserved.
// Proprietary source code. See LICENSE and COPYRIGHT-TRADEMARK.md.
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once
#include <JuceHeader.h>

namespace IDs
{
    // Source & Capture
    static const juce::String sourceMode = "sourceMode";       // 0: Live Capture, 1: Loaded Sample, 2: Hybrid, 3: Sliced
    static const juce::String captureMode = "captureMode";     // 0: Threshold, 1: 1/16, 2: 1/8, 3: 1/4, 4: 1/2, 5: 1 Bar, 6: 2 Bars, 7: Manual
    static const juce::String thresh = "thresh";
    static const juce::String captureSlot = "captureSlot";     // 0..7 (Capture History)
    static const juce::String captureLock = "captureLock";     // Lock active capture

    // Voice Swarm Engine (Up to 32 Voices)
    static const juce::String voiceCount = "voiceCount";       // 1 to 32
    static const juce::String voiceDensity = "voiceDensity";   // Curve controlling when voices join
    static const juce::String voiceAge = "voiceAge";           // Degrades/darkens earlier voices
    static const juce::String progReveal = "progReveal";       // Progressive reveal: fragments -> full phrase
    static const juce::String voiceDirection = "voiceDir";     // 0: Forward, 1: Reverse, 2: Alternating, 3: Random
    static const juce::String character = "character";         // 0: Clean Digital, 1: Analog Ensemble, 2: Bucket-Brigade, 3: Tape Choir, 4: Dimension, 5: String Ensemble, 6: Granular Cloud, 7: Lo-Fi Choral
    static const juce::String humanize = "humanize";           // Organic micro-variations
    static const juce::String grainSize = "grainSize";         // Grain size for granular cloud (10ms - 200ms)

    // Convergence Engine & Macros
    static const juce::String macro = "macro";                 // Global Convergence Macro (scales all dimensions)
    static const juce::String freeze = "freeze";               // Suspends convergence & holds ensemble
    static const juce::String revConverge = "revConverge";     // Reverse convergence: unified -> scattered
    static const juce::String postRelease = "postRelease";     // 0: Cut at Impact, 1: Sustain Chorus, 2: Scatter Out
    static const juce::String timeSpread = "timeSpread";       // How far back voices begin
    static const juce::String timeConverge = "timeConverge";   // How tightly voices align at climax
    static const juce::String pitchSpread = "pitchSpread";     // Pitch offset scatter (+/- semitones)
    static const juce::String detune = "detune";               // Cents microdetune
    static const juce::String pitchConverge = "pitchConverge"; // How strongly pitches glide into unison
    static const juce::String scaleLock = "scaleLock";         // 0: Chromatic, 1: Major, 2: Minor, 3: Pentatonic, 4: Octaves/5ths
    static const juce::String panSpread = "panSpread";         // Stereo spread
    static const juce::String panConverge = "panConverge";     // -1: Bloom outward, 0: Static, 1: Collapse to center
    static const juce::String toneConverge = "toneConverge";   // Cutoff convergence
    static const juce::String focus = "focus";                 // Focus: accelerates convergence into laser focus

    // Physics & 3D Distance
    static const juce::String attraction = "attraction";       // Pull strength towards target
    static const juce::String turbulence = "turbulence";       // Organic flutter / jitter
    static const juce::String overshoot = "overshoot";         // Spring-like overshoot past unison
    static const juce::String orbit = "orbit";                 // Orbital stereo circulation
    static const juce::String distance = "distance";           // 3D Distance approach (far cavern -> upfront dry)
    static const juce::String seed = "seed";                   // Deterministic random seed

    // Swell & Tone Shaping
    static const juce::String tail = "tail";
    static const juce::String shape = "shape";
    static const juce::String tone = "tone";
    static const juce::String basscut = "basscut";
    static const juce::String resonance = "resonance";         // Filter Q
    static const juce::String tilt = "tilt";                   // Tilt EQ (-1: dark, +1: bright)
    static const juce::String presence = "presence";           // 10kHz vocal air sheen
    static const juce::String air = "air";                     // Filtered HF excitation to target & ensemble
    static const juce::String space = "space";
    static const juce::String drive = "drive";                 // Saturation & soft clipping
    static const juce::String transients = "transients";       // -1: Soften, +1: Preserve punch
    static const juce::String formant = "formant";             // Vocal formant shift
    static const juce::String monoBass = "monoBass";           // Mono bass crossover (Hz)
    static const juce::String ducking = "ducking";             // Sidechain ducking

    // Mix & PDC & Sequence
    static const juce::String dry = "dry", wet = "wet";
    static const juce::String dryReplace = "dryReplace";       // Swarm replaces or blends with dry hit
    static const juce::String align = "align";                 // PDC downbeat alignment
    static const juce::String sync = "sync", syncLen = "syncLen";
    static const juce::String sequence = "sequence";           // Target Sequence

    // Envelopes & Trim
    static const juce::String trimStart = "trimStart", trimEnd = "trimEnd";
    static const juce::String pitch = "pitch", pitchRange = "pitchRange", pitchTension = "pitchTension";
    static const juce::String volStart = "volStart", volEnd = "volEnd", volTension = "volTension";
}

inline float tensionCurve (float x, float t)
{
    x = juce::jlimit (0.0f, 1.0f, x);
    if (std::abs (t) < 0.001f) return x;
    const float k = 1.0f + 5.0f * std::abs (t);
    return t > 0.0f ? std::pow (x, k) : 1.0f - std::pow (1.0f - x, k);
}

struct RenderedSample
{
    juce::AudioBuffer<float> audio;
    int hitIndex = -1;
    double sampleRate = 44100.0;
    int beats = 0;
    int beatsPerBar = 4;
    double fullLengthSec = 0.0;
    double trimStartSec = 0.0, trimEndSec = 0.0;
    std::vector<float> pitchSemi;
    std::vector<float> gainLin;
    static constexpr int envStep = 256;
};

class PreChorusProcessor : public juce::AudioProcessor,
                           private juce::Timer,
                           private juce::AudioProcessorValueTreeState::Listener
{
public:
    PreChorusProcessor();
    ~PreChorusProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Sample loading
    bool loadSampleFile (const juce::File& f, bool previewAfter = false);
    void nextSample();
    void prevSample();
    juce::File getCurrentFile() const { return currentFile; }
    int getSampleIndex() const { return currentIndex; }
    int getSampleCount() const { return folderFiles.size(); }

    // Live Capture & History
    enum class CaptureState { idle, armed, recording, done };
    void armCapture();
    void triggerManualCapture();
    void stopCapture();
    bool isCapturing() const { return captureState.load() == CaptureState::recording; }
    bool isCaptureArmed() const { return captureState.load() == CaptureState::armed; }
    CaptureState getCaptureState() const { return captureState.load(); }
    float getLiveInputMeter() const { return inputMeter.load(); }
    float getOutputLevel() const { return outputMeter.load(); }

    void selectCaptureSlot (int slotIdx);
    int getActiveCaptureSlot() const { return activeSlot.load(); }
    bool isSlotFilled (int slotIdx) const;
    void setCaptureLock (bool locked);
    bool isCaptureLocked() const { return captureLockState.load(); }

    float getTargetConfidence() const { return targetConfidence.load(); }

    // Swarm Playback & Preview
    void triggerPreview() { triggerRequest = 1; }
    void stopAll() { stopRequest = 1; }
    bool exportWav (const juce::File& dest);
    void resetEdits();
    void randomizePreChorus();
    void regenerateSeed();
    void loadFactoryPreset (int index);
    static juce::StringArray getFactoryPresetNames();

    // A/B compare and portable user presets (parameter state only; source audio is never embedded)
    bool storeCompareState (int slot);
    bool recallCompareState (int slot);
    int getCurrentCompareSlot() const { return currentCompareSlot.load(); }
    bool saveUserPreset (const juce::File& file) const;
    bool loadUserPreset (const juce::File& file);

    std::shared_ptr<const RenderedSample> getRendered() const;
    int getPlayheadPosition() const { return playhead.load(); }
    double getHostBpm() const { return hostBpm.load(); }
    float param (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    void setParam (const juce::String& id, float value);

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged (const juce::String&, float) override { dirty = true; }
    void timerCallback() override;
    void render();
    void refreshFolderList (const juce::File& f);

    struct Voice { bool active = false; int pos = 0; float gain = 1.0f; juce::uint32 id = 0; };
    void startVoice (float gain);
    void renderRange (juce::AudioBuffer<float>& out, const RenderedSample& r, int start, int num, float dry, float wet, float duckGain);

    // Source Buffers
    juce::AudioFormatManager formatManager;
    juce::CriticalSection sourceLock;
    juce::AudioBuffer<float> loadedBuffer;
    double loadedSR = 44100.0;
    juce::File currentFile;
    juce::Array<juce::File> folderFiles;
    int currentIndex = -1;

    // Capture History
    static constexpr int kNumHistorySlots = 8;
    std::array<juce::AudioBuffer<float>, kNumHistorySlots> captureSlots;
    std::array<double, kNumHistorySlots> captureSlotSRs { 44100.0 };
    std::array<bool, kNumHistorySlots> captureSlotFilled { false };
    std::atomic<int> activeSlot { 0 };
    std::atomic<bool> captureLockState { false };

    // Live Capture state
    std::atomic<CaptureState> captureState { CaptureState::idle };
    std::atomic<float> inputMeter { 0.0f };
    std::atomic<float> outputMeter { 0.0f };
    juce::AudioBuffer<float> captureRingBuffer;
    int captureWritePos = 0;
    int captureTargetSamples = 0;
    int captureSilenceCounter = 0;

    // Target Confidence & Transport Recovery
    std::atomic<float> targetConfidence { 1.0f };
    juce::int64 lastPlayheadSample = -1;
    double lastKnownBpm = 120.0;

    // Sidechain ducking envelope follower
    float duckEnv = 0.0f;

    mutable juce::SpinLock renderLock;
    std::shared_ptr<RenderedSample> rendered;

    double hostSampleRate = 44100.0;
    std::atomic<double> hostBpm { 120.0 };
    double lastRenderBpm = 0.0;
    std::atomic<bool> dirty { false }, previewAfterRender { false };
    std::atomic<int> triggerRequest { 0 }, stopRequest { 0 }, playhead { -1 };
    std::array<juce::ValueTree, 2> compareStates;
    std::atomic<int> currentCompareSlot { 0 };

    std::array<Voice, 16> voices;
    juce::uint32 voiceCounter = 0;
    std::atomic<float>* dryParam = nullptr;
    std::atomic<float>* wetParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreChorusProcessor)
};
