#pragma once
#include <JuceHeader.h>

//==============================================================================
//  COLONY
//
//  The living layer behind the constellation display. Every orb is one audio
//  element (one swarm voice); the oscillator bank is the modulation that moves
//  them. Clicks breed orbs, gravity and enzyme change the pace, gamma mutates
//  pitch and arms a green orb, and a green orb meeting a red one detonates.
//  A middle-clicked orb falls pregnant, lays eggs, and the hatchlings come out
//  in colours of their own and whistle when they feel like it.
//
//  Lives entirely on the message thread: the processor's 30 Hz timer steps it
//  and re-renders the swell whenever an event actually changes the sound.
//==============================================================================

class Colony
{
public:
    static constexpr int kMaxOrbs      = 128;    // a big clutch needs room to hatch into
    static constexpr int kMaxEggs      = 96;
    static constexpr int kIntrinsicOsc = 3;   // orbit rate, character LFO 1, character LFO 2
    static constexpr int kMaxOsc       = 12;

    static constexpr int kLeftClicksToSplit      = 4;
    static constexpr int kRightClicksToSpawn     = 3;
    static constexpr int kMiddleClicksToImpregnate = 2;
    static constexpr int kOrbsPerSplit           = 3;
    static constexpr float kGreenSeconds         = 8.0f;
    static constexpr float kHatchlingMinLife     = 300.0f;    // 5 minutes
    static constexpr float kHatchlingMaxLife     = 1800.0f;   // 30 minutes
    static constexpr float kDeathToneMinHz       = 40.0f;
    static constexpr float kDeathToneMaxHz       = 10000.0f;
    static constexpr int   kWaterToDrown          = 20;       // pours until it is barely audible
    static constexpr int   kDestructionsPerReversal = 20;     // every 20th destruction flips time
    static constexpr int   kGreenHitsToReverse    = 2;        // green touching a plain orb twice
    static constexpr float kReverseSeconds        = 3.5f;     // slow, stop, then back up to speed
    static constexpr float kFastDragSpeed         = 2.4f;     // normalised units / second
    static constexpr float kSlowDragSpeed         = 0.9f;

    // Clutch sizes, and what a clutch of exactly that size does to the pace
    static constexpr int   kClutchHalfSpeed       = 20;       // slows everything to half
    static constexpr int   kClutchDoubleSpeed     = 30;       // doubles everything
    static constexpr int   kClutchWildcard        = 10;       // 3x slower .. 4x faster, at random
    static constexpr float kClutchGlideSeconds    = 6.0f;     // the change comes on slowly

    // Things that happen on their own
    static constexpr float kAsteroidPeriod        = 1200.0f;  // a roll every 20 minutes
    static constexpr int   kAsteroidOdds          = 100;      // ...and a 1 in 100 chance
    static constexpr int   kAsteroidOrbs          = 100;      // what it leaves behind
    static constexpr float kSpawnPeriod           = 300.0f;   // a roll every 5 minutes
    static constexpr int   kSpawnOdds             = 4;        // ...and a 1 in 4 chance
    static constexpr float kResizePeriod          = 2400.0f;  // a roll every 40 minutes
    static constexpr int   kResizeOdds            = 40;       // ...and a 1 in 40 chance
    static constexpr float kResizeMin             = 0.1f;     // down to a tenth
    static constexpr float kResizeMax             = 4.0f;     // up to four times
    static constexpr float kResizeSeconds         = 9.0f;     // and it takes its time

    // Leaning on a button: more than five presses inside ten seconds is an overdose
    static constexpr int   kOverdosePresses       = 5;
    static constexpr float kOverdoseWindow        = 10.0f;
    static constexpr float kOverdoseWearOff       = 1800.0f;  // 30 minutes back to normal
    static constexpr float kStretchMin            = 0.5f;     // half as slow...
    static constexpr float kStretchMax            = 3.0f;     // ...to three times as slow

    // A passing visitor: drops in, stays a few minutes, whizzes back out
    static constexpr float kVisitorPeriod         = 240.0f;   // a roll every 4 minutes
    static constexpr int   kVisitorOdds           = 4;        // ...and a 1 in 4 chance
    static constexpr float kVisitorMinStay        = 240.0f;   // 4 minutes
    static constexpr float kVisitorMaxStay        = 300.0f;   // 5 minutes
    static constexpr float kVisitorExitSeconds    = 1.6f;     // how long the exit takes

