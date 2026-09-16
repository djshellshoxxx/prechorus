#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Theme.h"
#include "Controls.h"
#include "Pages.h"

//==============================================================================
//  Plugin-local colour roles, mapped onto the shared visual identity palette.
//  The neutrals, layout and control shapes are identical across the range;
//  only the accents carry meaning inside PreChorus.
//==============================================================================
namespace PCColours
{
    const juce::Colour bg        = PCTheme::bgBase;
    const juce::Colour panel     = PCTheme::panel;
    const juce::Colour panel2    = PCTheme::panelRaise;
    const juce::Colour outline   = PCTheme::edge;
    const juce::Colour text      = PCTheme::textPrimary;
    const juce::Colour textDim   = PCTheme::textMuted;
    const juce::Colour accent    = PCTheme::accent;    // primary  - active values, peaks, selection
    const juce::Colour neon      = PCTheme::accent2;   // secondary- modulation, links between controls
    const juce::Colour hitCol    = PCTheme::warning;   // impact / near-clip
    const juce::Colour recCol    = PCTheme::clipRed;   // recording
    const juce::Colour freezeCol = PCTheme::accent2;   // frozen ensemble

    juce::Colour swellColour (float toneHz, float bassCutHz);
}

//==============================================================================
// Animated 32-Voice Convergence & Orbital Constellation
class VoiceOrbitVisualizer : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    explicit VoiceOrbitVisualizer (PreChorusProcessor& p) : proc (p) { setInterceptsMouseClicks (true, false); startTimerHz (30); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    juce::Point<float> toPixels (float nx, float ny) const;
    juce::Point<float> toNormalised (juce::Point<float> px) const;
    void drawFractal (juce::Graphics&, juce::Point<float> from, float angle, float len,
                      int depth, juce::Colour, float alpha) const;
    PreChorusProcessor& proc;
    float phase = 0.0f;
    float smoothedOut = 0.0f, flashRing = 0.0f;
    juce::Point<float> centre;
    float maxRadius = 1.0f;
    bool dragging = false;
};

//==============================================================================
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
    void mouseMove (const juce::MouseEvent&) override;
    void rebuild();
private:
    enum class Drag { none, trimEnd, trimStart, volStart, volEnd, volTension };
    void timerCallback() override;
    juce::Rectangle<float> plot() const;
    float volY (float level) const;
    PreChorusProcessor& proc;
    std::shared_ptr<const RenderedSample> cached;
    juce::Path swellPath, hitPath;
    int total = 0, hitIndex = -1, lastPlayhead = -2;
    float lastTone = -1, lastBass = -1, lastV0 = -1, lastV1 = -1, lastVT = -9;
    Drag drag = Drag::none, hover = Drag::none;
    juce::Point<float> downPos;
    float downA = 0, downB = 0, downSpan = 1;
    bool moved = false;
};

//==============================================================================
class TensionBox : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    TensionBox (PreChorusProcessor& p, const juce::String& id) : proc (p), paramId (id) { startTimerHz (15); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override { if (! menuOpen) { proc.setParam (paramId, juce::jlimit (-1.0f, 1.0f, downT + (float) (downY - e.y) / 60.0f)); repaint(); } }
    void mouseDoubleClick (const juce::MouseEvent&) override { proc.setParam (paramId, 0.0f); repaint(); }
private:
    void timerCallback() override { const float t = proc.param (paramId); if (t != shown) { shown = t; repaint(); } }
    PreChorusProcessor& proc;
    juce::String paramId;
    float downT = 0, shown = -9; int downY = 0; bool menuOpen = false;
};

//==============================================================================
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

//==============================================================================
/** Small gear glyph button that opens the Options page. */
class GearButton : public juce::Button
{
public:
    GearButton() : juce::Button ("Options") {}
    void paintButton (juce::Graphics&, bool isOver, bool isDown) override;
};

//==============================================================================
class PreChorusEditor : public juce::AudioProcessorEditor,
                        public juce::DragAndDropContainer,
                        public juce::FileDragAndDropTarget,
                        private juce::Timer
{
public:
    explicit PreChorusEditor (PreChorusProcessor&);
    ~PreChorusEditor() override;
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;
    bool keyPressed (const juce::KeyPress& key) override;

private:
    struct Knob
    {
        std::unique_ptr<PCSlider> slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    struct Group { juce::String name; juce::Rectangle<int> bounds; bool separatorRight = false; };

    void timerCallback() override;
    Knob& makeKnob (const juce::String& id, const juce::String& text);
    void layoutKnobs (juce::Rectangle<int> area, std::initializer_list<Knob*> ks);
    std::unique_ptr<PCComboBox> makeCombo (const juce::String& id, const juce::StringArray& items,
                                           std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& att);
    std::unique_ptr<PCToggleButton> makeToggle (const juce::String& id, const juce::String& text,
                                                std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& att);

    void showFileMenu();
    void doOpenPreset();
    void doSavePreset (bool forceSaveAs);
    void doExportWav();
    void refreshTooltipMode();

    PreChorusProcessor& proc;
    PCLookAndFeel lnf;
    std::unique_ptr<juce::TooltipWindow> tooltips;

    // Header strip
    juce::Label title, subtitle;
    PCTextButton menuButton { "File" }, helpButton { "?" };
    GearButton gearButton;
    PCTextButton aButton { "A" }, bButton { "B" }, copyABButton { "Copy" };
    juce::ComboBox presetCombo;
    LevelMeter outputMeter;
    OutputLed outputLed;

    juce::Label fileLabel, countLabel, rangeLabel, confidenceLabel;
    PCTextButton prevButton { "<" }, nextButton { ">" }, loadButton { "Load" }, playButton { "Play" },
                 exportButton { "Export WAV" }, resetButton { "Reset" }, randomButton { "Random" },
                 regenSeedButton { "Regen" };

    // Colony controls
    PCTextButton gravityAddButton { "Add Gravity" }, gravityReleaseButton { "Release Gravity" },
                 enzymeButton { "Add Enzyme" }, gammaButton { "Radiate" }, waterButton { "Add Water" };
    juce::Label colonyStatus, scoreLabel;

    // Live Capture UI & History
    PCTextButton captureButton { "Live Capture" }, armButton { "Arm" }, lockButton { "Lock" };
    std::unique_ptr<PCComboBox> captureCombo, sourceModeCombo, scaleCombo, dirCombo, postReleaseCombo, charCombo, seqCombo, syncCombo, rangeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> captureComboAtt, sourceModeAtt, scaleComboAtt, dirComboAtt, postReleaseAtt, charAtt, seqAtt, syncComboAtt, rangeComboAtt;
    std::array<std::unique_ptr<PCTextButton>, 8> historySlotButtons;

    // Toggles
    std::unique_ptr<PCToggleButton> freezeToggle, revConvergeToggle, alignToggle, syncToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAtt, revConvergeAtt, alignAtt, syncAtt;

    WaveformDisplay waveform;
    VoiceOrbitVisualizer visualizer;
    DragOutPad dragPad;
    TensionBox pitchTension;
    HelpPage help;
    OptionsPage options;

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
    Knob *kDry, *kWet, *kDryReplace, *kThresh, *kDucking;
    // Pitch & Volume Envelopes
    Knob *kPitch, *kVolStart, *kVolEnd, *kVolTension;

    std::vector<Group> groups;
    juce::Rectangle<int> headerStrip, rowSplit;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreChorusEditor)
};
