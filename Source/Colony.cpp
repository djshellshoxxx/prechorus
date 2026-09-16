#include "Colony.h"

namespace
{
    constexpr float kGravityRateDrag   = 0.45f;   // a gravity well drags one oscillator down
    constexpr float kGravityGlobal     = 0.75f;   // ...and slows the whole rotation with it
    constexpr float kEnzymeGlobal      = 1.35f;   // enzyme winds everything up
    constexpr float kMinGlobalRate     = 0.12f;
    constexpr float kMaxGlobalRate     = 4.00f;
    constexpr float kGreenHuntStrength = 0.55f;   // how hard a green orb seeks the nearest red
    constexpr float kTrailInterval     = 0.045f;  // seconds between ghost-trail samples
    constexpr float kDragGrabRadius    = 0.22f;   // how near the pointer must be to grab an orb

    /** Slow down, stop, hang there a moment, then wind back up the other way. */
    float directionCurve (float t, float from, float to)
    {
        t = juce::jlimit (0.0f, 1.0f, t);
        auto smooth = [] (float k) { return k * k * (3.0f - 2.0f * k); };

        if (t < 0.40f) return from * (1.0f - smooth (t / 0.40f));    // decelerate to a stop
        if (t < 0.55f) return 0.0f;                                   // stopped
        return to * smooth ((t - 0.55f) / 0.45f);                     // wind back up, backwards
    }

    float oscShape (int shape, float phase)
    {
        const float t = phase - std::floor (phase);          // 0..1
        switch (shape)
        {
            case 1:  return 4.0f * std::abs (t - 0.5f) - 1.0f;                  // triangle
            case 2:  return t < 0.5f ? 1.0f : -1.0f;                            // square
            case 3:  return 2.0f * t - 1.0f;                                    // ramp
            default: return std::sin (t * juce::MathConstants<float>::twoPi);    // sine
        }
    }
}

Colony::Colony() : rng (juce::Random::getSystemRandom().nextInt64())
{
    resetAll (12);
}

//==============================================================================
//  Lifecycle
//==============================================================================

void Colony::seedOrb (Orb& o, int index, const Orb* parent)
{
    if (parent != nullptr)
    {
        o = *parent;
        o.angle += 0.22f + rng.nextFloat() * 0.5f;
        o.radius = juce::jlimit (0.22f, 0.98f, parent->radius + (rng.nextFloat() - 0.5f) * 0.22f);
        o.angularVel = parent->angularVel * (0.82f + rng.nextFloat() * 0.36f);
        o.size = juce::jlimit (0.6f, 1.4f, parent->size * 0.88f);
        o.pregnant = false;
        o.eggsRemaining = 0;
        o.clutchSize = 0;
        o.eggCooldown = 0.0f;
    }
    else
    {
        const float n = (float) index / (float) juce::jmax (1, kMaxOrbs - 1);
        o = {};
        o.angle = rng.nextFloat() * juce::MathConstants<float>::twoPi;
        o.radius = 0.32f + n * 0.5f + rng.nextFloat() * 0.14f;
        o.angularVel = (0.22f + rng.nextFloat() * 0.5f) * (rng.nextBool() ? 1.0f : -1.0f);
        o.size = 0.85f + rng.nextFloat() * 0.4f;
        o.kind = Kind::normal;
    }

    o.wobblePhase = rng.nextFloat() * juce::MathConstants<float>::twoPi;
    o.wobbleRate = 0.4f + rng.nextFloat() * 1.6f;
    o.wobbleDepth = 0.02f + rng.nextFloat() * 0.07f;
    o.greenSeconds = 0.0f;
    o.birth = 0.0f;
    o.alive = true;
    o.trailWrite = 0;
    o.trailCount = 0;
    o.fade = 1.0f;
    if (parent == nullptr)
    {
        o.hatchling = false;
        o.lifeSeconds = 0.0f;
        o.lifeTotal = 0.0f;
        o.hue = 0.0f;
    }
    o.visitor = false;
    o.visitSeconds = 0.0f;
    o.exitT = 0.0f;
    o.fuseSeconds = 0.0f;
    o.fuseRepeat = false;
}

void Colony::resetAll (int voiceCount)
{
    for (auto& g : gammas) g = {};
    for (auto& o : orbs) o.alive = false;
    for (auto& e : eggs) e.alive = false;

    for (int i = 0; i < kMaxOsc; ++i)
    {
        auto& osc = oscillators[(size_t) i];
        osc = {};
        osc.intrinsic = i < kIntrinsicOsc;
        osc.active = osc.intrinsic;
        osc.rateScale = 1.0f;
    }

    gravityStack.clear();
    waterCount = 0;
    globalRate = 1.0f;
    clutchRate = clutchRateTarget = 1.0f;
    timeDirection = 1.0f;
    reverseActive = false;
    reverseT = 0.0f;
    draggedOrb = -1;
    dragSpeed = 0.0f;
    destroyCooldown = mutateCooldown = 0.0f;
    fractalAmount = 0.0f;
    score = 0.0;
    totalDestructions = 0;
    recentDestructions.clear();
    explosionFlash = 0.0f;
    elapsed = 0.0f;
    trailClock = 0.0f;
    asteroidClock = spawnClock = asteroidFlash = resizeClock = 0.0f;
    babbleClock = babbleSeconds = visitorClock = 0.0f;
    visitorCount = 0;
    waterPresses.clear();
    radiatePresses.clear();
    enzymePresses.clear();
    washout = 0.0f;
    stretchTarget = 1.0f;
    stretchRemaining = 0.0f;
    ghosting = false;
    leftClicks = rightClicks = middleClicks = 0;
    lastEvent = "Colony reset";

    syncToVoiceCount (voiceCount);
}

void Colony::syncToVoiceCount (int voiceCount)
{
    const int wanted = juce::jlimit (1, juce::jmin (32, kMaxOrbs), voiceCount);
    int live = getLiveOrbCount();

    for (int i = 0; i < kMaxOrbs && live < wanted; ++i)
    {
        if (orbs[(size_t) i].alive) continue;
        seedOrb (orbs[(size_t) i], i, nullptr);
        gammas[(size_t) i] = {};
        ++live;
    }

    for (int i = kMaxOrbs - 1; i >= 0 && live > wanted; --i)
    {
        if (! orbs[(size_t) i].alive) continue;
        orbs[(size_t) i].alive = false;
        gammas[(size_t) i] = {};
        --live;
    }

    recomputePositions();
}

int Colony::getLiveOrbCount() const
{
    int n = 0;
    for (const auto& o : orbs) if (o.alive) ++n;
    return n;
}

int Colony::getLiveEggCount() const
{
    int n = 0;
    for (const auto& e : eggs) if (e.alive) ++n;
    return n;
}

int Colony::getEnzymeOscCount() const
{
    int n = 0;
    for (int i = kIntrinsicOsc; i < kMaxOsc; ++i)
        if (oscillators[(size_t) i].active) ++n;
    return n;
}

//==============================================================================
//  Simulation
//==============================================================================

void Colony::recomputePositions()
{
    for (auto& o : orbs)
    {
        if (! o.alive) continue;
        const float depth = o.wobbleDepth + getBabbleAmount() * 0.09f;
        const float r = juce::jlimit (0.1f, 1.0f, o.radius + std::sin (o.wobblePhase) * depth);
        o.x = std::cos (o.angle) * r;
        o.y = std::sin (o.angle) * r;
    }

    for (auto& e : eggs)
    {
        if (! e.alive) continue;
        const float r = juce::jlimit (0.1f, 1.0f, e.radius + std::sin (e.wobblePhase) * 0.03f);
        e.x = std::cos (e.angle) * r;
        e.y = std::sin (e.angle) * r;
    }
}

