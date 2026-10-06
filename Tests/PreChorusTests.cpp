// PreChorus™
// Copyright © 2026 Sheldon Davidson. All rights reserved.
// Proprietary source code. See LICENSE and COPYRIGHT-TRADEMARK.md.
// SPDX-License-Identifier: LicenseRef-Proprietary

#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"

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
            expect (restored.getCurrentFile() == juce::File());

            restored.serviceDeferredStateRestore();
            expect (restored.getCurrentFile() == wavFile);
            expectEquals ((int) restored.param (IDs::sourceMode), 0);
            wavFile.deleteFile();
        }

        beginTest ("A/B snapshots recall independent parameter states");
        {
            PreChorusProcessor p;
            p.setParam (IDs::voiceCount, 6.0f);
            p.setParam (IDs::sourceMode, 2.0f);
            p.setParam (IDs::captureMode, 7.0f);
            p.setParam (IDs::captureSlot, 4.0f);
            p.setParam (IDs::captureLock, 1.0f);
            expect (p.storeCompareState (0));
            p.setParam (IDs::voiceCount, 28.0f);
            expect (p.storeCompareState (1));

            p.setParam (IDs::sourceMode, 0.0f);
            p.setParam (IDs::captureMode, 0.0f);
            p.setParam (IDs::captureSlot, 1.0f);
            p.setParam (IDs::captureLock, 0.0f);
            expect (p.recallCompareState (0));
            expectEquals ((int) p.param (IDs::voiceCount), 6);
            expectEquals ((int) p.param (IDs::sourceMode), 0);
            expectEquals ((int) p.param (IDs::captureMode), 0);
            expectEquals ((int) p.param (IDs::captureSlot), 1);
            expectEquals ((int) p.param (IDs::captureLock), 0);
            expect (p.recallCompareState (1));
            expectEquals ((int) p.param (IDs::voiceCount), 28);
            expectEquals ((int) p.param (IDs::sourceMode), 0);
            expectEquals ((int) p.param (IDs::captureSlot), 1);

            juce::MemoryBlock saved;
            p.getStateInformation (saved);
            PreChorusProcessor restored;
            restored.setStateInformation (saved.getData(), (int) saved.getSize());
            expect (restored.recallCompareState (0));
            expectEquals ((int) restored.param (IDs::voiceCount), 6);
            expect (restored.recallCompareState (1));
            expectEquals ((int) restored.param (IDs::voiceCount), 28);
        }

        beginTest ("factory preset search is case-insensitive and stable");
        {
            const auto shimmer = PreChorusProcessor::getMatchingFactoryPresetNames ("shimmer");
            expectEquals (shimmer.size(), 1);
            expectEquals (shimmer[0], juce::String ("Future Bass Shimmer"));

            const auto vocal = PreChorusProcessor::getMatchingFactoryPresetNames ("VOCAL");
            expect (vocal.contains ("Pop Vocal Double"));
            expect (vocal.contains ("Lo-Fi Bedroom Vocal"));
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
                            .getNonexistentChildFile ("prechorus-test", ".pchpreset", false);
            juce::MemoryBlock session;
            p.getStateInformation (session);
            p.setStateInformation (session.getData(), (int) session.getSize());

            expect (p.saveUserPreset (file));
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
            expect (p.loadUserPreset (file));
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
                            .getNonexistentChildFile ("prechorus-bad", ".pchpreset", false);
            file.replaceWithText ("not a preset");
            expect (! p.loadUserPreset (file));
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
                                    .getNonexistentChildFile ("prechorus-wrong-product", ".pchpreset", false);
            wrongProduct.replaceWithText (makePreset ("OtherProduct", 1));
            expect (! p.loadUserPreset (wrongProduct));
            expectWithinAbsoluteError (p.param (IDs::space), 0.42f, 0.011f);
            wrongProduct.deleteFile();

            auto futureVersion = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                     .getNonexistentChildFile ("prechorus-future", ".pchpreset", false);
            futureVersion.replaceWithText (makePreset ("PreChorus", 999));
            expect (! p.loadUserPreset (futureVersion));
            expectWithinAbsoluteError (p.param (IDs::space), 0.42f, 0.011f);
            futureVersion.deleteFile();
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