    // Hues are how a coloured orb says what it is. Blue and pink do not mix.
    static constexpr float kBlueHue               = 0.58f;
    static constexpr float kPinkHue               = 0.90f;
    static constexpr float kYellowHue             = 0.15f;
    // Clicking a yellow orb starts a fuse, and it goes off more than once
    static constexpr float kYellowFuseMin         = 40.0f;    // 40 seconds
    static constexpr float kYellowFuseMax         = 240.0f;   // 4 minutes
    static constexpr float kYellowRepeatMin       = 180.0f;   // 3 minutes
    static constexpr float kYellowRepeatMax       = 540.0f;   // 9 minutes
    static constexpr float kHueTolerance          = 0.07f;

    // The voxbox losing its composure
    static constexpr float kBabblePeriod          = 240.0f;   // a roll every 4 minutes
    static constexpr int   kBabbleOdds            = 30;       // ...and a 1 in 30 chance
    static constexpr float kBabbleSeconds         = 30.0f;    // how long it shakes everything

    enum class Kind { normal, red, green };

    struct Orb
    {
        float angle = 0.0f;         // orbital position
        float radius = 0.6f;        // normalised 0..1
        float angularVel = 0.4f;    // radians / second
        float wobblePhase = 0.0f;
        float wobbleRate = 1.0f;
        float wobbleDepth = 0.04f;
        float x = 0.0f, y = 0.0f;   // normalised -1..1, recomputed every step
        float size = 1.0f;
        float birth = 0.0f;         // 0..1 pop-in animation
        Kind  kind = Kind::normal;
        float greenSeconds = 0.0f;
        bool  alive = true;

        // Green-orb contact tally: two brushes with a plain orb turns time around
        int   greenHits = 0;
        float hitCooldown = 0.0f;

        // Breeding
        bool  pregnant = false;
        int   eggsRemaining = 0;
        int   clutchSize = 0;
        float eggCooldown = 0.0f;

        // Hatchlings carry their own colour, whistle on their own schedule, and
        // live somewhere between five and thirty minutes before they die off.
        bool  hatchling = false;
        float hue = 0.0f;
        float whistleCooldown = 0.0f;
        // A visitor is only passing through, and leaves in a hurry
        // A clicked yellow orb is counting down to something
        float fuseSeconds = 0.0f;       // > 0 while a fuse is burning
        bool  fuseRepeat = false;       // true once it has gone off and is waiting to repeat

        bool  visitor = false;
        float visitSeconds = 0.0f;      // time left before it is expelled
        float exitT = 0.0f;             // 0 = still here, > 0 = on its way out

        // A slow swell or shrink, with a rising tone riding along with it
        float sizeTarget = 0.0f;        // 0 = not resizing
        float sizeFrom = 1.0f;
        float resizeT = 0.0f;

        float lifeSeconds = 0.0f;       // remaining, 0 = immortal (non-hatchlings)
        float lifeTotal = 0.0f;
        float fade = 1.0f;              // 1 alive, drops to 0 as it dies away

        // Ghost trail, filled while the distress call is echoing
        static constexpr int kTrail = 10;
        std::array<juce::Point<float>, kTrail> trail {};
        int trailWrite = 0;
        int trailCount = 0;
    };

    struct Egg
    {
        bool  alive = false;
        float angle = 0.0f, radius = 0.6f, angularVel = 0.2f;
        float x = 0.0f, y = 0.0f;
        float hatchIn = 6.0f;
        float wobblePhase = 0.0f;
        float hue = 0.0f;
    };

    struct Oscillator
    {
        bool  active = false;
        bool  intrinsic = false;
        float ttlSeconds = 0.0f;    // 0 = permanent; anything else expires
        float rateHz = 1.0f;
        float depthSemis = 0.0f;
        int   shape = 0;            // 0 sine, 1 triangle, 2 square, 3 ramp
        float phase = 0.0f;
        float rateScale = 1.0f;     // gravity drags this one down
    };

    /** A sound the colony asked for this step. */
    struct Tone
    {
        float hz = 1000.0f;
        int   flavour = 0;              // see WhistleBank::Flavour
        float hue = -1.0f;              // >= 0 when the sound belongs to a coloured orb
    };

    /** What a single simulation step produced. */
    struct StepResult
    {
        bool audioChanged = false;      // element count changed - the swell must re-render
        bool detonated = false;
        int  died = 0;                  // hatchlings that reached the end of their life
        bool reversalStarted = false;
        bool asteroidHit = false;
        bool babbleStarted = false;
        bool pleaForFoodAndShelter = false;
        int  blueOrbsLost = 0;          // each one makes the voxbox pipe up
        int  yellowFusesFired = 0;      // urgent outbursts, with delay on them
        int  numTones = 0;
        std::array<Tone, 24> tones {};