void Colony::pushTrail (Orb& o)
{
    o.trail[(size_t) o.trailWrite] = { o.x, o.y };
    o.trailWrite = (o.trailWrite + 1) % Orb::kTrail;
    o.trailCount = juce::jmin (Orb::kTrail, o.trailCount + 1);
}

Colony::StepResult Colony::step (float dt)
{
    StepResult result;

    dt = juce::jlimit (0.0f, 0.1f, dt);
    elapsed += dt;
    explosionFlash = juce::jmax (0.0f, explosionFlash - dt * 1.6f);

    // Ease through a turnaround if one is under way.
    if (reverseActive)
    {
        reverseT += dt / kReverseSeconds;
        timeDirection = directionCurve (reverseT, reverseFrom, reverseTo);
        if (reverseT >= 1.0f)
        {
            reverseT = 0.0f;
            reverseActive = false;
            timeDirection = reverseTo;
            lastEvent = timeDirection < 0.0f ? "Time running backwards" : "Time running forwards again";
        }
    }

    // A clutch changes the pace, but it comes on slowly rather than snapping.
    if (std::abs (clutchRateTarget - clutchRate) > 1.0e-4f)
    {
        const float k = 1.0f - std::exp (-dt / (kClutchGlideSeconds / 3.0f));
        clutchRate += (clutchRateTarget - clutchRate) * k;
    }
    else
    {
        clutchRate = clutchRateTarget;
    }

    asteroidFlash = juce::jmax (0.0f, asteroidFlash - dt * 0.7f);
    babbleSeconds = juce::jmax (0.0f, babbleSeconds - dt);
    if (stretchRemaining > 0.0f)
    {
        stretchRemaining = juce::jmax (0.0f, stretchRemaining - dt);
        if (stretchRemaining <= 0.0f)
        {
            stretchTarget = 1.0f;
            lastEvent = "Time stretch wore off";
        }
    }

    // Temporary oscillators (the babble's) time out on their own.
    for (int i = kIntrinsicOsc; i < kMaxOsc; ++i)
    {
        auto& osc = oscillators[(size_t) i];
        if (! osc.active || osc.ttlSeconds <= 0.0f) continue;
        osc.ttlSeconds -= dt;
        if (osc.ttlSeconds <= 0.0f)
        {
            osc.active = false;
            osc.depthSemis = 0.0f;
            osc.ttlSeconds = 0.0f;
        }
    }

    // Every four minutes, a one-in-four chance that someone drops in.
    if (autonomous) visitorClock += dt;
    if (visitorClock >= kVisitorPeriod)
    {
        visitorClock = 0.0f;
        if (rng.nextInt (kVisitorOdds) == 0)
        {
            triggerVisitor (result);
            result.audioChanged = true;
        }
    }

    // Every four minutes, a one-in-thirty chance the voxbox starts babbling.
    if (autonomous) babbleClock += dt;
    if (babbleClock >= kBabblePeriod)
    {
        babbleClock = 0.0f;
        if (rng.nextInt (kBabbleOdds) == 0)
        {
            triggerBabble (result);
            result.audioChanged = true;
        }
    }
    fractalAmount = juce::jmax (0.0f, fractalAmount - dt * 0.9f);

    // Everything below here is the colony acting on its own, which the user can
    // switch off wholesale without losing the buttons or the click gestures.
    if (! autonomous)
    {
        spawnClock = asteroidClock = resizeClock = visitorClock = babbleClock = 0.0f;
    }

    // Every five minutes, a one-in-four chance that something new turns up.
    if (autonomous) spawnClock += dt;
    if (spawnClock >= kSpawnPeriod)
    {
        spawnClock = 0.0f;
        if (rng.nextInt (kSpawnOdds) == 0 && spawnColouredOrb (result, true) >= 0)
        {
            result.audioChanged = true;
            addScore (5.0);
            lastEvent = "A coloured orb appeared";
        }
    }

    // Every forty minutes, a one-in-forty chance that one of them changes size.
    if (autonomous) resizeClock += dt;
    if (resizeClock >= kResizePeriod)
    {
        resizeClock = 0.0f;
        if (rng.nextInt (kResizeOdds) == 0) triggerResize (result);
    }

    // Every twenty minutes, a one-in-a-hundred chance of something much worse.
    if (autonomous) asteroidClock += dt;
    if (asteroidClock >= kAsteroidPeriod)
    {
        asteroidClock = 0.0f;
        if (rng.nextInt (kAsteroidOdds) == 0)
        {
            triggerAsteroid (result);
            result.audioChanged = true;
        }
    }
    destroyCooldown = juce::jmax (0.0f, destroyCooldown - dt);
    mutateCooldown = juce::jmax (0.0f, mutateCooldown - dt);

    const float flow = globalRate * clutchRate * timeDirection;

    for (auto& o : orbs)
    {
        if (! o.alive) continue;
        o.hitCooldown = juce::jmax (0.0f, o.hitCooldown - dt);

        if (o.sizeTarget > 0.0f)
        {
            o.resizeT += dt / kResizeSeconds;
            const float k = juce::jlimit (0.0f, 1.0f, o.resizeT);
            o.size = o.sizeFrom + (o.sizeTarget - o.sizeFrom) * (k * k * (3.0f - 2.0f * k));
            if (o.resizeT >= 1.0f) { o.size = o.sizeTarget; o.sizeTarget = 0.0f; o.resizeT = 0.0f; }
        }
        if ((int) (&o - orbs.data()) == draggedOrb) { o.birth = 1.0f; continue; }
        o.angle += o.angularVel * flow * dt;
        o.wobblePhase += (o.wobbleRate + getBabbleAmount() * 9.0f) * std::abs (flow) * dt;
        o.birth = juce::jmin (1.0f, o.birth + dt * 3.0f);

        if (o.kind == Kind::green)
        {
            o.greenSeconds -= dt;
            if (o.greenSeconds <= 0.0f) { o.kind = Kind::red; o.greenSeconds = 0.0f; }
        }
    }

    for (auto& e : eggs)
    {
        if (! e.alive) continue;
        e.angle += e.angularVel * flow * dt;
        e.wobblePhase += 2.2f * std::abs (flow) * dt;
    }

    // --- Egg laying ---------------------------------------------------------
    for (auto& o : orbs)
    {
        if (! o.alive || ! o.pregnant) continue;
        o.eggCooldown -= dt;
        if (o.eggCooldown > 0.0f) continue;

        // No free nest? Wait rather than losing an egg - the clutch size matters.
        if (getLiveEggCount() >= kMaxEggs) { o.eggCooldown = 0.5f; continue; }

        layEgg (o);
        o.eggCooldown = 0.6f + rng.nextFloat() * 3.4f;
        if (--o.eggsRemaining <= 0)
        {
            o.pregnant = false;
            applyClutchOutcome (o.clutchSize);
        }
    }

    // --- Hatching -----------------------------------------------------------
    for (auto& e : eggs)
    {
        if (! e.alive) continue;
        e.hatchIn -= dt;
        if (e.hatchIn > 0.0f) continue;

        e.alive = false;
        if (hatchEgg (e) >= 0)
        {
            result.audioChanged = true;
            addScore (8.0);
            lastEvent = "An egg hatched";
        }
    }

    // --- Yellow fuses -------------------------------------------------------
    for (auto& o : orbs)
    {
        if (! o.alive || o.fuseSeconds <= 0.0f) continue;

        o.fuseSeconds -= dt;
        if (o.fuseSeconds > 0.0f) continue;

        ++result.yellowFusesFired;

        if (! o.fuseRepeat)
        {
            // It says its piece once, then waits a while and says it again.
            o.fuseRepeat = true;
            o.fuseSeconds = kYellowRepeatMin + rng.nextFloat() * (kYellowRepeatMax - kYellowRepeatMin);
            lastEvent = "Yellow fuse blew - it will say it again in "
                      + juce::String (juce::roundToInt (o.fuseSeconds / 60.0f)) + " min";
        }
        else
        {
            o.fuseRepeat = false;
            o.fuseSeconds = 0.0f;
            lastEvent = "Yellow fuse blew again";
        }

        addScore (11.0);
    }

    // --- Visitors overstay, then whizz off ----------------------------------
    for (auto& o : orbs)
    {
        if (! o.alive || ! o.visitor) continue;

        if (o.exitT <= 0.0f)
        {
            o.visitSeconds -= dt;
            if (o.visitSeconds > 0.0f) continue;

            // Time is up: it is expelled, whizzing away as it goes.
            o.exitT = 0.0001f;
            result.addTone (popHzForHue (o.hue), 7, o.hue);
            lastEvent = "The visitor was expelled";
            continue;
        }

        // Flung outward, shrinking, until it is gone.
        o.exitT += dt / kVisitorExitSeconds;
        o.radius += dt * 2.4f * (1.0f + o.exitT * 3.0f);
        o.angularVel *= 1.0f + dt * 2.0f;
        o.size = juce::jmax (0.05f, o.size * (1.0f - dt * 0.8f));

        if (o.exitT >= 1.0f || o.radius > 1.6f)
        {
            killOrb ((int) (&o - orbs.data()), result, false);
            result.audioChanged = true;
            addScore (3.0);
        }
    }

    // --- Hatchlings age, and eventually die off ------------------------------
    for (auto& o : orbs)
    {
        if (! o.alive || ! o.hatchling || o.lifeSeconds <= 0.0f) continue;

        o.lifeSeconds -= dt;

        // Dim over the last twenty seconds so you can see one going.
        o.fade = juce::jlimit (0.15f, 1.0f, o.lifeSeconds / 20.0f);

        if (o.lifeSeconds <= 0.0f)
        {
            o.lifeSeconds = 0.0f;
            killOrb ((int) (&o - orbs.data()), result, true);
            ++result.died;
            result.audioChanged = true;
            addScore (1.0);
            lastEvent = "A hatchling died of old age";
        }
    }

    // --- Hatchling whistles -------------------------------------------------
    for (auto& o : orbs)
    {
        if (! o.alive || ! o.hatchling) continue;
        o.whistleCooldown -= dt;
        if (o.whistleCooldown > 0.0f) continue;

        o.whistleCooldown = 3.0f + rng.nextFloat() * 12.0f;
        if (result.numTones < (int) result.tones.size())
        {
            // Random pitch, weighted towards the bright end so it reads as a whistle.
            const float hz = 620.0f * std::pow (2.0f, rng.nextFloat() * 2.6f);
            result.tones[(size_t) result.numTones++] = { hz, 0 };
        }
    }

    // --- Green orb hunts the nearest red one --------------------------------
    for (int gi = 0; gi < kMaxOrbs; ++gi)
    {
        auto& green = orbs[(size_t) gi];
        if (! green.alive || green.kind != Kind::green) continue;

        int nearest = -1;
        float bestDist = 1.0e9f;
        for (int ri = 0; ri < kMaxOrbs; ++ri)
        {
            const auto& red = orbs[(size_t) ri];
            if (! red.alive || red.kind != Kind::red) continue;
            const float d = std::hypot (red.x - green.x, red.y - green.y);
            if (d < bestDist) { bestDist = d; nearest = ri; }
        }
        if (nearest < 0) continue;

        const auto& red = orbs[(size_t) nearest];
        green.radius += (red.radius - green.radius) * kGreenHuntStrength * dt;
        float dAngle = red.angle - green.angle;
        while (dAngle >  juce::MathConstants<float>::pi) dAngle -= juce::MathConstants<float>::twoPi;
        while (dAngle < -juce::MathConstants<float>::pi) dAngle += juce::MathConstants<float>::twoPi;
        green.angle += dAngle * kGreenHuntStrength * dt;
    }

    recomputePositions();

    // --- Ghost trails -------------------------------------------------------
    if (ghosting)
    {
        trailClock += dt;
        if (trailClock >= kTrailInterval)
        {
            trailClock = 0.0f;
            for (auto& o : orbs)
                if (o.alive) pushTrail (o);
        }
    }
    else
    {
        trailClock = 0.0f;
        for (auto& o : orbs)
            if (o.trailCount > 0) { o.trailCount = juce::jmax (0, o.trailCount - 1); }
    }

    // --- Blue meets pink: the blue one does not survive it -------------------
    for (int bi = 0; bi < kMaxOrbs; ++bi)
    {
        auto& blue = orbs[(size_t) bi];
        if (! blue.alive || ! isBlue (blue) || blue.hitCooldown > 0.0f) continue;

        for (int pi = 0; pi < kMaxOrbs; ++pi)
        {
            if (pi == bi) continue;
            const auto& pink = orbs[(size_t) pi];
            if (! pink.alive || ! isPink (pink)) continue;

            const float contact = 0.055f * (blue.size + pink.size) * 0.5f + 0.035f;
            if (std::hypot (pink.x - blue.x, pink.y - blue.y) >= contact) continue;

            blue.hitCooldown = 0.6f;
            killOrb (bi, result, true);
            registerDestruction (1, result);
            ++result.blueOrbsLost;
            result.audioChanged = true;
            lastEvent = "A blue orb crashed into a pink one";
            break;
        }
    }

    // --- Contact ------------------------------------------------------------
    for (int gi = 0; gi < kMaxOrbs; ++gi)
    {
        auto& green = orbs[(size_t) gi];
        if (! green.alive || green.kind != Kind::green) continue;

        for (int oi = 0; oi < kMaxOrbs; ++oi)
        {
            if (oi == gi) continue;
            const auto& other = orbs[(size_t) oi];
            if (! other.alive) continue;

            const float contact = 0.055f * (green.size + other.size) * 0.5f + 0.035f;
            if (std::hypot (other.x - green.x, other.y - green.y) >= contact) continue;

            if (other.kind == Kind::red)
            {
                // Green meets red: the whole colony goes up.
                const int before = getLiveOrbCount();
                detonate (gi, result);
                registerDestruction (juce::jmax (1, before - getLiveOrbCount()), result);
                result.audioChanged = true;
                result.detonated = true;
                return result;
            }

            if (other.kind == Kind::normal && green.hitCooldown <= 0.0f)
            {
                // Green brushing a plain orb: twice and time turns around.
                green.hitCooldown = 0.6f;
                if (++green.greenHits >= kGreenHitsToReverse)
                {
                    green.greenHits = 0;
                    beginReversal();
                    result.reversalStarted = true;
                    lastEvent = "Green orb struck twice - time turning around";
                }
                else
                {
                    lastEvent = "Green orb glanced off - one more turns time";
                }
            }
        }
    }

    return result;
}

