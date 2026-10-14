// PreChorus™
// Copyright © 2026 Sheldon Davidson. All rights reserved.
// Proprietary source code. See LICENSE and COPYRIGHT-TRADEMARK.md.
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

namespace PCColours
{
    const juce::Colour bg       { 0xff0b0e14 };
    const juce::Colour panel    { 0xff151821 };
    const juce::Colour panel2   { 0xff1c202d };
    const juce::Colour outline  { 0xff2a3040 };
    const juce::Colour text     { 0xffe6eaf2 };
    const juce::Colour textDim  { 0xff8992a6 };
    const juce::Colour accent   { 0xffa855f7 }; // Lush choral violet
    const juce::Colour neon     { 0xff06b6d4 }; // Cyber cyan
    const juce::Colour hitCol   { 0xfff59e0b }; // Climax amber
    const juce::Colour recCol   { 0xffef4444 }; // Recording ruby red
    const juce::Colour freezeCol{ 0xff38bdf8 }; // Ice blue

    juce::Colour swellColour (float toneHz, float bassCutHz);
}

class PCLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PCLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (10.5f)); }
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;
};

// Animated 32-Voice Convergence & Orbital Constellation
class VoiceOrbitVisualizer : public juce::Component, private juce::Timer
{
public:
    explicit VoiceOrbitVisualizer (PreChorusProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); startTimerHz (30); }
    void paint (juce::Graphics&) override;
    void setReducedMotion (bool shouldReduce) { reducedMotion = shouldReduce; }
private:
    void timerCallback() override;
    PreChorusProcessor& proc;
    float phase = 0.0f;
    float smoothedOut = 0.0f, flashRing = 0.0f;
    bool reducedMotion = false;
};

// Interactive Waveform Display with Tension Curves and Trim
class WaveformDisplay : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    explicit WaveformDisplay (PreChorusProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override { rebuild(); }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void rebuild();
private:
    enum class Drag { none, volStart, volEnd, volTension };
    static const juce::String& dragParam (Drag d);
    void timerCallback() override;
    juce::Rectangle<float> plot() const;
    float volY (float level) const;
    PreChorusProcessor& proc;
    std::shared_ptr<const RenderedSample> cached;
    juce::Path swellPath, hitPath;
    int total = 0, hitIndex = -1, lastPlayhead = -2;
    float lastTone = -1, lastBass = -1, lastV0 = -1, lastV1 = -1, lastVT = -9;
    Drag drag = Drag::none;
    juce::Point<float> downPos;
    float downA = 0;
};

class TensionBox : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    TensionBox (PreChorusProcessor& p, const juce::String& id) : proc (p), paramId (id) { startTimerHz (15); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { downT = proc.param (paramId); downY = e.y; proc.beginGesture (paramId); }
    void mouseDrag (const juce::MouseEvent& e) override { proc.setParam (paramId, juce::jlimit (-1.0f, 1.0f, downT + (float) (downY - e.y) / 60.0f)); repaint(); }
    void mouseUp (const juce::MouseEvent&) override { proc.endGesture (paramId); }
    void mouseDoubleClick (const juce::MouseEvent&) override { proc.setParam (paramId, 0.0f); repaint(); }
private:
    void timerCallback() override { const float t = proc.param (paramId); if (! juce::exactlyEqual (t, shown)) { shown = t; repaint(); } }
    PreChorusProcessor& proc;
    juce::String paramId;
    float downT = 0, shown = -9; int downY = 0;
};

