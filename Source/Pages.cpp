#include "Pages.h"

#if JucePlugin_Build_Standalone
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

//==============================================================================
//  PCOverlay
//==============================================================================

PCOverlay::PCOverlay (juce::String titleText) : title (std::move (titleText))
{
    setAlwaysOnTop (true);
    setWantsKeyboardFocus (true);
    addAndMakeVisible (closeButton);
    closeButton.onClick = [this] { setVisible (false); };
}

juce::Rectangle<int> PCOverlay::panelBounds() const
{
    return getLocalBounds().reduced (juce::jmax (24, getWidth() / 8),
                                     juce::jmax (24, getHeight() / 12));
}

void PCOverlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.72f));

    auto panel = panelBounds().toFloat();
    PCTheme::dropShadow (g, panel, PCTheme::Metrics::radiusWindow);
    g.setColour (PCTheme::bgBase);
    g.fillRoundedRectangle (panel, PCTheme::Metrics::radiusWindow);
    g.setColour (PCTheme::edge);
    g.drawRoundedRectangle (panel.reduced (0.5f), PCTheme::Metrics::radiusWindow, 1.0f);

    // Title strip, matching the plugin header
    auto strip = panel.withHeight ((float) PCTheme::Metrics::headerHeight);
    g.setColour (PCTheme::panel);
    g.fillRoundedRectangle (strip, PCTheme::Metrics::radiusWindow);
    g.fillRect (strip.withTop (strip.getBottom() - PCTheme::Metrics::radiusWindow));
    g.setColour (PCTheme::textPrimary);
    PCTheme::drawTracked (g, title.toUpperCase(),
                          strip.reduced ((float) PCTheme::Metrics::windowPad, 0.0f),
                          juce::Justification::centredLeft, PCTheme::titleFont (13.0f));
    PCTheme::drawSeparator (g, (int) panel.getX(), (int) panel.getRight(), (int) strip.getBottom());

    // Signature notch + version stamp, exactly as on the main window
    PCTheme::drawSignatureNotch (g, panelBounds(), PCTheme::accent);
    PCTheme::drawVersionStamp (g, panelBounds(), "v" + PreChorusProcessor::getVersionString());
}

void PCOverlay::resized()
{
    auto panel = panelBounds();
    panel.removeFromTop (PCTheme::Metrics::headerHeight);
    panel = panel.reduced (PCTheme::Metrics::windowPad);

    auto footer = panel.removeFromBottom (PCTheme::Metrics::buttonHeight);
    closeButton.setBounds (footer.removeFromRight (120));
    panel.removeFromBottom (PCTheme::Metrics::grid);

    layoutContent (panel);
}

void PCOverlay::mouseDown (const juce::MouseEvent& e)
{
    if (! panelBounds().contains (e.getPosition())) setVisible (false);
}

bool PCOverlay::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey) { setVisible (false); return true; }
    return false;
}

void PCOverlay::visibilityChanged()
{
    if (isVisible()) { toFront (true); grabKeyboardFocus(); }
    else if (auto* parent = getParentComponent()) parent->grabKeyboardFocus();
}

//==============================================================================
//  Help / manual
//==============================================================================