void Colony::detonate (int greenIndex, StepResult& out)
{
    explosionCentre = { orbs[(size_t) greenIndex].x, orbs[(size_t) greenIndex].y };
    explosionFlash = 1.0f;

    std::vector<int> living;
    for (int i = 0; i < kMaxOrbs; ++i)
        if (orbs[(size_t) i].alive) living.push_back (i);

    const int toKill = juce::jmax (1, (int) living.size() / 2);
    for (int n = 0; n < toKill && living.size() > 1; ++n)
    {
        const int pick = rng.nextInt ((int) living.size());
        const int idx = living[(size_t) pick];
        living.erase (living.begin() + pick);
        killOrb (idx, out, n < 4);
    }

    // The green orb is consumed by its own reaction.
    killOrb (greenIndex, out, false);

    // Half of the added oscillations go with them.
    std::vector<int> added;
    for (int i = kIntrinsicOsc; i < kMaxOsc; ++i)
        if (oscillators[(size_t) i].active) added.push_back (i);

    const int oscToKill = (int) added.size() / 2;
    for (int n = 0; n < oscToKill; ++n)
    {
        const int pick = rng.nextInt ((int) added.size());
        const int idx = added[(size_t) pick];
        oscillators[(size_t) idx].active = false;
        oscillators[(size_t) idx].depthSemis = 0.0f;
        added.erase (added.begin() + pick);
        gravityStack.erase (std::remove (gravityStack.begin(), gravityStack.end(), idx), gravityStack.end());
    }

    lastEvent = "Detonation - half the colony lost";
}

