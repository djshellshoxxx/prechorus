#pragma once
#include <JuceHeader.h>
#include "Theme.h"
#include "Controls.h"
#include "PluginProcessor.h"

//==============================================================================
/** Shared modal-style overlay: dimmed backdrop, panel, title bar, close button. */
class PCOverlay : public juce::Component
{
public:
    explicit PCOverlay (juce::String titleText);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void visibilityChanged() override;

protected:
    /** Content area inside the panel, below the title bar. */
    virtual void layoutContent (juce::Rectangle<int> area) = 0;
    juce::Rectangle<int> panelBounds() const;

    juce::String title;
    PCTextButton closeButton { "Close" };
};

//==============================================================================
/** The in-plugin manual: every feature, the workflow, the GUI tour, the version. */
class HelpPage : public PCOverlay
{
public:
    HelpPage();
    static juce::String manualText();

protected:
    void layoutContent (juce::Rectangle<int> area) override;

private:
    juce::TextEditor body;
};

//==============================================================================
/** Options: tooltips, MIDI mappings, and audio / MIDI device selection. */
class OptionsPage : public PCOverlay,
                    private juce::Timer
{
public:
    explicit OptionsPage (PreChorusProcessor&);

protected:
    void layoutContent (juce::Rectangle<int> area) override;

private:
    void timerCallback() override;
    void refreshMappings();

    PreChorusProcessor& proc;

    juce::Label tooltipsHeading, tooltipsHint, midiHeading, deviceHeading, deviceHint, presetHeading,
                distressHeading, distressHint;
    PCTextButton tooltipsToggle { "Tooltips: On" };
    PCTextButton distressToggle { "Distress Call: On" };
    PCTextButton colonyLifeToggle { "Colony Life: On" };
    PCTextButton distressTestButton { "Hear It Now" };
    PCTextButton clearMappings  { "Clear All MIDI Mappings" };
    PCTextButton deviceButton   { "Audio / MIDI Device Setup..." };
    PCTextButton presetFolderButton { "Open Preset Folder" };
    juce::TextEditor mappingList;
    juce::String lastMappingText;
};
