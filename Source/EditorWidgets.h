// PreChorus™
// Copyright © 2026 Sheldon Davidson. All rights reserved.
// Proprietary source code. See LICENSE and COPYRIGHT-TRADEMARK.md.
// SPDX-License-Identifier: LicenseRef-Proprietary

// Editor building blocks: keyboard/MIDI-learn slider, 16-step stutter editor, swarm view and the preset browser.
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PresetLibrary.h"

namespace PCColours
{
    // Mutable so the editor can switch to a high-contrast palette (accessibility).
    inline juce::Colour bg       { 0xff0b0e14 };
    inline juce::Colour panel    { 0xff151821 };
    inline juce::Colour panel2   { 0xff1c202d };
    inline juce::Colour outline  { 0xff2a3040 };
    inline juce::Colour text     { 0xffe6eaf2 };
    inline juce::Colour textDim  { 0xff8992a6 };
    inline juce::Colour accent   { 0xffa855f7 }; // Lush choral violet
    inline juce::Colour neon     { 0xff06b6d4 }; // Cyber cyan
    inline juce::Colour hitCol   { 0xfff59e0b }; // Climax amber
    inline juce::Colour recCol   { 0xffef4444 }; // Recording ruby red
    inline juce::Colour freezeCol{ 0xff38bdf8 }; // Ice blue

    inline void applyHighContrast (bool on)
    {
        if (on)
        {
            bg = juce::Colour (0xff000000); panel = juce::Colour (0xff05060a); panel2 = juce::Colour (0xff141824);
            outline = juce::Colour (0xff9aa4bc); text = juce::Colour (0xffffffff); textDim = juce::Colour (0xffd2d8e6);
            accent = juce::Colour (0xffd9a0ff); neon = juce::Colour (0xff4fe3ff); hitCol = juce::Colour (0xffffc14d);
            recCol = juce::Colour (0xffff7070); freezeCol = juce::Colour (0xff7fd4ff);
        }
        else
        {
            bg = juce::Colour (0xff0b0e14); panel = juce::Colour (0xff151821); panel2 = juce::Colour (0xff1c202d);
            outline = juce::Colour (0xff2a3040); text = juce::Colour (0xffe6eaf2); textDim = juce::Colour (0xff8992a6);
            accent = juce::Colour (0xffa855f7); neon = juce::Colour (0xff06b6d4); hitCol = juce::Colour (0xfff59e0b);
            recCol = juce::Colour (0xffef4444); freezeCol = juce::Colour (0xff38bdf8);
        }
    }
}

// Rotary slider with keyboard control (arrows, PageUp/Down, Home/End) and a right-click hook for MIDI learn.
class PCSlider : public juce::Slider
{
public:
    std::function<void (const juce::MouseEvent&)> onContextClick;

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { if (onContextClick) onContextClick (e); return; }
        juce::Slider::mouseDown (e);
    }

    bool keyPressed (const juce::KeyPress& k) override
    {
        const double lo = getMinimum(), hi = getMaximum();
        const double range = hi - lo;
        double step = getInterval() > 0.0 ? getInterval() : range / 100.0;
        step = juce::jmax (step, range / 200.0);
        if (k.getModifiers().isShiftDown()) step = juce::jmax (getInterval(), range / 1000.0);
        const int code = k.getKeyCode();
        double v = getValue();
        if (code == juce::KeyPress::upKey || code == juce::KeyPress::rightKey) v += step;
        else if (code == juce::KeyPress::downKey || code == juce::KeyPress::leftKey) v -= step;
        else if (code == juce::KeyPress::pageUpKey) v += range * 0.1;
        else if (code == juce::KeyPress::pageDownKey) v -= range * 0.1;
        else if (code == juce::KeyPress::homeKey) v = lo;
        else if (code == juce::KeyPress::endKey) v = hi;
        else return false;                     // let Space / Esc / R / G / B / H reach the editor
        setValue (juce::jlimit (lo, hi, v), juce::sendNotificationSync);
        return true;
    }
};