//==============================================================================
//  Score, destruction and time direction
//==============================================================================

void Colony::killOrb (int index, StepResult& out, bool withDeathTone)
{
    if (index < 0 || index >= kMaxOrbs) return;
    auto& o = orbs[(size_t) index];
    if (! o.alive) return;

    const float hue = isTinted (o) ? o.hue : -1.0f;
    const bool wasBlue = isBlue (o);
    o.alive = false;
    gammas[(size_t) index] = {};

    if (wasBlue) ++out.blueOrbsLost;

    // Every orb leaves the inverse of the pop it arrived with.
    out.addTone (popHzForHue (hue >= 0.0f ? hue : 0.35f), 5, hue);

    if (withDeathTone)
    {
        const float n01 = rng.nextFloat();
        out.addTone (kDeathToneMinHz * std::pow (kDeathToneMaxHz / kDeathToneMinHz, n01), 1);
    }
}

int Colony::spawnColouredOrb (StepResult& out, bool announce)
{
    const int idx = spawnOrb (Kind::normal, nullptr);
    if (idx < 0) return -1;

    auto& o = orbs[(size_t) idx];
    o.hatchling = true;                       // it lives a hatchling's life and whistles
    o.hue = rng.nextFloat();
    o.size = 0.6f + rng.nextFloat() * 0.5f;
    o.whistleCooldown = 1.0f + rng.nextFloat() * 10.0f;
    o.lifeTotal = kHatchlingMinLife + rng.nextFloat() * (kHatchlingMaxLife - kHatchlingMinLife);
    o.lifeSeconds = o.lifeTotal;

    if (announce) out.addTone (popHzForHue (o.hue), 4, o.hue);   // its colour, its pop
    return idx;
}

void Colony::triggerResize (StepResult& out)
{
    std::vector<int> candidates;
    for (int i = 0; i < kMaxOrbs; ++i)
        if (orbs[(size_t) i].alive && orbs[(size_t) i].sizeTarget <= 0.0f) candidates.push_back (i);
    if (candidates.empty()) return;

    auto& o = orbs[(size_t) candidates[(size_t) rng.nextInt ((int) candidates.size())]];

    // Anywhere from a tenth of its size to four times it, reached slowly.
    const float factor = kResizeMin * std::pow (kResizeMax / kResizeMin, rng.nextFloat());
    o.sizeFrom = o.size;
    o.sizeTarget = juce::jlimit (0.06f, 6.0f, o.size * factor);
    o.resizeT = 0.0f;

    // A rising tone rides along with the change for as long as it lasts.
    out.addTone (popHzForHue (o.hatchling ? o.hue : 0.3f) * 0.75f, 6, o.hatchling ? o.hue : -1.0f);

    addScore (7.0);
    lastEvent = "An orb is changing size (x" + juce::String (factor, 2) + ")";
}

void Colony::triggerAsteroid (StepResult& out)
{
    asteroidFlash = 1.0f;
    out.asteroidHit = true;

    // Everything that was here is blown apart...
    int destroyed = 0;
    for (int i = 0; i < kMaxOrbs; ++i)
    {
        if (! orbs[(size_t) i].alive) continue;
        killOrb (i, out, destroyed < 6);      // only the first few get a full death tone
        ++destroyed;
    }
    for (auto& e : eggs) e.alive = false;

    // ...and a hundred coloured orbs come out of it.
    for (int i = 0; i < kAsteroidOrbs; ++i)
        spawnColouredOrb (out, i < 10);       // only a handful pop, or it is a wall of noise

    // And the whole place runs anywhere from one to a hundred times faster.
    clutchRateTarget = juce::jlimit (1.0f, 100.0f, 1.0f + rng.nextFloat() * 99.0f);

    registerDestruction (destroyed, out);
    addScore (100.0);
    lastEvent = "ASTEROID - colony shattered into " + juce::String (kAsteroidOrbs)
              + ", pace x" + juce::String (clutchRateTarget, 1);
}

float Colony::getTimeStretch() const
{
    if (stretchRemaining <= 0.0f) return 1.0f;
    const float k = juce::jlimit (0.0f, 1.0f, stretchRemaining / kOverdoseWearOff);
    return 1.0f + (stretchTarget - 1.0f) * k;      // creeps back to normal over 30 minutes
}

bool Colony::logPressAndCheck (std::vector<float>& log)
{
    log.push_back (elapsed);
    while (! log.empty() && elapsed - log.front() > kOverdoseWindow)
        log.erase (log.begin());

    if ((int) log.size() <= kOverdosePresses) return false;
    log.clear();                                   // one overdose per burst
    return true;
}

void Colony::addTemporaryOscillators (int count, float ttlSeconds)
{
    int added = 0;
    for (int i = kIntrinsicOsc; i < kMaxOsc && added < count; ++i)
    {
        auto& osc = oscillators[(size_t) i];
        if (osc.active) continue;

        osc.active = true;
        osc.intrinsic = false;
        osc.ttlSeconds = ttlSeconds;
        osc.rateHz = 0.4f + rng.nextFloat() * 11.0f;
        osc.depthSemis = 0.3f + rng.nextFloat() * 1.6f;
        osc.shape = rng.nextInt (4);
        osc.phase = rng.nextFloat();
        osc.rateScale = 1.0f;
        ++added;
    }
}

void Colony::triggerVisitor (StepResult& out)
{
    const int idx = spawnColouredOrb (out, true);      // arrives with its colour's pop
    if (idx < 0) return;

    auto& o = orbs[(size_t) idx];
    o.visitor = true;
    o.visitSeconds = kVisitorMinStay + rng.nextFloat() * (kVisitorMaxStay - kVisitorMinStay);
    o.exitT = 0.0f;
    o.lifeSeconds = 0.0f;                              // the clock that matters is the visit
    o.lifeTotal = 0.0f;

    addScore (6.0);
    lastEvent = "A visitor dropped in for "
              + juce::String (juce::roundToInt (o.visitSeconds / 60.0f)) + " minutes";

    // Every fifth arrival, the voxbox asks the room for food and shelter.
    if (++visitorCount % 5 == 0)
    {
        out.pleaForFoodAndShelter = true;
        lastEvent = "A visitor arrived - the voxbox is begging for food and shelter";
    }
}

void Colony::triggerBabble (StepResult& out)
{
    babbleSeconds = kBabbleSeconds;
    out.babbleStarted = true;

    // The shaking is literal: temporary oscillators in the sound, and the same
    // unsettled motion in the field, for as long as the babble echoes.
    addTemporaryOscillators (4, kBabbleSeconds);

    addScore (15.0);
    lastEvent = "VOXBOX BABBLING - everything is shaking";
}

