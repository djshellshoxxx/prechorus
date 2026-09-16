#include "Talkbox.h"

namespace
{
    // F1 / F2 / F3 centres for a spread of vowels. Picking freely between these
    // and gliding through them is what makes the cry read as speech.
    struct Vowel { float f1, f2, f3; float openness; };

    const Vowel kVowels[] =
    {
        { 730.0f, 1090.0f, 2440.0f, 1.00f },   // "ah"
        { 660.0f, 1720.0f, 2410.0f, 0.95f },   // "eh"
        { 530.0f, 1840.0f, 2480.0f, 0.88f },   // "ay"
        { 270.0f, 2290.0f, 3010.0f, 0.72f },   // "ee"
        { 570.0f,  840.0f, 2410.0f, 0.92f },   // "oh"
        { 300.0f,  870.0f, 2240.0f, 0.70f },   // "oo"
        { 490.0f, 1350.0f, 1690.0f, 0.86f },   // "er"
        { 500.0f, 1500.0f, 2500.0f, 0.80f },   // neutral
    };
    constexpr int kNumVowels = (int) (sizeof (kVowels) / sizeof (kVowels[0]));

    // "food and shelter", written as vowels and the consonants around them.
    // The words stay put; the voice saying them is rolled fresh every time.
    struct PhraseStep { int vowel; float dur; float fricative; float burst; float pitchStep; };
    const PhraseStep kFoodAndShelter[] =
    {
        { 5, 0.26f, 0.62f, 0.10f,  0.0f },   // "foo"  - f + oo
        { 5, 0.10f, 0.05f, 0.85f, -1.0f },   //   "d"  - closing plosive
        { 0, 0.15f, 0.10f, 0.30f,  2.0f },   // "and"
        { 1, 0.22f, 0.70f, 0.15f,  3.0f },   // "shel" - sh + eh
        { 6, 0.34f, 0.12f, 0.45f,  2.0f },   // "ter"  - t + er
    };
    constexpr int kFoodAndShelterLen = (int) (sizeof (kFoodAndShelter) / sizeof (kFoodAndShelter[0]));

    // "are you having fun - I'm hungry"
    const PhraseStep kHavingFun[] =
    {
        { 0, 0.14f, 0.08f, 0.15f,  0.0f },   // "are"
        { 3, 0.12f, 0.05f, 0.10f,  2.0f },   // "you"
        { 0, 0.13f, 0.20f, 0.55f,  1.0f },   // "ha"
        { 7, 0.11f, 0.30f, 0.25f, -1.0f },   // "ving"
        { 4, 0.26f, 0.45f, 0.35f,  4.0f },   // "fun"  - rising question
        { 3, 0.16f, 0.05f, 0.10f, -3.0f },   // "I"
        { 0, 0.13f, 0.18f, 0.60f,  1.0f },   // "hun"
        { 6, 0.28f, 0.25f, 0.40f, -2.0f },   // "gry"
    };
    constexpr int kHavingFunLen = (int) (sizeof (kHavingFun) / sizeof (kHavingFun[0]));

    // "help me tie my shoes"
    const PhraseStep kTieMyShoes[] =
    {
        { 1, 0.17f, 0.28f, 0.70f,  0.0f },   // "help"
        { 3, 0.12f, 0.05f, 0.30f,  1.0f },   // "me"
        { 0, 0.13f, 0.10f, 0.65f,  2.0f },   // "ti-"
        { 3, 0.10f, 0.05f, 0.08f,  1.0f },   //   "-e"
        { 0, 0.11f, 0.08f, 0.20f, -1.0f },   // "m-"
        { 3, 0.09f, 0.05f, 0.08f,  1.0f },   //   "-y"
        { 5, 0.32f, 0.62f, 0.22f,  3.0f },   // "shoes"
    };
    constexpr int kTieMyShoesLen = (int) (sizeof (kTieMyShoes) / sizeof (kTieMyShoes[0]));

