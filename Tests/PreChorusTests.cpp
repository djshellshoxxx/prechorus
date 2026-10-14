// PreChorus™
// Copyright © 2026 Sheldon Davidson. All rights reserved.
// Proprietary source code. See LICENSE and COPYRIGHT-TRADEMARK.md.
// SPDX-License-Identifier: LicenseRef-Proprietary

#include <JuceHeader.h>
#include <iostream>
#include "../Source/PluginProcessor.h"
#include "../Source/DemoSource.h"
#include "../Source/Dsp.h"
#include "../Source/PresetLibrary.h"

class PreChorusCoreTests : public juce::UnitTest
{
public:
    PreChorusCoreTests() : juce::UnitTest ("PreChorus core", "PreChorus") {}

    void runTest() override
    {
        beginTest ("supports mono/stereo input and stereo output");
        {
            PreChorusProcessor p;
            juce::AudioProcessor::BusesLayout stereo;
            stereo.inputBuses.add (juce::AudioChannelSet::stereo());
            stereo.outputBuses.add (juce::AudioChannelSet::stereo());
            expect (p.isBusesLayoutSupported (stereo));

            juce::AudioProcessor::BusesLayout mono;
            mono.inputBuses.add (juce::AudioChannelSet::mono());
            mono.outputBuses.add (juce::AudioChannelSet::stereo());
            expect (p.isBusesLayoutSupported (mono));

            juce::AudioProcessor::BusesLayout badOut;
            badOut.inputBuses.add (juce::AudioChannelSet::stereo());
            badOut.outputBuses.add (juce::AudioChannelSet::mono());
            expect (! p.isBusesLayoutSupported (badOut));
        }

        beginTest ("automatic capture defers history allocation off the audio thread");
        {
            PreChorusProcessor p;
            p.prepareToPlay (44100.0, 128);
            p.selectCaptureSlot (1);
            p.setParam (IDs::captureMode, 1.0f);
            p.armCapture();

            for (int block = 0; block < 55; ++block)
            {
                juce::AudioBuffer<float> buffer (2, 128);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 128; ++i)
                        buffer.setSample (ch, i, 0.5f);
                juce::MidiBuffer midi;
                p.processBlock (buffer, midi);
            }

            expect (p.getCaptureState() == PreChorusProcessor::CaptureState::done);
            expect (! p.isSlotFilled (1));

            p.stopCapture();
            expect (p.getCaptureState() == PreChorusProcessor::CaptureState::idle);
            expect (p.isSlotFilled (1));

            p.selectCaptureSlot (2);
            p.armCapture();
            for (int block = 0; block < 55; ++block)
            {
                juce::AudioBuffer<float> buffer (2, 128);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 128; ++i)
                        buffer.setSample (ch, i, 0.5f);
                juce::MidiBuffer midi;
                p.processBlock (buffer, midi);
            }
            expect (p.getCaptureState() == PreChorusProcessor::CaptureState::done);
            p.armCapture();
            expect (p.isSlotFilled (2));
            expect (p.getCaptureState() == PreChorusProcessor::CaptureState::armed);
        }

        beginTest ("input meter detects signal on either stereo channel");
        {
            PreChorusProcessor p;
            p.prepareToPlay (44100.0, 64);
            juce::AudioBuffer<float> buffer (2, 64);
            buffer.clear();
            buffer.applyGain (1, 0, 64, 0.0f);
            for (int i = 0; i < 64; ++i)
                buffer.setSample (1, i, 0.8f);
            juce::MidiBuffer midi;
            p.processBlock (buffer, midi);
            expectGreaterThan (p.getLiveInputMeter(), 0.79f);
        }

        beginTest ("rendered swarm reports a non-zero host tail");
        {
            PreChorusProcessor p;
            p.prepareToPlay (44100.0, 512);
            auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getNonexistentChildFile ("prechorus-tail", ".wav", false);
            expect (p.exportWav (file));
            expectGreaterThan (p.getTailLengthSeconds(), 0.1);
            file.deleteFile();
        }

        beginTest ("repeated render publication remains playable and finite");
        {
            PreChorusProcessor p;
            p.prepareToPlay (44100.0, 128);
            auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getNonexistentChildFile ("prechorus-publication", ".wav", false);

            bool allFinite = true;
            for (int pass = 0; pass < 8; ++pass)
            {
                p.setParam (IDs::air, 0.1f * (float) pass);
                expect (p.exportWav (file));
                p.triggerPreview();

                juce::AudioBuffer<float> buffer (2, 128);
                buffer.clear();
                juce::MidiBuffer midi;
                p.processBlock (buffer, midi);

                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                    for (int i = 0; i < buffer.getNumSamples(); ++i)
                        allFinite = allFinite && std::isfinite (buffer.getSample (ch, i));
            }
            expect (allFinite);
            file.deleteFile();
        }

        beginTest ("host state round-trip restores parameters");
        {
            PreChorusProcessor a;
            a.setParam (IDs::voiceCount, 23.0f);
            a.setParam (IDs::air, 0.71f);
            juce::MemoryBlock state;
            a.getStateInformation (state);

            PreChorusProcessor b;
            b.setStateInformation (state.getData(), (int) state.getSize());
            expectEquals ((int) b.param (IDs::voiceCount), 23);
            expectWithinAbsoluteError (b.param (IDs::air), 0.71f, 0.011f);
        }

        beginTest ("host recall preserves saved source mode when an external file exists");
        {
            auto wavFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                               .getNonexistentChildFile ("prechorus-state-source", ".wav", false);
            {
                juce::WavAudioFormat format;
                auto stream = wavFile.createOutputStream();
                std::unique_ptr<juce::AudioFormatWriter> writer (
                    format.createWriterFor (stream.get(), 44100.0, 2, 16, {}, 0));
                expect (writer != nullptr);
                if (writer != nullptr)
                {
                    stream.release();
                    juce::AudioBuffer<float> silence (2, 128);
                    silence.clear();
                    writer->writeFromAudioSampleBuffer (silence, 0, silence.getNumSamples());
                }
            }

            PreChorusProcessor source;
            expect (source.loadSampleFile (wavFile, false));
            source.setParam (IDs::sourceMode, 0.0f);

            juce::MemoryBlock state;
            source.getStateInformation (state);
            PreChorusProcessor restored;
            restored.setStateInformation (state.getData(), (int) state.getSize());
            expectEquals ((int) restored.param (IDs::sourceMode), 0);
            expect (restored.getCurrentFile() == wavFile);
            expectEquals ((int) restored.param (IDs::sourceMode), 0);
            wavFile.deleteFile();
        }

        beginTest ("A/B snapshots recall independent sound-design states");
        {
            PreChorusProcessor p;
            p.setParam (IDs::voiceCount, 6.0f);
            p.setParam (IDs::sourceMode, 2.0f);
            p.switchABSlot();                       // A stored (6), now editing B (copy of A)
            expect (p.isSlotBActive());
            expectEquals ((int) p.param (IDs::voiceCount), 6);
            p.setParam (IDs::voiceCount, 28.0f);
            p.setParam (IDs::sourceMode, 0.0f);     // not part of a snapshot
            p.switchABSlot();                       // back to A
            expect (! p.isSlotBActive());
            expectEquals ((int) p.param (IDs::voiceCount), 6);
            expectEquals ((int) p.param (IDs::sourceMode), 0);
            p.switchABSlot();                       // B again
            expectEquals ((int) p.param (IDs::voiceCount), 28);
            p.copyCurrentToOtherAB();
            p.switchABSlot();
            expectEquals ((int) p.param (IDs::voiceCount), 28);
        }

        beginTest ("factory presets do not leak settings from the previous preset");
        {
            PreChorusProcessor p;
            p.loadFactoryPreset (7); // Ambient Drone Freeze enables Freeze
            expect (p.param (IDs::freeze) > 0.5f);
            p.loadFactoryPreset (0); // Pop Vocal Double
            expect (p.param (IDs::freeze) < 0.5f);
            p.loadFactoryPreset (3); // Dubstep enables Reverse Convergence
            p.loadFactoryPreset (1);
            expect (p.param (IDs::revConverge) < 0.5f);
        }

        beginTest ("PDC reports latency and delays the dry input by the same amount");
        {
            PreChorusProcessor p;
            p.setParam (IDs::sync, 0.0f);
            p.setParam (IDs::tail, 0.1f);
            p.prepareToPlay (44100.0, 256);
            auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getNonexistentChildFile ("prechorus-pdc", ".wav", false);
            expect (p.exportWav (file)); // forces a render
            file.deleteFile();
            const int lat = p.getLatencySamples();
            expectGreaterThan (lat, 1000);

            int firstNonZero = -1, processed = 0;
            for (int block = 0; block < 40 && firstNonZero < 0; ++block)
            {
                juce::AudioBuffer<float> buffer (2, 256);
                buffer.clear();
                if (block == 0) { buffer.setSample (0, 0, 1.0f); buffer.setSample (1, 0, 1.0f); }
                juce::MidiBuffer midi;
                p.processBlock (buffer, midi);
                for (int i = 0; i < 256 && firstNonZero < 0; ++i)
                    if (std::abs (buffer.getSample (0, i)) > 0.5f) firstNonZero = processed + i;
                processed += 256;
            }
            expectEquals (firstNonZero, lat);
        }

        beginTest ("MIDI triggers are sample-accurate and keytrack/stutter output stays finite");
        {
            PreChorusProcessor p;
            p.setParam (IDs::align, 0.0f);
            p.setParam (IDs::dry, 0.0f);
            p.setParam (IDs::stutter, 1.0f);
            p.setParam (IDs::keytrack, 1.0f);
            p.prepareToPlay (48000.0, 512);
            auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getNonexistentChildFile ("prechorus-midi", ".wav", false);
            expect (p.exportWav (file));
            file.deleteFile();

            juce::AudioBuffer<float> buffer (2, 512);
            buffer.clear();
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 72, (juce::uint8) 100), 300);
            p.processBlock (buffer, midi);
            for (int i = 0; i < 300; ++i) expectEquals (buffer.getSample (0, i), 0.0f);

            bool finite = true;
            for (int block = 0; block < 200; ++block)
            {
                buffer.clear();
                juce::MidiBuffer none;
                p.processBlock (buffer, none);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 512; ++i) finite = finite && std::isfinite (buffer.getSample (ch, i));
            }
            expect (finite);
        }

        beginTest ("malformed and future-version host state is ignored");
        {
            PreChorusProcessor p;
            p.setParam (IDs::voiceCount, 9.0f);
            const char junk[] = "not a state";
            p.setStateInformation (junk, (int) sizeof (junk));
            expectEquals ((int) p.param (IDs::voiceCount), 9);

            auto state = p.apvts.copyState();
            state.setProperty ("stateVersion", 999, nullptr);
            state.setProperty (juce::Identifier (IDs::voiceCount), 30, nullptr);
            juce::MemoryBlock mb;
            juce::AudioProcessor::copyXmlToBinary (*state.createXml(), mb);
            p.setStateInformation (mb.getData(), (int) mb.getSize());
            expectEquals ((int) p.param (IDs::voiceCount), 9);
        }

        beginTest ("user preset round-trip excludes source media and restores parameters");
        {
            PreChorusProcessor p;
            p.setParam (IDs::pitchSpread, 13.0f);
            p.setParam (IDs::space, 0.61f);
            p.setParam (IDs::sourceMode, 0.0f);
            p.setParam (IDs::captureMode, 7.0f);
            p.setParam (IDs::captureSlot, 4.0f);
            p.setParam (IDs::captureLock, 1.0f);

            auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getNonexistentChildFile ("prechorus-test", ".pcpreset", false);
            juce::MemoryBlock session;
            p.getStateInformation (session);
            p.setStateInformation (session.getData(), (int) session.getSize());

            juce::String err;
            expect (p.saveUserPreset (file, err));
            const auto presetText = file.loadFileAsString();
            expect (! presetText.contains ("activeSlot="));
            expect (! presetText.contains ("compareA="));
            expect (! presetText.contains ("compareB="));
            expect (! presetText.contains ("file="));

            p.setParam (IDs::pitchSpread, 1.0f);
            p.setParam (IDs::space, 0.05f);
            p.setParam (IDs::sourceMode, 2.0f);
            p.setParam (IDs::captureMode, 0.0f);
            p.setParam (IDs::captureSlot, 2.0f);
            p.setParam (IDs::captureLock, 0.0f);
            expect (p.loadUserPreset (file, err));
            expectWithinAbsoluteError (p.param (IDs::pitchSpread), 13.0f, 0.01f);
            expectWithinAbsoluteError (p.param (IDs::space), 0.61f, 0.011f);
            expectEquals ((int) p.param (IDs::sourceMode), 2);
            expectEquals ((int) p.param (IDs::captureMode), 0);
            expectEquals ((int) p.param (IDs::captureSlot), 2);
            expectEquals ((int) p.param (IDs::captureLock), 0);
            file.deleteFile();
        }

        beginTest ("malformed user preset is rejected without changing sound");
        {
            PreChorusProcessor p;
            p.setParam (IDs::detune, 19.0f);
            auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getNonexistentChildFile ("prechorus-bad", ".pcpreset", false);
            file.replaceWithText ("not a preset");
            juce::String err;
            expect (! p.loadUserPreset (file, err));
            expect (err.isNotEmpty());
            expectWithinAbsoluteError (p.param (IDs::detune), 19.0f, 0.01f);
            file.deleteFile();
        }

        beginTest ("wrong-product and future-version presets are rejected atomically");
        {
            PreChorusProcessor p;
            p.setParam (IDs::space, 0.42f);

            auto makePreset = [&] (const juce::String& product, int version)
            {
                juce::ValueTree wrapper ("PRECHORUS_PRESET");
                wrapper.setProperty ("schemaVersion", version, nullptr);
                wrapper.setProperty ("product", product, nullptr);
                wrapper.addChild (p.apvts.copyState(), -1, nullptr);
                return wrapper.createXml()->toString();
            };

            auto wrongProduct = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                    .getNonexistentChildFile ("prechorus-wrong-product", ".pcpreset", false);
            wrongProduct.replaceWithText (makePreset ("OtherProduct", 1));
            juce::String err;
            expect (! p.loadUserPreset (wrongProduct, err));
            expectWithinAbsoluteError (p.param (IDs::space), 0.42f, 0.011f);
            wrongProduct.deleteFile();

            auto futureVersion = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                     .getNonexistentChildFile ("prechorus-future", ".pcpreset", false);
            futureVersion.replaceWithText (makePreset ("PreChorus", 999));
            expect (! p.loadUserPreset (futureVersion, err));
            expectWithinAbsoluteError (p.param (IDs::space), 0.42f, 0.011f);
            futureVersion.deleteFile();
        }

        beginTest ("demo sources are deterministic, finite and level-safe");
        {
            for (int k = 0; k < PCDemo::numKinds; ++k)
            {
                auto a = PCDemo::generate (k, 48000.0);
                auto b = PCDemo::generate (k, 48000.0);
                expectEquals (a.getNumSamples(), b.getNumSamples());
                bool same = true, finite = true;
                for (int i = 0; i < a.getNumSamples(); ++i)
                {
                    const float x = a.getSample (0, i);
                    if (! std::isfinite (x)) finite = false;
                    if (! juce::exactlyEqual (x, b.getSample (0, i))) same = false;
                }
                expect (same);
                expect (finite);
                expect (a.getMagnitude (0, a.getNumSamples()) > 0.5f);
                expect (a.getMagnitude (0, a.getNumSamples()) <= 0.71f);
            }
        }

        beginTest ("captured audio and loaded source are embedded in the project and survive a restore");
        {
            PreChorusProcessor a;
            a.prepareToPlay (48000.0, 256);
            juce::AudioBuffer<float> cap (2, 24000);
            for (int i = 0; i < cap.getNumSamples(); ++i)
            {
                cap.setSample (0, i, 0.5f * std::sin (0.05f * (float) i));
                cap.setSample (1, i, 0.25f * std::sin (0.031f * (float) i));
            }
            {
                const juce::ScopedLock sl (a.sourceLock);
                a.captureSlots[3] = cap; a.captureSlotSRs[3] = 48000.0; a.captureSlotFilled[3] = true;
                a.slotEmbedCache[3].valid = false;
            }
            a.setParam (IDs::captureSlot, 3.0f);
            a.setParam (IDs::sourceMode, 0.0f);          // Live Capture must stay Live Capture
            a.setParam (IDs::space, 0.37f);

            juce::MemoryBlock blob;
            a.getStateInformation (blob);
            expect (blob.getSize() > 1000);

            // repeated saves must not accumulate embedded blobs
            juce::MemoryBlock blob2;
            a.getStateInformation (blob2);
            expect (blob2.getSize() == blob.getSize());

            PreChorusProcessor b;
            b.prepareToPlay (48000.0, 256);
            b.setStateInformation (blob.getData(), (int) blob.getSize());
            expect (b.isSlotFilled (3));
            expect (b.isSlotFilled (0));                 // startup chord restored, never embedded
            expect (! b.isSlotFilled (5));
            expectEquals ((int) b.param (IDs::sourceMode), 0);
            expectWithinAbsoluteError (b.param (IDs::space), 0.37f, 0.01f);
            expectEquals (b.getActiveCaptureSlot(), 3);
            {
                const juce::ScopedLock sl (b.sourceLock);
                expectEquals (b.captureSlots[3].getNumSamples(), 24000);
                float maxErr = 0.0f;
                for (int i = 0; i < 24000; ++i)
                {
                    maxErr = juce::jmax (maxErr, std::abs (b.captureSlots[3].getSample (0, i) - cap.getSample (0, i)));
                    maxErr = juce::jmax (maxErr, std::abs (b.captureSlots[3].getSample (1, i) - cap.getSample (1, i)));
                }
                expect (maxErr < 1.0e-5f);               // 24-bit FLAC is effectively lossless
                expectEquals (b.captureSlotSRs[3], 48000.0);
            }
            expect (b.getEmbedSkipped() == 0);
        }

        beginTest ("embedded loaded source restores without the original file");
        {
            auto wav = juce::File::getSpecialLocation (juce::File::tempDirectory).getNonexistentChildFile ("prechorus-embed", ".wav", false);
            {
                juce::AudioBuffer<float> tone (2, 20000);
                for (int i = 0; i < 20000; ++i) { tone.setSample (0, i, 0.4f * std::sin (0.07f * (float) i)); tone.setSample (1, i, 0.4f * std::sin (0.07f * (float) i)); }
                juce::WavAudioFormat fmt;
                std::unique_ptr<juce::AudioFormatWriter> w (fmt.createWriterFor (new juce::FileOutputStream (wav), 44100.0, 2, 24, {}, 0));
                expect (w != nullptr);
                w->writeFromAudioSampleBuffer (tone, 0, 20000);
            }
            PreChorusProcessor a;
            expect (a.loadSampleFile (wav, false, false));
            juce::MemoryBlock blob;
            a.getStateInformation (blob);
            wav.deleteFile();

            PreChorusProcessor b;
            b.setStateInformation (blob.getData(), (int) blob.getSize());
            const juce::ScopedLock sl (b.sourceLock);
            expectEquals (b.loadedBuffer.getNumSamples(), 20000);
            expectEquals (b.loadedSR, 44100.0);
            expect (b.loadedBuffer.getMagnitude (0, 20000) > 0.39f);
        }

        beginTest ("demo source is regenerated (not embedded) and keeps source mode on restore");
        {
            PreChorusProcessor a;
            a.prepareToPlay (44100.0, 256);
            a.loadDemoSource (1);
            a.setParam (IDs::sourceMode, 2.0f);
            juce::MemoryBlock blob;
            a.getStateInformation (blob);
            expect (blob.getSize() < 20000);             // nothing audio-sized in the state

            PreChorusProcessor b;
            b.prepareToPlay (44100.0, 256);
            b.setStateInformation (blob.getData(), (int) blob.getSize());
            expectEquals (b.demoKind, 1);
            expectEquals ((int) b.param (IDs::sourceMode), 2);
            expect (b.loadedBuffer.getNumSamples() > 1000);
        }

        beginTest ("corrupt embedded audio is ignored safely and old projects without embeds still load");
        {
            PreChorusProcessor a;
            a.prepareToPlay (44100.0, 256);
            juce::MemoryBlock blob;
            a.getStateInformation (blob);
            auto xml = juce::AudioProcessor::getXmlFromBinary (blob.getData(), (int) blob.getSize());
            expect (xml != nullptr);
            auto* embed = xml->getChildByName ("EMBED");
            expect (embed != nullptr);
            auto* item = embed->createNewChildElement ("AUDIO");
            item->setAttribute ("kind", "slot");
            item->setAttribute ("idx", 2);
            item->setAttribute ("data", "AAAA!!!not base64 or flac");
            auto* bad = embed->createNewChildElement ("AUDIO");
            bad->setAttribute ("kind", "slot");
            bad->setAttribute ("idx", 99);
            bad->setAttribute ("data", "QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVo=");
            juce::MemoryBlock corrupt;
            juce::AudioProcessor::copyXmlToBinary (*xml, corrupt);

            PreChorusProcessor b;
            b.prepareToPlay (44100.0, 256);
            b.setStateInformation (corrupt.getData(), (int) corrupt.getSize());
            expect (! b.isSlotFilled (2));
            expect (b.isSlotFilled (0));

            // Old (v0.0.1) state: no EMBED node at all -> existing slots are left alone.
            xml->deleteAllChildElementsWithTagName ("EMBED");
            juce::MemoryBlock legacy;
            juce::AudioProcessor::copyXmlToBinary (*xml, legacy);
            PreChorusProcessor c;
            c.prepareToPlay (44100.0, 256);
            { const juce::ScopedLock sl (c.sourceLock); c.captureSlots[4] = juce::AudioBuffer<float> (2, 100); c.captureSlotFilled[4] = true; }
            c.setStateInformation (legacy.getData(), (int) legacy.getSize());
            expect (c.isSlotFilled (4));
        }

        beginTest ("diagnostics report is useful and leaks no folder paths");
        {
            PreChorusProcessor a;
            a.prepareToPlay (48000.0, 512);
            a.logStatus ("Loaded: vocal.wav");
            auto wav = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("secret-folder-name").getChildFile ("x.wav");
            a.currentFile = wav;
            const auto rep = a.getDiagnosticsReport();
            expect (rep.contains ("PreChorus diagnostics"));
            expect (rep.contains ("Block size: 512"));
            expect (rep.contains ("Loaded: vocal.wav"));
            expect (rep.contains ("x.wav"));
            expect (! rep.contains ("secret-folder-name"));
            expect (rep.length() < 8000);
        }

        beginTest ("keytrack keeps the hit on the reported latency for every transposition");
        {
            PreChorusProcessor p;
            p.prepareToPlay (48000.0, 256);
            p.setParam (IDs::align, 1.0f);
            p.render();
            auto r = p.getRendered();
            expect (r != nullptr && r->hitIndex > 1000);
            const int H = r->hitIndex;
            expectEquals (p.reportedLatency.load(), H);
            for (double rate : { 0.25, 0.5, 0.7071, 0.9439, 1.0, 1.0595, 1.5, 2.0, 4.0 })
            {
                for (auto& v : p.voices) v.active = false;
                p.startVoice (1.0f, rate, 0);
                const auto& v = p.voices[0];
                expect (v.active);
                const double samplesToHit = ((double) H - v.pos) / v.rate;
                expectWithinAbsoluteError (samplesToHit, (double) H, 1.0);
            }
            // a note offset inside the block is preserved on top of the alignment
            for (auto& v : p.voices) v.active = false;
            p.startVoice (1.0f, 2.0, 100);
            expectWithinAbsoluteError (((double) H - p.voices[0].pos) / p.voices[0].rate, (double) H + 100.0, 1.0);
        }

        beginTest ("undo/redo restores sound-design edits as grouped steps and ignores project loads");
        {
            PreChorusProcessor p;
            p.undoIdleMs = 0.0;                         // every tick closes the burst (no waiting in tests)
            auto tick = [&] { p.timerCallback(); p.timerCallback(); };
            expect (! p.canUndo());
            const float def = p.param (IDs::space);

            p.setParam (IDs::space, 0.2f); tick();
            p.setParam (IDs::space, 0.6f); tick();
            p.setParam (IDs::drive, 0.9f); p.setParam (IDs::air, 0.8f); tick();   // two params = one step
            expect (p.canUndo());

            p.undo();
            expectWithinAbsoluteError (p.param (IDs::drive), 0.2f, 0.011f);       // drive default
            expectWithinAbsoluteError (p.param (IDs::space), 0.6f, 0.011f);
            tick();
            p.undo();
            expectWithinAbsoluteError (p.param (IDs::space), 0.2f, 0.011f);
            tick();
            p.undo();
            expectWithinAbsoluteError (p.param (IDs::space), def, 0.011f);
            tick();
            expect (! p.canUndo());
            expect (p.canRedo());

            p.redo(); tick();
            expectWithinAbsoluteError (p.param (IDs::space), 0.2f, 0.011f);
            p.redo(); tick();
            expectWithinAbsoluteError (p.param (IDs::space), 0.6f, 0.011f);

            // a new edit after undo discards the redo branch
            p.undo(); tick();
            p.setParam (IDs::space, 0.33f); tick();
            expect (! p.canRedo());

            // project load starts a fresh history
            juce::MemoryBlock blob; p.getStateInformation (blob);
            p.setStateInformation (blob.getData(), (int) blob.getSize());
            tick();
            expect (! p.canUndo());
        }

        beginTest ("background render publishes without blocking, never overwrites a newer sync render, and audio keeps running");
        {
            PreChorusProcessor p;
            p.prepareToPlay (48000.0, 256);
            p.setParam (IDs::voiceCount, 32.0f);
            expect (p.getRendered() == nullptr);

            const double t0 = juce::Time::getMillisecondCounterHiRes();
            p.timerCallback();                            // consumes the startup dirty flag and hands it to the worker
            const double kickMs = juce::Time::getMillisecondCounterHiRes() - t0;
            expect (kickMs < 100.0);                      // the message thread is not blocked by the render

            // audio thread keeps processing while the worker renders
            juce::AudioBuffer<float> buf (2, 256);
            juce::MidiBuffer midi;
            int blocks = 0;
            const double deadline = juce::Time::getMillisecondCounterHiRes() + 20000.0;
            while (p.getRendered() == nullptr && juce::Time::getMillisecondCounterHiRes() < deadline)
            {
                buf.clear();
                p.processBlock (buf, midi);
                ++blocks;
                p.timerCallback();
                juce::Thread::sleep (2);
            }
            expect (p.getRendered() != nullptr);
            expect (blocks > 0);

            // a change made after a synchronous render wins over any older in-flight result
            p.setParam (IDs::space, 0.77f);
            p.timerCallback();                            // starts a background job
            const double jobDeadline = juce::Time::getMillisecondCounterHiRes() + 5000.0;
            while (! p.renderInFlight.load() && juce::Time::getMillisecondCounterHiRes() < jobDeadline) juce::Thread::sleep (1);
            expect (p.renderInFlight.load());             // the job really is running before the sync render
            p.setParam (IDs::space, 0.11f);
            p.render();                                   // synchronous: makes the older job stale
            auto syncResult = p.getRendered();
            juce::Thread::sleep (1500);
            p.timerCallback();                            // would publish the stale result if it were not discarded
            expect (p.getRendered() == syncResult);
        }

        beginTest ("max source length limits file loads, resizes the capture buffer, and old projects keep 12 s");
        {
            auto wav = juce::File::getSpecialLocation (juce::File::tempDirectory).getNonexistentChildFile ("prechorus-long", ".wav", false);
            {
                juce::AudioBuffer<float> tone (2, 8000 * 40);                  // 40 s at 8 kHz
                for (int i = 0; i < tone.getNumSamples(); ++i) { const float v = 0.3f * std::sin (0.2f * (float) i); tone.setSample (0, i, v); tone.setSample (1, i, v); }
                juce::WavAudioFormat fmt;
                std::unique_ptr<juce::AudioFormatWriter> w (fmt.createWriterFor (new juce::FileOutputStream (wav), 8000.0, 2, 16, {}, 0));
                expect (w != nullptr);
                w->writeFromAudioSampleBuffer (tone, 0, tone.getNumSamples());
            }
            PreChorusProcessor p;
            p.prepareToPlay (48000.0, 256);
            expectEquals (p.getMaxSourceSeconds(), 30);
            expectEquals (p.captureRingBuffer.getNumSamples(), 48000 * 30);

            expect (p.loadSampleFile (wav, false, false));
            expectEquals (p.getLastLoadFileSeconds(), 40);
            expectEquals (p.getLastLoadUsedSeconds(), 30);
            { const juce::ScopedLock sl (p.sourceLock); expectEquals (p.loadedBuffer.getNumSamples(), 8000 * 30); }

            p.setMaxSourceSeconds (60);
            expectEquals (p.captureRingBuffer.getNumSamples(), 48000 * 60);
            expect (p.loadSampleFile (wav, false, false));
            expectEquals (p.getLastLoadUsedSeconds(), 40);
            p.setMaxSourceSeconds (999);                                       // snaps to the largest option
            expectEquals (p.getMaxSourceSeconds(), 120);
            p.setMaxSourceSeconds (12);
            expectEquals (p.captureRingBuffer.getNumSamples(), 48000 * 12);

            // the setting is stored in the project; a project without it (v0.0.1) restores to 12 s
            p.setMaxSourceSeconds (60);
            juce::MemoryBlock blob; p.getStateInformation (blob);
            PreChorusProcessor q; q.prepareToPlay (48000.0, 256);
            q.setStateInformation (blob.getData(), (int) blob.getSize());
            expectEquals (q.getMaxSourceSeconds(), 60);
            expectEquals (q.captureRingBuffer.getNumSamples(), 48000 * 60);

            auto xml = juce::AudioProcessor::getXmlFromBinary (blob.getData(), (int) blob.getSize());
            xml->removeAttribute ("maxSourceSec");
            juce::MemoryBlock legacy; juce::AudioProcessor::copyXmlToBinary (*xml, legacy);
            PreChorusProcessor r2; r2.prepareToPlay (48000.0, 256);
            r2.setStateInformation (legacy.getData(), (int) legacy.getSize());
            expectEquals (r2.getMaxSourceSeconds(), 12);
            wav.deleteFile();
        }

        beginTest ("fuzz: random parameter sets render finite, bounded audio in every source mode");
        {
            PreChorusProcessor p;
            p.prepareToPlay (44100.0, 512);
            p.asyncRender = false;
            juce::Random rng (2024);
            int badRender = 0, badAudio = 0;
            float worstPeak = 0.0f;
            for (int iter = 0; iter < 30; ++iter)
            {
                for (auto* prm : p.getParameters())
                    prm->setValueNotifyingHost (rng.nextFloat());
                p.setParam (IDs::sourceMode, (float) (iter % 4));
                p.setParam (IDs::voiceCount, (float) (1 + rng.nextInt (32)));
                p.render();
                auto r = p.getRendered();
                if (r == nullptr || r->audio.getNumSamples() == 0) { ++badRender; continue; }
                for (int ch = 0; ch < r->audio.getNumChannels(); ++ch)
                    for (int i = 0; i < r->audio.getNumSamples(); ++i)
                        if (! std::isfinite (r->audio.getSample (ch, i))) { ++badRender; ch = 99; break; }

                // play the swarm through the audio callback with MIDI triggers
                for (int blk = 0; blk < 40; ++blk)
                {
                    juce::AudioBuffer<float> buf (2, 512);
                    for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) buf.setSample (c, i, 0.2f * std::sin (0.03f * (float) (i + blk * 512)));
                    juce::MidiBuffer midi;
                    if (blk == 0) midi.addEvent (juce::MidiMessage::noteOn (1, 40 + rng.nextInt (50), 0.9f), 10);
                    p.processBlock (buf, midi);
                    for (int c = 0; c < 2; ++c)
                    {
                        worstPeak = juce::jmax (worstPeak, buf.getMagnitude (c, 0, 512));
                        for (int i = 0; i < 512; ++i) if (! std::isfinite (buf.getSample (c, i))) ++badAudio;
                    }
                }
            }
            expectEquals (badRender, 0);
            expectEquals (badAudio, 0);
            logMessage ("fuzz worst output peak: " + juce::String (worstPeak));
            expect (worstPeak < 16.0f);                 // not a limiter test: catches runaway gain / filter blow-ups
        }

        beginTest ("editor constructs, lays out and paints headlessly (set PRECHORUS_SNAPSHOT_DIR to save a PNG)");
        {
            PreChorusProcessor p;
            p.prepareToPlay (44100.0, 512);
            p.asyncRender = false;
            p.render();
            std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
            expect (ed != nullptr);
            if (ed != nullptr)
            {
                expect (ed->getWidth() > 400 && ed->getHeight() > 300);
                auto img = ed->createComponentSnapshot (ed->getLocalBounds());
                expect (img.isValid());
                const auto dir = juce::SystemStats::getEnvironmentVariable ("PRECHORUS_SNAPSHOT_DIR", {});
                if (dir.isNotEmpty() && img.isValid())
                {
                    juce::File out = juce::File (dir).getChildFile ("editor.png");
                    juce::FileOutputStream fos (out);
                    if (fos.openedOk()) { fos.setPosition (0); fos.truncate(); juce::PNGImageFormat().writeImageToStream (img, fos); }
                }
            }
            ed.reset();
            p.simpleView = false;
            std::unique_ptr<juce::AudioProcessorEditor> full (p.createEditor());
            expect (full != nullptr);
            if (full != nullptr)
            {
                auto img = full->createComponentSnapshot (full->getLocalBounds());
                expect (img.isValid());
                const auto dir = juce::SystemStats::getEnvironmentVariable ("PRECHORUS_SNAPSHOT_DIR", {});
                if (dir.isNotEmpty() && img.isValid())
                {
                    juce::File out = juce::File (dir).getChildFile ("editor_full.png");
                    juce::FileOutputStream fos (out);
                    if (fos.openedOk()) { fos.setPosition (0); fos.truncate(); juce::PNGImageFormat().writeImageToStream (img, fos); }
                }
            }
        }

        beginTest ("simple/full view preference is stored with the project; old projects open in full view");
        {
            PreChorusProcessor a;
            expect (a.simpleView);                                   // new instances start simple
            a.simpleView = false;
            juce::MemoryBlock blob; a.getStateInformation (blob);
            PreChorusProcessor b; b.setStateInformation (blob.getData(), (int) blob.getSize());
            expect (! b.simpleView);
            a.simpleView = true; a.getStateInformation (blob);
            b.setStateInformation (blob.getData(), (int) blob.getSize());
            expect (b.simpleView);

            auto xml = juce::AudioProcessor::getXmlFromBinary (blob.getData(), (int) blob.getSize());
            xml->removeAttribute ("simpleView");
            juce::MemoryBlock legacy; juce::AudioProcessor::copyXmlToBinary (*xml, legacy);
            PreChorusProcessor c; c.setStateInformation (legacy.getData(), (int) legacy.getSize());
            expect (! c.simpleView);
        }

        beginTest ("formant shift moves the spectral envelope while the pitch stays put");
        {
            const double sr = 44100.0; const int n = (int) (sr * 1.0);
            std::vector<float> x ((size_t) n, 0.0f);
            const double f0 = 200.0;
            for (int h = 1; h <= 40; ++h)
            {
                const double fh = f0 * h;
                const double amp = std::exp (-std::pow ((fh - 900.0) / 500.0, 2.0)) + 0.02;
                for (int i = 0; i < n; ++i) x[(size_t) i] += (float) (amp * std::sin (juce::MathConstants<double>::twoPi * fh * i / sr));
            }
            float mx = 0; for (float v : x) mx = juce::jmax (mx, std::abs (v));
            for (auto& v : x) v *= 0.5f / mx;
            auto centroid = [&] (const std::vector<float>& s)
            {
                PCDsp::FFT fft (8192);
                std::vector<std::complex<float>> buf (8192);
                for (int i = 0; i < 8192; ++i) buf[(size_t) i] = s[(size_t) (8192 + i)] * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / 8192.0f));
                fft.perform (buf.data(), false);
                double num = 0, den = 0, peak = 0; int peakBin = 0;
                for (int k = 1; k < 2048; ++k)
                {
                    const double m = std::abs (buf[(size_t) k]); num += m * k; den += m;
                    if (m > peak && k * sr / 8192.0 < 150.0) {}
                }
                // fundamental: strongest bin below 300 Hz
                for (int k = 1; k < (int) (300.0 * 8192 / sr); ++k) { const double m = std::abs (buf[(size_t) k]); if (m > peak) { peak = m; peakBin = k; } }
                return std::make_pair (num / den * sr / 8192.0, peakBin * sr / 8192.0);
            };
            auto before = centroid (x);
            std::vector<float> up = x, down = x;
            PCDsp::formantShiftChannel (up.data(), n, 7.0f, sr);
            PCDsp::formantShiftChannel (down.data(), n, -7.0f, sr);
            auto cu = centroid (up), cd = centroid (down);
            logMessage ("formant centroid: down " + juce::String (cd.first) + " / orig " + juce::String (before.first) + " / up " + juce::String (cu.first));
            expect (cu.first > before.first * 1.12);
            expect (cd.first < before.first * 0.88);
            expectWithinAbsoluteError (cu.second, before.second, 12.0);   // pitch unchanged
            expectWithinAbsoluteError (cd.second, before.second, 12.0);
            bool allFinite = true; for (float v : up) allFinite = allFinite && std::isfinite (v);
            expect (allFinite);
        }

        beginTest ("slice scatter splits at transients and falls back to equal slices");
        {
            const double sr = 44100.0; const int n = (int) (sr * 2.0);
            juce::AudioBuffer<float> b (2, n); b.clear();
            juce::Random r (5);
            const int hits[] = { 4000, 26000, 50000, 71000 };
            for (int h : hits)
                for (int i = 0; i < 6000 && h + i < n; ++i)
                {
                    const float s = (r.nextFloat() * 2.0f - 1.0f) * std::exp (-(float) i / 1500.0f);
                    b.setSample (0, h + i, s); b.setSample (1, h + i, s);
                }
            auto starts = PCDsp::detectSliceStarts (b, sr, 8);
            expect (starts.size() == 5);
            expectEquals (starts[0], 0);
            for (size_t k = 1; k < starts.size() && k <= 4; ++k)
                expect (std::abs (starts[k] - hits[k - 1]) < 700);
            juce::AudioBuffer<float> sine (2, n);
            for (int i = 0; i < n; ++i) { const float s = 0.3f * std::sin (0.05f * (float) i); sine.setSample (0, i, s); sine.setSample (1, i, s); }
            auto eq = PCDsp::detectSliceStarts (sine, sr, 8);
            expectEquals ((int) eq.size(), 8);
            expectEquals (eq[1], n / 8);
        }

        beginTest ("smart trim removes edge silence, keeps a minimum length and can normalise");
        {
            const double sr = 48000.0;
            juce::AudioBuffer<float> b (2, (int) (sr * 2.5)); b.clear();
            for (int i = 0; i < (int) (sr * 0.5); ++i) { const float s = 0.25f * std::sin (0.1f * (float) i); b.setSample (0, (int) (sr * 1.0) + i, s); b.setSample (1, (int) (sr * 1.0) + i, s); }
            expect (PCDsp::smartTrim (b, sr, -50.0f, 0.05, 10.0, true, -1.0f));
            expectWithinAbsoluteError ((double) b.getNumSamples() / sr, 0.5, 0.01);
            expectWithinAbsoluteError ((double) b.getMagnitude (0, b.getNumSamples()), (double) juce::Decibels::decibelsToGain (-1.0f), 0.02);
            expect (std::abs (b.getSample (0, 0)) < 0.01f);                 // faded in
            juce::AudioBuffer<float> tiny (2, (int) sr); tiny.clear();
            tiny.setSample (0, 24000, 0.5f);
            PCDsp::smartTrim (tiny, sr);
            expect (tiny.getNumSamples() >= (int) (sr * 0.05) - 2);
            juce::AudioBuffer<float> silent (2, 1000); silent.clear();
            expect (! PCDsp::smartTrim (silent, sr));                       // all silent: untouched
            expectEquals (silent.getNumSamples(), 1000);
        }

        beginTest ("sustain tail is kept under the target hit, layers extend the render and stay finite");
        {
            PreChorusProcessor p;
            p.prepareToPlay (44100.0, 512);
            p.asyncRender = false;
            p.setParam (IDs::sync, 0.0f); p.setParam (IDs::tail, 1.0f);
            p.setParam (IDs::postRelease, 1.0f); p.setParam (IDs::dryReplace, 0.0f);
            p.render();
            auto r0 = p.getRendered();
            const int hit = r0->hitIndex;
            expect (hit > 0);
            double e = 0; for (int i = hit + 8000; i < hit + 30000 && i < r0->audio.getNumSamples(); ++i) e += std::abs (r0->audio.getSample (0, i));
            expect (e > 1.0);
            const int baseLen = r0->audio.getNumSamples();
            p.setParam (IDs::laySub, 1.0f); p.setParam (IDs::layImpact, 1.0f); p.setParam (IDs::layRiser, 1.0f); p.setParam (IDs::layRev, 1.0f);
            p.render();
            auto r1 = p.getRendered();
            expectEquals (r1->hitIndex, hit);
            expect (r1->audio.getNumSamples() >= baseLen);
            double dc = 0; bool fin = true;
            for (int i = 0; i < r1->audio.getNumSamples(); ++i) { const float s = r1->audio.getSample (0, i); fin = fin && std::isfinite (s); dc += s; }
            expect (fin);
            expect (r1->audio.getMagnitude (0, r1->audio.getNumSamples()) <= 1.0f);
            // layers add energy in the pre-hit riser region
            double eRise0 = 0, eRise1 = 0;
            for (int i = hit - 20000; i < hit - 2000; ++i) { eRise0 += std::abs (r0->audio.getSample (0, i)); eRise1 += std::abs (r1->audio.getSample (0, i)); }
            expect (eRise1 != eRise0);
            juce::ignoreUnused (dc);
        }

        beginTest ("stutter grids: straight is unchanged, custom pattern gates, triplet/dotted/swing stay finite");
        {
            PreChorusProcessor p;
            p.prepareToPlay (44100.0, 512);
            p.asyncRender = false;
            p.setParam (IDs::sync, 0.0f); p.setParam (IDs::tail, 3.0f);
            p.setParam (IDs::stutter, 1.0f); p.setParam (IDs::space, 0.0f); p.setParam (IDs::distance, 0.0f);
            p.setParam (IDs::postRelease, 0.0f);
            p.setParam (IDs::stutterGrid, 3.0f); p.setParam (IDs::stutterPattern, 0.0f);   // every step off
            p.render();
            auto r = p.getRendered();
            const int hit = r->hitIndex;
            double e = 0; for (int i = hit * 6 / 10; i < hit - 2000; ++i) e += std::abs (r->audio.getSample (0, i));
            expect (e < 1.0e-3);
            p.setParam (IDs::stutterPattern, 65535.0f);                                    // every step on: fully open
            p.render();
            auto r2 = p.getRendered();
            double e2 = 0; for (int i = hit * 6 / 10; i < hit - 2000; ++i) e2 += std::abs (r2->audio.getSample (0, i));
            expect (e2 > 1.0);
            for (int grid = 0; grid < 4; ++grid)
                for (float swing : { 0.0f, 0.4f })
                {
                    p.setParam (IDs::stutterGrid, (float) grid); p.setParam (IDs::stutterSwing, swing); p.setParam (IDs::stutterPattern, 21845.0f);
                    p.render();
                    auto rr = p.getRendered();
                    bool ok = rr != nullptr;
                    for (int i = 0; ok && i < rr->audio.getNumSamples(); ++i) ok = std::isfinite (rr->audio.getSample (0, i));
                    expect (ok);
                }
        }

        beginTest ("new characters render finite, level-safe and deterministic");
        {
            PreChorusProcessor p;
            p.prepareToPlay (44100.0, 512);
            p.asyncRender = false;
            for (int ch = 0; ch <= 10; ++ch)
            {
                p.setParam (IDs::character, (float) ch);
                p.render();
                auto a = p.getRendered();
                p.render();
                auto b = p.getRendered();
                expect (a != nullptr && b != nullptr);
                bool same = a->audio.getNumSamples() == b->audio.getNumSamples();
                float peak = 0;
                for (int i = 0; same && i < a->audio.getNumSamples(); ++i)
                {
                    const float s = a->audio.getSample (0, i);
                    if (! std::isfinite (s) || s != b->audio.getSample (0, i)) same = false;
                    peak = juce::jmax (peak, std::abs (s));
                }
                expect (same);
                logMessage ("char " + juce::String (ch) + " peak " + juce::String (peak));
                expect (peak > 0.02f && peak <= 1.0f);
            }
        }

        beginTest ("formant knob and slice scatter render through the engine");
        {
            PreChorusProcessor p;
            p.prepareToPlay (44100.0, 512);
            p.asyncRender = false;
            p.setParam (IDs::formant, 5.0f);
            p.setParam (IDs::sourceMode, 3.0f);
            p.render();
            auto r = p.getRendered();
            expect (r != nullptr && r->audio.getNumSamples() > 1000);
            expect (std::isfinite (r->audio.getMagnitude (0, r->audio.getNumSamples())));
        }

        beginTest ("themed randomize is deterministic, honours lock groups and stays renderable");
        {
            PreChorusProcessor a, b;
            a.prepareToPlay (44100.0, 512); b.prepareToPlay (44100.0, 512);
            a.asyncRender = false; b.asyncRender = false;
            for (int theme = 0; theme < 6; ++theme)
            {
                a.setParam (IDs::seed, 777.0f); b.setParam (IDs::seed, 777.0f);
                a.randomizeThemed (theme); b.randomizeThemed (theme);
                bool same = true;
                for (auto& id : PreChorusProcessor::getSoundDesignIds()) same = same && juce::exactlyEqual (a.param (id), b.param (id));
                expect (same);
                a.render();
                auto r = a.getRendered();
                expect (r != nullptr && std::isfinite (r->audio.getMagnitude (0, r->audio.getNumSamples())));
            }
            a.setParam (IDs::seed, 31.0f); a.setParam (IDs::voiceCount, 12.0f); a.setParam (IDs::tilt, 0.0f);
            a.lockedGroups = (1u << 0);     // Swarm locked
            const float vc = a.param (IDs::voiceCount), seed = a.param (IDs::seed);
            a.randomizeThemed (3);
            expectEquals (a.param (IDs::voiceCount), vc);
            expectEquals (a.param (IDs::seed), seed);
            expect (a.param (IDs::turbulence) >= 0.3f);                 // unlocked groups still change
            a.lockedGroups = (1u << 3);     // Tone locked
            a.setParam (IDs::tilt, 0.37f);
            a.randomizeThemed (2);
            expectWithinAbsoluteError (a.param (IDs::tilt), 0.37f, 0.011f);
            a.lockedGroups = 0;
            expect (PreChorusProcessor::lockGroupOf (IDs::sourceMode) == -1);   // source controls are never randomized
        }

        beginTest ("A/B morph interpolates numeric parameters and steps choices at the midpoint");
        {
            PreChorusProcessor p;
            p.prepareToPlay (44100.0, 512);
            p.setParam (IDs::detune, 10.0f); p.setParam (IDs::character, 1.0f); p.setParam (IDs::space, 0.2f);
            p.switchABSlot();                                  // A stored, B starts as a copy
            p.setParam (IDs::detune, 30.0f); p.setParam (IDs::character, 4.0f); p.setParam (IDs::space, 0.6f);
            p.setParam (IDs::morph, 0.0f);
            p.applyMorph();
            expectWithinAbsoluteError (p.param (IDs::detune), 10.0f, 0.51f);
            expectEquals ((int) p.param (IDs::character), 1);
            p.setParam (IDs::morph, 1.0f); p.applyMorph();
            expectWithinAbsoluteError (p.param (IDs::detune), 30.0f, 0.51f);
            expectEquals ((int) p.param (IDs::character), 4);
            p.setParam (IDs::morph, 0.5f); p.applyMorph();
            expectWithinAbsoluteError (p.param (IDs::detune), 20.0f, 0.51f);
            expectWithinAbsoluteError (p.param (IDs::space), 0.4f, 0.011f);
            p.setParam (IDs::morph, 0.25f); p.applyMorph();
            expectEquals ((int) p.param (IDs::character), 1);
        }

        beginTest ("smart capture trims silence, honours the lock and offers a retake");
        {
            PreChorusProcessor p;
            p.prepareToPlay (48000.0, 512);
            p.setParam (IDs::captureMode, 0.0f); p.setParam (IDs::thresh, -30.0f);
            p.selectCaptureSlot (2);
            p.armCapture();
            auto feed = [&] (int blocks, float amp)
            {
                for (int b = 0; b < blocks; ++b)
                {
                    juce::AudioBuffer<float> buf (2, 512);
                    for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) buf.setSample (c, i, amp * std::sin (0.07f * (float) (i + b * 512)));
                    juce::MidiBuffer m; p.processBlock (buf, m);
                }
            };
            feed (10, 0.0f);          // silence: stays armed
            expect (p.isCaptureArmed());
            feed (30, 0.4f);          // ~0.32 s of tone
            feed (80, 0.0f);          // trailing silence ends the take
            p.commitFinishedCapture();
            expect (p.isSlotFilled (2));
            expectEquals (p.getLastCaptureSlot(), 2);
            expect (p.getLastCaptureSummary().contains ("Captured"));
            {
                const juce::ScopedLock sl (p.sourceLock);
                const double secs = p.captureSlots[2].getNumSamples() / 48000.0;
                expect (secs > 0.25 && secs < 0.5);                       // trailing silence trimmed away
            }
            p.setCaptureLock (true);
            p.armCapture(); feed (30, 0.4f); feed (80, 0.0f); p.commitFinishedCapture();
            expect (p.getActiveCaptureSlot() != 2);                       // locked take was not overwritten
            p.setCaptureLock (false);
            p.retakeCapture();
            expect (p.getActiveCaptureSlot() == p.getLastCaptureSlot() || p.isCaptureArmed());
        }

        beginTest ("export options write the chosen format, rate and level");
        {
            PreChorusProcessor p;
            p.prepareToPlay (44100.0, 512);
            p.asyncRender = false;
            p.setParam (IDs::sync, 0.0f); p.setParam (IDs::tail, 1.0f);
            p.render();
            auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("prechorus_export_test");
            dir.deleteRecursively(); dir.createDirectory();
            juce::AudioFormatManager fm; fm.registerBasicFormats();
            auto inspect = [&] (const juce::File& f, int& bits, double& rate, juce::int64& len, bool& isFloat)
            {
                std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (f));
                if (rd == nullptr) return false;
                bits = (int) rd->bitsPerSample; rate = rd->sampleRate; len = rd->lengthInSamples; isFloat = rd->usesFloatingPointData;
                return rd->numChannels == 2;
            };
            int bits; double rate; juce::int64 len; bool isFloat;
            PreChorusProcessor::ExportSettings s;
            for (int fmt = 0; fmt < 4; ++fmt)
            {
                s.format = fmt; s.rate = 0;
                auto f = dir.getChildFile ("t" + juce::String (fmt) + "." + PreChorusProcessor::exportExtension (s));
                juce::String err;
                expect (p.exportWithSettings (f, s, &err));
                expect (inspect (f, bits, rate, len, isFloat));
                expectEquals (bits, fmt == 0 ? 16 : fmt == 2 ? 32 : 24);
                expect (isFloat == (fmt == 2));
                expectWithinAbsoluteError (rate, 44100.0, 0.5);
            }
            s.format = 1; s.rate = 2;                     // 48 kHz
            auto f48 = dir.getChildFile ("t48.wav");
            expect (p.exportWithSettings (f48, s));
            expect (inspect (f48, bits, rate, len, isFloat));
            expectWithinAbsoluteError (rate, 48000.0, 0.5);
            expectWithinAbsoluteError ((double) len / 48000.0, (double) p.getRendered()->audio.getNumSamples() / 44100.0, 0.01);
            s.rate = 0; s.normalise = true; s.trimTail = true;
            auto fn = dir.getChildFile ("tn.wav");
            expect (p.exportWithSettings (fn, s));
            std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (fn));
            juce::AudioBuffer<float> got (2, (int) rd->lengthInSamples);
            rd->read (&got, 0, (int) rd->lengthInSamples, 0, true, true);
            expectWithinAbsoluteError ((double) got.getMagnitude (0, got.getNumSamples()), (double) juce::Decibels::decibelsToGain (-1.0f), 0.01);
            expect (rd->lengthInSamples <= p.getRendered()->audio.getNumSamples());
            expect (p.exportWav (dir.getChildFile ("legacy.wav")));
            // settings survive a project save/restore
            p.exportSettings.format = 3; p.exportSettings.rate = 3; p.exportSettings.normalise = true;
            juce::MemoryBlock blob; p.getStateInformation (blob);
            PreChorusProcessor q; q.setStateInformation (blob.getData(), (int) blob.getSize());
            expectEquals (q.exportSettings.format, 3); expectEquals (q.exportSettings.rate, 3); expect (q.exportSettings.normalise);
            dir.deleteRecursively();
        }

        beginTest ("MIDI learn maps a CC to a parameter, applies it on the message thread and persists");
        {
            PreChorusProcessor p;
            p.prepareToPlay (44100.0, 512);
            p.setParam (IDs::macro, 0.1f);
            p.startMidiLearn (IDs::macro);
            expect (p.getMidiLearnParam() == IDs::macro);
            auto sendCC = [&] (int cc, int val)
            {
                juce::AudioBuffer<float> buf (2, 256); buf.clear();
                juce::MidiBuffer m; m.addEvent (juce::MidiMessage::controllerEvent (1, cc, val), 5);
                p.processBlock (buf, m);
            };
            sendCC (20, 64);
            expectEquals (p.getMappedMidiSource (IDs::macro), 20);
            p.applyMidiMappings();
            expectWithinAbsoluteError (p.param (IDs::macro), 64.0f / 127.0f, 0.01f);
            sendCC (20, 127); p.applyMidiMappings();
            expectWithinAbsoluteError (p.param (IDs::macro), 1.0f, 0.01f);
            sendCC (21, 10); p.applyMidiMappings();                         // unmapped CC does nothing
            expectWithinAbsoluteError (p.param (IDs::macro), 1.0f, 0.01f);
            p.startMidiLearn (IDs::macro); sendCC (30, 0); p.applyMidiMappings();
            expectEquals (p.getMappedMidiSource (IDs::macro), 30);          // newest source wins, old one released
            expectEquals (p.getMappedMidiSource (IDs::space), -1);
            juce::MemoryBlock blob; p.getStateInformation (blob);
            PreChorusProcessor q; q.setStateInformation (blob.getData(), (int) blob.getSize());
            expectEquals (q.getMappedMidiSource (IDs::macro), 30);
            q.clearMidiMapping (IDs::macro);
            expectEquals (q.getMappedMidiSource (IDs::macro), -1);
            expect (PreChorusProcessor::midiSourceName (1) == "CC 1");
        }

        beginTest ("auto targets: next bar line, loop end and input transient land the hit exactly");
        {
            struct Host : juce::AudioPlayHead
            {
                double ppq = 0, bpm = 120; bool playing = true, looping = false; double loopStart = 0, loopEnd = 0; juce::int64 sample = 0;
                juce::Optional<PositionInfo> getPosition() const override
                {
                    PositionInfo i; i.setBpm (bpm); i.setPpqPosition (ppq); i.setIsPlaying (playing); i.setTimeInSamples (sample);
                    i.setTimeSignature (TimeSignature { 4, 4 }); i.setIsLooping (looping);
                    if (looping) i.setLoopPoints (LoopPoints { loopStart, loopEnd });
                    return i;
                }
            } host;
            const double sr = 44100.0; const int bs = 512;
            PreChorusProcessor p;
            p.prepareToPlay (sr, bs);
            p.setPlayHead (&host);
            p.asyncRender = false;
            p.setParam (IDs::sync, 0.0f); p.setParam (IDs::tail, 1.0f); p.setParam (IDs::tgtSource, 1.0f);
            p.render();
            const int hit = p.getRendered()->hitIndex;
            expect (hit > 10000);
            expectEquals (p.reportedLatency.load(), 0);             // auto modes schedule themselves, no PDC latency
            const double spp = sr * 60.0 / host.bpm;
            const double barSamples = 4.0 * spp;
            juce::int64 firstStartAbs = -1;
            bool foundStart = false;
            for (int blk = 0; blk < 200 && ! foundStart; ++blk)
            {
                host.sample = (juce::int64) blk * bs; host.ppq = (double) host.sample / spp;
                juce::AudioBuffer<float> buf (2, bs); buf.clear(); juce::MidiBuffer m;
                p.processBlock (buf, m);
                for (auto& v : p.voices)
                    if (v.active) { firstStartAbs = host.sample + (juce::int64) std::llround ((double) bs - v.pos); foundStart = true; break; }
            }
            // the first bar line after the transport start is bar 1 (bar 0 is already behind us at sample 0 -> partial start)
            expect (foundStart);
            const double hitAbs = (double) firstStartAbs + hit;
            const double nearestBar = std::round (hitAbs / barSamples) * barSamples;
            expectWithinAbsoluteError (hitAbs, nearestBar, 2.0);

            // loop end
            for (auto& v : p.voices) v.active = false;
            p.setParam (IDs::tgtSource, 2.0f);
            host.looping = true; host.loopStart = 0.0; host.loopEnd = 8.0;       // 2 bars
            juce::int64 startAbs = -1; bool foundLoop = false;
            for (int blk = 0; blk < 400 && ! foundLoop; ++blk)
            {
                host.sample = (juce::int64) blk * bs; host.ppq = (double) host.sample / spp;
                if (host.ppq >= host.loopEnd) break;
                juce::AudioBuffer<float> buf (2, bs); buf.clear(); juce::MidiBuffer m;
                p.processBlock (buf, m);
                for (auto& v : p.voices) if (v.active) { startAbs = host.sample + (juce::int64) std::llround ((double) bs - v.pos); foundLoop = true; break; }
            }
            expect (foundLoop);
            expectWithinAbsoluteError ((double) startAbs + hit, host.loopEnd * spp, 2.0);

            // input transient
            for (auto& v : p.voices) v.active = false;
            p.setParam (IDs::tgtSource, 3.0f); p.setParam (IDs::tgtThresh, -20.0f);
            host.playing = false; host.looping = false;
            { juce::AudioBuffer<float> quiet (2, bs); quiet.clear(); juce::MidiBuffer m; p.processBlock (quiet, m); }
            for (auto& v : p.voices) expect (! v.active);
            juce::AudioBuffer<float> click (2, bs); click.clear();
            click.setSample (0, 100, 0.8f); click.setSample (1, 100, 0.8f);
            juce::MidiBuffer m2; p.processBlock (click, m2);
            bool started = false;
            for (auto& v : p.voices) if (v.active) { started = true; expectWithinAbsoluteError (v.pos, (double) (bs - 100), 1.5); }
            expect (started);
        }

        beginTest ("preset library: tags, search, favourites, recents, rename/delete and bank round-trip");
        {
            auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("prechorus_lib_test");
            dir.deleteRecursively();
            auto presets = dir.getChildFile ("Presets"); presets.createDirectory();
            PreChorusProcessor p; juce::String err;
            expect (p.saveUserPreset (presets.getChildFile ("My Riser.pcpreset"), err));
            presets.getChildFile ("Sub").createDirectory();
            expect (p.saveUserPreset (presets.getChildFile ("Sub").getChildFile ("Deep Pad.pcpreset"), err));
            {
                PresetLibrary lib (presets, dir.getChildFile ("settings.xml"));
                expectEquals ((int) lib.entries().size(), PreChorusProcessor::getFactoryPresetNames().size() + 2);
                expectEquals (PreChorusProcessor::getFactoryPresetNames().size(), 24);
                for (int i = 0; i < PreChorusProcessor::getFactoryPresetNames().size(); ++i)
                    expect (! PreChorusProcessor::getFactoryPresetTags (i).isEmpty());
                auto dubs = lib.filter ("", juce::StringArray { "Dubstep" }, false, 0);
                expect (dubs.size() >= 3);
                auto hit = lib.filter ("riser edm", {}, false, 0);        // every word must match name/tags
                expect (! hit.empty());
                for (int i : hit) expect (lib.entries()[(size_t) i].name.containsIgnoreCase ("riser") || lib.entries()[(size_t) i].tags.contains ("EDM", true));
                expect (lib.filter ("zzzz-nothing", {}, false, 0).empty());
                auto users = lib.filter ("", juce::StringArray { "User" }, false, 0);
                expectEquals ((int) users.size(), 2);
                lib.toggleFavorite (users[0]);
                lib.markRecent (lib.entries()[(size_t) users[1]].key);
                expectEquals ((int) lib.filter ("", {}, true, 0).size(), 1);
                expect (lib.filter ("", {}, false, 1).front() == users[1]);   // most recent first
            }
            {
                PresetLibrary lib (presets, dir.getChildFile ("settings.xml"));     // favourites persist
                expectEquals ((int) lib.filter ("", {}, true, 0).size(), 1);
                const int idx = lib.indexOfKey ("U:My Riser.pcpreset");
                expect (idx >= 0);
                expect (lib.renameUser (idx, "Renamed Riser", err));
                expect (presets.getChildFile ("Renamed Riser.pcpreset").existsAsFile());
                expect (lib.indexOfKey ("U:Renamed Riser.pcpreset") >= 0);
                expect (lib.renameUser (lib.indexOfKey ("U:Renamed Riser.pcpreset"), "Sub/../evil", err));   // path separators are neutralised
                expect (! presets.getParentDirectory().getChildFile ("evil.pcpreset").exists() && ! presets.getChildFile ("evil.pcpreset").exists());
                for (auto& e : lib.entries()) if (e.name.containsIgnoreCase ("evil")) expect (e.file.getParentDirectory() == presets);
                for (auto& e : lib.entries()) if (e.name.containsIgnoreCase ("evil")) expect (lib.renameUser (lib.indexOfKey (e.key), "Renamed Riser", err));
                expect (! lib.renameUser (0, "x", err));                              // factory presets are read-only
                // bank export / import
                auto bank = dir.getChildFile ("test.pcbank");
                juce::Array<juce::File> files { presets.getChildFile ("Renamed Riser.pcpreset"), presets.getChildFile ("Sub").getChildFile ("Deep Pad.pcpreset") };
                expect (lib.exportBank (files, bank, "My Bank", "Tester", err));
                PresetLibrary other (dir.getChildFile ("Other"), dir.getChildFile ("o.xml"));
                juce::String report;
                const int nImp = other.importBank (bank, report); logMessage ("import: " + report);
                expectEquals (nImp, 2);
                expect (dir.getChildFile ("Other").getChildFile ("Banks").getChildFile ("My Bank").getChildFile ("Renamed Riser.pcpreset").existsAsFile());
                expectEquals ((int) other.filter ("", juce::StringArray { "User" }, false, 0).size(), 2);
                juce::ValueTree a, b;
                expect (PreChorusProcessor::parsePresetFile (files[0], a, err));
                expect (PreChorusProcessor::parsePresetFile (dir.getChildFile ("Other").getChildFile ("Banks").getChildFile ("My Bank").getChildFile ("Renamed Riser.pcpreset"), b, err));
                expect (a.isEquivalentTo (b));
                expect (lib.deleteUser (lib.indexOfKey ("U:Renamed Riser.pcpreset"), err));
                expect (! presets.getChildFile ("Renamed Riser.pcpreset").exists());
            }
            dir.deleteRecursively();
        }

        beginTest ("preset banks reject zip-slip, absolute paths, oversize entries, bad XML, duplicates and foreign products");
        {
            auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("prechorus_bank_test");
            dir.deleteRecursively(); dir.createDirectory();
            PreChorusProcessor p; juce::String err;
            auto good = dir.getChildFile ("good.pcpreset");
            expect (p.saveUserPreset (good, err));
            juce::MemoryBlock goodData; good.loadFileAsData (goodData);
            auto manifest = [] (const char* product, int schema)
            { return juce::String ("<PRECHORUS_BANK product=\"") + product + "\" schema=\"" + juce::String (schema) + "\" name=\"Evil\"/>"; };
            auto build = [&] (const juce::File& dest, const juce::String& man, std::vector<std::pair<juce::String, juce::MemoryBlock>> entries)
            {
                juce::ZipFile::Builder b;
                for (auto& e : entries) b.addEntry (new juce::MemoryInputStream (e.second, true), 5, e.first, juce::Time::getCurrentTime());
                if (man.isNotEmpty()) b.addEntry (new juce::MemoryInputStream (man.toRawUTF8(), (size_t) man.getNumBytesAsUTF8(), true), 5, "manifest.xml", juce::Time::getCurrentTime());
                dest.deleteFile(); juce::FileOutputStream os (dest); b.writeToStream (os, nullptr);
            };
            juce::MemoryBlock huge; huge.setSize (300 * 1024); huge.fillWith (' ');
            juce::MemoryBlock junk ("<not-a-preset/>", 15);
            const char garbageText[] = "\x01\x02 not xml at all";
            juce::MemoryBlock garbage (garbageText, sizeof (garbageText) - 1);
            auto bad = dir.getChildFile ("bad.pcbank");
            build (bad, manifest ("PreChorus", 1), { { "../../evil.pcpreset", goodData }, { "/abs/evil.pcpreset", goodData },
                                                      { "C:\\evil.pcpreset", goodData }, { "presets/ok.pcpreset", goodData },
                                                      { "presets/OK.pcpreset", goodData }, { "presets/huge.pcpreset", huge },
                                                      { "presets/junk.pcpreset", junk }, { "presets/garbage.pcpreset", garbage },
                                                      { "presets/notpreset.txt", goodData } });
            PresetLibrary lib (dir.getChildFile ("Presets"), dir.getChildFile ("s.xml"));
            juce::String report;
            expectEquals (lib.importBank (bad, report), 1);                         // only presets/ok.pcpreset survives
            expect (! dir.getChildFile ("evil.pcpreset").exists() && ! dir.getParentDirectory().getChildFile ("evil.pcpreset").exists());
            auto foreign = dir.getChildFile ("foreign.pcbank");
            build (foreign, manifest ("SomethingElse", 1), { { "presets/a.pcpreset", goodData } });
            expectEquals (lib.importBank (foreign, report), 0);
            auto future = dir.getChildFile ("future.pcbank");
            build (future, manifest ("PreChorus", 2), { { "presets/a.pcpreset", goodData } });
            expectEquals (lib.importBank (future, report), 0);
            auto noMan = dir.getChildFile ("noman.pcbank");
            build (noMan, {}, { { "presets/a.pcpreset", goodData } });
            expectEquals (lib.importBank (noMan, report), 0);
            auto notZip = dir.getChildFile ("notzip.pcbank"); notZip.replaceWithText ("hello");
            expectEquals (lib.importBank (notZip, report), 0);
            expect (! PresetLibrary::isSafeEntryName ("a/../b") && PresetLibrary::isSafeEntryName ("presets/a.pcpreset"));
            dir.deleteRecursively();
        }

        beginTest ("optional sidechain input drives the detectors and stem outputs sum to the main output");
        {
            PreChorusProcessor p;
            juce::AudioProcessor::BusesLayout lay;
            lay.inputBuses.add (juce::AudioChannelSet::stereo());
            lay.inputBuses.add (juce::AudioChannelSet::stereo());
            for (int i = 0; i < 4; ++i) lay.outputBuses.add (juce::AudioChannelSet::stereo());
            expect (p.isBusesLayoutSupported (lay));
            expect (p.setBusesLayout (lay));
            const int bs = 512;
            p.prepareToPlay (44100.0, bs);
            p.asyncRender = false;
            p.setParam (IDs::sync, 0.0f); p.setParam (IDs::tail, 0.4f);
            p.render();
            expectEquals (p.getTotalNumInputChannels(), 4);
            expectEquals (p.getTotalNumOutputChannels(), 8);

            // threshold capture reacts to the sidechain only when the detector source is Sidechain
            p.setParam (IDs::captureMode, 0.0f); p.setParam (IDs::thresh, -30.0f);
            p.armCapture();
            auto block = [&] (float mainAmp, float scAmp, juce::MidiBuffer m = {})
            {
                juce::AudioBuffer<float> b (8, bs); b.clear();
                for (int i = 0; i < bs; ++i)
                {
                    const float s = std::sin (0.05f * (float) i);
                    for (int c = 0; c < 2; ++c) b.setSample (c, i, mainAmp * s);          // main input
                    for (int c = 2; c < 4; ++c) b.setSample (c, i, scAmp * s);            // sidechain input
                }
                p.processBlock (b, m);
                return b;
            };
            p.setParam (IDs::scSource, 0.0f);
            block (0.0f, 0.8f); block (0.0f, 0.8f);
            expect (! p.isCapturing());                          // main input is silent: detector ignores the sidechain
            p.setParam (IDs::scSource, 1.0f);
            block (0.0f, 0.8f); block (0.0f, 0.8f);
            expect (p.isCapturing());                            // sidechain loud: capture starts
            p.stopCapture(); block (0.0f, 0.0f); block (0.0f, 0.0f);
            for (int i = 0; i < 600; ++i) { block (0.0f, 0.0f); if (p.getCaptureState() == PreChorusProcessor::CaptureState::idle) break; }
            p.setParam (IDs::scSource, 0.0f);

            // stems: Swarm + Hit + Dry == main output, and Swarm/Hit actually carry audio
            p.setParam (IDs::wet, 1.0f); p.setParam (IDs::dry, 1.0f);
            double maxErr = 0.0, eSwarm = 0.0, eHit = 0.0, eDry = 0.0;
            for (int blk = 0; blk < 120; ++blk)
            {
                juce::MidiBuffer m;
                if (blk == 0) m.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 4);
                auto b = block (0.25f, 0.0f, m);
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < bs; ++i)
                    {
                        const float mainV = b.getSample (c, i);
                        const float swarmV = b.getSample (2 + c, i), hitV = b.getSample (4 + c, i), dryV = b.getSample (6 + c, i);
                        maxErr = juce::jmax (maxErr, (double) std::abs (mainV - (swarmV + hitV + dryV)));
                        eSwarm += std::abs (swarmV); eHit += std::abs (hitV); eDry += std::abs (dryV);
                    }
            }
            expect (maxErr < 1.0e-5);
            expect (eSwarm > 1.0 && eHit > 1.0 && eDry > 1.0);

            // back to the default layout: stems and sidechain disabled, processing unchanged
            juce::AudioProcessor::BusesLayout plain;
            plain.inputBuses.add (juce::AudioChannelSet::stereo()); plain.inputBuses.add (juce::AudioChannelSet::disabled());
            plain.outputBuses.add (juce::AudioChannelSet::stereo());
            for (int i = 0; i < 3; ++i) plain.outputBuses.add (juce::AudioChannelSet::disabled());
            expect (p.setBusesLayout (plain));
            p.prepareToPlay (44100.0, bs);
            juce::AudioBuffer<float> b2 (2, bs); b2.clear(); juce::MidiBuffer m2;
            p.processBlock (b2, m2);
            expect (std::isfinite (b2.getMagnitude (0, bs)));
        }

        beginTest ("performance variations store, recall, persist and switch by MIDI note");
        {
            PreChorusProcessor p;
            p.prepareToPlay (44100.0, 512);
            p.asyncRender = false;
            p.setParam (IDs::detune, 11.0f); p.setParam (IDs::character, 2.0f);
            p.storeVariation (0);
            p.setParam (IDs::detune, 33.0f); p.setParam (IDs::character, 5.0f);
            p.storeVariation (2);
            expect (p.isVariationStored (0) && p.isVariationStored (2) && ! p.isVariationStored (1));
            p.recallVariation (0);
            expectWithinAbsoluteError (p.param (IDs::detune), 11.0f, 0.51f);
            expectEquals ((int) p.param (IDs::character), 2);
            expectEquals (p.getActiveVariation(), 0);
            p.recallVariation (1);                                  // empty slot: nothing changes
            expectEquals (p.getActiveVariation(), 0);
            juce::MemoryBlock blob; p.getStateInformation (blob);
            PreChorusProcessor q; q.setStateInformation (blob.getData(), (int) blob.getSize());
            expect (q.isVariationStored (0) && q.isVariationStored (2) && ! q.isVariationStored (1));
            q.recallVariation (2);
            expectWithinAbsoluteError (q.param (IDs::detune), 33.0f, 0.51f);
            // MIDI note switching: selects instead of triggering
            q.variationNotes = true; q.variationBaseNote = 24;
            juce::AudioBuffer<float> buf (2, 256); buf.clear();
            juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 24, 1.0f), 3);
            q.processBlock (buf, m);
            for (auto& v : q.voices) expect (! v.active);            // did not trigger the swarm
            q.timerCallback();                                       // message thread applies the pending switch
            expectEquals (q.getActiveVariation(), 0);
            expectWithinAbsoluteError (q.param (IDs::detune), 11.0f, 0.51f);
            juce::MidiBuffer m2; m2.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 3);
            q.processBlock (buf, m2);
            bool any = false; for (auto& v : q.voices) any = any || v.active;
            expect (any);                                            // ordinary notes still trigger
            q.clearVariation (0);
            expect (! q.isVariationStored (0));
        }
    }
};

static PreChorusCoreTests preChorusCoreTests;

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runTestsInCategory ("PreChorus");

    // JUCE only logs to the debugger on Windows, so print a console summary that CI (ctest --output-on-failure) can show.
    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        if (const auto* result = runner.getResult (i))
        {
            failures += result->failures;
            std::cout << (result->failures == 0 ? "[ ok ] " : "[FAIL] ") << result->unitTestName << " / " << result->subcategoryName
                      << " (" << result->passes << " passed, " << result->failures << " failed)" << std::endl;
            for (auto& m : result->messages) std::cout << "         " << m << std::endl;
        }
    std::cout << "PreChorus tests: " << runner.getNumResults() << " groups, " << failures << " failed expectations" << std::endl;

    return runner.getNumResults() > 0 && failures == 0 ? 0 : 1;
}