// 16-step pattern for the Custom stutter grid (bit 15 = the last step before the hit).
class StepPatternEditor : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    explicit StepPatternEditor (PreChorusProcessor& p) : proc (p)
    {
        setWantsKeyboardFocus (true);
        setTitle ("Stutter step pattern");
        setDescription ("16 steps. Arrow keys move, Return toggles a step. Used when the stutter grid is Custom.");
        startTimerHz (15);
    }

    void paint (juce::Graphics& g) override
    {
        const int pattern = (int) proc.param (IDs::stutterPattern);
        const bool active = (int) proc.param (IDs::stutterGrid) == 3;
        auto r = getLocalBounds().toFloat();
        const float cw = r.getWidth() / 16.0f;
        for (int i = 0; i < 16; ++i)
        {
            const int bit = i;                          // cell i (left = earliest) <-> bit i; bit 15 is the step before the hit
            const bool on = ((pattern >> bit) & 1) != 0;
            auto cell = juce::Rectangle<float> (r.getX() + cw * (float) i + 1.0f, r.getY() + 2.0f, cw - 2.0f, r.getHeight() - 4.0f);
            g.setColour (on ? PCColours::hitCol.withAlpha (active ? 0.95f : 0.4f) : PCColours::panel2);
            g.fillRoundedRectangle (cell, 2.5f);
            g.setColour ((i % 4 == 0) ? PCColours::textDim : PCColours::outline);
            g.drawRoundedRectangle (cell, 2.5f, 1.0f);
            if (hasKeyboardFocus (false) && i == cursor)
            {
                g.setColour (PCColours::neon);
                g.drawRoundedRectangle (cell.expanded (1.0f), 3.0f, 1.6f);
            }
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        grabKeyboardFocus();
        const int i = cellAt (e.position.x);
        paintOn = ! stepOn (i);
        proc.beginGesture (IDs::stutterPattern);
        setStep (i, paintOn);
    }
    void mouseDrag (const juce::MouseEvent& e) override { setStep (cellAt (e.position.x), paintOn); }
    void mouseUp (const juce::MouseEvent&) override { proc.endGesture (IDs::stutterPattern); }

    bool keyPressed (const juce::KeyPress& k) override
    {
        if (k.getKeyCode() == juce::KeyPress::leftKey) { cursor = juce::jmax (0, cursor - 1); repaint(); return true; }
        if (k.getKeyCode() == juce::KeyPress::rightKey) { cursor = juce::jmin (15, cursor + 1); repaint(); return true; }
        if (k.getKeyCode() == juce::KeyPress::returnKey)
        {
            proc.beginGesture (IDs::stutterPattern);
            setStep (cursor, ! stepOn (cursor));
            proc.endGesture (IDs::stutterPattern);
            return true;
        }
        return false;
    }

private:
    int cellAt (float x) const { return juce::jlimit (0, 15, (int) (x / (float) getWidth() * 16.0f)); }
    bool stepOn (int i) const { return (((int) proc.param (IDs::stutterPattern) >> i) & 1) != 0; }
    void setStep (int i, bool on)
    {
        int pattern = (int) proc.param (IDs::stutterPattern);
        const int next = on ? (pattern | (1 << i)) : (pattern & ~(1 << i));
        if (next != pattern) { proc.setParam (IDs::stutterPattern, (float) next); repaint(); }
    }
    void timerCallback() override
    {
        const int p = (int) proc.param (IDs::stutterPattern) | ((int) proc.param (IDs::stutterGrid) << 16);
        if (p != lastShown) { lastShown = p; repaint(); }
    }

    PreChorusProcessor& proc;
    int cursor = 0, lastShown = -1;
    bool paintOn = true;
};