    // "ayuda - socorro, por favor". Spanish is syllable-timed, so the durations
    // sit far more evenly than the English phrases above - which is most of what
    // makes it read as Spanish rather than as the same voice in another mood.
    const PhraseStep kSpanishPlea[] =
    {
        { 0, 0.15f, 0.05f, 0.25f,  0.0f },   // "a"
        { 5, 0.14f, 0.05f, 0.15f,  2.0f },   // "yu"
        { 0, 0.17f, 0.05f, 0.30f, -1.0f },   // "da"
        { 4, 0.14f, 0.38f, 0.20f,  2.0f },   // "so"
        { 4, 0.14f, 0.05f, 0.35f,  1.0f },   // "co"
        { 4, 0.19f, 0.12f, 0.65f, -2.0f },   // "rro"  - rolled r
        { 4, 0.15f, 0.05f, 0.30f,  1.0f },   // "por"
        { 0, 0.14f, 0.42f, 0.25f,  1.0f },   // "fa"
        { 4, 0.31f, 0.10f, 0.35f,  3.0f },   // "vor"  - rising
    };
    constexpr int kSpanishPleaLen = (int) (sizeof (kSpanishPlea) / sizeof (kSpanishPlea[0]));

    // "au secours - aidez-moi, s'il vous plait". French leans on nasal, rounded
    // vowels and ends phrases on a rise, so it sits apart from the Spanish one.
    const PhraseStep kFrenchPlea[] =
    {
        { 4, 0.16f, 0.05f, 0.18f,  0.0f },   // "au"
        { 7, 0.13f, 0.45f, 0.20f,  1.0f },   // "se"
        { 5, 0.26f, 0.15f, 0.30f,  3.0f },   // "cours"
        { 2, 0.15f, 0.05f, 0.25f, -2.0f },   // "ai"
        { 2, 0.13f, 0.20f, 0.30f,  1.0f },   // "dez"
        { 3, 0.20f, 0.05f, 0.20f,  2.0f },   // "moi"
        { 3, 0.11f, 0.35f, 0.15f, -1.0f },   // "s'il"
        { 5, 0.14f, 0.10f, 0.25f,  1.0f },   // "vous"
        { 2, 0.30f, 0.20f, 0.35f,  4.0f },   // "plait" - rising
    };
    constexpr int kFrenchPleaLen = (int) (sizeof (kFrenchPlea) / sizeof (kFrenchPlea[0]));

    // "ranch and mayo in my hair"
    const PhraseStep kRanchAndMayo[] =
    {
        { 0, 0.19f, 0.28f, 0.50f,  0.0f },   // "ranch"
        { 0, 0.12f, 0.10f, 0.35f,  1.0f },   // "and"
        { 2, 0.14f, 0.05f, 0.42f,  2.0f },   // "may"
        { 4, 0.17f, 0.05f, 0.10f, -1.0f },   //    "-o"
        { 3, 0.11f, 0.08f, 0.28f,  1.0f },   // "in"
        { 0, 0.10f, 0.05f, 0.20f, -1.0f },   // "m-"
        { 3, 0.09f, 0.05f, 0.08f,  1.0f },   //   "-y"
        { 1, 0.16f, 0.32f, 0.38f,  1.0f },   // "hai-"
        { 6, 0.29f, 0.08f, 0.20f,  3.0f },   //    "-r"
    };
    constexpr int kRanchAndMayoLen = (int) (sizeof (kRanchAndMayo) / sizeof (kRanchAndMayo[0]));

    // Daft things it comes out with. One of these, picked at random.
    const PhraseStep kFunnyToast[] =        // "my toast is haunted"
    {
        { 0, 0.11f, 0.05f, 0.25f,  0.0f },   // "my"
        { 3, 0.09f, 0.05f, 0.08f,  1.0f },
        { 4, 0.22f, 0.30f, 0.55f,  2.0f },   // "toast"
        { 3, 0.11f, 0.35f, 0.20f, -1.0f },   // "is"
        { 4, 0.15f, 0.20f, 0.45f,  2.0f },   // "haun-"
        { 1, 0.26f, 0.15f, 0.50f, -2.0f },   //    "-ted"
    };
    const PhraseStep kFunnyCat[] =          // "the cat owes me money"
    {
        { 7, 0.10f, 0.25f, 0.15f,  0.0f },   // "the"
        { 0, 0.16f, 0.10f, 0.60f,  2.0f },   // "cat"
        { 4, 0.20f, 0.08f, 0.20f,  1.0f },   // "owes"
        { 3, 0.12f, 0.05f, 0.25f, -1.0f },   // "me"
        { 0, 0.14f, 0.15f, 0.40f,  2.0f },   // "mon-"
        { 3, 0.24f, 0.05f, 0.10f, -3.0f },   //    "-ey"
    };
    const PhraseStep kFunnyLegs[] =         // "i left my legs in the car"
    {
        { 3, 0.12f, 0.05f, 0.15f,  0.0f },   // "i"
        { 1, 0.15f, 0.20f, 0.50f,  1.0f },   // "left"
        { 0, 0.10f, 0.05f, 0.25f, -1.0f },   // "my"
        { 1, 0.16f, 0.15f, 0.45f,  2.0f },   // "legs"
        { 3, 0.10f, 0.08f, 0.25f,  1.0f },   // "in"
        { 7, 0.09f, 0.25f, 0.12f, -1.0f },   // "the"
        { 0, 0.28f, 0.10f, 0.55f, -2.0f },   // "car"
    };

