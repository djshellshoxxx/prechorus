// PreChorus™
// Copyright © 2026 Sheldon Davidson. All rights reserved.
// Proprietary source code. See LICENSE and COPYRIGHT-TRADEMARK.md.
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once
#include <JuceHeader.h>

// Procedural demo sources so a new user hears PreChorus instantly, with no sample files and no
// licensing concerns. Fully deterministic (fixed noise seed), generated on the message thread only.
namespace PCDemo
{
    enum Kind { ahh = 0, hey = 1, chord = 2, numKinds };

    inline juce::StringArray names() { return { "Ahh (sustained vowel)", "Hey (vocal shout)", "Ooh chord (3 voices)" }; }

    struct Formants { float f1, f2, f3; };

    // Adds a formant-filtered additive "voice" into buf (stereo identical) from sample 'start' for 'len' samples.
    inline void addVowel (juce::AudioBuffer<float>& buf, double sr, int start, int len,
                          float f0A, float f0B, Formants a, Formants b, float level, float vibratoDepthSemi)
    {
        const double twoPi = juce::MathConstants<double>::twoPi;
        double phase = 0.0;
        const int total = buf.getNumSamples();
        for (int i = 0; i < len && start + i < total; ++i)
        {
            const double t = (double) i / (double) juce::jmax (1, len - 1);
            const double sec = (double) i / sr;
            const double vib = std::sin (twoPi * 5.5 * sec) * vibratoDepthSemi * juce::jlimit (0.0, 1.0, sec / 0.4);
            const double f0 = (double) juce::jmap ((float) t, f0A, f0B) * std::pow (2.0, vib / 12.0);
            phase += twoPi * f0 / sr;
            if (phase > twoPi) phase -= twoPi;

            const float f1 = juce::jmap ((float) t, a.f1, b.f1), f2 = juce::jmap ((float) t, a.f2, b.f2), f3 = juce::jmap ((float) t, a.f3, b.f3);
            double s = 0.0;
            for (int h = 1; h <= 40; ++h)
            {
                const double fh = f0 * h;
                if (fh > 0.45 * sr || fh > 7000.0) break;
                auto peak = [&] (float fc, float bw, float g) { const double d = (fh - fc) / bw; return g * std::exp (-0.5 * d * d); };
                const double formantGain = 0.04 + peak (f1, 110.0f, 1.0f) + peak (f2, 160.0f, 0.6f) + peak (f3, 260.0f, 0.3f);
                s += std::sin ((double) h * phase) * formantGain / (double) h;
            }
            // attack 35 ms, release 250 ms
            const double att = juce::jlimit (0.0, 1.0, sec / 0.035);
            const double rel = juce::jlimit (0.0, 1.0, ((double) (len - i) / sr) / 0.25);
            const float v = (float) (s * att * rel) * level;
            buf.addSample (0, start + i, v);
            buf.addSample (1, start + i, v);
        }
    }

    inline juce::AudioBuffer<float> generate (int kind, double sr)
    {
        sr = juce::jlimit (8000.0, 384000.0, sr);
        kind = juce::jlimit (0, (int) numKinds - 1, kind);
        const Formants ah { 800.0f, 1150.0f, 2900.0f }, eh { 530.0f, 1840.0f, 2480.0f },
                       ay { 400.0f, 2000.0f, 2600.0f }, oo { 300.0f, 870.0f, 2240.0f };

        const double seconds = kind == hey ? 1.4 : 2.2;
        const int n = (int) (sr * seconds);
        juce::AudioBuffer<float> b (2, n);
        b.clear();

        if (kind == ahh)
        {
            addVowel (b, sr, 0, n, 220.0f, 220.0f, ah, ah, 1.0f, 0.35f);
        }
        else if (kind == hey)
        {
            // breathy "h" onset (deterministic noise), then eh -> ay with a rising shout pitch
            juce::Random rng (12345);
            const int hLen = (int) (sr * 0.07);
            for (int i = 0; i < hLen; ++i)
            {
                const float env = std::sin (juce::MathConstants<float>::pi * (float) i / (float) hLen);
                const float v = (rng.nextFloat() * 2.0f - 1.0f) * 0.25f * env;
                b.addSample (0, i, v); b.addSample (1, i, v);
            }
            addVowel (b, sr, hLen / 2, n - hLen / 2, 247.0f, 311.0f, eh, ay, 1.0f, 0.2f);
        }
        else
        {
            addVowel (b, sr, 0, n, 220.00f, 220.00f, oo, oo, 0.7f, 0.3f);   // A3
            addVowel (b, sr, 0, n, 277.18f, 277.18f, oo, oo, 0.6f, 0.3f);   // C#4
            addVowel (b, sr, 0, n, 329.63f, 329.63f, oo, oo, 0.6f, 0.3f);   // E4
        }

        const float peak = b.getMagnitude (0, n);
        if (peak > 1.0e-6f) b.applyGain (0.7f / peak);
        return b;
    }
}