void Colony::addScore (double points)
{
    score += points;
}

void Colony::beginReversal()
{
    // Already turning? Let the current turn finish rather than stacking them.
    if (reverseActive) return;

    reverseActive = true;
    reverseT = 0.0f;
    reverseFrom = timeDirection;
    reverseTo = timeDirection >= 0.0f ? -1.0f : 1.0f;
}

void Colony::registerDestruction (int count, StepResult& out)
{
    if (count <= 0) return;

    for (int i = 0; i < count; ++i)
    {
        ++totalDestructions;
        recentDestructions.push_back (elapsed);

        // Every twentieth thing destroyed turns time around.
        if (totalDestructions % kDestructionsPerReversal == 0)
        {
            beginReversal();
            out.reversalStarted = true;
        }
    }

    // How much damage in the last three seconds?
    while (! recentDestructions.empty() && elapsed - recentDestructions.front() > 3.0f)
        recentDestructions.erase (recentDestructions.begin());

    const int burst = (int) recentDestructions.size();
    if (burst > 8)
    {
        // Wrecking the place. Keep a fraction of what you had.
        score *= 0.4;
        lastEvent = "Too much damage too fast - score fractioned";
    }
    else if (burst > 4)
    {
        score -= 5.0 * (double) (burst - 4);
        lastEvent = "Destroying too quickly - score docked";
    }
}

//==============================================================================
//  Dragging
//==============================================================================

int Colony::nearestOrbTo (float nx, float ny, float maxDist) const
{
    int best = -1;
    float bestDist = maxDist;
    for (int i = 0; i < kMaxOrbs; ++i)
    {
        const auto& o = orbs[(size_t) i];
        if (! o.alive) continue;
        const float d = std::hypot (o.x - nx, o.y - ny);
        if (d < bestDist) { bestDist = d; best = i; }
    }
    return best;
}

bool Colony::clickAt (float nx, float ny)
{
    const int idx = nearestOrbTo (nx, ny, 0.16f);
    if (idx < 0) return false;

    auto& o = orbs[(size_t) idx];
    if (! isYellow (o) || o.fuseSeconds > 0.0f) return false;

    o.fuseSeconds = kYellowFuseMin + rng.nextFloat() * (kYellowFuseMax - kYellowFuseMin);
    o.fuseRepeat = false;
    addScore (9.0);
    lastEvent = "Yellow orb lit - " + juce::String (juce::roundToInt (o.fuseSeconds)) + "s on the clock";
    return true;
}

bool Colony::beginDrag (float nx, float ny)
{
    draggedOrb = nearestOrbTo (nx, ny, kDragGrabRadius);
    lastDragX = nx;
    lastDragY = ny;
    dragSpeed = 0.0f;
    if (draggedOrb >= 0) lastEvent = "Orb grabbed";
    return draggedOrb >= 0;
}

void Colony::endDrag()
{
    draggedOrb = -1;
    dragSpeed = 0.0f;
}

Colony::StepResult Colony::dragTo (float nx, float ny, float dtSeconds)
{
    StepResult result;
    if (draggedOrb < 0 || ! orbs[(size_t) draggedOrb].alive) { draggedOrb = -1; return result; }

    const float dt = juce::jlimit (0.001f, 0.2f, dtSeconds);
    const float moved = std::hypot (nx - lastDragX, ny - lastDragY);
    const float speed = moved / dt;
    dragSpeed = dragSpeed * 0.6f + speed * 0.4f;

    // Put the orb where the pointer is.
    auto& o = orbs[(size_t) draggedOrb];
    o.radius = juce::jlimit (0.08f, 1.0f, std::hypot (nx, ny));
    o.angle = std::atan2 (ny, nx);
    o.x = nx;
    o.y = ny;
    lastDragX = nx;
    lastDragY = ny;

    if (dragSpeed > kFastDragSpeed && destroyCooldown <= 0.0f)
    {
        // Hauled across the field: it takes others out with it.
        destroyCooldown = 0.18f;

        std::vector<int> victims;
        for (int i = 0; i < kMaxOrbs; ++i)
            if (orbs[(size_t) i].alive && i != draggedOrb) victims.push_back (i);

        int killed = 0;
        const int toKill = juce::jmin ((int) victims.size() - 1, 1 + rng.nextInt (2));
        for (int n = 0; n < toKill && victims.size() > 1; ++n)
        {
            const int pick = rng.nextInt ((int) victims.size());
            const int idx = victims[(size_t) pick];
            victims.erase (victims.begin() + pick);

            killOrb (idx, result, true);
            ++killed;
        }

        // ...and it damages the sound as well as the colony.
        for (int i = kIntrinsicOsc; i < kMaxOsc; ++i)
        {
            auto& osc = oscillators[(size_t) i];
            if (! osc.active) continue;
            osc.active = false;
            osc.depthSemis = 0.0f;
            gravityStack.erase (std::remove (gravityStack.begin(), gravityStack.end(), i), gravityStack.end());
            break;
        }

        if (killed > 0)
        {
            registerDestruction (killed, result);
            result.audioChanged = true;
            lastEvent = "Dragged too fast - " + juce::String (killed) + " destroyed";
        }
    }
    else if (dragSpeed > 0.05f && dragSpeed < kSlowDragSpeed && mutateCooldown <= 0.0f)
    {
        // Eased around slowly: the sound mutates fast and the field fractures.
        mutateCooldown = 0.12f;
        fractalAmount = juce::jmin (1.0f, fractalAmount + 0.45f);

        auto& g = gammas[(size_t) draggedOrb];
        g.active = true;
        g.semis = (rng.nextFloat() * 10.0f) - 5.0f;
        g.pace = 0.4f + rng.nextFloat() * 3.0f;

        for (int i = kIntrinsicOsc; i < kMaxOsc; ++i)
        {
            auto& osc = oscillators[(size_t) i];
            if (! osc.active) continue;
            osc.rateHz = 0.2f + rng.nextFloat() * 8.8f;
            osc.depthSemis = 0.1f + rng.nextFloat() * 0.9f;
            osc.shape = rng.nextInt (4);
            break;
        }

        addScore (2.0);
        result.audioChanged = true;
        lastEvent = "Slow drag - mutating";
    }

    return result;
}

//==============================================================================
//  Breeding
//==============================================================================

int Colony::rollClutchSize()
{
    const float r = rng.nextFloat();
    if (r < 0.10f)  return kClutchWildcard;      // 1 in 10
    if (r < 0.35f)  return kClutchDoubleSpeed;   // 1 in 4
    if (r < 0.55f)  return kClutchHalfSpeed;     // 1 in 5
    return 2 + rng.nextInt (87);                 // the rest of the time, 2..88
}

void Colony::applyClutchOutcome (int size)
{
    if (size == kClutchHalfSpeed)
    {
        clutchRateTarget *= 0.5f;
        lastEvent = "Clutch of 20 - everything slowing to half speed";
    }
    else if (size == kClutchDoubleSpeed)
    {
        clutchRateTarget *= 2.0f;
        lastEvent = "Clutch of 30 - everything doubling in speed";
    }
    else if (size == kClutchWildcard)
    {
        // Anywhere from three times slower to four times faster.
        const float lo = 1.0f / 3.0f;
        const float factor = lo * std::pow (4.0f / lo, rng.nextFloat());
        clutchRateTarget *= factor;
        lastEvent = "Clutch of 10 - pace rolled to " + juce::String (factor, 2) + "x";
    }
    else
    {
        lastEvent = "Clutch of " + juce::String (size) + " complete";
        return;
    }

    clutchRateTarget = juce::jlimit (0.05f, 100.0f, clutchRateTarget);
    addScore (12.0);
}