// Timeline of the voices in the current render: one row per voice (ordered by pan), bar = audible span,
// colour = pitch offset, convergence curve overlaid, playhead on top.
class SwarmView : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    explicit SwarmView (PreChorusProcessor& p) : proc (p)
    {
        setTitle ("Swarm view");
        setDescription ("Timeline of every voice: when it enters, its pitch offset and its pan position.");
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (PCColours::panel);
        g.fillRoundedRectangle (b, 8.0f);
        g.setColour (PCColours::outline);
        g.drawRoundedRectangle (b, 8.0f, 1.0f);

        auto r = proc.getRendered();
        auto p = b.reduced (8.0f, 16.0f);
        g.setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
        g.setColour (PCColours::textDim);
        g.drawText ("SWARM VIEW  -  top = left, bottom = right  -  colour = pitch offset", b.withTrimmedTop (3.0f), juce::Justification::centredTop);
        if (r == nullptr || r->voices.empty() || r->audio.getNumSamples() == 0 || r->sampleRate <= 0.0)
            return;

        const double totalSec = (double) r->audio.getNumSamples() / r->sampleRate;
        auto xOf = [&] (double sec) { return p.getX() + (float) juce::jlimit (0.0, 1.0, sec / juce::jmax (0.001, totalSec)) * p.getWidth(); };

        std::vector<int> order (r->voices.size());
        std::iota (order.begin(), order.end(), 0);
        std::stable_sort (order.begin(), order.end(), [&] (int a, int c) { return r->voices[(size_t) a].pan < r->voices[(size_t) c].pan; });
        const float rowH = p.getHeight() / (float) order.size();
        const float hitX = r->hitIndex >= 0 ? xOf ((double) r->hitIndex / r->sampleRate) : p.getRight();
        rows.clear();
        for (size_t row = 0; row < order.size(); ++row)
        {
            const auto& v = r->voices[(size_t) order[row]];
            const float x0 = xOf ((double) v.startSec);
            const float y = p.getY() + (float) row * rowH;
            const float t = juce::jlimit (0.0f, 1.0f, (v.pitchSemi + 12.0f) / 24.0f);
            const auto col = PCColours::neon.interpolatedWith (PCColours::accent, t).interpolatedWith (PCColours::hitCol, juce::jmax (0.0f, t - 0.6f) * 1.5f);
            auto bar = juce::Rectangle<float> (x0, y + rowH * 0.15f, juce::jmax (1.5f, hitX - x0), rowH * 0.7f);
            g.setColour (col.withAlpha (v.reversed ? 0.45f : 0.75f));
            g.fillRoundedRectangle (bar, juce::jmin (3.0f, rowH * 0.3f));
            if (v.reversed) { g.setColour (juce::Colours::white.withAlpha (0.35f)); g.drawLine (bar.getX(), bar.getCentreY(), bar.getRight(), bar.getCentreY(), 1.0f); }
            rows.push_back ({ bar.expanded (0.0f, rowH * 0.15f), order[row] });
        }

        // convergence curve: how scattered the pitches still are (1 = fully spread, 0 = unison) over the swell
        juce::Path curve;
        const float focus = proc.param (IDs::focus), attraction = proc.param (IDs::attraction);
        const float conv = proc.param (IDs::pitchConverge) * proc.param (IDs::macro);
        const bool rev = proc.param (IDs::revConverge) > 0.5f, frozen = proc.param (IDs::freeze) > 0.5f;
        for (int i = 0; i <= 80; ++i)
        {
            const float prog = (float) i / 80.0f;
            float eff = frozen ? 0.5f : (rev ? 1.0f - prog : prog);
            const float spread = 1.0f - conv * tensionCurve (std::pow (eff, 1.0f + focus * 2.0f), attraction);
            const float x = xOf (prog * r->swellSec);
            const float y = p.getBottom() - spread * p.getHeight();
            if (i == 0) curve.startNewSubPath (x, y); else curve.lineTo (x, y);
        }
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.strokePath (curve, juce::PathStrokeType (1.6f));

        g.setColour (PCColours::hitCol.withAlpha (0.9f));
        g.drawVerticalLine ((int) hitX, p.getY(), p.getBottom());

        const int ph = proc.getPlayheadPosition();
        if (ph >= 0)
        {
            g.setColour (juce::Colours::white);
            g.drawVerticalLine ((int) xOf ((double) ph / r->sampleRate), p.getY(), p.getBottom());
        }
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        auto r = proc.getRendered();
        juce::String tip;
        if (r != nullptr)
            for (auto& row : rows)
                if (row.area.contains (e.position))
                {
                    const auto& v = r->voices[(size_t) row.voice];
                    tip = "Voice " + juce::String (row.voice + 1) + ": enters " + juce::String (v.startSec, 2) + " s, pitch "
                          + juce::String (v.pitchSemi, 1) + " st, pan " + juce::String (v.pan, 2) + (v.reversed ? ", reversed" : "");
                    break;
                }
        setTooltip (tip.isEmpty() ? "Every voice of the current render: when it enters, its pitch offset and its pan." : tip);
    }
    void mouseDown (const juce::MouseEvent&) override { proc.triggerPreview(); }

