#include "Controls.h"

//==============================================================================
//  LookAndFeel
//==============================================================================

PCLookAndFeel::PCLookAndFeel()
{
    setColour (juce::Label::textColourId,                    PCTheme::textPrimary);
    setColour (juce::TextButton::textColourOffId,            PCTheme::textMuted);
    setColour (juce::TextButton::textColourOnId,             PCTheme::accent);
    setColour (juce::TextButton::buttonColourId,             PCTheme::panel);
    setColour (juce::ComboBox::backgroundColourId,           PCTheme::panel);
    setColour (juce::ComboBox::textColourId,                 PCTheme::textPrimary);
    setColour (juce::ComboBox::outlineColourId,              PCTheme::edge);
    setColour (juce::ComboBox::arrowColourId,                PCTheme::accent);
    setColour (juce::PopupMenu::backgroundColourId,          PCTheme::panel);
    setColour (juce::PopupMenu::textColourId,                PCTheme::textPrimary);
    setColour (juce::PopupMenu::headerTextColourId,          PCTheme::textMuted);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, PCTheme::accent.withAlpha (0.15f));
    setColour (juce::PopupMenu::highlightedTextColourId,     PCTheme::accent);
    setColour (juce::Slider::textBoxTextColourId,            PCTheme::textPrimary);
    setColour (juce::Slider::textBoxBackgroundColourId,      juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId,         juce::Colours::transparentBlack);
    setColour (juce::Slider::rotarySliderFillColourId,       PCTheme::accent);
    setColour (juce::TooltipWindow::backgroundColourId,      PCTheme::panel);
    setColour (juce::TooltipWindow::textColourId,            PCTheme::textMuted);
    setColour (juce::TooltipWindow::outlineColourId,         PCTheme::edge);
    setColour (juce::AlertWindow::backgroundColourId,        PCTheme::bgBase);
    setColour (juce::AlertWindow::textColourId,              PCTheme::textPrimary);
    setColour (juce::AlertWindow::outlineColourId,           PCTheme::edge);
    setColour (juce::TextEditor::backgroundColourId,         PCTheme::panel);
    setColour (juce::TextEditor::textColourId,               PCTheme::textPrimary);
    setColour (juce::TextEditor::outlineColourId,            PCTheme::edge);
    setColour (juce::TextEditor::focusedOutlineColourId,     PCTheme::accent);
    setColour (juce::TextEditor::highlightColourId,          PCTheme::accent.withAlpha (0.3f));
    setColour (juce::CaretComponent::caretColourId,          PCTheme::accent);
    setColour (juce::ScrollBar::thumbColourId,               PCTheme::edge.brighter (0.2f));
    setColour (juce::ResizableWindow::backgroundColourId,    PCTheme::bgBase);
}

void PCLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                      float pos, float startAngle, float endAngle, juce::Slider& s)
{
    auto* pc = dynamic_cast<PCSlider*> (&s);
    if (pc != nullptr) pos = pc->getDisplayPosition();     // 80 ms ease-out on every value change

    const bool hovering = pc != nullptr ? pc->isHovering() : s.isMouseOverOrDragging();
    const float brighten = hovering ? 0.08f : 0.0f;        // hover brightens ~8%

    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat();
    const auto centre = bounds.getCentre();
    const float outer = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;

    const float arcRadius  = outer - PCTheme::Metrics::arcThickness * 0.5f;
    const float knobRadius = arcRadius - PCTheme::Metrics::arcThickness * 0.5f - PCTheme::Metrics::arcGap;
    if (knobRadius <= 2.0f) return;

    const float currentAngle = startAngle + pos * (endAngle - startAngle);
    const juce::Colour fill = s.findColour (juce::Slider::rotarySliderFillColourId).brighter (brighten);

    // --- value arc, drawn outside the body with a 4px gap -------------------
    juce::Path bgArc;
    bgArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (PCTheme::edge.withAlpha (0.6f));
    g.strokePath (bgArc, juce::PathStrokeType (PCTheme::Metrics::arcThickness,
                                               juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float anchorPos = (pc != nullptr && pc->isBipolar()) ? 0.5f : 0.0f;
    const float anchorAngle = startAngle + anchorPos * (endAngle - startAngle);
    if (std::abs (currentAngle - anchorAngle) > 0.005f)
    {
        juce::Path valArc;
        valArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                              juce::jmin (anchorAngle, currentAngle),
                              juce::jmax (anchorAngle, currentAngle), true);
        g.setColour (fill);
        g.strokePath (valArc, juce::PathStrokeType (PCTheme::Metrics::arcThickness,
                                                    juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // --- flat-shaded body ---------------------------------------------------
    auto body = juce::Rectangle<float> (centre.x - knobRadius, centre.y - knobRadius,
                                        knobRadius * 2.0f, knobRadius * 2.0f);
    juce::ColourGradient grad (PCTheme::knobTop.brighter (brighten),    centre.x, body.getY(),
                               PCTheme::knobBottom.brighter (brighten), centre.x, body.getBottom(), false);
    g.setGradientFill (grad);
    g.fillEllipse (body);
    g.setColour (PCTheme::edge);
    g.drawEllipse (body.reduced (0.5f), 1.0f);

    // --- indicator: single 2px accent line, centre to rim, rounded cap ------
    juce::Path ind;
    ind.startNewSubPath (centre.x + std::sin (currentAngle) * (knobRadius * 0.20f),
                         centre.y - std::cos (currentAngle) * (knobRadius * 0.20f));
    ind.lineTo (centre.x + std::sin (currentAngle) * (knobRadius * 0.92f),
                centre.y - std::cos (currentAngle) * (knobRadius * 0.92f));
    g.setColour (fill);
    g.strokePath (ind, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // --- 4px centre dot, muted while the control sits at its default --------
    const bool atDefault = pc != nullptr && std::abs (pos - pc->getDefaultPosition()) < 0.002f;
    g.setColour (atDefault ? PCTheme::textMuted.withAlpha (0.55f) : fill);
    g.fillEllipse (centre.x - 2.0f, centre.y - 2.0f, 4.0f, 4.0f);

    // --- MIDI learn armed ---------------------------------------------------
    if (pc != nullptr && pc->isLearning())
    {
        g.setColour (PCTheme::warning);
        g.drawEllipse (body.expanded (PCTheme::Metrics::arcGap), 1.0f);
    }
}

void PCLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                                      float sliderPos, float, float,
                                      juce::Slider::SliderStyle style, juce::Slider& s)
{
    const bool vertical = (style == juce::Slider::LinearVertical || style == juce::Slider::LinearBarVertical);
    auto area = juce::Rectangle<int> (x, y, w, h).toFloat();
    const juce::Colour fill = s.findColour (juce::Slider::rotarySliderFillColourId);

    // 4px track with rounded ends
    auto track = vertical ? juce::Rectangle<float> (area.getCentreX() - 2.0f, area.getY(), 4.0f, area.getHeight())
                          : juce::Rectangle<float> (area.getX(), area.getCentreY() - 2.0f, area.getWidth(), 4.0f);
    g.setColour (PCTheme::edge);
    g.fillRoundedRectangle (track, 2.0f);

    auto filled = track;
    if (vertical) filled = filled.withTop (sliderPos);
    else          filled = filled.withRight (sliderPos);
    g.setColour (fill);
    g.fillRoundedRectangle (filled, 2.0f);

    // 16x24 rounded-rect thumb with the knob body gradient and a 1px accent stroke
    const float tw = vertical ? 24.0f : 16.0f;
    const float th = vertical ? 16.0f : 24.0f;
    auto thumb = vertical ? juce::Rectangle<float> (area.getCentreX() - tw * 0.5f, sliderPos - th * 0.5f, tw, th)
                          : juce::Rectangle<float> (sliderPos - tw * 0.5f, area.getCentreY() - th * 0.5f, tw, th);
    juce::ColourGradient grad (PCTheme::knobTop, thumb.getCentreX(), thumb.getY(),
                               PCTheme::knobBottom, thumb.getCentreX(), thumb.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (thumb, PCTheme::Metrics::radiusPanel);
    g.setColour (fill);
    g.drawRoundedRectangle (thumb.reduced (0.5f), PCTheme::Metrics::radiusPanel, 1.0f);
}

void PCLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                          bool isOver, bool isDown)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);

    juce::Colour accentCol = PCTheme::accent;
    float flash = 0.0f;
    if (auto* pb = dynamic_cast<PCTextButton*> (&b)) { accentCol = pb->getAccentColour(); flash = pb->getFlash(); }
    if (b.isColourSpecified (juce::TextButton::buttonOnColourId))
        accentCol = b.findColour (juce::TextButton::buttonOnColourId);

    const bool on = b.getToggleState();
    juce::Colour fill = on ? accentCol.withAlpha (0.15f) : PCTheme::panel;
    if (isDown)      fill = fill.brighter (0.16f);
    else if (isOver) fill = fill.brighter (0.08f);
    if (flash > 0.0f) fill = fill.interpolatedWith (accentCol.withAlpha (0.45f), flash);

    g.setColour (fill);
    g.fillRoundedRectangle (r, PCTheme::Metrics::radiusPanel);
    g.setColour (on || flash > 0.0f ? accentCol : PCTheme::edge);
    g.drawRoundedRectangle (r, PCTheme::Metrics::radiusPanel, 1.0f);
}

void PCLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    juce::Colour accentCol = PCTheme::accent;
    float flash = 0.0f;
    if (auto* pb = dynamic_cast<PCTextButton*> (&b)) { accentCol = pb->getAccentColour(); flash = pb->getFlash(); }
    if (b.isColourSpecified (juce::TextButton::buttonOnColourId))
        accentCol = b.findColour (juce::TextButton::buttonOnColourId);

    juce::Colour text = b.getToggleState() ? accentCol : PCTheme::textMuted;
    if (b.isMouseOver (true)) text = text.brighter (0.25f);
    if (flash > 0.0f) text = text.interpolatedWith (accentCol, flash);
    if (! b.isEnabled()) text = text.withAlpha (0.4f);

    g.setColour (text);
    PCTheme::drawTracked (g, b.getButtonText().toUpperCase(), b.getLocalBounds().toFloat().reduced (6.0f, 0.0f),
                          juce::Justification::centred, PCTheme::headerFont (10.0f));
}

void PCLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool isOver, bool isDown)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();

    juce::Colour fill = on ? PCTheme::accent.withAlpha (0.15f) : PCTheme::panel;
    if (isDown)      fill = fill.brighter (0.16f);
    else if (isOver) fill = fill.brighter (0.08f);

    g.setColour (fill);
    g.fillRoundedRectangle (r, PCTheme::Metrics::radiusPanel);
    g.setColour (on ? PCTheme::accent : PCTheme::edge);
    g.drawRoundedRectangle (r, PCTheme::Metrics::radiusPanel, 1.0f);

    juce::Colour text = on ? PCTheme::accent : PCTheme::textMuted;
    if (isOver) text = text.brighter (0.25f);
    g.setColour (text);
    PCTheme::drawTracked (g, b.getButtonText().toUpperCase(), r.reduced (6.0f, 0.0f),
                          juce::Justification::centred, PCTheme::headerFont (10.0f));
}