void Colony::layEgg (const Orb& mother)
{
    for (auto& e : eggs)
    {
        if (e.alive) continue;
        e.alive = true;
        e.angle = mother.angle + (rng.nextFloat() - 0.5f) * 0.5f;
        e.radius = juce::jlimit (0.2f, 0.98f, mother.radius + (rng.nextFloat() - 0.5f) * 0.14f);
        e.angularVel = mother.angularVel * (0.55f + rng.nextFloat() * 0.35f);
        e.wobblePhase = rng.nextFloat() * juce::MathConstants<float>::twoPi;
        e.hatchIn = 4.0f + rng.nextFloat() * 10.0f;        // hatches at a random time
        e.hue = rng.nextFloat();                            // and in a colour of its own
        lastEvent = "Egg laid";
        return;
    }
}

int Colony::hatchEgg (const Egg& egg)
{
    for (int i = 0; i < kMaxOrbs; ++i)
    {
        auto& o = orbs[(size_t) i];
        if (o.alive) continue;

        seedOrb (o, i, nullptr);
        o.angle = egg.angle;
        o.radius = egg.radius;
        o.angularVel = egg.angularVel * (0.9f + rng.nextFloat() * 0.5f);
        o.size = 0.62f + rng.nextFloat() * 0.3f;
        o.kind = Kind::normal;
        o.hatchling = true;
        o.hue = egg.hue;
        o.whistleCooldown = 1.0f + rng.nextFloat() * 8.0f;
        o.lifeTotal = kHatchlingMinLife + rng.nextFloat() * (kHatchlingMaxLife - kHatchlingMinLife);
        o.lifeSeconds = o.lifeTotal;
        o.fade = 1.0f;
        gammas[(size_t) i] = {};
        return i;
    }
    return -1;
}

//==============================================================================
//  Interaction
//==============================================================================

int Colony::spawnOrb (Kind kind, const Orb* parent)
{
    for (int i = 0; i < kMaxOrbs; ++i)
    {
        if (orbs[(size_t) i].alive) continue;
        seedOrb (orbs[(size_t) i], i, parent);
        orbs[(size_t) i].kind = kind;
        gammas[(size_t) i] = {};
        return i;
    }
    return -1;
}

bool Colony::registerLeftClick()
{
    if (++leftClicks < kLeftClicksToSplit) return false;
    leftClicks = 0;

    std::vector<int> living;
    for (int i = 0; i < kMaxOrbs; ++i)
        if (orbs[(size_t) i].alive) living.push_back (i);
    if (living.empty()) return false;

    int split = 0;
    for (int n = 0; n < kOrbsPerSplit && ! living.empty(); ++n)
    {
        const int pick = rng.nextInt ((int) living.size());
        const Orb parent = orbs[(size_t) living[(size_t) pick]];
        living.erase (living.begin() + pick);

        if (spawnOrb (parent.kind, &parent) >= 0) ++split;
    }

    addScore (6.0 * (double) split);
    lastEvent = split > 0 ? juce::String (split) + " orbs split and replicated"
                          : juce::String ("Colony full - nothing split");
    recomputePositions();
    return split > 0;
}

bool Colony::registerRightClick()
{
    if (++rightClicks < kRightClicksToSpawn) return false;
    rightClicks = 0;

    const bool grew = spawnOrb (Kind::red, nullptr) >= 0;
    if (grew) addScore (4.0);
    lastEvent = grew ? "Red orb born" : "Colony full - no room for a red orb";
    recomputePositions();
    return grew;
}

bool Colony::registerMiddleClick()
{
    if (++middleClicks < kMiddleClicksToImpregnate) return false;
    middleClicks = 0;

    // Pick an orb that is not already carrying and not mid-reaction.
    std::vector<int> candidates;
    for (int i = 0; i < kMaxOrbs; ++i)
    {
        const auto& o = orbs[(size_t) i];
        if (o.alive && ! o.pregnant && o.kind != Kind::green) candidates.push_back (i);
    }
    if (candidates.empty()) { lastEvent = "No orb free to impregnate"; return false; }

    auto& mother = orbs[(size_t) candidates[(size_t) rng.nextInt ((int) candidates.size())]];
    mother.pregnant = true;
    mother.clutchSize = rollClutchSize();
    mother.eggsRemaining = mother.clutchSize;
    mother.eggCooldown = 1.2f + rng.nextFloat() * 2.5f;

    lastEvent = "Orb impregnated - clutch of " + juce::String (mother.clutchSize);
    return false;   // no audio change yet; the hatchlings are what add elements
}

bool Colony::addGravity()
{
    std::vector<int> candidates;
    for (int i = 0; i < kMaxOsc; ++i)
        if (oscillators[(size_t) i].active) candidates.push_back (i);

    juce::String which = "rotation";
    if (! candidates.empty())
    {
        const int idx = candidates[(size_t) rng.nextInt ((int) candidates.size())];
        oscillators[(size_t) idx].rateScale *= kGravityRateDrag;
        gravityStack.push_back (idx);
        which = "osc " + juce::String (idx + 1);
    }

    globalRate = juce::jmax (kMinGlobalRate, globalRate * kGravityGlobal);
    addScore (3.0);
    lastEvent = "Gravity added - " + which + " slowed";
    return true;
}

bool Colony::releaseGravity()
{
    if (! gravityStack.empty())
    {
        const int idx = gravityStack.back();
        gravityStack.pop_back();
        oscillators[(size_t) idx].rateScale /= kGravityRateDrag;
        lastEvent = "Gravity released - osc " + juce::String (idx + 1) + " freed";
    }
    else
    {
        lastEvent = "Gravity released - colony speeding up";
    }

    globalRate = juce::jmin (kMaxGlobalRate, globalRate / kGravityGlobal);
    addScore (3.0);
    return true;
}

bool Colony::addEnzyme()
{
    globalRate = juce::jmin (kMaxGlobalRate, globalRate * kEnzymeGlobal);

    int added = 0;
    for (int i = kIntrinsicOsc; i < kMaxOsc && added < 3; ++i)
    {
        auto& osc = oscillators[(size_t) i];
        if (osc.active) continue;

        osc.active = true;
        osc.intrinsic = false;
        osc.rateHz = 0.2f + rng.nextFloat() * 8.8f;
        osc.depthSemis = 0.12f + rng.nextFloat() * 1.05f;
        osc.shape = rng.nextInt (4);
        osc.phase = rng.nextFloat();
        osc.rateScale = 1.0f;
        ++added;
    }

    addScore (5.0 + 3.0 * (double) added);

    // Leaning on it does the opposite of over-radiating: it compresses time.
    if (logPressAndCheck (enzymePresses))
    {
        const float f = kStretchMin * std::pow (kStretchMax / kStretchMin, rng.nextFloat());
        stretchTarget = 1.0f / f;
        stretchRemaining = kOverdoseWearOff;
        lastEvent = "Enzyme overdose - time compressed to " + juce::String (1.0f / f, 2) + "x";
        return true;
    }

    lastEvent = added > 0 ? "Enzyme - " + juce::String (added) + " oscillations added, colony accelerating"
                          : juce::String ("Enzyme - oscillator bank full, colony accelerating");
    return true;
}

bool Colony::addWater()
{
    ++waterCount;
    addScore (2.0);

    // Leaning on it: the whole thing goes watered down and washed out.
    if (logPressAndCheck (waterPresses))
    {
        washout = juce::jmin (1.0f, washout + 0.35f);
        lastEvent = "Too much water at once - washed out";
        return true;
    }

    const int remaining = juce::jmax (0, kWaterToDrown - waterCount);
    if (remaining > 0)
        lastEvent = "Water added - " + juce::String (remaining) + " more and it drowns";
    else
        lastEvent = "Drowned - the colony is barely audible";

    return true;
}