private:
    void timerCallback() override
    {
        if (! isShowing()) return;
        auto r = proc.getRendered();
        const int ph = proc.getPlayheadPosition();
        if (r.get() != lastRender || ph != lastPh) { lastRender = r.get(); lastPh = ph; repaint(); }
    }
    struct Row { juce::Rectangle<float> area; int voice; };
    PreChorusProcessor& proc;
    std::vector<Row> rows;
    const RenderedSample* lastRender = nullptr;
    int lastPh = -2;
};

// Popup preset browser: search, tag filters, favourites, sorting, user-preset management and banks.
class PresetBrowser : public juce::Component, private juce::ListBoxModel
{
public:
    struct Callbacks
    {
        std::function<void (const PresetLibrary::Entry&)> load;      // loads + reports in the status bar
        std::function<void()> save, init;
        std::function<void (const juce::String&)> status;
    };

    PresetBrowser (PresetLibrary& lib, Callbacks cb) : library (lib), calls (std::move (cb))
    {
        setSize (500, 540);
        setTitle ("Preset browser");
        search.setTextToShowWhenEmpty ("Search presets or tags...", PCColours::textDim);
        search.setFont (juce::Font (juce::FontOptions (12.0f)));
        search.setTitle ("Search presets");
        search.onTextChange = [this] { refilter(); };
        addAndMakeVisible (search);

        static const char* tagNames[] = { "Vocal", "EDM", "Dubstep", "Pop", "FX", "Subtle", "Extreme", "User", "Favorites" };
        for (auto* t : tagNames)
        {
            auto* b = tagButtons.add (new juce::TextButton (juce::String (t) == "Favorites" ? juce::String (juce::CharPointer_UTF8 ("\xe2\x98\x85 Favorites")) : juce::String (t)));
            b->setClickingTogglesState (true);
            b->setColour (juce::TextButton::buttonOnColourId, PCColours::accent);
            b->setTitle (juce::String (t) + " filter");
            b->onClick = [this] { refilter(); };
            addAndMakeVisible (b);
        }
        sortCombo.addItemList ({ "Sort: name", "Sort: recent" }, 1);
        sortCombo.setSelectedId (1, juce::dontSendNotification);
        sortCombo.setTitle ("Sort order");
        sortCombo.onChange = [this] { refilter(); };
        addAndMakeVisible (sortCombo);

        list.setModel (this);
        list.setRowHeight (24);
        list.setTitle ("Presets");
        list.setColour (juce::ListBox::backgroundColourId, PCColours::panel);
        addAndMakeVisible (list);

        for (auto* b : { &initButton, &saveButton, &importButton, &exportBankButton, &folderButton })
        {
            addAndMakeVisible (b);
            b->setTitle (b->getButtonText());
        }
        initButton.setTooltip ("Reset every sound-design control to its default (your audio is untouched).");
        saveButton.setTooltip ("Save the current sound as a user preset.");
        importButton.setTooltip ("Import a .pcpreset file or a .pcbank bank.");
        exportBankButton.setTooltip ("Bundle all your user presets into a shareable .pcbank file.");
        folderButton.setTooltip ("Open the user preset folder.");
        initButton.onClick = [this] { if (calls.init) calls.init(); };
        saveButton.onClick = [this] { if (calls.save) calls.save(); };
        importButton.onClick = [this] { importFile(); };
        exportBankButton.onClick = [this] { exportBank(); };
        folderButton.onClick = [this] { library.getFolder().createDirectory(); library.getFolder().revealToUser(); };
        statusLabel.setFont (juce::Font (juce::FontOptions (10.0f)));
        statusLabel.setColour (juce::Label::textColourId, PCColours::textDim);
        addAndMakeVisible (statusLabel);
        refilter();
    }

