#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
//  Hover-over tooltip copy. One short sentence per control so the user can
//  learn the instrument without leaving the plugin.
//==============================================================================

namespace PCTips
{
    inline juce::String forParam (const juce::String& id)
    {
        // Source & capture
        if (id == IDs::sourceMode)     return "Where the swarm gets its audio: live input, a loaded sample, both layered, or scattered slices.";
        if (id == IDs::captureMode)    return "How a live capture is ended - by input threshold, by a musical length, or manually.";
        if (id == IDs::thresh)         return "Input level that arms a threshold capture. Lower values trigger on quieter material.";
        if (id == IDs::captureSlot)    return "Which of the eight capture history slots is feeding the swarm.";
        if (id == IDs::captureLock)    return "Protects the active slot: new captures land in the next free slot instead of overwriting.";

        // Swarm engine
        if (id == IDs::voiceCount)     return "Number of voices in the swarm, 1 to 32. More voices means a thicker, wider cloud.";
        if (id == IDs::voiceDensity)   return "Curve controlling when voices join. Negative front-loads them, positive saves them for the end.";
        if (id == IDs::voiceAge)       return "Darkens and degrades the voices that entered earliest, pushing them behind the newer ones.";
        if (id == IDs::progReveal)     return "Progressive reveal: at 0 every voice is the full phrase, at 1 they start as fragments and grow.";
        if (id == IDs::voiceDirection) return "Playback direction per voice: forward, reverse, alternating, or random.";
        if (id == IDs::character)      return "Ensemble colour model - digital, analog, bucket-brigade, tape, dimension, strings, granular, or lo-fi.";
        if (id == IDs::humanize)       return "Organic micro-variation in timing, pitch and level so the swarm never sounds cloned.";
        if (id == IDs::grainSize)      return "Grain length used by the Granular Cloud character, 10 to 200 ms.";

        // Convergence
        if (id == IDs::macro)          return "Master convergence amount. Scales time, pitch, pan and tone convergence together.";
        if (id == IDs::freeze)         return "Suspends convergence and holds the ensemble where it is.";
        if (id == IDs::revConverge)    return "Runs convergence backwards: starts unified and scatters outward towards the target.";
        if (id == IDs::postRelease)    return "What happens at the target: cut dead, sustain the chorus, or scatter back out.";
        if (id == IDs::timeSpread)     return "How far back in time the earliest voices begin, in seconds.";
        if (id == IDs::timeConverge)   return "How tightly the voices line up in time at the climax.";
        if (id == IDs::pitchSpread)    return "Pitch scatter across the swarm, in semitones.";
        if (id == IDs::detune)         return "Fine microdetune between voices, in cents.";
        if (id == IDs::pitchConverge)  return "How strongly the scattered pitches glide back into unison.";
        if (id == IDs::scaleLock)      return "Quantises the pitch scatter to a scale so the cloud stays in key.";
        if (id == IDs::panSpread)      return "Stereo width of the swarm at its widest point.";
        if (id == IDs::panConverge)    return "Negative blooms outward, zero holds still, positive collapses the swarm to the centre.";
        if (id == IDs::toneConverge)   return "How far the filter cutoff of each voice converges on the target's.";
        if (id == IDs::focus)          return "Accelerates convergence near the drop, snapping the swarm into laser focus.";

        // Physics & space
        if (id == IDs::attraction)     return "Strength of the pull towards the target. Higher values converge harder and sooner.";
        if (id == IDs::turbulence)     return "Organic flutter and jitter applied to the voices in flight.";
        if (id == IDs::overshoot)      return "Spring-like overshoot past unison before settling.";
        if (id == IDs::orbit)          return "Orbital stereo circulation of the voices around the centre.";
        if (id == IDs::distance)       return "3D depth staging, from a far cavern wash to an upfront dry impact.";
        if (id == IDs::seed)           return "Deterministic random seed. The same seed always rebuilds the same swarm.";

        // Tone shaping
        if (id == IDs::tail)           return "Length of the swell, in seconds. Disabled while SYNC is on.";
        if (id == IDs::shape)          return "Bends the swell envelope: negative is slow then sudden, positive is immediate then held.";
        if (id == IDs::tone)           return "Low-pass cutoff for the swell. Lower values pull the cloud behind the mix.";
        if (id == IDs::basscut)        return "High-pass cutoff. Clears low end out of the swarm so the drop keeps its weight.";
        if (id == IDs::resonance)      return "Filter Q for the tone and bass-cut filters.";
        if (id == IDs::tilt)           return "Tilt EQ pivot: negative tilts dark, positive tilts bright.";
        if (id == IDs::presence)       return "Presence lift around the vocal intelligibility band.";
        if (id == IDs::air)            return "Filtered high-frequency excitation added to the target and the ensemble.";
        if (id == IDs::space)          return "Ambience and diffusion around the swarm.";
        if (id == IDs::drive)          return "Saturation and soft clipping. Adds density and glue.";
        if (id == IDs::transients)     return "Negative softens attacks, positive preserves punch.";
        if (id == IDs::formant)        return "Shifts vocal formants independently of pitch, in semitones.";
        if (id == IDs::monoBass)       return "High-passes the side channel below this frequency for a solid mono low end.";
        if (id == IDs::ducking)        return "Ducks the swell whenever the live input hits, keeping the source clear.";

        // Mix
        if (id == IDs::dry)            return "Level of the original hit after the target point.";
        if (id == IDs::wet)            return "Level of the swarm before the target point.";
        if (id == IDs::dryReplace)     return "Crossfades the swarm from blending with the dry hit to replacing it.";
        if (id == IDs::align)          return "Uses plugin delay compensation so the impact lands exactly on the note.";
        if (id == IDs::sync)           return "Locks the swell length to the host tempo instead of the LENGTH knob.";
        if (id == IDs::syncLen)        return "Musical length of the swell when SYNC is on.";
        if (id == IDs::sequence)       return "Which incoming notes are allowed to trigger the swarm.";

        // Envelopes
        if (id == IDs::pitch)          return "Pitch sweep across the swell. Negative falls into the target, positive rises.";
        if (id == IDs::pitchRange)     return "Total span of the pitch sweep, in octaves.";
        if (id == IDs::pitchTension)   return "Bends the pitch sweep curve. Drag up or down; double-click to straighten.";
        if (id == IDs::volStart)       return "Swell level at the start. Drag the left handle on the waveform too.";
        if (id == IDs::volEnd)         return "Swell level at the target. Drag the right handle on the waveform too.";
        if (id == IDs::volTension)     return "Bends the volume ramp between the start and end levels.";
        if (id == IDs::trimStart)      return "Trims audio off the front of the source.";
        if (id == IDs::trimEnd)        return "Trims audio off the end of the source.";

        return {};
    }

