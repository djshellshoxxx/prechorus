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
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (12.0f)); }
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;
};

// Animated Multi-Voice Orbital / Constellation Visualizer
class VoiceOrbitVisualizer : public juce::Component, private juce::Timer
{
public:
    explicit VoiceOrbitVisualizer (PreChorusProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); startTimerHz (30); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { phase += 0.02f; repaint(); }
    PreChorusProcessor& proc;
    float phase = 0.0f;
};

// Interactive Waveform Display with Tension Curves and Trim
class WaveformDisplay : public juce::Component, private juce::Timer
{
public:
    explicit WaveformDisplay (PreChorusProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override { rebuild(); }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
private:
    enum class Drag { none, trimEnd, trimStart, volStart, volEnd, volTension };
    void timerCallback() override;
    void rebuild();
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

class TensionBox : public juce::Component, private juce::Timer
{
public:
    TensionBox (PreChorusProcessor& p, const juce::String& id) : proc (p), paramId (id) { startTimerHz (15); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { downT = proc.param (paramId); downY = e.y; }
    void mouseDrag (const juce::MouseEvent& e) override { proc.setParam (paramId, juce::jlimit (-1.0f, 1.0f, downT + (float) (downY - e.y) / 60.0f)); repaint(); }
    void mouseDoubleClick (const juce::MouseEvent&) override { proc.setParam (paramId, 0.0f); repaint(); }
private:
    void timerCallback() override { const float t = proc.param (paramId); if (t != shown) { shown = t; repaint(); } }
    PreChorusProcessor& proc;
    juce::String paramId;
    float downT = 0, shown = -9; int downY = 0;
};

class DragOutPad : public juce::Component
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
private:
    juce::TextEditor body;
    juce::TextButton closeButton { "CLOSE" };
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

    PreChorusProcessor& proc;
    PCLookAndFeel lnf;

    juce::Label title, subtitle, fileLabel, countLabel, syncLabel, rangeLabel, liveLabel;
    juce::TextButton prevButton { "<" }, nextButton { ">" }, loadButton { "LOAD" }, playButton { "PLAY" },
                     exportButton { "EXPORT WAV" }, resetButton { "RESET EDITS" }, randomButton { "RANDOM" }, helpButton { "?" };

    // Live Capture UI
    juce::TextButton captureButton { "LIVE CAPTURE" }, armButton { "ARM" };
    juce::ComboBox captureCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> captureComboAtt;

    // Transport / Sync
    juce::ToggleButton alignToggle { "Hit on note (PDC)" }, syncToggle { "SYNC" };
    juce::ComboBox syncCombo, rangeCombo, harmonyCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> alignAtt, syncAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncComboAtt, rangeComboAtt, harmonyComboAtt;

    WaveformDisplay waveform;
    VoiceOrbitVisualizer visualizer;
    DragOutPad dragPad;
    TensionBox pitchTension;
    HelpOverlay help;

    std::vector<std::unique_ptr<Knob>> knobs;
    Knob *kVoices, *kSpread, *kDetune, *kPanSpread, *kSpace, *kReverse;
    Knob *kTail, *kShape, *kTone, *kBass, *kDry, *kWet, *kThresh;
    Knob *kPitch, *kVolStart, *kVolEnd, *kVolTension;

    std::vector<Group> groups;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreChorusEditor)
};