juce::Font PCLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return PCTheme::headerFont (10.0f);
}

juce::Label* PCLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (PCTheme::monoFont (10.5f));                 // tabular numeric readout
    l->setJustificationType (juce::Justification::centred);
    // Let hover and the vertical-resize cursor belong to the knob, but keep the
    // editor clickable once right-click > Set Value has opened it.
    l->setInterceptsMouseClicks (false, true);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundWhenEditingColourId, PCTheme::panel);
    l->setColour (juce::Label::textWhenEditingColourId, PCTheme::textPrimary);
    l->setColour (juce::Label::outlineWhenEditingColourId, PCTheme::accent);
    return l;
}

void PCLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool isDown, int, int, int, int, juce::ComboBox& b)
{
    auto r = juce::Rectangle<int> (0, 0, w, h).toFloat().reduced (0.5f);
    juce::Colour fill = PCTheme::panel;
    if (isDown)                   fill = fill.brighter (0.16f);
    else if (b.isMouseOver (true))fill = fill.brighter (0.08f);

    g.setColour (fill);
    g.fillRoundedRectangle (r, PCTheme::Metrics::radiusPanel);
    g.setColour (isDown ? PCTheme::accent : PCTheme::edge);
    g.drawRoundedRectangle (r, PCTheme::Metrics::radiusPanel, 1.0f);

    juce::Path arrow;
    const float ax = (float) w - 11.0f;
    const float ay = (float) h * 0.5f;
    arrow.startNewSubPath (ax - 3.5f, ay - 2.0f);
    arrow.lineTo (ax, ay + 2.0f);
    arrow.lineTo (ax + 3.5f, ay - 2.0f);
    g.setColour (PCTheme::accent);
    g.strokePath (arrow, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void PCLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 24, box.getHeight() - 2);
    label.setFont (PCTheme::labelFont (10.5f));
    label.setColour (juce::Label::textColourId, PCTheme::textPrimary);
}

void PCLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
    g.setColour (PCTheme::panel);
    g.fillRoundedRectangle (r, PCTheme::Metrics::radiusPanel);
    g.setColour (PCTheme::edge);
    g.drawRoundedRectangle (r, PCTheme::Metrics::radiusPanel, 1.0f);
}

juce::TextLayout PCLookAndFeel::layoutTooltipText (const juce::String& text, juce::Colour colour, float maxWidth)
{
    juce::AttributedString s;
    s.setJustification (juce::Justification::centredLeft);
    s.append (text, PCTheme::valueFont (12.0f), colour);

    juce::TextLayout tl;
    tl.createLayout (s, maxWidth);
    return tl;
}

juce::Rectangle<int> PCLookAndFeel::getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos,
                                                      juce::Rectangle<int> parentArea)
{
    const auto tl = layoutTooltipText (tipText, PCTheme::textMuted, 280.0f);
    const int w = (int) (tl.getWidth() + 20.0f);
    const int h = (int) (tl.getHeight() + 14.0f);

    return juce::Rectangle<int> (screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 18,
                                 screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6)  : screenPos.y + 6,
                                 w, h).constrainedWithin (parentArea);
}

void PCLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    // Dark pill, muted text.
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
    g.setColour (PCTheme::bgBase.withAlpha (0.97f));
    g.fillRoundedRectangle (r, (float) height * 0.5f);
    g.setColour (PCTheme::edge);
    g.drawRoundedRectangle (r, (float) height * 0.5f, 1.0f);

    layoutTooltipText (text, PCTheme::textMuted, (float) width - 20.0f)
        .draw (g, juce::Rectangle<float> (10.0f, 7.0f, (float) width - 20.0f, (float) height - 14.0f));
}

//==============================================================================
//  Right-click parameter menu
//==============================================================================

namespace
{
    enum ParamMenuIds
    {
        miReset = 1,
        miTypeValue,
        miLearn,
        miClearOne,
        miClearAll,
        miChoiceBase = 100
    };