bool Colony::addGamma()
{
    std::vector<int> living;
    for (int i = 0; i < kMaxOrbs; ++i)
        if (orbs[(size_t) i].alive) living.push_back (i);

    int mutated = 0;
    for (int n = 0; n < 3 && ! living.empty(); ++n)
    {
        const int pick = rng.nextInt ((int) living.size());
        const int idx = living[(size_t) pick];
        living.erase (living.begin() + pick);

        auto& g = gammas[(size_t) idx];
        g.active = true;
        g.semis = (1.0f + rng.nextFloat() * 6.0f) * (rng.nextBool() ? 1.0f : -1.0f);
        g.pace = 0.15f + rng.nextFloat() * 2.4f;
        ++mutated;
    }

    int target = pickRandomLiveOrb (Kind::red, false);
    if (target < 0) target = pickRandomLiveOrb (Kind::normal, true);

    if (target >= 0)
    {
        orbs[(size_t) target].kind = Kind::green;
        orbs[(size_t) target].greenSeconds = kGreenSeconds;
    }

    addScore (4.0 * (double) mutated);

    // Leaning on it: the audio time-stretches, granulates and drags.
    if (logPressAndCheck (radiatePresses))
    {
        const float f = kStretchMin * std::pow (kStretchMax / kStretchMin, rng.nextFloat());
        stretchTarget = f;
        stretchRemaining = kOverdoseWearOff;
        lastEvent = "Over-radiated - time stretched to " + juce::String (f, 2) + "x, granulating";
        return true;
    }

    lastEvent = "Gamma - " + juce::String (mutated) + " elements mutated"
              + (target >= 0 ? ", orb turned green" : "");
    return mutated > 0;
}

int Colony::pickRandomLiveOrb (Kind wanted, bool anyKind)
{
    std::vector<int> matches;
    for (int i = 0; i < kMaxOrbs; ++i)
    {
        const auto& o = orbs[(size_t) i];
        if (! o.alive) continue;
        if (o.kind == Kind::green) continue;          // already reacting
        if (anyKind || o.kind == wanted) matches.push_back (i);
    }
    if (matches.empty()) return -1;
    return matches[(size_t) rng.nextInt ((int) matches.size())];
}

//==============================================================================
//  Audio queries
//==============================================================================

bool Colony::isVoiceDead (int voiceIndex) const
{
    if (voiceIndex < 0 || voiceIndex >= kMaxOrbs) return false;
    return ! orbs[(size_t) voiceIndex].alive;
}

float Colony::oscillatorPitchMod (int voiceIndex, float timeSeconds) const
{
    float sum = 0.0f;
    const float voicePhase = (float) voiceIndex * 0.137f;

    for (int i = kIntrinsicOsc; i < kMaxOsc; ++i)
    {
        const auto& osc = oscillators[(size_t) i];
        if (! osc.active || osc.depthSemis <= 0.0f) continue;

        const float rate = osc.rateHz * osc.rateScale * globalRate;
        sum += oscShape (osc.shape, osc.phase + voicePhase + timeSeconds * rate) * osc.depthSemis;
    }
    return sum;
}

float Colony::gammaPitchOffset (int voiceIndex, float timeSeconds) const
{
    if (voiceIndex < 0 || voiceIndex >= kMaxOrbs) return 0.0f;
    const auto& g = gammas[(size_t) voiceIndex];
    if (! g.active) return 0.0f;

    const float progress = juce::jlimit (0.0f, 1.0f, timeSeconds * g.pace);
    return g.semis * progress;
}

juce::String Colony::getStatusLine() const
{
    juce::String s = "ORBS " + juce::String (getLiveOrbCount());
    const int eggCount = getLiveEggCount();
    if (eggCount > 0) s += "   EGGS " + juce::String (eggCount);
    s += "   OSC " + juce::String (getEnzymeOscCount())
       + "   GRAV " + juce::String (getGravityDepth());
    if (waterCount > 0) s += "   H2O " + juce::String (waterCount) + "/" + juce::String (kWaterToDrown);
    if (washout > 0.01f) s += "   WASH " + juce::String (juce::roundToInt (washout * 100.0f)) + "%";
    if (isGranulating()) s += "   STRETCH " + juce::String (getTimeStretch(), 2) + "x";
    if (babbleSeconds > 0.0f) s += "   BABBLE " + juce::String (juce::roundToInt (babbleSeconds)) + "s";
    s += "   RATE " + juce::String (globalRate * clutchRate * timeDirection, 2) + "x";
    if (reverseActive) s += " (turning)";
    else if (timeDirection < 0.0f) s += " (reversed)";
    s += "   " + lastEvent;
    return s;
}

//==============================================================================
//  Persistence
//==============================================================================

juce::ValueTree Colony::toValueTree() const
{
    juce::ValueTree t ("COLONY");
    t.setProperty ("globalRate", globalRate, nullptr);
    t.setProperty ("leftClicks", leftClicks, nullptr);
    t.setProperty ("rightClicks", rightClicks, nullptr);
    t.setProperty ("middleClicks", middleClicks, nullptr);
    t.setProperty ("water", waterCount, nullptr);
    t.setProperty ("score", score, nullptr);
    t.setProperty ("destructions", totalDestructions, nullptr);
    t.setProperty ("timeDir", timeDirection, nullptr);
    t.setProperty ("clutchRate", clutchRateTarget, nullptr);
    t.setProperty ("visitors", visitorCount, nullptr);
    t.setProperty ("washout", washout, nullptr);
    t.setProperty ("stretchTarget", stretchTarget, nullptr);
    t.setProperty ("stretchRemaining", stretchRemaining, nullptr);

    juce::String gravity;
    for (auto idx : gravityStack) gravity += juce::String (idx) + " ";
    t.setProperty ("gravity", gravity.trim(), nullptr);

    for (int i = 0; i < kMaxOrbs; ++i)
    {
        const auto& o = orbs[(size_t) i];
        if (! o.alive) continue;
        juce::ValueTree n ("ORB");
        n.setProperty ("i", i, nullptr);
        n.setProperty ("angle", o.angle, nullptr);
        n.setProperty ("radius", o.radius, nullptr);
        n.setProperty ("vel", o.angularVel, nullptr);
        n.setProperty ("size", o.size, nullptr);
        n.setProperty ("kind", (int) o.kind, nullptr);
        n.setProperty ("green", o.greenSeconds, nullptr);
        n.setProperty ("preg", o.pregnant, nullptr);
        n.setProperty ("eggs", o.eggsRemaining, nullptr);
        n.setProperty ("clutch", o.clutchSize, nullptr);
        n.setProperty ("fuse", o.fuseSeconds, nullptr);
        n.setProperty ("fuseRep", o.fuseRepeat, nullptr);
        n.setProperty ("visitor", o.visitor, nullptr);
        n.setProperty ("visitSec", o.visitSeconds, nullptr);
        n.setProperty ("sizeTgt", o.sizeTarget, nullptr);
        n.setProperty ("sizeFrom", o.sizeFrom, nullptr);
        n.setProperty ("resizeT", o.resizeT, nullptr);
        n.setProperty ("hatch", o.hatchling, nullptr);
        n.setProperty ("hue", o.hue, nullptr);
        n.setProperty ("life", o.lifeSeconds, nullptr);
        n.setProperty ("lifeTot", o.lifeTotal, nullptr);
        n.setProperty ("gSemis", gammas[(size_t) i].active ? gammas[(size_t) i].semis : 0.0f, nullptr);
        n.setProperty ("gPace", gammas[(size_t) i].pace, nullptr);
        t.appendChild (n, nullptr);
    }

    for (const auto& e : eggs)
    {
        if (! e.alive) continue;
        juce::ValueTree n ("EGG");
        n.setProperty ("angle", e.angle, nullptr);
        n.setProperty ("radius", e.radius, nullptr);
        n.setProperty ("vel", e.angularVel, nullptr);
        n.setProperty ("hatchIn", e.hatchIn, nullptr);
        n.setProperty ("hue", e.hue, nullptr);
        t.appendChild (n, nullptr);
    }

    for (int i = 0; i < kMaxOsc; ++i)
    {
        const auto& o = oscillators[(size_t) i];
        if (! o.active) continue;
        juce::ValueTree n ("OSC");
        n.setProperty ("i", i, nullptr);
        n.setProperty ("rate", o.rateHz, nullptr);
        n.setProperty ("depth", o.depthSemis, nullptr);
        n.setProperty ("shape", o.shape, nullptr);
        n.setProperty ("phase", o.phase, nullptr);
        n.setProperty ("scale", o.rateScale, nullptr);
        n.setProperty ("intr", o.intrinsic, nullptr);
        t.appendChild (n, nullptr);
    }
    return t;
}