juce::String HelpPage::manualText()
{
    return
    "PRECHORUS  -  32-VOICE SWARM & CONVERGENCE ENGINE\n"
    "Version " + PreChorusProcessor::getVersionString() + "    (c) SheldonDavidson\n"
    "\n"
    "WHAT IT DOES\n"
    "PreChorus builds a cloud of related voices out of one piece of audio and makes that cloud\n"
    "become progressively more recognisable and coherent until it meets the original drop or\n"
    "event. Scattered in time, pitch and space at the start; a single unified hit at the target.\n"
    "\n"
    "-----------------------------------------------------------------------------------------\n"
    "QUICK WORKFLOW\n"
    "-----------------------------------------------------------------------------------------\n"
    "1. Give it audio. Either drag a file onto the window / press LOAD, or set SOURCE to Live\n"
    "   Capture, press ARM and play the part - it records automatically once the input crosses\n"
    "   THRESH. Captures land in one of the eight history slots.\n"
    "2. Pick a starting point from the FACTORY PRESETS dropdown in the header.\n"
    "3. Set the length. Leave SYNC on and choose a musical length, or switch SYNC off and dial\n"
    "   LENGTH in seconds.\n"
    "4. Shape the swarm: VOICES for thickness, TIME/PITCH/PAN SPREAD for how scattered it starts,\n"
    "   the matching CONV controls for how tightly it lands, MACRO to scale all of it at once.\n"
    "5. Press PLAY (or the spacebar) to audition. Click the waveform to retrigger.\n"
    "6. Keep ALIGN (PDC) on so the impact lands exactly on the note, then play the part from your\n"
    "   DAW - incoming MIDI notes trigger the swarm.\n"
    "7. When you like it: EXPORT WAV, or drag from the DRAG TO DAW pad straight into a track.\n"
    "   File > Save Preset As... stores the whole setup for later.\n"
    "\n"
    "-----------------------------------------------------------------------------------------\n"
    "THE GUI, TOP TO BOTTOM\n"
    "-----------------------------------------------------------------------------------------\n"
    "HEADER STRIP - plugin name on the left with the FILE menu and the ? manual button. On the\n"
    "right: the output meter with peak hold, the A / B compare buttons and COPY, the factory\n"
    "preset dropdown, and the gear icon for Options.\n"
    "\n"
    "FILE MENU - Open Preset, Save Preset, Save Preset As, Export Audio to WAV, Reset to\n"
    "Defaults, Open Preset Folder, Options and this manual.\n"
    "\n"
    "SOURCE ROW - source mode and character selector, the file browser (< > LOAD) and the file\n"
    "name of whatever is loaded.\n"
    "\n"
    "CAPTURE ROW - ARM, LIVE CAPTURE, the capture length selector, the eight history slots\n"
    "(orange = active, teal = filled), LOCK, and the target confidence readout.\n"
    "\n"
    "CONSTELLATION - a live view of the voices orbiting and converging on the target. It reacts\n"
    "to the audio, so you can see the swarm tighten as it approaches the impact.\n"
    "\n"
    "WAVEFORM - the rendered swell (accent colour) and the impact (yellow). Click anywhere to\n"
    "audition. Drag the left and right dots to set the start and end level of the swell, and the\n"
    "middle dot to bend the ramp between them. The playhead sweeps through during playback.\n"
    "\n"
    "TRANSPORT ROW - PLAY, EXPORT WAV, DRAG TO DAW, RESET, RANDOM, REGEN seed, FREEZE and\n"
    "REV CONV; on the right the trigger sequence, PDC alignment, SYNC and the sync length.\n"
    "\n"
    "COLONY ROW - ADD GRAVITY, RELEASE GRAVITY, ADD ENZYME, RADIATE and ADD WATER, the score,\n"
    "and a live census of the colony with the last thing that happened to it.\n"
    "\n"
    "KNOB PANELS - six labelled sections, described below.\n"
    "\n"
    "-----------------------------------------------------------------------------------------\n"
    "SOURCE & CAPTURE\n"
    "-----------------------------------------------------------------------------------------\n"
    "SOURCE MODE   Live Capture, Loaded Sample, Hybrid Layer (both at once) or Slice Scatter\n"
    "              (the source is chopped and the fragments are spread across the swarm).\n"
    "CAPTURE MODE  How a capture ends: on input threshold, after a musical length (1/16 up to\n"
    "              2 bars) or manually.\n"
    "THRESH        Input level that starts and ends a threshold capture.\n"
    "ARM           Arms the engine; it waits silently for the input to cross THRESH.\n"
    "LIVE CAPTURE  Starts or stops recording immediately.\n"
    "1 - 8         Capture history. Click a slot to make it the swarm source.\n"
    "LOCK          Protects the active slot; new captures go to the next free slot instead.\n"
    "\n"
    "-----------------------------------------------------------------------------------------\n"
    "SWARM ENGINE\n"
    "-----------------------------------------------------------------------------------------\n"
    "VOICES        1 to 32 voices. The headline control for thickness and width.\n"
    "DENSITY       When the voices join. Negative front-loads them, positive holds them back.\n"
    "VOICE AGE     Darkens and degrades the earliest voices so newer ones sit in front.\n"
    "REVEAL        Progressive reveal. At 0 every voice is the full phrase; at 1 they start as\n"
    "              fragments and grow into the whole phrase as they converge.\n"
    "HUMANIZE      Organic micro-variation in timing, pitch and level.\n"
    "GRAIN MS      Grain length for the Granular Cloud character.\n"
    "DIRECTION     Forward, Reverse, Alternating or Random playback per voice.\n"
    "\n"
    "CHARACTER MODES\n"
    "  Clean Digital     Pristine, transparent interpolation.\n"
    "  Analog Ensemble   Warm saturation, gentle drift, bandwidth contouring.\n"
    "  Bucket-Brigade    Darker repeats, BBD clock roll-off, companding and clock noise.\n"
    "  Tape Choir        Wow and flutter pitch modulation with tape head saturation.\n"
    "  Dimension         Ultra-wide cross-coupled chorusing that keeps a solid mono centre.\n"
    "  String Ensemble   Solina-style multi-rate dual-LFO modulation.\n"
    "  Granular Cloud    Micro-grain cloud with Hann windowing and variable grain size.\n"
    "  Lo-Fi Choral      Vintage bit-depth and sample-rate reduction.\n"
    "\n"
    "-----------------------------------------------------------------------------------------\n"
    "CONVERGENCE & MACRO\n"
    "-----------------------------------------------------------------------------------------\n"
    "MACRO         Scales time, pitch, pan and tone convergence together. One knob for the move.\n"
    "TIME SPREAD   How far back the earliest voices begin, in seconds.\n"
    "TIME CONV     How tightly the voices align at the climax.\n"
    "PITCH SPREAD  Pitch scatter in semitones.\n"
    "DETUNE        Fine microdetune in cents.\n"
    "PITCH CONV    How strongly pitches glide back into unison.\n"
    "PAN SPREAD    Stereo width at the widest point.\n"
    "WIDTH CONV    Negative blooms outward, zero holds, positive collapses to the centre.\n"
    "TONE CONV     How far each voice's cutoff converges on the target's.\n"
    "FOCUS         Accelerates convergence near the drop into a laser focus.\n"
    "SCALE LOCK    Quantises the pitch scatter to Chromatic, Major, Minor, Pentatonic or\n"
    "              Octaves / 5ths so the cloud stays in key.\n"
    "FREEZE        Suspends convergence and holds the ensemble where it is.\n"
    "REV CONV      Runs convergence backwards - starts unified, scatters outward.\n"
    "\n"
    "-----------------------------------------------------------------------------------------\n"
    "PHYSICS & 3D DISTANCE\n"
    "-----------------------------------------------------------------------------------------\n"
    "ATTRACT       Strength of the pull towards the target.\n"
    "TURBULENCE    Organic flutter and jitter in flight.\n"
    "OVERSHOOT     Spring-like overshoot past unison before settling.\n"
    "ORBIT         Orbital stereo circulation around the centre.\n"
    "3D DISTANCE   Depth staging, from a far cavern wash to an upfront dry impact.\n"
    "REGEN         Rolls a new random seed. The same seed always rebuilds the same swarm.\n"
    "\n"
    "-----------------------------------------------------------------------------------------\n"
    "TONE SHAPING, ACOUSTICS & COLOUR\n"
    "-----------------------------------------------------------------------------------------\n"
    "LENGTH        Swell length in seconds (disabled while SYNC is on).\n"
    "SHAPE         Bends the swell envelope - slow then sudden, or immediate then held.\n"
    "TONE          Low-pass cutoff for the swell.\n"
    "BASS CUT      High-pass cutoff, so the drop keeps its weight.\n"
    "RESONANCE     Filter Q for both filters.\n"
    "TILT EQ       Spectral pivot: negative is darker, positive is brighter.\n"
    "PRESENCE      Lift around the vocal intelligibility band.\n"
    "AIR           Filtered high-frequency excitation on the target and the ensemble.\n"
    "SPACE         Ambience and diffusion around the swarm.\n"
    "DRIVE         Saturation and soft clipping for density and glue.\n"
    "TRANSIENTS    Negative softens attacks, positive preserves punch.\n"
    "FORMANT       Shifts vocal formants independently of pitch.\n"
    "MONO BASS     High-passes the side channel below this frequency for a mono low end.\n"
    "\n"
    "-----------------------------------------------------------------------------------------\n"
    "MIX, CAPTURE & DUCK\n"
    "-----------------------------------------------------------------------------------------\n"
    "HIT DRY       Level of the original hit after the target point.\n"
    "SWARM WET     Level of the swarm before the target point.\n"
    "REPLACE       Crossfades the swarm from blending with the dry hit to replacing it.\n"
    "DUCKING       Ducks the swell whenever the live input hits.\n"
    "POST-TARGET   What happens at the impact: Cut at Impact, Sustain Chorus or Scatter Out.\n"
    "\n"
    "-----------------------------------------------------------------------------------------\n"
    "PITCH & VOLUME\n"
    "-----------------------------------------------------------------------------------------\n"
    "PITCH         Pitch sweep across the swell; RANGE sets its span in octaves.\n"
    "BEND          Drag the small curve box to bend the pitch sweep; double-click to straighten.\n"
    "START / END   Swell level at the beginning and at the target.\n"
    "TENSION       Bends the volume ramp between them.\n"
    "\n"
    "-----------------------------------------------------------------------------------------\n"
    "THE COLONY\n"
    "-----------------------------------------------------------------------------------------\n"
    "The constellation is alive. Every orb is one audio element, so what happens to the orbs\n"
    "happens to the sound. All of it can be switched off in Options (Colony Life).\n"
    "\n"
    "CLICK GESTURES (in the constellation)\n"
    "Left-click x4    Three orbs split and replicate.\n"
    "Right-click x3   A red orb is born.\n"
    "Middle-click x2  An orb falls pregnant and lays a clutch of eggs.\n"
    "Click a yellow   Lights a fuse of 40 seconds to 4 minutes. When it blows, the voxbox says\n"
    "                 something urgent at full volume with delay on it - then says it again\n"
    "                 3 to 9 minutes later.\n"
    "The pips along the bottom of the constellation show how far into each gesture you are.\n"
    "\n"
    "DRAGGING AN ORB\n"
    "Slowly           The sound mutates fast and fractals branch off the field.\n"
    "Fast             It destroys other orbs and damages the sound as it goes.\n"
    "\n"
    "COLONY BUTTONS\n"
    "ADD GRAVITY      Slows one random oscillator and drags the whole rotation down with it.\n"
    "RELEASE GRAVITY  Frees the newest gravity well and speeds the colony back up.\n"
    "ADD ENZYME       Speeds everything up, adds three oscillations of a random nature.\n"
    "RADIATE          Gamma: shifts the pitch of three random elements at random paces and\n"
    "                 turns a red orb green for a while.\n"
    "ADD WATER        Thins the swell. Twenty pours and it is barely audible. Each pour throws\n"
    "                 off sparkles - random count, random pitches, arriving anywhere from a\n"
    "                 millisecond to three minutes later. One sparkle in a hundred falls away\n"
    "                 instead, for one to three seconds.\n"
    "\n"
    "PRESSING A BUTTON TOO HARD (more than five times in ten seconds)\n"
    "Water            Watered down and washed out - the top comes off and it smears.\n"
    "Radiate          The audio time-stretches and granulates, dragging between half and three\n"
    "                 times slower. Wears off over 30 minutes.\n"
    "Enzyme           The opposite: time compresses. Also wears off over 30 minutes.\n"
    "\n"
    "RED, GREEN, BLUE, PINK AND YELLOW\n"
    "A green orb hunts the nearest red one. Green meeting red detonates: half the colony and\n"
    "half the added oscillations are lost. Green brushing a plain orb twice turns time around.\n"
    "A blue orb crashing into a pink one dies, and the voxbox has something to say about it.\n"
    "\n"
    "RUNNING BACKWARDS\n"
    "Every twentieth destruction, and every second green-on-plain contact, reverses both the\n"
    "audio and the animation. It never snaps: it slows, stops, hangs there, then winds back up\n"
    "to the same speed going the other way.\n"
    "\n"
    "EGGS AND HATCHLINGS\n"
    "A clutch is 10 orbs one time in ten, 30 one time in four, 20 one time in five, and\n"
    "somewhere between 2 and 88 the rest of the time. A clutch of exactly 20 slows everything\n"
    "to half speed; exactly 30 doubles it; exactly 10 rolls the pace anywhere from three times\n"
    "slower to four times faster. The change comes on slowly.\n"
    "Eggs hatch at random times into coloured orbs. Those orbs whistle at random pitches on\n"
    "their own schedule, live between 5 and 30 minutes, and leave an oscillation between 40 Hz\n"
    "and 10 kHz behind when they go. Each colour has its own pop when it arrives, and the\n"
    "inverse of that pop when it dies.\n"
    "\n"
    "THINGS THAT HAPPEN ON THEIR OWN\n"
    "Every 5 minutes    1 in 4    A randomly coloured orb appears, popping in its own colour.\n"
    "Every 4 minutes    1 in 4    A visitor drops in, stays 4-5 minutes and whizzes out again.\n"
    "                             Every fifth visitor, the voxbox begs for food and shelter.\n"
    "Every 4 minutes    1 in 30   The voxbox babbles urgently at full volume. Everything\n"
    "                             oscillates for 30 seconds and reverberates for a while after.\n"
    "Every 20 minutes   1 in 100  An asteroid. The colony shatters into 100 coloured orbs and\n"
    "                             everything speeds up between 1x and 100x.\n"
    "Every 40 minutes   1 in 40   An orb slowly changes size, between a tenth and four times,\n"
    "                             with a rising tone riding along.\n"
    "\n"
    "THE SCORE\n"
    "Points for every change you make to the sound. Destroying orbs too quickly docks it, and\n"
    "very fast damage leaves you with a fraction of what you had. It means nothing.\n"
    "\n"
    "-----------------------------------------------------------------------------------------\n"
    "THE VOXBOX\n"
    "-----------------------------------------------------------------------------------------\n"
    "A formant-synthesis voice. Every utterance is built fresh - a different throat, pitch and\n"
    "pace each time - so it always sounds like someone saying something, and never twice the\n"
    "same way. One invocation in five comes out a full octave lower.\n"
    "\n"
    "Left alone for 10 minutes it starts calling for help, and keeps calling at random\n"
    "intervals. When it does, the orbs trail ghosts and the swell sags in pitch for a moment.\n"
    "\n"
    "It also speaks when:\n"
    "- a blue orb dies (asks whether you are having fun, and that it is hungry)\n"
    "- a yellow fuse blows (urgent, at full volume, with delay)\n"
    "- every fifth visitor arrives (asks for food and shelter)\n"
    "- an explosion happens (1 in 10, arriving 4 to 30 minutes later)\n"
    "- the RANDOM button was pressed (1 in 3, 40 minutes to 4 hours later, in Spanish)\n"
    "- a preset was loaded (1 in 40, 2 to 10 minutes later, in French)\n"
    "- the plugin was loaded (1 in 30, 20 to 30 minutes later, about ranch and mayo)\n"
    "- a sample was dropped in (1 in 100, 2 to 4 minutes later, something daft)\n"
    "- you right-clicked the constellation (1 in 100, 4 to 9 minutes later, three times over)\n"
    "- the window is closed or minimised (1 in 100 every 8 minutes, twice, with echo)\n"
    "\n"
    "All of it is switchable in Options.\n"
    "\n"
    "-----------------------------------------------------------------------------------------\n"
    "EVERY CONTROL SUPPORTS\n"
    "-----------------------------------------------------------------------------------------\n"
    "Right-click    Opens a menu: reset the control to its default, set it to a specific number,\n"
    "               MIDI Learn (move a controller to bind it), or clear a mapping.\n"
    "Double-click   Resets the control to its default.\n"
    "Drag           Vertical drag for fine control. Hold Shift for coarse, Ctrl for ultra-fine.\n"
    "Hover          Shows the value above the knob and a tooltip explaining what it does.\n"
    "               Tooltips can be switched off in Options.\n"
    "\n"
    "BUTTONS\n"
    "RESET          Puts every setting and function back to its factory default.\n"
    "RANDOM         Randomises for a new sound. After the first press, each further press resets\n"
    "               everything first, so every click gives a genuinely new set of settings.\n"
    "REGEN          Rolls a new seed while keeping your settings.\n"
    "A / B / COPY   Two full snapshots to compare. COPY duplicates the current slot into the other.\n"
    "EXPORT WAV     Writes the rendered swell and impact to a 24-bit WAV file.\n"
    "DRAG TO DAW    Drag from the pad to drop that same WAV straight into your project.\n"
    "\n"
    "-----------------------------------------------------------------------------------------\n"
    "KEYBOARD\n"
    "-----------------------------------------------------------------------------------------\n"
    "Spacebar       Audition the swell and impact.\n"
    "Escape         Close this manual or the Options page.\n"
    "\n"
    "-----------------------------------------------------------------------------------------\n"
    "FACTORY PRESETS\n"
    "-----------------------------------------------------------------------------------------\n"
    "Pop Vocal Double, EDM Riser Swarm, Future Bass Shimmer, Dubstep Chaos Impact, Intimate\n"
    "Whisper Build, Cinematic Choir Pad, Lo-Fi Bedroom Vocal, Ambient Drone Freeze, Aggressive\n"
    "Distortion Drop and Trap Vocal Stutter. Presets shape the swarm, tone and convergence only\n"
    "- they never touch your loaded or captured audio.\n"
    "\n"
    "Your own presets are saved as .pcpreset files and live in\n"
    "Documents / PreChorus / Presets by default. Options has a shortcut to that folder.\n";
}

