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

        beginTest ("A/B snapshots recall independent parameter states");
        {
            PreChorusProcessor p;
            p.setParam (IDs::voiceCount, 6.0f);
            expect (p.storeCompareState (0));
            p.setParam (IDs::voiceCount, 28.0f);
            expect (p.storeCompareState (1));

            expect (p.recallCompareState (0));
            expectEquals ((int) p.param (IDs::voiceCount), 6);
            expect (p.recallCompareState (1));
            expectEquals ((int) p.param (IDs::voiceCount), 28);
        }

        beginTest ("user preset round-trip excludes source media and restores parameters");
        {
            PreChorusProcessor p;
            p.setParam (IDs::pitchSpread, 13.0f);
            p.setParam (IDs::space, 0.61f);

            auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getNonexistentChildFile ("prechorus-test", ".pchpreset", false);
            expect (p.saveUserPreset (file));

            p.setParam (IDs::pitchSpread, 1.0f);
            p.setParam (IDs::space, 0.05f);
            expect (p.loadUserPreset (file));
            expectWithinAbsoluteError (p.param (IDs::pitchSpread), 13.0f, 0.01f);
            expectWithinAbsoluteError (p.param (IDs::space), 0.61f, 0.011f);
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
    }
};

static PreChorusCoreTests preChorusCoreTests;

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::UnitTestRunner runner;
    runner.runTestsInCategory ("PreChorus");
    return runner.getNumResults() > 0 && runner.getResult (0)->failures == 0 ? 0 : 1;
}