    void addSharedItems (juce::PopupMenu& m, PreChorusProcessor& proc, const juce::String& paramID)
    {
        const int cc = proc.getMidiCcForParam (paramID);
        m.addSeparator();
        m.addItem (miLearn, proc.isLearningMidi (paramID)
                                ? juce::String ("Cancel MIDI Learn")
                                : (cc >= 0 ? "Re-learn MIDI  (now CC " + juce::String (cc) + ")"
                                           : juce::String ("MIDI Learn - move a controller")));
        m.addItem (miClearOne, "Clear MIDI Mapping", cc >= 0);
        m.addItem (miClearAll, "Clear All MIDI Mappings");
    }

    void handleSharedResult (int result, PreChorusProcessor& proc, const juce::String& paramID)
    {
        switch (result)
        {
            case miLearn:    proc.isLearningMidi (paramID) ? proc.cancelMidiLearn() : proc.beginMidiLearn (paramID); break;
            case miClearOne: proc.clearMidiMappingFor (paramID); break;
            case miClearAll: proc.clearAllMidiMappings(); break;
            default: break;
        }
    }

    void resetParamToDefault (PreChorusProcessor& proc, const juce::String& paramID)
    {
        if (auto* p = proc.apvts.getParameter (paramID))
            p->setValueNotifyingHost (p->getDefaultValue());
    }
}

void showParamContextMenu (juce::Component& owner, PreChorusProcessor& proc, const juce::String& paramID)
{
    auto* param = proc.apvts.getParameter (paramID);
    if (param == nullptr) return;

    juce::PopupMenu m;
    m.setLookAndFeel (&owner.getLookAndFeel());
    m.addSectionHeader (param->getName (40));
    m.addItem (miReset, "Reset to Default");

    auto* slider = dynamic_cast<juce::Slider*> (&owner);
    const auto choices = param->getAllValueStrings();
    const bool isChoice = slider == nullptr && choices.size() > 1;

    if (slider != nullptr)
    {
        m.addItem (miTypeValue, "Set Value...");
    }
    else if (isChoice)
    {
        juce::PopupMenu sub;
        const int current = juce::roundToInt (param->convertFrom0to1 (param->getValue()));
        for (int i = 0; i < choices.size(); ++i)
            sub.addItem (miChoiceBase + i, choices[i], true, i == current);
        m.addSubMenu ("Set Value", sub);
    }

    addSharedItems (m, proc, paramID);

    juce::Component::SafePointer<juce::Component> safeOwner (&owner);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&owner),
        [safeOwner, &proc, paramID, param, choices] (int result)
        {
            if (result == 0) return;

            if (result == miReset)
            {
                resetParamToDefault (proc, paramID);
            }
            else if (result == miTypeValue)
            {
                if (auto* s = dynamic_cast<juce::Slider*> (safeOwner.getComponent()))
                {
                    s->setColour (juce::Slider::textBoxTextColourId, PCTheme::textPrimary);
                    s->showTextBox();
                }
            }
            else if (result >= miChoiceBase && result < miChoiceBase + choices.size())
            {
                const int idx = result - miChoiceBase;
                param->beginChangeGesture();
                param->setValueNotifyingHost (choices.size() > 1 ? (float) idx / (float) (choices.size() - 1) : 0.0f);
                param->endChangeGesture();
            }
            else
            {
                handleSharedResult (result, proc, paramID);
            }

            if (auto* c = safeOwner.getComponent()) c->repaint();
        });
}

//==============================================================================
//  PCSlider
//==============================================================================

