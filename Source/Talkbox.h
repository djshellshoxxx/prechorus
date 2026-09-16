#pragma once
#include <JuceHeader.h>

//==============================================================================
//  TALKBOX DISTRESS CALL
//
//  A formant-synthesis voice that calls out when the plugin has been left alone.
//  Every utterance is planned fresh from random material - a different number of
//  syllables, different vowels, a different throat size and a different melodic
//  contour - so it always sounds like someone saying something, never the same
//  something twice.
//
//  Planning happens on the message thread while the voice is idle; playback is
//  allocation-free and runs on the audio thread.
//==============================================================================

class TalkboxVoice
{
public:
    static constexpr int kMaxSyllables = 24;

    void prepare (double sampleRate);

    enum class Mode
    {
        plea,             // a short call for help
        urgentBabble,     // long, fast, agitated - urgent-sounding speech
        foodAndShelter,   // the same words every time, in a different voice every time
        havingFun,        // "are you having fun - I am hungry"
        tieMyShoes,       // "help me tie my shoes"
        spanishPlea,      // "ayuda - socorro por favor"
        frenchPlea,       // "au secours - aidez-moi s'il vous plait"
        ranchAndMayo,     // "ranch and mayo in my hair"
        somethingFunny,   // one of several daft things, picked at random
        randomOutburst    // whatever else is on its mind
    };

    /** Builds a new random utterance and arms it. Safe to call only while idle. */
    void planUtterance (Mode mode = Mode::plea);
    void planUtterance (bool urgent) { planUtterance (urgent ? Mode::urgentBabble : Mode::plea); }

    bool  isActive() const noexcept   { return active.load (std::memory_order_acquire); }
    float getLevel() const noexcept   { return lastLevel.load (std::memory_order_relaxed); }
    float getProgress() const noexcept;
    double getPlannedLengthSeconds() const noexcept { return totalSeconds; }
    void  stop() noexcept;

    /** Adds the cry into the buffer. Does nothing when idle. */
    void render (juce::AudioBuffer<float>& buffer, int numSamples, float gain);

private:
    struct Syllable
    {
        float durSec = 0.2f;
        float pitchHz = 180.0f;
        float endPitchHz = 180.0f;
        float f1 = 700.0f, f2 = 1200.0f, f3 = 2500.0f;
        float openness = 1.0f;      // amplitude weight
        float noiseAmt = 0.0f;      // fricative content
        float onsetBurst = 0.0f;    // plosive transient at the syllable edge
    };

    struct Formants { float f1, f2, f3; };

    // Chamberlin state-variable bandpass
    struct SVF
    {
        float low = 0.0f, band = 0.0f;
        void reset() { low = band = 0.0f; }
        float process (float in, float f, float q)
        {
            const float high = in - low - q * band;
            band += f * high;
            low  += f * band;
            return band;
        }
    };

    std::array<Syllable, kMaxSyllables> plan;
    int numSyllables = 0;
    double totalSeconds = 0.0;

    std::atomic<bool> active { false };
    std::atomic<float> lastLevel { 0.0f };

    double sr = 44100.0;
    double posSec = 0.0;
    double carrierPhase = 0.0;
    float  vibPhase = 0.0f, vibRate = 5.0f, vibDepth = 0.02f;
    float  formantScale = 1.0f;
    float  pitchScale = 1.0f;       // 0.5 when it drops an octave
    float  breath = 0.0f;
    float  tilt = 0.0f;
    SVF f1a, f1b, f2a, f2b, f3a, f3b;
    juce::Random rng;
};

//==============================================================================
//  WHISTLE BANK
//
//  The hatchlings' voices. Each whistle is a short breathy tone at whatever
//  pitch the colony asked for. Triggers arrive from the message thread through
//  a single-producer queue; playback is allocation-free on the audio thread.
//==============================================================================

class WhistleBank
{
public:
    static constexpr int kVoices = 24;
    static constexpr int kQueueSize = 96;

    enum Flavour { whistle = 0, deathOscillation = 1, sparkle = 2, falling = 3,
                   pop = 4, inversePop = 5, rising = 6, whizz = 7 };

    void prepare (double sampleRate);
    /** Fresh seed from the system generator, so two instances never march in step. */
    void reseed() { rng.setSeed (juce::Random::getSystemRandom().nextInt64()); }
    /** delaySeconds staggers a burst so a handful of tones scatter instead of stacking. */
    void trigger (float pitchHz, int flavour = 0, float delaySeconds = 0.0f, float hue = -1.0f);
    void render (juce::AudioBuffer<float>& buffer, int numSamples, float gain);   // audio thread
    void reset();
    float getLevel() const noexcept { return lastLevel.load (std::memory_order_relaxed); }

private:
    struct Voice
    {
        bool  active = false;
        double phase = 0.0;
        float hz = 1000.0f;
        double posSec = 0.0, lenSec = 0.6;
        float vibPhase = 0.0f, vibRate = 5.0f, vibDepth = 0.012f;
        float bendSemis = 0.0f;
        float pan = 0.0f;
        float breath = 0.05f;
        int   flavour = 0;
        double delaySec = 0.0;
        float hue = -1.0f;
        int   partials = 2;
        float decay = 24.0f;
    };

    struct Request { float hz = 1000.0f; int flavour = 0; float delaySec = 0.0f; float hue = -1.0f; };

    std::array<Voice, kVoices> voices;
    std::array<Request, kQueueSize> queue {};
    std::atomic<int> writeIdx { 0 }, readIdx { 0 };
    std::atomic<float> lastLevel { 0.0f };
    double sr = 44100.0;
    juce::Random rng;
};