HelpPage::HelpPage() : PCOverlay ("PreChorus Manual")
{
    body.setMultiLine (true);
    body.setReadOnly (true);
    body.setCaretVisible (false);
    body.setScrollbarsShown (true);
    body.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    body.setColour (juce::TextEditor::textColourId, PCTheme::textPrimary);
    body.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    body.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    body.setFont (PCTheme::monoFont (11.5f));
    body.setText (manualText(), false);
    addAndMakeVisible (body);
}

void HelpPage::layoutContent (juce::Rectangle<int> area)
{
    body.setBounds (area);
}

//==============================================================================
//  Options
//==============================================================================

OptionsPage::OptionsPage (PreChorusProcessor& p) : PCOverlay ("Options"), proc (p)
{
    auto heading = [this] (juce::Label& l, const juce::String& text)
    {
        l.setText (text.toUpperCase(), juce::dontSendNotification);
        l.setFont (PCTheme::headerFont (PCTheme::Metrics::labelSize));
        l.setColour (juce::Label::textColourId, PCTheme::textPrimary);
        addAndMakeVisible (l);
    };
    auto hint = [this] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (PCTheme::valueFont (12.0f));
        l.setColour (juce::Label::textColourId, PCTheme::textMuted);
        l.setJustificationType (juce::Justification::topLeft);
        addAndMakeVisible (l);
    };

    heading (tooltipsHeading, "Hover Tooltips");
    hint (tooltipsHint, "Short explanations appear after a moment when you hover over a control.");
    tooltipsToggle.setClickingTogglesState (true);
    tooltipsToggle.setToggleState (proc.areTooltipsEnabled(), juce::dontSendNotification);
    tooltipsToggle.onClick = [this]
    {
        proc.setTooltipsEnabled (tooltipsToggle.getToggleState());
        tooltipsToggle.setButtonText (tooltipsToggle.getToggleState() ? "Tooltips: On" : "Tooltips: Off");
    };
    tooltipsToggle.setButtonText (proc.areTooltipsEnabled() ? "Tooltips: On" : "Tooltips: Off");
    addAndMakeVisible (tooltipsToggle);

    heading (distressHeading, "Talkbox Distress Call");
    hint (distressHint, "Left untouched for ten minutes, PreChorus calls out - a different voice saying "
                        "something different every time. The orbs trail ghosts and the swell sags in pitch "
                        "while it echoes.");
    distressToggle.setClickingTogglesState (true);
    distressToggle.setToggleState (proc.isDistressEnabled(), juce::dontSendNotification);
    distressToggle.setButtonText (proc.isDistressEnabled() ? "Distress Call: On" : "Distress Call: Off");
    distressToggle.onClick = [this]
    {
        proc.setDistressEnabled (distressToggle.getToggleState());
        distressToggle.setButtonText (distressToggle.getToggleState() ? "Distress Call: On" : "Distress Call: Off");
    };
    addAndMakeVisible (distressToggle);

    colonyLifeToggle.setClickingTogglesState (true);
    colonyLifeToggle.setToggleState (proc.isColonyLifeEnabled(), juce::dontSendNotification);
    colonyLifeToggle.setButtonText (proc.isColonyLifeEnabled() ? "Colony Life: On" : "Colony Life: Off");
    colonyLifeToggle.setTooltip ("Everything the colony does unprompted - asteroids, visitors, "
                                 "spontaneous orbs, resizes, babbling and the delayed outbursts. "
                                 "Switch it off and only the buttons and click gestures act.");
    colonyLifeToggle.onClick = [this]
    {
        proc.setColonyLifeEnabled (colonyLifeToggle.getToggleState());
        colonyLifeToggle.setButtonText (colonyLifeToggle.getToggleState() ? "Colony Life: On" : "Colony Life: Off");
    };
    addAndMakeVisible (colonyLifeToggle);

    distressTestButton.setTooltip ("Hear the distress call now instead of waiting for ten minutes of silence.");
    distressTestButton.onClick = [this] { proc.triggerDistressCall(); };
    addAndMakeVisible (distressTestButton);

    heading (midiHeading, "MIDI Control Mappings");
    mappingList.setMultiLine (true);
    mappingList.setReadOnly (true);
    mappingList.setCaretVisible (false);
    mappingList.setScrollbarsShown (true);
    mappingList.setFont (PCTheme::monoFont (11.0f));
    mappingList.setColour (juce::TextEditor::backgroundColourId, PCTheme::panel);
    mappingList.setColour (juce::TextEditor::textColourId, PCTheme::textMuted);
    mappingList.setColour (juce::TextEditor::outlineColourId, PCTheme::edge);
    mappingList.setColour (juce::TextEditor::focusedOutlineColourId, PCTheme::edge);
    addAndMakeVisible (mappingList);

    clearMappings.onClick = [this] { proc.clearAllMidiMappings(); refreshMappings(); };
    addAndMakeVisible (clearMappings);

    heading (deviceHeading, "Audio & MIDI Devices");
   #if JucePlugin_Build_Standalone
    if (juce::StandalonePluginHolder::getInstance() != nullptr)
        hint (deviceHint, "Choose the audio card, sample rate, buffer size and MIDI inputs used by the "
                          "standalone app.");
    else
   #endif
        hint (deviceHint, "PreChorus is running as a plugin, so the audio card, sample rate and MIDI "
                          "inputs are chosen by your host. Open the standalone app to pick them here.");

    deviceButton.onClick = [this]
    {
       #if JucePlugin_Build_Standalone
        if (auto* holder = juce::StandalonePluginHolder::getInstance())
        {
            holder->showAudioSettingsDialog();
            return;
        }
       #endif
        juce::NativeMessageBox::showMessageBoxAsync (
            juce::MessageBoxIconType::InfoIcon, "Audio / MIDI Device Setup",
            "Device selection belongs to the host when PreChorus runs as a plugin.\n\n"
            "Set the audio card, sample rate, buffer size and MIDI inputs in your DAW's own "
            "audio preferences, or launch the PreChorus standalone app to choose them here.");
    };
    addAndMakeVisible (deviceButton);

    heading (presetHeading, "Presets");
    presetFolderButton.onClick = [] { PreChorusProcessor::getUserPresetFolder().revealToUser(); };
    addAndMakeVisible (presetFolderButton);

    refreshMappings();
    startTimerHz (4);
}