PCSlider::PCSlider (PreChorusProcessor& p, juce::String paramID)
    : proc (p), paramId (std::move (paramID))
{
    // Set these before setTextBoxStyle: the readout Label copies them at creation
    // time, which is long before this slider is parented to the editor's LookAndFeel.
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, PCTheme::accent.withAlpha (0.3f));
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::transparentBlack);

    setSliderStyle (juce::Slider::RotaryVerticalDrag);      // vertical drag for fine control
    setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                         juce::MathConstants<float>::pi * 2.75f, true);   // 270 degree sweep
    setTextBoxStyle (juce::Slider::TextBoxAbove, false, 68, 13);
    setColour (juce::Slider::rotarySliderFillColourId, PCTheme::accent);
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    setVelocityBasedMode (false);
    setMouseDragSensitivity (250);

    if (auto* rp = proc.apvts.getParameter (paramId))
    {
        defaultPos = rp->getDefaultValue();
        animPos = targetPos = rp->getValue();
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (rp))
        {
            const auto& range = ranged->getNormalisableRange();
            bipolar = range.start < 0.0f && range.end > 0.0f;
        }
    }
    animInit = true;
}

void PCSlider::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        showParamContextMenu (*this, proc, paramId);
        return;
    }

    // Shift = coarse, Ctrl/Cmd = ultra-fine, plain vertical drag = fine.
    setMouseDragSensitivity (e.mods.isShiftDown() ? 70 : (e.mods.isCommandDown() ? 1600 : 250));
    juce::Slider::mouseDown (e);
    refreshReadout();
}

void PCSlider::mouseEnter (const juce::MouseEvent& e)
{
    hovering = true;
    refreshReadout();
    juce::Slider::mouseEnter (e);
}

void PCSlider::mouseExit (const juce::MouseEvent& e)
{
    hovering = false;
    refreshReadout();
    juce::Slider::mouseExit (e);
}

void PCSlider::mouseDoubleClick (const juce::MouseEvent&)
{
    resetParamToDefault (proc, paramId);      // double-click resets to default
}

void PCSlider::valueChanged()
{
    juce::Slider::valueChanged();
    targetPos = (float) valueToProportionOfLength (getValue());
    if (! animInit) { animPos = targetPos; animInit = true; }
    if (! isTimerRunning()) startTimerHz (60);
}

void PCSlider::timerCallback()
{
    const float next = PCTheme::easeTowards (animPos, targetPos, 60);
    if (std::abs (next - animPos) > 1.0e-5f) { animPos = next; repaint(); }
    else { animPos = targetPos; repaint(); stopTimer(); }
}

void PCSlider::refreshReadout()
{
    // Value readout above the knob is only visible on hover or during a drag.
    const bool show = hovering || isMouseButtonDown();
    setColour (juce::Slider::textBoxTextColourId, show ? PCTheme::textPrimary : juce::Colours::transparentBlack);
    repaint();
}

//==============================================================================
//  PCComboBox / PCToggleButton / PCTextButton
//==============================================================================

void PCComboBox::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { showParamContextMenu (*this, proc, paramId); return; }
    juce::ComboBox::mouseDown (e);
}

void PCToggleButton::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { showParamContextMenu (*this, proc, paramId); return; }
    juce::ToggleButton::mouseDown (e);
}

void PCTextButton::clicked()
{
    flash = 1.0f;                       // momentary presses show a brief 100 ms accent flash
    startTimerHz (60);
    repaint();
    juce::TextButton::clicked();
}

void PCTextButton::timerCallback()
{
    flash -= 1.0f / 6.0f;               // 6 frames at 60 Hz = 100 ms
    if (flash <= 0.0f) { flash = 0.0f; stopTimer(); }
    repaint();
}

//==============================================================================
//  OutputLed
//==============================================================================

OutputLed::OutputLed (std::function<float()> levelSource) : getLevel (std::move (levelSource))
{
    setInterceptsMouseClicks (true, false);
    startTimerHz (30);
}

void OutputLed::timerCallback()
{
    const float lin = getLevel != nullptr ? juce::jmax (0.0f, getLevel()) : 0.0f;
    const float db = juce::Decibels::gainToDecibels (lin, -60.0f);

    over0dB = db >= 0.0f;                                   // latches until the output drops back under 0 dB

    const float target = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
    const float next = brightness + (target - brightness) * (target > brightness ? 0.6f : 0.15f);
    if (std::abs (next - brightness) > 0.002f || over0dB != (brightness >= 1.0f))
    {
        brightness = next;
        repaint();
    }
}