        void addTone (float hz, int flavour, float hue = -1.0f)
        {
            if (numTones < (int) tones.size()) tones[(size_t) numTones++] = { hz, flavour, hue };
        }
    };

    Colony();

    //== lifecycle ============================================================
    void resetAll (int voiceCount);
    void syncToVoiceCount (int voiceCount);
    StepResult step (float dtSeconds);

    //== interaction ==========================================================
    bool registerLeftClick();       // 4 clicks -> 3 orbs split and replicate
    bool registerRightClick();      // 3 clicks -> a red orb is born
    bool registerMiddleClick();     // 2 clicks -> an orb falls pregnant

    bool addGravity();
    bool releaseGravity();
    bool addEnzyme();
    bool addGamma();
    bool addWater();

    //== dragging =============================================================
    /** Clicking a yellow orb lights its fuse. Returns true when one was lit. */
    bool clickAt (float nx, float ny);

    /** Grabs the orb nearest to this normalised point, if one is close enough. */
    bool beginDrag (float nx, float ny);
    /** Moves the held orb. Fast drags destroy, slow drags mutate. */
    StepResult dragTo (float nx, float ny, float dtSeconds);
    void endDrag();
    bool isDragging() const { return draggedOrb >= 0; }
    int  getDraggedOrb() const { return draggedOrb; }
    float getDragSpeed() const { return dragSpeed; }

    //== time direction =======================================================
    /** +1 forwards, 0 stopped, -1 fully reversed. Eases through the turn. */
    float getTimeDirection() const { return timeDirection; }
    bool  isReversing() const { return reverseActive; }
    bool  isReversed() const { return timeDirection < 0.0f; }
    void  beginReversal();

    //== score ================================================================
    double getScore() const { return score; }
    float  getFractalAmount() const { return fractalAmount; }
    float  getAsteroidFlash() const { return asteroidFlash; }

    //== overdose state =======================================================
    /** 0 = clean, 1 = fully washed out. Too much water, too fast. */
    float getWashout() const { return washout; }
    /** Playback stretch: > 1 slower and granular, < 1 faster. Wears off over 30 minutes. */
    float getTimeStretch() const;
    bool  isGranulating() const { return std::abs (getTimeStretch() - 1.0f) > 0.02f; }
    /** Seconds of babble-driven shaking left, and how hard it shakes. */
    float getBabbleSeconds() const { return babbleSeconds; }
    float getBabbleAmount() const { return juce::jlimit (0.0f, 1.0f, babbleSeconds / 4.0f); }

    /** Hues are how a coloured orb says what it is. Blue and pink do not mix. */
    static bool isHue (float hue, float target)
    {
        if (hue < 0.0f) return false;
        const float d = std::abs (hue - target);
        return juce::jmin (d, 1.0f - d) <= kHueTolerance;
    }
    static bool isTinted (const Orb& o) { return o.hatchling || o.visitor; }
    static bool isBlue (const Orb& o)   { return isTinted (o) && isHue (o.hue, kBlueHue); }
    static bool isPink (const Orb& o)   { return isTinted (o) && isHue (o.hue, kPinkHue); }
    static bool isYellow (const Orb& o) { return isTinted (o) && isHue (o.hue, kYellowHue); }
    /** Pitch a colour pops at. Same hue, same note, every time. */
    static float popHzForHue (float hue)
    {
        return 180.0f * std::pow (2.0f, juce::jlimit (0.0f, 1.0f, hue) * 4.2f);
    }
    int    getTotalDestructions() const { return totalDestructions; }

    /** Smears a trail behind every orb while the distress echo lasts. */
    void setGhosting (bool shouldGhost) { ghosting = shouldGhost; }

    /** Master switch for everything the colony does on its own - the asteroid,
        the visitors, the spontaneous spawns, the resizes and the babble. The
        buttons and the click gestures keep working either way. */
    void setAutonomyEnabled (bool shouldRun) { autonomous = shouldRun; }
    bool isAutonomyEnabled() const { return autonomous; }