class DragOutPad : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit DragOutPad (PreChorusProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { over = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { over = false; repaint(); }
private:
    PreChorusProcessor& proc;
    bool dragging = false, over = false;
};

class HelpOverlay : public juce::Component
{
public:
    HelpOverlay();
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override { setVisible (false); }
    bool keyPressed (const juce::KeyPress& k) override { if (k == juce::KeyPress::escapeKey) { setVisible (false); return true; } return false; }
private:
    juce::TextEditor body;
    juce::TextButton closeButton { "CLOSE" };
    juce::HyperlinkButton siteLink { "circuitdriftlabs", juce::URL ("https://djshellshoxxx.github.io/circuitdriftlabs/") };
};

class PreChorusEditor : public juce::AudioProcessorEditor,
                        public juce::DragAndDropContainer,
                        public juce::FileDragAndDropTarget,
                        private juce::Timer
{
public:
    explicit PreChorusEditor (PreChorusProcessor&);
    ~PreChorusEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;
    bool keyPressed (const juce::KeyPress& key) override;

private:
    struct Knob
    {
        juce::Slider slider; juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    struct Group { juce::String name; juce::Rectangle<int> bounds; };

    void timerCallback() override;
    Knob& makeKnob (const juce::String& id, const juce::String& text);
    void layoutKnobs (juce::Rectangle<int> area, std::initializer_list<Knob*> ks);
    void setStatus (const juce::String& text);
    void doUndoRedo (bool redo);
    void applyTooltipSetting();
    void showOptionsMenu();
    void refreshPresetMenu();
    void openHelp();
    void rebuildWaveform() { waveform.rebuild(); }
    static constexpr int kBaseW = 1320, kBaseH = 860;
    juce::Array<juce::File> userPresetFiles;
    int lastPresetId = 0;

    PreChorusProcessor& proc;
    PCLookAndFeel lnf;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    bool tooltipsEnabled = true;

    juce::Label title, subtitle, fileLabel, countLabel, rangeLabel, confidenceLabel, statusLabel, outputLabel;
    juce::TextButton prevButton { "<" }, nextButton { ">" }, loadButton { "LOAD" }, playButton { "PLAY" },
                     exportButton { "EXPORT WAV" }, resetButton { "RESET EDITS" }, randomButton { "RANDOM" },
                     regenSeedButton { "REGEN" }, optionsButton { "OPTIONS" }, helpButton { "?" },
                     abButton { "A" }, abCopyButton { "COPY" }, viewButton { "FULL VIEW" };

    // Live Capture UI & History
    juce::TextButton captureButton { "LIVE CAPTURE" }, armButton { "ARM" }, lockButton { "LOCK" };
    juce::ComboBox captureCombo, sourceModeCombo, scaleCombo, dirCombo, postReleaseCombo, charCombo, seqCombo, presetCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> captureComboAtt, sourceModeAtt, scaleComboAtt, dirComboAtt, postReleaseAtt, charAtt, seqAtt;
    std::array<juce::TextButton, 8> historySlotButtons;

    // Toggles & Alignment
    juce::ToggleButton freezeToggle { "FREEZE" }, revConvergeToggle { "REV CONV" }, alignToggle { "Hit on note (PDC)" }, syncToggle { "SYNC" },
                       reducedMotionToggle { "REDUCED MOTION" }, keytrackToggle { "KEYTRACK" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAtt, revConvergeAtt, alignAtt, syncAtt, keytrackAtt;
    juce::ComboBox syncCombo, rangeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncComboAtt, rangeComboAtt;

    WaveformDisplay waveform;
    VoiceOrbitVisualizer visualizer;
    DragOutPad dragPad;
    TensionBox pitchTension;
    HelpOverlay help;

    std::vector<std::unique_ptr<Knob>> knobs;

    // Swarm Voice Engine
    Knob *kVoiceCount, *kVoiceDensity, *kVoiceAge, *kProgReveal, *kHumanize, *kGrainSize;

    // Convergence Engine & Macro
    Knob *kMacro, *kTimeSpread, *kTimeConverge, *kPitchSpread, *kDetune, *kPitchConverge, *kPanSpread, *kPanConverge, *kToneConverge, *kFocus;

    // Physics & Spatial
    Knob *kAttraction, *kTurbulence, *kOvershoot, *kOrbit, *kDistance;

    // Swell & Tone Shaping
    Knob *kTail, *kShape, *kTone, *kBass, *kResonance, *kTilt, *kPresence, *kAir, *kSpace, *kDrive, *kTransients, *kFormant, *kMonoBass;

    // Mix, Capture & Ducking
    Knob *kDry, *kWet, *kDryReplace, *kThresh, *kDucking, *kStutter;

    // Pitch & Volume Envelopes
    Knob *kPitch, *kVolStart, *kVolEnd, *kVolTension, *kTrimStart, *kTrimEnd;

    std::vector<Group> groups;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreChorusEditor)
};