    // Non-parameter controls
    inline const char* loadButton    () { return "Load a vocal or sample stem from disk. You can also drag a file onto the window."; }
    inline const char* prevButton    () { return "Previous file in the same folder."; }
    inline const char* nextButton    () { return "Next file in the same folder."; }
    inline const char* playButton    () { return "Audition the swell and impact. Spacebar does the same."; }
    inline const char* exportButton  () { return "Export the rendered swell and impact to a 24-bit WAV file."; }
    inline const char* resetButton   () { return "Reset every setting back to its factory default."; }
    inline const char* randomButton  () { return "Randomise for a new sound. After the first press each click resets first, so every result is fresh."; }
    inline const char* regenButton   () { return "Roll a new random seed while keeping all your other settings."; }
    inline const char* helpButton    () { return "Open the manual: every control, the workflow, and the version number."; }
    inline const char* menuButton    () { return "File menu: open, save, save as, export, reset and options."; }
    inline const char* gearButton    () { return "Options: tooltips, MIDI mappings, and audio / MIDI device setup."; }
    inline const char* armButton     () { return "Arm the live capture engine and wait for the input to cross the threshold."; }
    inline const char* captureButton () { return "Start or stop a live capture straight away."; }
    inline const char* lockButton    () { return "Lock the active history slot so new captures do not overwrite it."; }
    inline const char* historySlot   () { return "Capture history slot. Click to make it the swarm's source."; }
    inline const char* presetCombo   () { return "Factory presets. Each one shapes the swarm, tone and convergence only."; }
    inline const char* abButton      () { return "A/B compare. Switch slots to audition two full settings; COPY duplicates the current slot into the other."; }
    inline const char* dragPad       () { return "Drag from here to drop the rendered WAV straight into your DAW."; }
    inline const char* meter         () { return "Output level with peak hold and a numeric peak readout in dB."; }
    inline const char* outputLed     () { return "Output LED: dark when silent, brightening to white as you approach 0 dB, red while over."; }
    inline const char* waveform      () { return "Source waveform. Click to audition, drag the handles to shape the volume ramp."; }
    inline const char* visualizer    () { return "The colony. Left-click 4x to split orbs, right-click 3x to breed a red one, middle-click 2x to start a clutch of eggs. Drag an orb: slowly to mutate, fast to destroy."; }
    inline const char* gravityAdd    () { return "Adds a gravity well: slows one random oscillator and drags the whole colony's rotation down."; }
    inline const char* gravityRelease() { return "Releases the newest gravity well, freeing that oscillator and speeding the colony back up."; }
    inline const char* enzyme        () { return "Speeds everything up and adds three oscillations of a random nature. More than five presses in ten seconds compresses time - the opposite of over-radiating."; }
    inline const char* gamma         () { return "Gamma radiation: shifts the pitch of three random elements at random paces and turns a red orb green. More than five presses in ten seconds time-stretches and granulates the audio."; }
    inline const char* water         () { return "Pours water in: thins the swell a little more each time, twenty pours and it is barely audible. More than five pours in ten seconds leaves it watered down and washed out. RESET clears it."; }
    inline const char* score         () { return "Points for every change you make to the sound. Wrecking orbs too fast docks it, and very fast damage leaves you a fraction."; }
    inline const char* colonyStatus  () { return "Live colony census: orbs, eggs, added oscillations, gravity wells, rate, and the last thing that happened."; }
    inline const char* distressTest  () { return "Hear the talkbox distress call now instead of waiting for ten minutes of silence."; }
}