    struct Phrase { const PhraseStep* steps; int len; };
    const Phrase kFunnyPhrases[] =
    {
        { kFunnyToast, (int) (sizeof (kFunnyToast) / sizeof (kFunnyToast[0])) },
        { kFunnyCat,   (int) (sizeof (kFunnyCat)   / sizeof (kFunnyCat[0])) },
        { kFunnyLegs,  (int) (sizeof (kFunnyLegs)  / sizeof (kFunnyLegs[0])) },
    };
    constexpr int kNumFunnyPhrases = (int) (sizeof (kFunnyPhrases) / sizeof (kFunnyPhrases[0]));

    inline float svfCoeff (float freqHz, double sampleRate)
    {
        const float f = 2.0f * std::sin (juce::MathConstants<float>::pi
                                         * juce::jlimit (40.0f, (float) (sampleRate * 0.22), freqHz)
                                         / (float) sampleRate);
        return juce::jlimit (0.0f, 1.4f, f);
    }
}

void TalkboxVoice::prepare (double sampleRate)
{
    sr = sampleRate > 1000.0 ? sampleRate : 44100.0;
    stop();
}

void TalkboxVoice::stop() noexcept
{
    active.store (false, std::memory_order_release);
    posSec = 0.0;
    carrierPhase = 0.0;
    lastLevel.store (0.0f, std::memory_order_relaxed);
    f1a.reset(); f1b.reset(); f2a.reset(); f2b.reset(); f3a.reset(); f3b.reset();
}

float TalkboxVoice::getProgress() const noexcept
{
    if (totalSeconds <= 0.0) return 0.0f;
    return juce::jlimit (0.0f, 1.0f, (float) (posSec / totalSeconds));
}

