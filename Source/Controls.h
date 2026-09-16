#pragma once
#include <JuceHeader.h>
#include "Theme.h"
#include "PluginProcessor.h"

//==============================================================================
//  Shared control set. Every control here obeys the visual identity spec and
//  offers the right-click menu (MIDI map / reset / type a value) required of
//  all plugins in the range.
//==============================================================================

class PCSlider;
class PCTextButton;

//==============================================================================
class PCLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PCLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;

    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return PCTheme::labelFont (10.5f); }
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    juce::Font getPopupMenuFont() override { return PCTheme::valueFont (12.0f); }

    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos,
                                           juce::Rectangle<int> parentArea) override;
    void drawTooltip (juce::Graphics&, const juce::String& text, int width, int height) override;

private:
    static juce::TextLayout layoutTooltipText (const juce::String&, juce::Colour, float maxWidth);
};

//==============================================================================
/** Right-click menu shared by every parameter-bound control. */
void showParamContextMenu (juce::Component& owner, PreChorusProcessor& proc, const juce::String& paramID);

//==============================================================================
class PCSlider : public juce::Slider,
                 private juce::Timer
{
public:
    PCSlider (PreChorusProcessor& p, juce::String paramID);

    void mouseDown (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit  (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    float getDisplayPosition() const { return animPos; }
    float getDefaultPosition() const { return defaultPos; }
    bool  isBipolar() const { return bipolar; }
    bool  isHovering() const { return hovering; }
    bool  isLearning() const { return proc.isLearningMidi (paramId); }
    const juce::String& getParamId() const { return paramId; }

protected:
    void valueChanged() override;

private:
    void timerCallback() override;
    void refreshReadout();

    PreChorusProcessor& proc;
    juce::String paramId;
    float animPos = 0.0f, targetPos = 0.0f, defaultPos = 0.0f;
    bool bipolar = false, hovering = false, animInit = false;
};

//==============================================================================
class PCComboBox : public juce::ComboBox
{
public:
    PCComboBox (PreChorusProcessor& p, juce::String paramID) : proc (p), paramId (std::move (paramID)) {}
    void mouseDown (const juce::MouseEvent&) override;

private:
    PreChorusProcessor& proc;
    juce::String paramId;
};

//==============================================================================
class PCToggleButton : public juce::ToggleButton
{
public:
    PCToggleButton (PreChorusProcessor& p, juce::String paramID, const juce::String& text)
        : juce::ToggleButton (text), proc (p), paramId (std::move (paramID)) {}
    void mouseDown (const juce::MouseEvent&) override;

private:
    PreChorusProcessor& proc;
    juce::String paramId;
};

//==============================================================================
/** Text button with the spec's 100 ms accent flash on press. */
class PCTextButton : public juce::TextButton,
                     private juce::Timer
{
public:
    explicit PCTextButton (const juce::String& text = {}) : juce::TextButton (text) {}
    void clicked() override;
    float getFlash() const { return flash; }
    void setAccentColour (juce::Colour c) { accentOverride = c; repaint(); }
    juce::Colour getAccentColour() const { return accentOverride; }

private:
    void timerCallback() override;
    float flash = 0.0f;
    juce::Colour accentOverride = PCTheme::accent;
};

//==============================================================================
/** Output LED: dark grey at -inf, brightening to white as the output approaches
    0 dB, and latching red for as long as the output stays over 0 dB. */
class OutputLed : public juce::Component,
                  public juce::SettableTooltipClient,
                  private juce::Timer
{
public:
    explicit OutputLed (std::function<float()> levelSource);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    std::function<float()> getLevel;
    float brightness = 0.0f;
    bool over0dB = false;
};

//==============================================================================
/** Smooth-gradient meter with 1.5 s peak hold, 20 dB/s fall and a mono readout. */
class LevelMeter : public juce::Component,
                   public juce::SettableTooltipClient,
                   private juce::Timer
{
public:
    LevelMeter (std::function<float()> levelSource, juce::String caption);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    static float dbToNorm (float db) { return juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f); }

    std::function<float()> getLevel;
    juce::String caption;
    float displayNorm = 0.0f, peakNorm = 0.0f, peakDb = -100.0f;
    int holdTicks = 0;
};