void Colony::fromValueTree (const juce::ValueTree& t)
{
    if (! t.isValid() || ! t.hasType ("COLONY")) return;

    for (auto& o : orbs) o.alive = false;
    for (auto& e : eggs) e.alive = false;
    for (auto& g : gammas) g = {};
    for (int i = 0; i < kMaxOsc; ++i)
    {
        oscillators[(size_t) i] = {};
        oscillators[(size_t) i].intrinsic = i < kIntrinsicOsc;
        oscillators[(size_t) i].active = i < kIntrinsicOsc;
    }
    gravityStack.clear();

    globalRate = juce::jlimit (kMinGlobalRate, kMaxGlobalRate, (float) t.getProperty ("globalRate", 1.0));
    leftClicks = juce::jlimit (0, kLeftClicksToSplit - 1, (int) t.getProperty ("leftClicks", 0));
    rightClicks = juce::jlimit (0, kRightClicksToSpawn - 1, (int) t.getProperty ("rightClicks", 0));
    middleClicks = juce::jlimit (0, kMiddleClicksToImpregnate - 1, (int) t.getProperty ("middleClicks", 0));
    waterCount = juce::jmax (0, (int) t.getProperty ("water", 0));
    score = (double) t.getProperty ("score", 0.0);
    totalDestructions = juce::jmax (0, (int) t.getProperty ("destructions", 0));
    timeDirection = juce::jlimit (-1.0f, 1.0f, (float) t.getProperty ("timeDir", 1.0));
    clutchRateTarget = juce::jlimit (0.05f, 100.0f, (float) t.getProperty ("clutchRate", 1.0));
    clutchRate = clutchRateTarget;
    visitorCount = juce::jmax (0, (int) t.getProperty ("visitors", 0));
    washout = juce::jlimit (0.0f, 1.0f, (float) t.getProperty ("washout", 0.0));
    stretchTarget = juce::jlimit (0.2f, 5.0f, (float) t.getProperty ("stretchTarget", 1.0));
    stretchRemaining = juce::jmax (0.0f, (float) t.getProperty ("stretchRemaining", 0.0));
    reverseActive = false;
    reverseT = 0.0f;
    recentDestructions.clear();

    int eggSlot = 0;
    for (int c = 0; c < t.getNumChildren(); ++c)
    {
        auto n = t.getChild (c);

        if (n.hasType ("ORB"))
        {
            const int i = (int) n.getProperty ("i", -1);
            if (i < 0 || i >= kMaxOrbs) continue;

            auto& o = orbs[(size_t) i];
            seedOrb (o, i, nullptr);
            o.angle = (float) n.getProperty ("angle", 0.0);
            o.radius = (float) n.getProperty ("radius", 0.6);
            o.angularVel = (float) n.getProperty ("vel", 0.4);
            o.size = (float) n.getProperty ("size", 1.0);
            o.kind = (Kind) juce::jlimit (0, 2, (int) n.getProperty ("kind", 0));
            o.greenSeconds = (float) n.getProperty ("green", 0.0);
            o.pregnant = (bool) n.getProperty ("preg", false);
            o.eggsRemaining = (int) n.getProperty ("eggs", 0);
            o.clutchSize = (int) n.getProperty ("clutch", 0);
            o.fuseSeconds = (float) n.getProperty ("fuse", 0.0);
            o.fuseRepeat = (bool) n.getProperty ("fuseRep", false);
            o.visitor = (bool) n.getProperty ("visitor", false);
            o.visitSeconds = (float) n.getProperty ("visitSec", 0.0);
            o.sizeTarget = (float) n.getProperty ("sizeTgt", 0.0);
            o.sizeFrom = (float) n.getProperty ("sizeFrom", 1.0);
            o.resizeT = (float) n.getProperty ("resizeT", 0.0);
            o.hatchling = (bool) n.getProperty ("hatch", false);
            o.hue = (float) n.getProperty ("hue", 0.0);
            o.whistleCooldown = 2.0f + rng.nextFloat() * 10.0f;
            o.lifeSeconds = (float) n.getProperty ("life", 0.0);
            o.lifeTotal = (float) n.getProperty ("lifeTot", 0.0);
            o.fade = o.hatchling && o.lifeSeconds > 0.0f
                        ? juce::jlimit (0.15f, 1.0f, o.lifeSeconds / 20.0f) : 1.0f;
            o.birth = 1.0f;

            const float semis = (float) n.getProperty ("gSemis", 0.0);
            if (std::abs (semis) > 0.001f)
            {
                gammas[(size_t) i].active = true;
                gammas[(size_t) i].semis = semis;
                gammas[(size_t) i].pace = (float) n.getProperty ("gPace", 1.0);
            }
        }
        else if (n.hasType ("EGG") && eggSlot < kMaxEggs)
        {
            auto& e = eggs[(size_t) eggSlot++];
            e.alive = true;
            e.angle = (float) n.getProperty ("angle", 0.0);
            e.radius = (float) n.getProperty ("radius", 0.6);
            e.angularVel = (float) n.getProperty ("vel", 0.2);
            e.hatchIn = (float) n.getProperty ("hatchIn", 6.0);
            e.hue = (float) n.getProperty ("hue", 0.0);
            e.wobblePhase = rng.nextFloat() * juce::MathConstants<float>::twoPi;
        }
        else if (n.hasType ("OSC"))
        {
            const int i = (int) n.getProperty ("i", -1);
            if (i < 0 || i >= kMaxOsc) continue;

            auto& o = oscillators[(size_t) i];
            o.active = true;
            o.intrinsic = (bool) n.getProperty ("intr", i < kIntrinsicOsc);
            o.rateHz = (float) n.getProperty ("rate", 1.0);
            o.depthSemis = (float) n.getProperty ("depth", 0.0);
            o.shape = juce::jlimit (0, 3, (int) n.getProperty ("shape", 0));
            o.phase = (float) n.getProperty ("phase", 0.0);
            o.rateScale = (float) n.getProperty ("scale", 1.0);
        }
    }

    auto gravity = t.getProperty ("gravity", "").toString();
    for (const auto& tok : juce::StringArray::fromTokens (gravity, " ", ""))
        if (tok.isNotEmpty()) gravityStack.push_back (juce::jlimit (0, kMaxOsc - 1, tok.getIntValue()));

    if (getLiveOrbCount() == 0) syncToVoiceCount (12);
    recomputePositions();
    lastEvent = "Colony restored";
}