    //== visual state =========================================================
    const std::array<Orb, kMaxOrbs>& getOrbs() const { return orbs; }
    const std::array<Egg, kMaxEggs>& getEggs() const { return eggs; }
    int   getLiveOrbCount() const;
    int   getLiveEggCount() const;
    int   getLeftClickProgress() const   { return leftClicks; }
    int   getRightClickProgress() const  { return rightClicks; }
    int   getMiddleClickProgress() const { return middleClicks; }
    float getExplosionFlash() const      { return explosionFlash; }
    int   getGravityDepth() const        { return (int) gravityStack.size(); }
    int   getEnzymeOscCount() const;
    int   getWaterCount() const          { return waterCount; }
    /** 0 = undiluted, 1 = drowned. Reached after kWaterToDrown pours. */
    float getWaterDilution() const
    {
        return juce::jlimit (0.0f, 1.0f, (float) waterCount / (float) kWaterToDrown);
    }
    float getGlobalRate() const          { return globalRate; }
    /** Pace multiplier a clutch left behind. Glides to its target over a few seconds. */
    float getClutchRate() const          { return clutchRate; }
    /** Everything the colony moves by: rate, clutch effect and time direction together. */
    float getFlow() const                { return globalRate * clutchRate * timeDirection; }
    juce::String getStatusLine() const;
    juce::Point<float> getExplosionCentre() const { return explosionCentre; }

    //== audio queries (read by render()) =====================================
    bool  isVoiceDead (int voiceIndex) const;
    float oscillatorPitchMod (int voiceIndex, float timeSeconds) const;
    float gammaPitchOffset (int voiceIndex, float timeSeconds) const;

    float globalRateScale() const { return globalRate; }
    float orbitRateScale() const  { return oscillators[0].rateScale * globalRate; }
    float lfo1RateScale() const   { return oscillators[1].rateScale * globalRate; }
    float lfo2RateScale() const   { return oscillators[2].rateScale * globalRate; }

    //== persistence ==========================================================
    juce::ValueTree toValueTree() const;
    void fromValueTree (const juce::ValueTree&);

private:
    struct Gamma
    {
        bool  active = false;
        float semis = 0.0f;
        float pace = 1.0f;
    };

    int   spawnOrb (Kind kind, const Orb* parent);
    void  recomputePositions();
    void  detonate (int orbIndex, StepResult& out);
    int   pickRandomLiveOrb (Kind wanted, bool anyKind);
    void  seedOrb (Orb& o, int index, const Orb* parent);
    void  layEgg (const Orb& mother);
    int   hatchEgg (const Egg& egg);
    void  pushTrail (Orb& o);
    void  registerDestruction (int count, StepResult& out);
    int   rollClutchSize();
    void  applyClutchOutcome (int size);
    void  triggerAsteroid (StepResult& out);
    void  triggerResize (StepResult& out);
    void  triggerBabble (StepResult& out);
    void  triggerVisitor (StepResult& out);
    bool  logPressAndCheck (std::vector<float>& log);
    void  addTemporaryOscillators (int count, float ttlSeconds);
    int   spawnColouredOrb (StepResult& out, bool announce);
    void  killOrb (int index, StepResult& out, bool withDeathTone);
    void  addScore (double points);
    int   nearestOrbTo (float nx, float ny, float maxDist) const;

    std::array<Orb, kMaxOrbs> orbs;
    std::array<Egg, kMaxEggs> eggs;
    std::array<Oscillator, kMaxOsc> oscillators;
    std::array<Gamma, kMaxOrbs> gammas;

    std::vector<int> gravityStack;
    int waterCount = 0;
    float globalRate = 1.0f;
    float clutchRate = 1.0f, clutchRateTarget = 1.0f;

    // Time direction
    float timeDirection = 1.0f;
    bool  reverseActive = false;
    float reverseT = 0.0f, reverseFrom = 1.0f, reverseTo = -1.0f;

    // Dragging
    int   draggedOrb = -1;
    float dragSpeed = 0.0f;
    float lastDragX = 0.0f, lastDragY = 0.0f;
    float destroyCooldown = 0.0f, mutateCooldown = 0.0f;
    float fractalAmount = 0.0f;

    // Score-keeping. It means nothing, but it keeps score.
    double score = 0.0;
    int totalDestructions = 0;
    std::vector<float> recentDestructions;
    float explosionFlash = 0.0f;
    juce::Point<float> explosionCentre { 0.0f, 0.0f };
    float elapsed = 0.0f;
    float trailClock = 0.0f;
    float asteroidClock = 0.0f, spawnClock = 0.0f, asteroidFlash = 0.0f, resizeClock = 0.0f;
    float babbleClock = 0.0f, babbleSeconds = 0.0f, visitorClock = 0.0f;
    int visitorCount = 0;

    // Overdose bookkeeping
    std::vector<float> waterPresses, radiatePresses, enzymePresses;
    float washout = 0.0f;
    float stretchTarget = 1.0f, stretchRemaining = 0.0f;
    bool  ghosting = false;
    bool  autonomous = true;
    int leftClicks = 0, rightClicks = 0, middleClicks = 0;
    juce::String lastEvent { "Colony idle" };

    juce::Random rng;
};