void OptionsPage::timerCallback()
{
    if (isVisible()) refreshMappings();
}

void OptionsPage::refreshMappings()
{
    const auto mappings = proc.getMidiMappings();
    juce::String text;

    if (proc.isLearningAnything())
        text << "Listening... move a controller to bind it.\n\n";

    if (mappings.isEmpty())
    {
        text << "No MIDI mappings yet.\n\n"
                "Right-click any knob, menu or switch and choose MIDI Learn, then move a\n"
                "controller on your hardware to bind it.";
    }
    else
    {
        for (const auto& m : mappings)
        {
            juce::String name = m.second;
            if (auto* p = proc.apvts.getParameter (m.second)) name = p->getName (40);
            text << "CC " << juce::String (m.first).paddedLeft (' ', 3) << "   ->   " << name << "\n";
        }
    }

    if (text != lastMappingText)
    {
        lastMappingText = text;
        mappingList.setText (text, false);
    }
}

void OptionsPage::layoutContent (juce::Rectangle<int> area)
{
    const int g = PCTheme::Metrics::grid;
    const int bh = PCTheme::Metrics::buttonHeight;

    tooltipsHeading.setBounds (area.removeFromTop (16));
    area.removeFromTop (g / 2);
    tooltipsHint.setBounds (area.removeFromTop (18));
    area.removeFromTop (g);
    tooltipsToggle.setBounds (area.removeFromTop (bh).removeFromLeft (180));
    area.removeFromTop (g * 3);

    distressHeading.setBounds (area.removeFromTop (16));
    area.removeFromTop (g / 2);
    distressHint.setBounds (area.removeFromTop (36));
    area.removeFromTop (g);
    {
        auto row = area.removeFromTop (bh);
        distressToggle.setBounds (row.removeFromLeft (200));
        row.removeFromLeft (g);
        distressTestButton.setBounds (row.removeFromLeft (150));
        row.removeFromLeft (g);
        colonyLifeToggle.setBounds (row.removeFromLeft (190));
    }
    area.removeFromTop (g * 3);

    deviceHeading.setBounds (area.removeFromTop (16));
    area.removeFromTop (g / 2);
    deviceHint.setBounds (area.removeFromTop (36));
    area.removeFromTop (g);
    deviceButton.setBounds (area.removeFromTop (bh).removeFromLeft (260));
    area.removeFromTop (g * 3);

    presetHeading.setBounds (area.removeFromTop (16));
    area.removeFromTop (g);
    presetFolderButton.setBounds (area.removeFromTop (bh).removeFromLeft (200));
    area.removeFromTop (g * 3);

    midiHeading.setBounds (area.removeFromTop (16));
    area.removeFromTop (g);
    auto footer = area.removeFromBottom (bh);
    clearMappings.setBounds (footer.removeFromLeft (240));
    area.removeFromBottom (g);
    mappingList.setBounds (area);
}
