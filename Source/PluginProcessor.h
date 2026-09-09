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

    // Convergence Engine
    static const juce::String timeSpread = "timeSpread";       // How far back voices begin
    static const juce::String timeConverge = "timeConverge";   // How tightly voices align at climax
    static const juce::String pitchSpread = "pitchSpread";     // Pitch offset scatter (+/- semitones)
    static const juce::String detune = "detune";               // Cents microdetune
    static const juce::String pitchConverge = "pitchConverge"; // How strongly pitches glide into unison
    static const juce::String scaleLock = "scaleLock";         // 0: Chromatic, 1: Major, 2: Minor, 3: Pentatonic, 4: Octaves/5ths
    static const juce::String panSpread = "panSpread";         // Stereo spread
    static const juce::String panConverge = "panConverge";     // -1: Bloom outward, 0: Static, 1: Collapse to center
    static const juce::String toneConverge = "toneConverge";   // Cutoff convergence

    // Physics & Modulation
    static const juce::String attraction = "attraction";       // Pull strength towards target
    static const juce::String turbulence = "turbulence";       // Organic flutter / jitter
    static const juce::String overshoot = "overshoot";         // Spring-like overshoot past unison
    static const juce::String orbit = "orbit";                 // Orbital stereo circulation
    static const juce::String seed = "seed";                   // Deterministic random seed

    // Swell & Tone
    static const juce::String tail = "tail";
    static const juce::String shape = "shape";
    static const juce::String tone = "tone";
    static const juce::String basscut = "basscut";
    static const juce::String space = "space";

    // Mix & PDC
    static const juce::String dry = "dry", wet = "wet";
    static const juce::String dryReplace = "dryReplace";       // Swarm replaces or blends with dry hit
    static const juce::String align = "align";                 // PDC downbeat alignment
    static const juce::String sync = "sync", syncLen = "syncLen";

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

    void selectCaptureSlot (int slotIdx);
    int getActiveCaptureSlot() const { return activeSlot.load(); }
    bool isSlotFilled (int slotIdx) const;
    void setCaptureLock (bool locked);
    bool isCaptureLocked() const { return captureLockState.load(); }

    // Swarm Playback & Preview
    void triggerPreview() { triggerRequest = 1; }
    void stopAll() { stopRequest = 1; }
    bool exportWav (const juce::File& dest);
    void resetEdits();
    void randomizePreChorus();
    void regenerateSeed();

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
    void renderRange (juce::AudioBuffer<float>& out, const RenderedSample& r, int start, int num, float dry, float wet);

    // Source Buffers
    juce::AudioFormatManager formatManager;
    juce::CriticalSection sourceLock;
    juce::AudioBuffer<float> loadedBuffer;       // Loaded file
    double loadedSR = 44100.0;
    juce::File currentFile;
    juce::Array<juce::File> folderFiles;
    int currentIndex = -1;

    // Capture History (8 memory slots)
    static constexpr int kNumHistorySlots = 8;
    std::array<juce::AudioBuffer<float>, kNumHistorySlots> captureSlots;
    std::array<double, kNumHistorySlots> captureSlotSRs { 44100.0 };
    std::array<bool, kNumHistorySlots> captureSlotFilled { false };
    std::atomic<int> activeSlot { 0 };
    std::atomic<bool> captureLockState { false };

    // Live Capture recording state
    std::atomic<CaptureState> captureState { CaptureState::idle };
    std::atomic<float> inputMeter { 0.0f };
    juce::AudioBuffer<float> captureRingBuffer;
    int captureWritePos = 0;
    int captureTargetSamples = 0;
    int captureSilenceCounter = 0;

    mutable juce::SpinLock renderLock;
    std::shared_ptr<RenderedSample> rendered;

    double hostSampleRate = 44100.0;
    std::atomic<double> hostBpm { 120.0 };
    double lastRenderBpm = 0.0;
    std::atomic<bool> dirty { false }, previewAfterRender { false };
    std::atomic<int> triggerRequest { 0 }, stopRequest { 0 }, playhead { -1 };

    std::array<Voice, 16> voices;
    juce::uint32 voiceCounter = 0;
    std::atomic<float>* dryParam = nullptr;
    std::atomic<float>* wetParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreChorusProcessor)
};
