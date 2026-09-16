#pragma once
#include <JuceHeader.h>
#include "Colony.h"
#include "Talkbox.h"

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
    void resetAllToDefaults();
    void randomizePreChorus();
    void regenerateSeed();
    void loadFactoryPreset (int index);
    static juce::StringArray getFactoryPresetNames();

    // User preset files (.pcpreset)
    bool savePresetToFile (const juce::File& dest);
    bool loadPresetFromFile (const juce::File& src);
    juce::File getCurrentPresetFile() const { return currentPresetFile; }
    static juce::String getPresetExtension() { return ".pcpreset"; }
    static juce::File getUserPresetFolder();

    // A/B compare
    void setABSlot (int slot);
    int  getABSlot() const { return abSlot; }
    void copyABSlot();

    // MIDI learn / CC mapping. Indices are positions in getParameters().
    void beginMidiLearn (const juce::String& paramID);
    void cancelMidiLearn() { midiLearnTarget.store (-1); }
    bool isLearningMidi (const juce::String& paramID) const;
    bool isLearningAnything() const { return midiLearnTarget.load() >= 0; }
    int  getMidiCcForParam (const juce::String& paramID) const;   // -1 if unmapped
    void clearMidiMappingFor (const juce::String& paramID);
    void clearAllMidiMappings();
    juce::Array<std::pair<int, juce::String>> getMidiMappings() const; // { cc, paramID }

    // Colony: the interactive orb ecosystem behind the constellation display.
    Colony& getColony() { return colony; }
    const Colony& getColony() const { return colony; }
    void colonyLeftClick();
    void colonyRightClick();
    void colonyMiddleClick();
    void colonyClickAt (float nx, float ny);
    void colonyBeginDrag (float nx, float ny);
    void colonyDragTo (float nx, float ny);
    void colonyEndDrag();
    void colonyAddGravity();
    void colonyReleaseGravity();
    void colonyAddEnzyme();
    void colonyAddGamma();
    void colonyAddWater();
    void colonyReset();
    bool isColonyLifeEnabled() const { return colony.isAutonomyEnabled(); }
    void setColonyLifeEnabled (bool shouldRun);

    // Talkbox distress call: after a long silence the plugin calls out for help.
    static constexpr double kIdleSecondsBeforeDistress = 600.0;   // 10 minutes
    void triggerDistressCall();                  // also used by the Options test button
    bool isDistressCallActive() const { return talkbox.isActive(); }
    bool isDistressEnabled() const { return distressEnabled.load(); }
    void setDistressEnabled (bool shouldBeEnabled);
    double getIdleSeconds() const;
    /** Seconds of ghost-tracer trails left on the orbs, 0 when none. */
    float getGhostSeconds() const { return ghostSeconds.load(); }
    float getGhostAmount() const;
    /** Current playback rate of the swell: dips below 1 after a distress call. */
    float getPitchDipAmount() const;
    void  noteActivity();
    /** True when the plugin window is closed or minimised - nobody is watching. */
    bool  isUiHidden() const;

    // Options
    bool areTooltipsEnabled() const { return tooltipsEnabled.load(); }
    void setTooltipsEnabled (bool shouldBeEnabled);

    static juce::String getVersionString() { return JucePlugin_VersionString; }

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

    struct Voice
    {
        bool active = false;
        double pos = 0.0;                 // read head into the rendered swell
        float gain = 1.0f;
        juce::uint32 id = 0;

        // Granular time-stretch state: two overlapping grains keep the pitch put
        // while the read head crawls, so the swell drags without dropping an octave.
        struct Grain { bool active = false; double start = 0.0; double phase = 0.0; };
        Grain grains[2];
        double sinceGrain = 0.0;
        bool headDone = false;      // read head hit the end; let the grains ring out
    };
    void startVoice (float gain);
    int  indexOfParam (const juce::String& paramID) const;
    void handleMidiCc (int ccNumber, int ccValue);
    void applyPendingMidi();
    juce::ValueTree buildFullState() const;
    void applyFullState (const juce::ValueTree& state);
    void syncVoiceCountToColony();
    double renderRange (juce::AudioBuffer<float>& out, const RenderedSample& r, double startPos, int num,
                        float dry, float wet, float duckGain, double rate);
    void renderVoiceGranular (juce::AudioBuffer<float>& out, const RenderedSample& r, Voice& v, int num,
                              float dry, float wet, float duckGain, double rate, double stretch);
    void updateDistress();
    void armFrenchCry();
    void speak (TalkboxVoice::Mode mode, float gain, int repeats,
                float reverb, float delay, float repeatGapSeconds = 0.35f);
    void firePendingSparkles();
    void consumeColonyEvents (const Colony::StepResult& ev);

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

    std::array<Voice, 16> voices;
    juce::uint32 voiceCounter = 0;
    std::atomic<float>* dryParam = nullptr;
    std::atomic<float>* wetParam = nullptr;

    // MIDI learn: the audio thread only ever touches atomics; the 30Hz timer
    // applies the queued values on the message thread.
    static constexpr int kNumCc = 128;
    std::array<std::atomic<int>,   kNumCc> ccToParamIndex;
    std::array<std::atomic<float>, kNumCc> ccPendingValue;
    std::array<std::atomic<bool>,  kNumCc> ccHasPending;
    std::atomic<int> midiLearnTarget { -1 };

    // A/B compare, user presets, options
    juce::ValueTree abStates[2];
    int abSlot = 0;
    juce::File currentPresetFile;
    std::atomic<bool> tooltipsEnabled { true };
    bool hasRandomizedOnce = false;

    Colony colony;
    int lastKnownVoiceCount = -1;
    juce::uint32 lastColonyStepMs = 0;

    // Distress call & its after-effects
    TalkboxVoice talkbox;
    WhistleBank whistles;
    std::atomic<bool> distressEnabled { true };
    std::atomic<juce::uint32> lastActivityMs { 0 };
    std::atomic<float> ghostSeconds { 0.0f };
    std::atomic<float> pitchDipSeconds { 0.0f };
    std::atomic<float> talkboxGain { 0.0f };
    std::atomic<float> sparkleGain { 0.14f };

    // Water sparkles are scheduled, not queued: one pour can keep glinting for
    // up to three minutes, so holding a synth voice open for each is no good.
    struct PendingSparkle { juce::uint32 dueMs; float hz; };
    static constexpr int kMaxPendingSparkles = 512;
    static constexpr int kMaxSparkleDelayMs = 180000;    // three minutes
    std::vector<PendingSparkle> pendingSparkles;
    juce::uint32 lastDragMs = 0;
    std::atomic<float> colonyTimeDirection { 1.0f };
    std::atomic<float> colonyClutchRate { 1.0f };
    std::atomic<float> colonyStretch { 1.0f };
    std::atomic<float> colonyWashout { 0.0f };

    // A short delay line, used when the voxbox has to be heard twice over
    juce::AudioBuffer<float> delayBuffer;
    int delayWritePos = 0;
    std::atomic<float> delayAmount { 0.0f };

    // Washed-out water and the babble's tail both live in this one reverb.
    juce::Reverb outputReverb;
    std::atomic<float> reverbAmount { 0.0f };
    float reverbDecayPerTick = 0.0f;
    std::array<float, 2> washFilterState { 0.0f, 0.0f };
    std::atomic<int> renderedLength { 0 };
    double secondsUntilNextCry = 0.0;
    float shoeClock = 0.0f;
    // An explosion sometimes leaves a delayed cry hanging over the colony
    float delayedCrySeconds = 0.0f;
    // RANDOM lights a long fuse of its own
    float spanishCrySeconds = 0.0f;
    // Loading a preset sometimes leaves a French one waiting in the wings
    float frenchCrySeconds = 0.0f;
    // Rolled once, when the plugin is first loaded
    float ranchCrySeconds = 0.0f;
    // Dropping a sample in occasionally gives it an idea
    float funnyCrySeconds = 0.0f;
    // A right-click in the animation occasionally sets off three calls, each
    // waiting out its own 4-to-9 minute gap before it arrives
    float tripleCrySeconds = 0.0f;
    int tripleCriesLeft = 0;
    int talkboxRepeatsLeft = 0;
    float talkboxRepeatGap = 0.0f;
    TalkboxVoice::Mode talkboxRepeatMode = TalkboxVoice::Mode::plea;
    float talkboxRepeatGain = 1.0f;
    float playbackRate = 1.0f;
    juce::Random distressRng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreChorusProcessor)
};