void TalkboxVoice::planUtterance (Mode mode)
{
    if (isActive()) return;

    const bool urgentBabble = (mode == Mode::urgentBabble);

    rng.setSeed (juce::Random::getSystemRandom().nextInt64());

    // One invocation in five comes out a full octave below where it usually sits.
    pitchScale = rng.nextInt (5) == 0 ? 0.5f : 1.0f;

    // A throat: shifts every formant together, so one cry is a child and the next
    // is an adult, without changing anything else about the utterance.
    formantScale = 0.78f + rng.nextFloat() * 0.52f;
    breath = 0.04f + rng.nextFloat() * 0.22f;
    tilt = rng.nextFloat();
    vibRate = 3.6f + rng.nextFloat() * 3.6f;
    vibDepth = 0.008f + rng.nextFloat() * 0.035f;

    // A call for help is short. A babble runs on and on, quickly, and sounds
    // like someone shouting sentences at you in a language you almost know.
    const PhraseStep* phrase = nullptr;
    int phraseLen = 0;
    if (mode == Mode::foodAndShelter) { phrase = kFoodAndShelter; phraseLen = kFoodAndShelterLen; }
    else if (mode == Mode::havingFun) { phrase = kHavingFun;      phraseLen = kHavingFunLen; }
    else if (mode == Mode::tieMyShoes){ phrase = kTieMyShoes;     phraseLen = kTieMyShoesLen; }
    else if (mode == Mode::spanishPlea){ phrase = kSpanishPlea;    phraseLen = kSpanishPleaLen; }
    else if (mode == Mode::frenchPlea) { phrase = kFrenchPlea;     phraseLen = kFrenchPleaLen; }
    else if (mode == Mode::ranchAndMayo){ phrase = kRanchAndMayo;  phraseLen = kRanchAndMayoLen; }
    else if (mode == Mode::somethingFunny)
    {
        const auto& pick = kFunnyPhrases[rng.nextInt (kNumFunnyPhrases)];
        phrase = pick.steps;
        phraseLen = pick.len;
    }

    if (phrase != nullptr)
    {
        // Same words, new throat: pitch, pace and formant scale all rolled fresh,
        // so it is recognisably the same plea in a voice you have not heard.
        const float pace = 0.82f + rng.nextFloat() * 0.5f;
        float pitch = 150.0f + rng.nextFloat() * 130.0f;

        numSyllables = juce::jmin (kMaxSyllables, phraseLen);
        totalSeconds = 0.0;

        for (int i = 0; i < numSyllables; ++i)
        {
            const auto& step = phrase[i];
            const auto& v = kVowels[juce::jlimit (0, kNumVowels - 1, step.vowel)];
            auto& syl = plan[(size_t) i];

            syl.f1 = v.f1 * formantScale;
            syl.f2 = v.f2 * formantScale;
            syl.f3 = v.f3 * formantScale;
            syl.openness = v.openness * (0.85f + rng.nextFloat() * 0.3f);
            syl.noiseAmt = step.fricative;
            syl.onsetBurst = step.burst;
            syl.durSec = step.dur * pace * (0.9f + rng.nextFloat() * 0.25f);

            syl.pitchHz = pitch;
            pitch = juce::jlimit (95.0f, 520.0f,
                                  pitch * std::pow (2.0f, (step.pitchStep + rng.nextFloat() - 0.5f) / 12.0f));
            // A plea rises on the last word.
            syl.endPitchHz = (i == numSyllables - 1)
                                 ? juce::jlimit (95.0f, 560.0f, pitch * std::pow (2.0f, (2.0f + rng.nextFloat() * 3.0f) / 12.0f))
                                 : pitch;

            totalSeconds += syl.durSec;
        }

        posSec = 0.0;
        carrierPhase = 0.0;
        vibPhase = 0.0f;
        f1a.reset(); f1b.reset(); f2a.reset(); f2b.reset(); f3a.reset(); f3b.reset();
        active.store (true, std::memory_order_release);
        return;
    }

    // A random outburst is somewhere between a plea and a full babble.
    const bool outburst = (mode == Mode::randomOutburst);
    numSyllables = urgentBabble ? (10 + rng.nextInt (kMaxSyllables - 10))
                 : outburst     ? (4 + rng.nextInt (9))
                                : (2 + rng.nextInt (5));
    const float basePitch = urgentBabble ? (168.0f + rng.nextFloat() * 170.0f)
                                         : (132.0f + rng.nextFloat() * 148.0f);
    if (urgentBabble)
    {
        vibRate = 5.0f + rng.nextFloat() * 4.0f;
        vibDepth = 0.014f + rng.nextFloat() * 0.03f;
    }

    // Calls for help climb. Build a contour that mostly rises, with the last
    // syllable held and bent further up - a plea, not a statement.
    float pitch = basePitch;
    totalSeconds = 0.0;

    for (int i = 0; i < numSyllables; ++i)
    {
        auto& s = plan[(size_t) i];
        const bool isLast = (i == numSyllables - 1);
        const auto& v = kVowels[rng.nextInt (kNumVowels)];

        s.f1 = v.f1 * formantScale;
        s.f2 = v.f2 * formantScale;
        s.f3 = v.f3 * formantScale;
        s.openness = v.openness * (0.7f + rng.nextFloat() * 0.45f);

        // Babble keeps hopping about within a phrase; a plea just climbs.
        const float interval = isLast ? (2.0f + rng.nextFloat() * 5.0f)
                             : (urgentBabble ? ((rng.nextFloat() * 11.0f) - 5.0f)
                                             : ((rng.nextFloat() * 6.0f) - 2.0f));
        s.pitchHz = pitch;
        pitch = juce::jlimit (90.0f, 520.0f, pitch * std::pow (2.0f, interval / 12.0f));
        s.endPitchHz = isLast ? juce::jlimit (90.0f, 560.0f, pitch * std::pow (2.0f, (1.0f + rng.nextFloat() * 3.0f) / 12.0f))
                              : pitch;

        if (urgentBabble)
            s.durSec = isLast ? (0.16f + rng.nextFloat() * 0.26f)
                              : (0.055f + rng.nextFloat() * 0.105f);   // fast, clipped syllables
        else
            s.durSec = isLast ? (0.28f + rng.nextFloat() * 0.45f)
                              : (0.09f + rng.nextFloat() * 0.19f);

        // Consonants: a noise bed on some syllables, a transient on others.
        // Urgent speech is far more consonant-heavy, which is what sells it.
        const float fricChance = urgentBabble ? 0.55f : 0.35f;
        const float burstChance = urgentBabble ? 0.8f : 0.45f;
        s.noiseAmt = rng.nextFloat() < fricChance ? (0.15f + rng.nextFloat() * 0.5f) : 0.0f;
        s.onsetBurst = rng.nextFloat() < burstChance ? (0.35f + rng.nextFloat() * 0.75f) : 0.0f;

        totalSeconds += s.durSec;
    }

    posSec = 0.0;
    carrierPhase = 0.0;
    vibPhase = 0.0f;
    f1a.reset(); f1b.reset(); f2a.reset(); f2b.reset(); f3a.reset(); f3b.reset();

    active.store (true, std::memory_order_release);
}