void OutputLed::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float d = juce::jmin (r.getWidth(), r.getHeight());
    auto lamp = juce::Rectangle<float> (d, d).withCentre (r.getCentre());

    // Unlit is dark grey; it brightens all the way to white as the level nears 0 dB.
    const juce::Colour unlit { 0xff2A303A };
    const juce::Colour lit = over0dB ? PCTheme::clipRed
                                     : unlit.interpolatedWith (juce::Colours::white, brightness);

    const float glow = over0dB ? 1.0f : brightness;
    if (glow > 0.05f)
    {
        g.setColour (lit.withAlpha (0.22f * glow));
        g.fillEllipse (lamp.expanded (d * 0.55f * glow));
    }

    g.setColour (lit);
    g.fillEllipse (lamp);
    g.setColour (PCTheme::bgBase.withAlpha (0.6f));
    g.drawEllipse (lamp.reduced (0.5f), 1.0f);
}

//==============================================================================
//  LevelMeter
//==============================================================================

LevelMeter::LevelMeter (std::function<float()> levelSource, juce::String cap)
    : getLevel (std::move (levelSource)), caption (std::move (cap))
{
    startTimerHz (30);
}

void LevelMeter::timerCallback()
{
    const float lin = getLevel != nullptr ? juce::jlimit (0.0f, 2.0f, getLevel()) : 0.0f;
    const float db = juce::Decibels::gainToDecibels (lin, -60.0f);
    const float norm = dbToNorm (db);

    displayNorm += (norm - displayNorm) * (norm > displayNorm ? 0.7f : 0.25f);

    if (norm >= peakNorm)
    {
        peakNorm = norm;
        peakDb = db;
        holdTicks = 45;                                 // 1.5 s hold at 30 Hz
    }
    else if (holdTicks > 0)
    {
        --holdTicks;
    }
    else
    {
        peakDb -= 20.0f / 30.0f;                        // then falls at 20 dB/s
        peakDb = juce::jmax (peakDb, -60.0f);
        peakNorm = dbToNorm (peakDb);
    }
    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    auto readout = r.removeFromRight (46);
    r.removeFromRight (6);

    auto bar = r.toFloat();
    g.setColour (PCTheme::bgBase);
    g.fillRoundedRectangle (bar, PCTheme::Metrics::radiusSmall);
    g.setColour (PCTheme::edge);
    g.drawRoundedRectangle (bar.reduced (0.5f), PCTheme::Metrics::radiusSmall, 1.0f);

    auto inner = bar.reduced (1.5f);
    if (displayNorm > 0.001f)
    {
        // Segmented look, rendered smoothly: a continuous gradient, no LED gaps.
        juce::ColourGradient grad (PCTheme::meterColour (0.0f), inner.getX(), 0.0f,
                                   PCTheme::meterColour (1.0f), inner.getRight(), 0.0f, false);
        grad.addColour (0.45, PCTheme::meterColour (0.45f));
        grad.addColour (0.80, PCTheme::meterColour (0.80f));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (inner.withWidth (inner.getWidth() * displayNorm), PCTheme::Metrics::radiusSmall);
    }

    if (peakNorm > 0.005f)
    {
        const float px = inner.getX() + inner.getWidth() * peakNorm;
        g.setColour (PCTheme::meterColour (peakNorm));
        g.drawLine (px, inner.getY(), px, inner.getBottom(), 1.0f);
    }

    g.setColour (peakDb > -1.0f ? PCTheme::clipRed : PCTheme::textMuted);
    g.setFont (PCTheme::monoFont (9.5f));
    g.drawText (peakDb <= -59.5f ? juce::String ("-inf") : juce::String (peakDb, 1),
                readout, juce::Justification::centredRight, false);
}
