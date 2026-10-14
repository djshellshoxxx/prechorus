// PreChorus™
// Copyright © 2026 Sheldon Davidson. All rights reserved.
// Proprietary source code. See LICENSE and COPYRIGHT-TRADEMARK.md.
// SPDX-License-Identifier: LicenseRef-Proprietary

// Preset library model for the browser: factory + user presets, tags, favourites, recents and shareable banks.
// No UI and no processor state here, so it is unit-testable. Banks never carry or read audio.
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class PresetLibrary
{
public:
    struct Entry
    {
        juce::String key;            // "F:<name>" for factory, "U:<relative path>" for user presets
        juce::String name;
        juce::StringArray tags;
        int factoryIndex = -1;       // >= 0: factory preset
        juce::File file;             // user presets
        juce::String folder;         // sub-folder of the user preset folder ("" = top level)
        bool favorite = false;
        bool isUser() const { return factoryIndex < 0; }
    };

    PresetLibrary (juce::File presetFolderToUse, juce::File settingsFileToUse)
        : root (std::move (presetFolderToUse)), settingsFile (std::move (settingsFileToUse))
    {
        loadSettings();
        rescan();
    }

    const juce::File& getFolder() const { return root; }

    void rescan()
    {
        items.clear();
        const auto names = PreChorusProcessor::getFactoryPresetNames();
        for (int i = 0; i < names.size(); ++i)
        {
            Entry e;
            e.key = "F:" + names[i]; e.name = names[i]; e.factoryIndex = i;
            e.tags = PreChorusProcessor::getFactoryPresetTags (i);
            items.push_back (e);
        }
        if (root.isDirectory())
        {
            auto files = root.findChildFiles (juce::File::findFiles, true, "*.pcpreset");
            files.sort();
            int count = 0;
            for (auto& f : files)
            {
                if (++count > 2000) break;
                Entry e;
                const auto rel = f.getRelativePathFrom (root).replaceCharacter ('\\', '/');
                e.key = "U:" + rel; e.name = f.getFileNameWithoutExtension(); e.file = f;
                e.folder = rel.contains ("/") ? rel.upToLastOccurrenceOf ("/", false, false) : juce::String();
                e.tags.add ("User");
                items.push_back (e);
            }
        }
        for (auto& e : items) e.favorite = favorites.contains (e.key);
    }

    const std::vector<Entry>& entries() const { return items; }

    // Indices into entries() that match every filter. sort: 0 = name, 1 = most recently used first.
    std::vector<int> filter (const juce::String& search, const juce::StringArray& requiredTags, bool favoritesOnly, int sort) const
    {
        std::vector<int> out;
        const auto needle = search.trim().toLowerCase();
        for (int i = 0; i < (int) items.size(); ++i)
        {
            const auto& e = items[(size_t) i];
            if (favoritesOnly && ! e.favorite) continue;
            bool ok = true;
            for (auto& t : requiredTags) if (! e.tags.contains (t, true)) ok = false;
            if (! ok) continue;
            if (needle.isNotEmpty())
            {
                const auto hay = (e.name + " " + e.tags.joinIntoString (" ") + " " + e.folder).toLowerCase();
                bool all = true;
                for (auto& w : juce::StringArray::fromTokens (needle, " ", "")) if (w.isNotEmpty() && ! hay.contains (w)) all = false;
                if (! all) continue;
            }
            out.push_back (i);
        }
        if (sort == 1)
            std::stable_sort (out.begin(), out.end(), [this] (int a, int b)
            {
                const int ra = recents.indexOf (items[(size_t) a].key), rb = recents.indexOf (items[(size_t) b].key);
                const int pa = ra < 0 ? 1 << 20 : ra, pb = rb < 0 ? 1 << 20 : rb;
                return pa < pb;
            });
        return out;
    }

    int indexOfKey (const juce::String& key) const
    {
        for (int i = 0; i < (int) items.size(); ++i) if (items[(size_t) i].key == key) return i;
        return -1;
    }

    void toggleFavorite (int index)
    {
        if (index < 0 || index >= (int) items.size()) return;
        auto& e = items[(size_t) index];
        e.favorite = ! e.favorite;
        if (e.favorite) favorites.addIfNotAlreadyThere (e.key); else favorites.removeString (e.key);
        saveSettings();
    }

    void markRecent (const juce::String& key)
    {
        recents.removeString (key);
        recents.insert (0, key);
        while (recents.size() > 40) recents.remove (recents.size() - 1);
        saveSettings();
    }

    bool renameUser (int index, const juce::String& newName, juce::String& error)
    {
        if (index < 0 || index >= (int) items.size() || ! items[(size_t) index].isUser()) { error = "Only user presets can be renamed."; return false; }
        const auto clean = sanitizeFileName (newName);
        if (clean.isEmpty()) { error = "Please type a valid name."; return false; }
        auto& e = items[(size_t) index];
        auto target = e.file.getSiblingFile (clean + ".pcpreset");
        if (target == e.file) return true;
        if (target.exists()) { error = "A preset with that name already exists."; return false; }
        if (! e.file.moveFileTo (target)) { error = "Could not rename the file."; return false; }
        const auto oldKey = e.key;
        rescan();
        // carry favourite/recent over to the new key
        const auto newKey = "U:" + target.getRelativePathFrom (root).replaceCharacter ('\\', '/');
        if (favorites.contains (oldKey)) { favorites.removeString (oldKey); favorites.addIfNotAlreadyThere (newKey); }
        const int ri = recents.indexOf (oldKey); if (ri >= 0) recents.set (ri, newKey);
        saveSettings();
        rescan();
        return true;
    }

    bool deleteUser (int index, juce::String& error)
    {
        if (index < 0 || index >= (int) items.size() || ! items[(size_t) index].isUser()) { error = "Only user presets can be deleted."; return false; }
        const auto e = items[(size_t) index];
        if (! e.file.moveToTrash() && ! e.file.deleteFile()) { error = "Could not delete the file."; return false; }
        favorites.removeString (e.key); recents.removeString (e.key);
        saveSettings();
        rescan();
        return true;
    }

    // ---- Preset banks (.pcbank): a zip of manifest.xml + presets/*.pcpreset ----
    bool exportBank (const juce::Array<juce::File>& files, const juce::File& dest, const juce::String& bankName,
                     const juce::String& author, juce::String& error) const
    {
        if (files.isEmpty()) { error = "No presets selected."; return false; }
        juce::ZipFile::Builder builder;
        juce::StringArray used;
        int n = 0;
        for (auto& f : files)
        {
            juce::ValueTree snap; juce::String why;
            if (! PreChorusProcessor::parsePresetFile (f, snap, why)) continue;     // never ship an invalid preset
            auto name = sanitizeFileName (f.getFileNameWithoutExtension());
            if (name.isEmpty()) name = "Preset";
            auto unique = name; int k = 2;
            while (used.contains (unique, true)) unique = name + " " + juce::String (k++);
            used.add (unique);
            juce::MemoryBlock raw; f.loadFileAsData (raw);
            builder.addEntry (new juce::MemoryInputStream (raw, true), 9, "presets/" + unique + ".pcpreset", juce::Time::getCurrentTime());
            ++n;
        }
        if (n == 0) { error = "None of the selected presets could be read."; return false; }
        juce::XmlElement manifest ("PRECHORUS_BANK");
        manifest.setAttribute ("product", "PreChorus");
        manifest.setAttribute ("schema", 1);
        manifest.setAttribute ("name", bankName.substring (0, 80));
        manifest.setAttribute ("author", author.substring (0, 80));
        manifest.setAttribute ("count", n);
        const auto xmlText = manifest.toString();
        builder.addEntry (new juce::MemoryInputStream (xmlText.toRawUTF8(), (size_t) xmlText.getNumBytesAsUTF8(), true), 9,
                          "manifest.xml", juce::Time::getCurrentTime());
        dest.deleteFile();
        juce::FileOutputStream os (dest);
        if (! os.openedOk()) { error = "Could not write the bank file."; return false; }
        if (! builder.writeToStream (os, nullptr)) { error = "Writing the bank failed."; return false; }
        return true;
    }

    // Installs every valid preset into <folder>/Banks/<bank name>/. Returns the number installed; `report` explains skips.
    // Rejects path traversal, absolute paths, duplicates, oversize entries, bad XML and wrong-product banks.
    int importBank (const juce::File& bankFile, juce::String& report)
    {
        constexpr int kMaxEntries = 500;
        constexpr juce::int64 kMaxEntryBytes = 256 * 1024;
        report.clear();
        if (! bankFile.existsAsFile() || bankFile.getSize() > 64 * 1024 * 1024) { report = "The bank file is missing or too large."; return 0; }
        juce::ZipFile zip (bankFile);
        if (zip.getNumEntries() <= 0 || zip.getNumEntries() > kMaxEntries + 1) { report = "Not a valid PreChorus bank."; return 0; }

        auto readEntry = [&] (int idx, juce::MemoryBlock& mb) -> bool
        {
            std::unique_ptr<juce::InputStream> in (zip.createStreamForEntry (idx));
            if (in == nullptr) return false;
            mb.reset();
            in->readIntoMemoryBlock (mb, (ssize_t) (kMaxEntryBytes + 1));
            return (juce::int64) mb.getSize() <= kMaxEntryBytes && mb.getSize() > 0;
        };

        juce::String bankName = bankFile.getFileNameWithoutExtension();
        const int mi = zip.getIndexOfFileName ("manifest.xml");
        if (mi < 0) { report = "Not a PreChorus bank (no manifest)."; return 0; }
        {
            juce::MemoryBlock mb;
            if (! readEntry (mi, mb)) { report = "The bank manifest is unreadable."; return 0; }
            auto xml = juce::XmlDocument::parse (mb.toString());
            if (xml == nullptr || ! xml->hasTagName ("PRECHORUS_BANK") || xml->getStringAttribute ("product") != "PreChorus")
            { report = "Not a PreChorus bank."; return 0; }
            if (xml->getIntAttribute ("schema", 0) != 1) { report = "Unsupported bank version."; return 0; }
            if (xml->getStringAttribute ("name").isNotEmpty()) bankName = xml->getStringAttribute ("name");
        }

        const auto folderName = sanitizeFileName (bankName).isEmpty() ? juce::String ("Imported Bank") : sanitizeFileName (bankName);
        auto dest = root.getChildFile ("Banks").getChildFile (folderName);
        juce::StringArray seen;
        int installed = 0, skipped = 0;
        for (int i = 0; i < zip.getNumEntries(); ++i)
        {
            const auto* ze = zip.getEntry (i);
            if (ze == nullptr) continue;
            const auto name = ze->filename;
            if (name == "manifest.xml") continue;
            if (! isSafeEntryName (name) || ! name.endsWithIgnoreCase (".pcpreset")) { ++skipped; continue; }
            const auto lower = name.toLowerCase();
            if (seen.contains (lower)) { ++skipped; continue; }
            seen.add (lower);
            juce::MemoryBlock mb;
            if (! readEntry (i, mb)) { ++skipped; continue; }
            auto xml = juce::XmlDocument::parse (mb.toString());
            juce::ValueTree snap; juce::String why;
            if (xml == nullptr || ! PreChorusProcessor::parsePresetXml (*xml, snap, why)) { ++skipped; continue; }
            if (! dest.createDirectory()) { report = "Could not create the bank folder."; return installed; }
            const auto base = sanitizeFileName (juce::File::createFileWithoutCheckingPath (name).getFileNameWithoutExtension());
            auto target = dest.getChildFile ((base.isEmpty() ? juce::String ("Preset") : base) + ".pcpreset");
            for (int k = 2; target.exists(); ++k) target = dest.getChildFile (base + " " + juce::String (k) + ".pcpreset");
            if (xml->writeTo (target)) ++installed; else ++skipped;
        }
        rescan();
        report = "Installed " + juce::String (installed) + " preset" + (installed == 1 ? "" : "s") + " into Banks/" + folderName
                 + (skipped > 0 ? " (" + juce::String (skipped) + " skipped as invalid)" : juce::String()) + ".";
        return installed;
    }

    static juce::String sanitizeFileName (const juce::String& in)
    {
        auto s = juce::File::createLegalFileName (in.trim()).trim();
        while (s.startsWithChar ('.')) s = s.substring (1);
        return s.substring (0, 80);
    }

    static bool isSafeEntryName (const juce::String& n)
    {
        if (n.isEmpty() || n.length() > 200) return false;
        if (n.startsWithChar ('/') || n.startsWithChar ('\\') || n.containsChar (':') || n.containsChar (0)) return false;
        for (auto& part : juce::StringArray::fromTokens (n.replaceCharacter ('\\', '/'), "/", ""))
            if (part == ".." || part == ".") return false;
        return true;
    }

private:
    void loadSettings()
    {
        favorites.clear(); recents.clear();
        if (! settingsFile.existsAsFile() || settingsFile.getSize() > 1024 * 1024) return;
        if (auto xml = juce::XmlDocument::parse (settingsFile))
        {
            if (! xml->hasTagName ("PRECHORUS_SETTINGS")) return;
            for (auto* e : xml->getChildWithTagNameIterator ("FAV")) favorites.addIfNotAlreadyThere (e->getStringAttribute ("key"));
            for (auto* e : xml->getChildWithTagNameIterator ("RECENT")) recents.add (e->getStringAttribute ("key"));
        }
    }

    void saveSettings() const
    {
        juce::XmlElement x ("PRECHORUS_SETTINGS");
        for (auto& k : favorites) x.createNewChildElement ("FAV")->setAttribute ("key", k);
        for (auto& k : recents) x.createNewChildElement ("RECENT")->setAttribute ("key", k);
        settingsFile.getParentDirectory().createDirectory();
        x.writeTo (settingsFile);
    }

    juce::File root, settingsFile;
    std::vector<Entry> items;
    juce::StringArray favorites, recents;
};