void TalkboxVoice::render (juce::AudioBuffer<float>& buffer, int numSamples, float gain)
{
    if (! isActive() || numSamples <= 0) return;

    const int chans = juce::jmin (buffer.getNumChannels(), 2);
    if (chans <= 0) return;

    const double dt = 1.0 / sr;
    float peak = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        if (posSec >= totalSeconds) { stop(); break; }

        // Locate the current syllable and how far through it we are.
        double acc = 0.0;
        int syl = 0;
        for (; syl < numSyllables; ++syl)
        {
            if (posSec < acc + plan[(size_t) syl].durSec) break;
            acc += plan[(size_t) syl].durSec;
        }
        if (syl >= numSyllables) { stop(); break; }

        const auto& s = plan[(size_t) syl];
        const float sylPos = (float) juce::jlimit (0.0, 1.0, (posSec - acc) / juce::jmax (1.0e-4, (double) s.durSec));

        // Glide the formants in from the previous syllable over the first third,
        // which is what turns separate tones into connected speech.
        float tf1 = s.f1, tf2 = s.f2, tf3 = s.f3;
        if (syl > 0 && sylPos < 0.34f)
        {
            const auto& prev = plan[(size_t) (syl - 1)];
            const float m = sylPos / 0.34f;
            tf1 = prev.f1 + (s.f1 - prev.f1) * m;
            tf2 = prev.f2 + (s.f2 - prev.f2) * m;
            tf3 = prev.f3 + (s.f3 - prev.f3) * m;
        }

        // Carrier: a glottal-ish pulse whose pitch bends across the syllable.
        const float pitchHz = (s.pitchHz + (s.endPitchHz - s.pitchHz) * sylPos) * pitchScale;
        vibPhase += (float) (juce::MathConstants<double>::twoPi * vibRate * dt);
        const float vib = 1.0f + std::sin (vibPhase) * vibDepth;

        carrierPhase += juce::MathConstants<double>::twoPi * (double) (pitchHz * vib) * dt;
        if (carrierPhase > juce::MathConstants<double>::twoPi) carrierPhase -= juce::MathConstants<double>::twoPi;

        const float ph = (float) (carrierPhase / juce::MathConstants<double>::twoPi);
        // Asymmetric pulse: rich in harmonics so the formants have something to bite on.
        const float pulse = std::pow (juce::jmax (0.0f, std::sin ((float) carrierPhase)), 2.0f)
                          - 0.5f * (ph < 0.5f ? 1.0f : -1.0f) * 0.35f;

        const float noise = rng.nextFloat() * 2.0f - 1.0f;
        const float excitation = pulse * (1.0f - s.noiseAmt) + noise * (s.noiseAmt + breath);

        // Plosive transient at the syllable edge.
        float burst = 0.0f;
        if (s.onsetBurst > 0.0f && sylPos < 0.035f)
            burst = noise * s.onsetBurst * (1.0f - sylPos / 0.035f);

        const float in = excitation + burst;

        // Three formant bands, two poles each for a tighter, more vocal resonance.
        const float c1 = svfCoeff (tf1, sr), c2 = svfCoeff (tf2, sr), c3 = svfCoeff (tf3, sr);
        const float b1 = f1b.process (f1a.process (in, c1, 0.22f), c1, 0.22f);
        const float b2 = f2b.process (f2a.process (in, c2, 0.26f), c2, 0.26f);
        const float b3 = f3b.process (f3a.process (in, c3, 0.34f), c3, 0.34f);

        float voiced = b1 * 1.0f + b2 * (0.62f + tilt * 0.4f) + b3 * (0.24f + tilt * 0.3f);

        // Per-syllable envelope, plus a whole-utterance fade so it never clicks.
        const float syllableEnv = std::sin (juce::jlimit (0.0f, 1.0f, sylPos) * juce::MathConstants<float>::pi);
        const float utterance = (float) juce::jlimit (0.0, 1.0,
                                    juce::jmin (posSec / 0.03, (totalSeconds - posSec) / 0.12));

        voiced *= syllableEnv * s.openness * utterance * 3.2f;
        voiced = std::tanh (voiced);

        const float outL = voiced * gain;
        const float outR = voiced * gain;
        buffer.getWritePointer (0)[i] += outL;
        if (chans > 1) buffer.getWritePointer (1)[i] += outR;

        peak = juce::jmax (peak, std::abs (voiced));
        posSec += dt;
    }

    lastLevel.store (peak, std::memory_order_relaxed);
}

