// PreChorus™
// Copyright © 2026 Sheldon Davidson. All rights reserved.
// Proprietary source code. See LICENSE and COPYRIGHT-TRADEMARK.md.
// SPDX-License-Identifier: LicenseRef-Proprietary

#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/DemoSource.h"

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
    }
};

static PreChorusCoreTests preChorusCoreTests;

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runTestsInCategory ("PreChorus");

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        if (const auto* result = runner.getResult (i))
            failures += result->failures;

    return runner.getNumResults() > 0 && failures == 0 ? 0 : 1;
}