    void paint (juce::Graphics& g) override { g.fillAll (PCColours::panel2); }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10);
        search.setBounds (r.removeFromTop (28));
        r.removeFromTop (6);
        auto tagRow = r.removeFromTop (24);
        const int tw = (tagRow.getWidth() - 4 * (tagButtons.size() - 1)) / tagButtons.size();
        for (auto* b : tagButtons) { b->setBounds (tagRow.removeFromLeft (tw)); tagRow.removeFromLeft (4); }
        r.removeFromTop (6);
        sortCombo.setBounds (r.removeFromTop (22).removeFromLeft (130));
        r.removeFromTop (6);
        auto bottom = r.removeFromBottom (28);
        statusLabel.setBounds (r.removeFromBottom (18));
        list.setBounds (r);
        const int bw = (bottom.getWidth() - 4 * 4) / 5;
        for (auto* b : { &initButton, &saveButton, &importButton, &exportBankButton, &folderButton })
        {
            b->setBounds (bottom.removeFromLeft (bw)); bottom.removeFromLeft (4);
        }
    }

    void setCurrentKey (const juce::String& key)
    {
        currentKey = key;
        for (int row = 0; row < (int) view.size(); ++row)
            if (library.entries()[(size_t) view[(size_t) row]].key == key) { list.selectRow (row, true, false); list.scrollToEnsureRowIsOnscreen (row); break; }
    }

    void focusSearch() { search.grabKeyboardFocus(); }