//==============================================================================
//  WhistleBank
//==============================================================================

void WhistleBank::prepare (double sampleRate)
{
    sr = sampleRate > 1000.0 ? sampleRate : 44100.0;
    reseed();
    reset();
}

void WhistleBank::reset()
{
    for (auto& v : voices) v.active = false;
    writeIdx.store (0, std::memory_order_relaxed);
    readIdx.store (0, std::memory_order_relaxed);
    lastLevel.store (0.0f, std::memory_order_relaxed);
}

void WhistleBank::trigger (float pitchHz, int flavour, float delaySeconds, float hue)
{
    const int w = writeIdx.load (std::memory_order_relaxed);
    const int next = (w + 1) % kQueueSize;
    if (next == readIdx.load (std::memory_order_acquire)) return;   // queue full: drop it

    queue[(size_t) w] = { juce::jlimit (30.0f, 14000.0f, pitchHz),
                          flavour,
                          juce::jlimit (0.0f, 4.0f, delaySeconds),
                          hue };
    writeIdx.store (next, std::memory_order_release);
}

void WhistleBank::render (juce::AudioBuffer<float>& buffer, int numSamples, float gain)
{
    // Pick up anything the colony queued since the last block.
    while (readIdx.load (std::memory_order_relaxed) != writeIdx.load (std::memory_order_acquire))
    {
        const int r = readIdx.load (std::memory_order_relaxed);
        const Request req = queue[(size_t) r];
        readIdx.store ((r + 1) % kQueueSize, std::memory_order_release);

        for (auto& v : voices)
        {
            if (v.active) continue;
            v.active = true;
            v.phase = 0.0;
            v.hz = req.hz;
            v.flavour = req.flavour;
            v.posSec = 0.0;
            v.delaySec = (double) req.delaySec;
            v.hue = req.hue;
            v.vibPhase = rng.nextFloat() * juce::MathConstants<float>::twoPi;
            v.pan = rng.nextFloat() * 1.6f - 0.8f;

            if (req.flavour == 7)
            {
                // A whizz: something leaving in a hurry, doppler and all.
                v.lenSec = 0.5 + rng.nextFloat() * 0.6;
                v.vibRate = 9.0f + rng.nextFloat() * 14.0f;
                v.vibDepth = 0.02f + rng.nextFloat() * 0.05f;
                v.bendSemis = -(14.0f + rng.nextFloat() * 26.0f);
                v.breath = 0.08f + rng.nextFloat() * 0.16f;
                v.pan = rng.nextBool() ? -0.9f : 0.9f;
            }
            else if (req.flavour == 6)
            {
                // The sound of an orb changing size: a long, slow climb.
                v.lenSec = 8.0 + rng.nextFloat() * 3.0;
                v.vibRate = 0.3f + rng.nextFloat() * 1.4f;
                v.vibDepth = 0.004f + rng.nextFloat() * 0.014f;
                v.bendSemis = 12.0f + rng.nextFloat() * 24.0f;
                v.breath = 0.01f + rng.nextFloat() * 0.03f;
            }
            else if (req.flavour == 4 || req.flavour == 5)
            {
                // A colour's pop. Hue picks the pitch, how many partials sit on
                // top of it and how fast it lets go - so every colour has its own
                // little sound, and the same colour always makes the same one.
                const float h = juce::jlimit (0.0f, 1.0f, req.hue < 0.0f ? 0.35f : req.hue);
                v.partials = 1 + (int) (h * 4.0f);
                v.decay = 16.0f + h * 46.0f;
                v.lenSec = 0.05 + (double) (1.0f - h) * 0.09;
                v.vibRate = 0.0f;
                v.vibDepth = 0.0f;
                v.breath = 0.004f + h * 0.02f;
                // A pop falls away; its inverse climbs into being.
                v.bendSemis = req.flavour == 4 ? -(2.0f + h * 5.0f) : (2.0f + h * 5.0f);
                v.pan = (h * 1.6f) - 0.8f;
            }
            else if (req.flavour == 3)
            {
                // A falling sound: one to three seconds of pitch sliding away,
                // starting wherever it likes and dropping at its own rate.
                v.lenSec = 1.0 + rng.nextFloat() * 2.0;
                v.vibRate = 0.5f + rng.nextFloat() * 4.0f;
                v.vibDepth = 0.003f + rng.nextFloat() * 0.02f;
                v.bendSemis = -(8.0f + rng.nextFloat() * 40.0f);      // random speed of fall
                v.breath = 0.01f + rng.nextFloat() * 0.06f;
                v.pan = rng.nextFloat() * 1.6f - 0.8f;
            }
            else if (req.flavour == 2)
            {
                // Water sparkle: a tiny bright ping that is gone almost at once.
                v.lenSec = 0.05 + rng.nextFloat() * 0.22;
                v.vibRate = 0.0f;
                v.vibDepth = 0.0f;
                v.bendSemis = rng.nextFloat() * 3.0f - 1.0f;
                v.breath = 0.0f;
                v.pan = rng.nextFloat() * 1.8f - 0.9f;
            }
            else if (req.flavour == 1)
            {
                // A death oscillation: longer, plainer, sagging away as it goes.
                v.lenSec = 0.9 + rng.nextFloat() * 2.2;
                v.vibRate = 0.8f + rng.nextFloat() * 3.0f;
                v.vibDepth = 0.002f + rng.nextFloat() * 0.012f;
                v.bendSemis = -(1.0f + rng.nextFloat() * 6.0f);
                v.breath = 0.01f + rng.nextFloat() * 0.04f;
            }
            else
            {
                v.lenSec = 0.22 + rng.nextFloat() * 0.75;
                v.vibRate = 3.5f + rng.nextFloat() * 5.5f;
                v.vibDepth = 0.004f + rng.nextFloat() * 0.022f;
                v.bendSemis = (rng.nextFloat() * 5.0f - 2.0f);
                v.breath = 0.02f + rng.nextFloat() * 0.09f;
            }
            break;
        }
    }

    const int chans = juce::jmin (buffer.getNumChannels(), 2);
    if (chans <= 0 || numSamples <= 0) return;

    const double dt = 1.0 / sr;
    float peak = 0.0f;

    for (auto& v : voices)
    {
        if (! v.active) continue;

        float* left = buffer.getWritePointer (0);
        float* right = chans > 1 ? buffer.getWritePointer (1) : nullptr;

        const float panL = std::cos ((v.pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
        const float panR = std::sin ((v.pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);

        for (int i = 0; i < numSamples; ++i)
        {
            if (v.delaySec > 0.0) { v.delaySec -= dt; continue; }   // staggered burst
            if (v.posSec >= v.lenSec) { v.active = false; break; }

            const float t = (float) (v.posSec / v.lenSec);
            v.vibPhase += (float) (juce::MathConstants<double>::twoPi * v.vibRate * dt);

            const float bend = std::pow (2.0f, (v.bendSemis * t) / 12.0f);
            const float vib = 1.0f + std::sin (v.vibPhase) * v.vibDepth;
            v.phase += juce::MathConstants<double>::twoPi * (double) (v.hz * bend * vib) * dt;
            if (v.phase > juce::MathConstants<double>::twoPi) v.phase -= juce::MathConstants<double>::twoPi;

            float tone, env;
            if (v.flavour == 7)
            {
                tone = std::sin ((float) v.phase) * 0.55f
                     + std::sin ((float) v.phase * 1.5f) * 0.25f
                     + (rng.nextFloat() * 2.0f - 1.0f) * v.breath;
                // Rushes in, tears past, gone.
                env = std::sin (juce::jlimit (0.0f, 1.0f, t) * juce::MathConstants<float>::pi);
                env *= env;
            }
            else if (v.flavour == 6)
            {
                tone = std::sin ((float) v.phase) * 0.7f
                     + std::sin ((float) v.phase * 2.0f) * 0.22f
                     + std::sin ((float) v.phase * 4.0f) * 0.08f
                     + (rng.nextFloat() * 2.0f - 1.0f) * v.breath;
                // Fades in, holds through the middle, fades out with the change.
                env = juce::jmin (1.0f, t * 5.0f) * juce::jmin (1.0f, (1.0f - t) * 5.0f) * 0.7f;
            }
            else if (v.flavour == 4 || v.flavour == 5)
            {
                tone = 0.0f;
                float norm = 0.0f;
                for (int h = 1; h <= v.partials; ++h)
                {
                    const float amp = 1.0f / (float) h;
                    tone += std::sin ((float) v.phase * (float) h) * amp;
                    norm += amp;
                }
                tone = tone / juce::jmax (0.001f, norm)
                     + (rng.nextFloat() * 2.0f - 1.0f) * v.breath;

                // The inverse pop is the pop played backwards: it swells, then stops.
                const float u = (v.flavour == 4) ? t : (1.0f - t);
                env = std::exp (-u * v.decay) * juce::jmin (1.0f, (v.flavour == 4 ? t : (1.0f - t)) * 400.0f + 0.05f);
            }
            else if (v.flavour == 3)
            {
                // Falling: a sine with a little grit, swelling in then away.
                tone = std::sin ((float) v.phase) * 0.78f
                     + std::sin ((float) v.phase * 2.0f) * 0.16f
                     + (rng.nextFloat() * 2.0f - 1.0f) * v.breath;
                env = juce::jmin (1.0f, t * 12.0f) * (1.0f - t) * (1.0f - t * 0.4f);
            }
            else if (v.flavour == 2)
            {
                // Sparkle: pure and glassy, struck and immediately decaying.
                tone = std::sin ((float) v.phase) * 0.8f
                     + std::sin ((float) v.phase * 3.0f) * 0.2f;
                env = std::exp (-t * 7.0f) * juce::jmin (1.0f, t * 200.0f);
            }
            else if (v.flavour == 1)
            {
                // Death oscillation: a fuller tone that rings on and decays away.
                tone = std::sin ((float) v.phase) * 0.72f
                     + std::sin ((float) v.phase * 2.0f) * 0.2f
                     + std::sin ((float) v.phase * 3.0f) * 0.1f
                     + (rng.nextFloat() * 2.0f - 1.0f) * v.breath;
                env = std::exp (-t * 3.2f) * juce::jmin (1.0f, t * 60.0f);
            }
            else
            {
                // Mostly a sine, with a touch of second harmonic and air.
                tone = std::sin ((float) v.phase) * 0.88f
                     + std::sin ((float) v.phase * 2.0f) * 0.12f
                     + (rng.nextFloat() * 2.0f - 1.0f) * v.breath;
                const float bell = std::sin (juce::jlimit (0.0f, 1.0f, t) * juce::MathConstants<float>::pi);
                env = bell * bell;
            }

            const float s = tone * env * gain;

            left[i] += s * panL;
            if (right != nullptr) right[i] += s * panR;

            peak = juce::jmax (peak, std::abs (s));
            v.posSec += dt;
        }
    }

    lastLevel.store (peak, std::memory_order_relaxed);
}