private:
    void refilter()
    {
        juce::StringArray tags;
        bool favs = false;
        for (int i = 0; i < tagButtons.size(); ++i)
        {
            if (! tagButtons[i]->getToggleState()) continue;
            if (i == tagButtons.size() - 1) favs = true; else tags.add (tagButtons[i]->getButtonText());
        }
        view = library.filter (search.getText(), tags, favs, sortCombo.getSelectedId() == 2 ? 1 : 0);
        list.updateContent();
        list.repaint();
        statusLabel.setText (juce::String ((int) view.size()) + " preset" + (view.size() == 1 ? "" : "s")
                             + "   |   click to load, star = favorite, right-click a user preset to rename or delete", juce::dontSendNotification);
    }

    // ---- ListBoxModel ----
    int getNumRows() override { return (int) view.size(); }
    juce::String getNameForRow (int row) override
    {
        return row >= 0 && row < (int) view.size() ? library.entries()[(size_t) view[(size_t) row]].name : juce::String();
    }
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        if (row < 0 || row >= (int) view.size()) return;
        const auto& e = library.entries()[(size_t) view[(size_t) row]];
        if (selected) { g.setColour (PCColours::accent.withAlpha (0.35f)); g.fillRect (0, 0, w, h); }
        g.setFont (juce::Font (juce::FontOptions (13.0f)));
        g.setColour (e.favorite ? PCColours::hitCol : PCColours::textDim);
        g.drawText (e.favorite ? juce::String (juce::CharPointer_UTF8 ("\xe2\x98\x85")) : juce::String (juce::CharPointer_UTF8 ("\xe2\x98\x86")),
                    2, 0, 22, h, juce::Justification::centred);
        g.setColour (PCColours::text);
        g.setFont (juce::Font (juce::FontOptions (12.0f, e.isUser() ? juce::Font::italic : juce::Font::plain)));
        g.drawText (e.folder.isNotEmpty() ? e.folder + " / " + e.name : e.name, 28, 0, w - 200, h, juce::Justification::centredLeft, true);
        g.setColour (PCColours::textDim);
        g.setFont (juce::Font (juce::FontOptions (10.0f)));
        g.drawText (e.tags.joinIntoString (" - "), w - 170, 0, 164, h, juce::Justification::centredRight, true);
    }
    void listBoxItemClicked (int row, const juce::MouseEvent& e) override
    {
        if (row < 0 || row >= (int) view.size()) return;
        const int idx = view[(size_t) row];
        if (e.mods.isPopupMenu()) { rowMenu (idx); return; }
        if (e.x < 26) { library.toggleFavorite (idx); refilter(); return; }
        loadIndex (idx);
    }
    void returnKeyPressed (int row) override
    {
        if (row >= 0 && row < (int) view.size()) loadIndex (view[(size_t) row]);
    }

    void loadIndex (int idx)
    {
        const auto& e = library.entries()[(size_t) idx];
        currentKey = e.key;
        library.markRecent (e.key);
        if (calls.load) calls.load (e);
    }

    void rowMenu (int idx)
    {
        const auto e = library.entries()[(size_t) idx];
        juce::PopupMenu m;
        m.addItem (1, e.favorite ? "Remove from favorites" : "Add to favorites");
        if (e.isUser())
        {
            m.addSeparator();
            m.addItem (2, "Rename...");
            m.addItem (3, "Delete");
            m.addItem (4, "Show in file manager");
        }
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&list), [this, idx, e] (int r)
        {
            juce::String err;
            if (r == 1) { library.toggleFavorite (idx); refilter(); }
            else if (r == 3)
            {
                if (library.deleteUser (idx, err)) { setNote ("Deleted " + e.name); refilter(); } else setNote (err);
            }
            else if (r == 4) e.file.revealToUser();
            else if (r == 2)
            {
                auto* aw = new juce::AlertWindow ("Rename preset", "New name:", juce::MessageBoxIconType::NoIcon);
                aw->addTextEditor ("name", e.name);
                aw->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey));
                aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
                const auto key = e.key;
                aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, aw, key] (int result)
                {
                    juce::String er;
                    if (result == 1)
                    {
                        const auto newName = aw->getTextEditorContents ("name");
                        if (library.renameUser (library.indexOfKey (key), newName, er)) setNote ("Renamed to " + newName);
                        else setNote (er);
                        refilter();
                    }
                    delete aw;
                }), false);
            }
        });
    }

    void setNote (const juce::String& s) { statusLabel.setText (s, juce::dontSendNotification); if (calls.status) calls.status (s); }

    void importFile()
    {
        chooser = std::make_shared<juce::FileChooser> ("Import preset or bank", juce::File(), "*.pcpreset;*.pcbank");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f == juce::File()) return;
            juce::String note;
            if (f.hasFileExtension ("pcbank")) library.importBank (f, note);
            else
            {
                juce::ValueTree snap; juce::String err;
                if (! PreChorusProcessor::parsePresetFile (f, snap, err)) note = "Import failed: " + err;
                else
                {
                    auto dest = library.getFolder().getChildFile ("Imported");
                    dest.createDirectory();
                    auto target = dest.getChildFile (PresetLibrary::sanitizeFileName (f.getFileNameWithoutExtension()) + ".pcpreset");
                    for (int k = 2; target.exists(); ++k) target = dest.getChildFile (PresetLibrary::sanitizeFileName (f.getFileNameWithoutExtension()) + " " + juce::String (k) + ".pcpreset");
                    note = f.copyFileTo (target) ? "Imported " + target.getFileNameWithoutExtension() : "Could not copy the preset into the library.";
                }
                library.rescan();
            }
            setNote (note);
            refilter();
        });
    }

    void exportBank()
    {
        chooser = std::make_shared<juce::FileChooser> ("Export user presets as a bank", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("PreChorus Bank.pcbank"), "*.pcbank");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting, [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f == juce::File()) return;
            juce::Array<juce::File> files;
            for (auto& e : library.entries()) if (e.isUser()) files.add (e.file);
            juce::String err;
            f = f.withFileExtension ("pcbank");
            setNote (library.exportBank (files, f, f.getFileNameWithoutExtension(), juce::SystemStats::getFullUserName(), err)
                         ? "Exported " + juce::String (files.size()) + " presets to " + f.getFileName() : "Bank export failed: " + err);
        });
    }

    PresetLibrary& library;
    Callbacks calls;
    juce::TextEditor search;
    juce::OwnedArray<juce::TextButton> tagButtons;
    juce::ComboBox sortCombo;
    juce::ListBox list;
    juce::TextButton initButton { "INIT" }, saveButton { "SAVE..." }, importButton { "IMPORT..." },
                     exportBankButton { "EXPORT BANK..." }, folderButton { "FOLDER" };
    juce::Label statusLabel;
    std::vector<int> view;
    juce::String currentKey;
    std::shared_ptr<juce::FileChooser> chooser;
};
