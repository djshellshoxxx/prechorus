// PreChorus™
// Copyright © 2026 Sheldon Davidson. All rights reserved.
// Proprietary source code. See LICENSE and COPYRIGHT-TRADEMARK.md.
// SPDX-License-Identifier: LicenseRef-Proprietary

// Offline (non-realtime) DSP helpers used by the render engine, capture post-processing and export.
// Everything here is pure: no UI, no plug-in state, deterministic for the same input.
#pragma once
#include <JuceHeader.h>
#include <complex>
#include <vector>

namespace PCDsp
{
    // ---- Minimal radix-2 complex FFT (size must be a power of two) ----
    class FFT
    {
    public:
        explicit FFT (int sizePow2) : n (sizePow2), tw ((size_t) sizePow2 / 2), rev ((size_t) sizePow2)
        {
            jassert (juce::isPowerOfTwo (n));
            int bits = 0;
            while ((1 << bits) < n) ++bits;
            for (int i = 0; i < n; ++i)
            {
                int r = 0;
                for (int b = 0; b < bits; ++b) if (i & (1 << b)) r |= 1 << (bits - 1 - b);
                rev[(size_t) i] = r;
            }
            for (int i = 0; i < n / 2; ++i)
                tw[(size_t) i] = std::polar (1.0f, -juce::MathConstants<float>::twoPi * (float) i / (float) n);
        }

        int size() const { return n; }

        // In-place transform. inverse = true applies the conjugate twiddles and scales by 1/N.
        void perform (std::complex<float>* x, bool inverse) const
        {
            for (int i = 0; i < n; ++i)
            {
                const int j = rev[(size_t) i];
                if (j > i) std::swap (x[i], x[j]);
            }
            for (int len = 2; len <= n; len <<= 1)
            {
                const int half = len / 2, step = n / len;
                for (int i = 0; i < n; i += len)
                    for (int k = 0; k < half; ++k)
                    {
                        auto w = tw[(size_t) (k * step)];
                        if (inverse) w = std::conj (w);
                        const auto a = x[i + k];
                        const auto b = x[i + k + half] * w;
                        x[i + k] = a + b;
                        x[i + k + half] = a - b;
                    }
            }
            if (inverse)
                for (int i = 0; i < n; ++i) x[i] /= (float) n;
        }

    private:
        int n;
        std::vector<std::complex<float>> tw;
        std::vector<int> rev;
    };

    // ---- Formant shift: moves the spectral envelope by `semitones` while the harmonics (pitch) stay put ----
    // STFT with a cepstrally smoothed envelope; the envelope is warped along frequency and applied as a gain.
    inline void formantShiftChannel (float* data, int numSamples, float semitones, double sampleRate)
    {
        if (numSamples < 256 || std::abs (semitones) < 0.01f) return;
        constexpr int N = 2048, hop = 512, bins = N / 2 + 1;
        const float ratio = std::pow (2.0f, semitones / 12.0f);
        const FFT fft (N);

        std::vector<float> win ((size_t) N);
        for (int i = 0; i < N; ++i)
            win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) N);

        const int padded = numSamples + 2 * N;
        std::vector<float> in ((size_t) padded, 0.0f), out ((size_t) padded, 0.0f), norm ((size_t) padded, 0.0f);
        std::copy (data, data + numSamples, in.begin() + N);

        // Quefrency cut-off: keeps the envelope, drops the pitch ripple (f0 down to ~70 Hz).
        const int lifter = juce::jlimit (12, 120, (int) (sampleRate / 380.0));

        std::vector<std::complex<float>> spec ((size_t) N), cep ((size_t) N);
        std::vector<float> logMag ((size_t) bins), env ((size_t) bins), warped ((size_t) bins);

        for (int pos = 0; pos + N <= padded; pos += hop)
        {
            for (int i = 0; i < N; ++i) spec[(size_t) i] = in[(size_t) (pos + i)] * win[(size_t) i];
            fft.perform (spec.data(), false);

            for (int k = 0; k < bins; ++k)
                logMag[(size_t) k] = std::log (std::abs (spec[(size_t) k]) + 1.0e-7f);

            // cepstrum of the (symmetric) log magnitude
            for (int k = 0; k < N; ++k)
                cep[(size_t) k] = logMag[(size_t) (k < bins ? k : N - k)];
            fft.perform (cep.data(), true);
            for (int q = 0; q < N; ++q)
            {
                const int qq = q <= N / 2 ? q : N - q;
                if (qq >= lifter) cep[(size_t) q] = 0.0f;
            }
            fft.perform (cep.data(), false);
            for (int k = 0; k < bins; ++k) env[(size_t) k] = cep[(size_t) k].real();

            // warped[k] = env(k / ratio): a ratio above 1 pushes formants up
            for (int k = 0; k < bins; ++k)
            {
                const float src = (float) k / ratio;
                const int i0 = juce::jlimit (0, bins - 1, (int) src);
                const int i1 = juce::jmin (bins - 1, i0 + 1);
                const float fr = src - (float) i0;
                warped[(size_t) k] = src >= (float) (bins - 1) ? env[(size_t) (bins - 1)] : env[(size_t) i0] + (env[(size_t) i1] - env[(size_t) i0]) * fr;
            }
            for (int k = 0; k < bins; ++k)
            {
                const float g = juce::jlimit (0.1f, 10.0f, std::exp (warped[(size_t) k] - env[(size_t) k]));
                spec[(size_t) k] *= g;
                if (k > 0 && k < N / 2) spec[(size_t) (N - k)] = std::conj (spec[(size_t) k]);
            }
            spec[0] = { spec[0].real(), 0.0f };
            spec[(size_t) (N / 2)] = { spec[(size_t) (N / 2)].real(), 0.0f };
            fft.perform (spec.data(), true);
            for (int i = 0; i < N; ++i)
            {
                out[(size_t) (pos + i)] += spec[(size_t) i].real() * win[(size_t) i];
                norm[(size_t) (pos + i)] += win[(size_t) i] * win[(size_t) i];
            }
        }
        for (int i = 0; i < numSamples; ++i)
        {
            const float nrm = norm[(size_t) (i + N)];
            data[i] = nrm > 1.0e-4f ? out[(size_t) (i + N)] / nrm : 0.0f;
        }
    }

    inline void formantShift (juce::AudioBuffer<float>& buf, float semitones, double sampleRate)
    {
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
            formantShiftChannel (buf.getWritePointer (ch), buf.getNumSamples(), semitones, sampleRate);
    }

    // ---- Transient slice detection for Slice Scatter ----
    // Returns ascending slice start indices (first is always 0). 8 equal slices when no clear transients exist.
    inline std::vector<int> detectSliceStarts (const juce::AudioBuffer<float>& src, double sr, int maxSlices = 8)
    {
        const int n = src.getNumSamples();
        std::vector<int> equal;
        for (int i = 0; i < maxSlices; ++i) equal.push_back (n / maxSlices * i);
        if (n < (int) (sr * 0.3) || src.getNumChannels() == 0) return equal;

        const int win = juce::jmax (16, (int) (sr * 0.005));
        const int frames = n / win;
        std::vector<float> e ((size_t) frames, 0.0f), flux ((size_t) frames, 0.0f);
        for (int f = 0; f < frames; ++f)
        {
            double acc = 0.0;
            for (int ch = 0; ch < src.getNumChannels(); ++ch)
            {
                const float* d = src.getReadPointer (ch) + f * win;
                for (int i = 0; i < win; ++i) acc += (double) d[i] * d[i];
            }
            e[(size_t) f] = std::log10 ((float) (acc / (win * src.getNumChannels())) + 1.0e-9f);
        }
        float maxFlux = 0.0f;
        for (int f = 2; f < frames; ++f)
        {
            // compare against a short look-back so slow swells do not count as transients
            const float ref = juce::jmax (e[(size_t) f - 1], e[(size_t) f - 2]);
            flux[(size_t) f] = juce::jmax (0.0f, e[(size_t) f] - ref);
            maxFlux = juce::jmax (maxFlux, flux[(size_t) f]);
        }
        if (maxFlux < 0.25f) return equal; // < ~2.5 dB jump anywhere: nothing percussive

        const float thr = juce::jmax (0.2f, maxFlux * 0.35f);
        const int minGap = juce::jmax (1, (int) (0.06 * sr / win));
        std::vector<std::pair<float, int>> peaks;
        for (int f = 2; f < frames - 1; ++f)
            if (flux[(size_t) f] >= thr && flux[(size_t) f] >= flux[(size_t) f - 1] && flux[(size_t) f] > flux[(size_t) f + 1])
                peaks.push_back ({ flux[(size_t) f], f });
        std::sort (peaks.begin(), peaks.end(), [] (auto& a, auto& b) { return a.first > b.first; });

        std::vector<int> chosen;
        for (auto& p : peaks)
        {
            bool ok = p.second * win > (int) (sr * 0.03);
            for (int c : chosen) if (std::abs (c - p.second) < minGap) ok = false;
            if (ok) chosen.push_back (p.second);
            if ((int) chosen.size() >= maxSlices - 1) break;
        }
        if (chosen.empty()) return equal;
        std::sort (chosen.begin(), chosen.end());
        std::vector<int> starts { 0 };
        for (int c : chosen) starts.push_back (juce::jmax (0, c * win - win)); // start a hair before the attack
        return starts;
    }

    // ---- Smart capture post-processing: trim leading/trailing silence, optional normalise ----
    // Never trims below minLenSec. Returns true if the buffer was changed.
    inline bool smartTrim (juce::AudioBuffer<float>& b, double sr, float thresholdDb = -50.0f, double minLenSec = 0.05,
                           double fadeMs = 10.0, bool normalise = false, float normaliseDb = -1.0f, bool trim = true)
    {
        const int n = b.getNumSamples();
        if (n <= 0) return false;
        const float thr = juce::Decibels::decibelsToGain (thresholdDb);
        auto loud = [&] (int i)
        {
            for (int ch = 0; ch < b.getNumChannels(); ++ch)
                if (std::abs (b.getSample (ch, i)) >= thr) return true;
            return false;
        };
        int first = 0, last = n - 1;
        if (trim)
        {
            while (first < n && ! loud (first)) ++first;
            if (first >= n) return false; // all silent: leave it alone
            while (last > first && ! loud (last)) --last;
        }
        const int minLen = (int) (sr * minLenSec);
        int len = last - first + 1;
        if (len < minLen)
        {
            const int extra = minLen - len;
            first = juce::jmax (0, first - extra / 2);
            len = juce::jmin (n - first, minLen);
        }
        bool changed = false;
        if (first > 0 || len < n)
        {
            juce::AudioBuffer<float> t (b.getNumChannels(), len);
            for (int ch = 0; ch < b.getNumChannels(); ++ch) t.copyFrom (ch, 0, b, ch, first, len);
            b = std::move (t);
            changed = true;
            const int fade = juce::jmin (len / 4, (int) (sr * fadeMs * 0.001));
            for (int ch = 0; ch < b.getNumChannels(); ++ch)
            {
                float* d = b.getWritePointer (ch);
                for (int i = 0; i < fade; ++i)
                {
                    const float g = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * (float) i / (float) fade);
                    d[i] *= g;
                    d[len - 1 - i] *= g;
                }
            }
        }
        if (normalise)
        {
            const float mag = b.getMagnitude (0, b.getNumSamples());
            if (mag > 1.0e-4f)
            {
                b.applyGain (juce::Decibels::decibelsToGain (normaliseDb) / mag);
                changed = true;
            }
        }
        return changed;
    }

    // ---- Simple state-variable filter (used by layers and the Vowel Choir character) ----
    struct SVF
    {
        float ic1 = 0.0f, ic2 = 0.0f;
        // Returns { lowpass, bandpass, highpass }
        inline void tick (float v0, float g, float k, float& lp, float& bp, float& hp)
        {
            const float a1 = 1.0f / (1.0f + g * (g + k));
            const float a2 = g * a1;
            const float a3 = g * a2;
            const float v3 = v0 - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            lp = v2; bp = v1; hp = v0 - k * v1 - v2;
        }
        static float gFor (float hz, double sr)
        {
            return std::tan (juce::MathConstants<float>::pi * juce::jlimit (20.0f, (float) sr * 0.45f, hz) / (float) sr);
        }
    };

    // ---- Impact & riser layers: synthesised offline into the render, `hit` is the hit sample index ----
    struct LayerSettings { float riser = 0, reverse = 0, sub = 0, impact = 0, tone = 0.5f; };

    inline int layerTailSamples (const LayerSettings& s, double sr)
    {
        return (s.sub > 0.001f || s.impact > 0.001f) ? (int) (sr * 1.4) : 0;
    }

    inline void addLayers (juce::AudioBuffer<float>& full, int hit, double sr, const LayerSettings& s, juce::int64 seed)
    {
        const int n = full.getNumSamples();
        if (hit <= 0 || hit > n) return;
        juce::Random rnd (seed ^ 0x5bd1e995);
        const float brightness = 0.35f + s.tone * 1.3f; // tone 0.5 ~ neutral
        auto write = [&] (int i, float l, float r)
        {
            if (i < 0 || i >= n) return;
            full.addSample (0, i, l);
            full.addSample (1, i, r);
        };

        if (s.riser > 0.001f)
        {
            SVF fl, fr;
            const int len = hit;
            for (int i = 0; i < len; ++i)
            {
                const float t = (float) i / (float) len;
                const float cutoff = (250.0f * std::pow (60.0f, t)) * brightness; // 250 Hz -> 15 kHz sweep
                const float g = SVF::gFor (cutoff, sr);
                float lp, bp, hp;
                fl.tick (rnd.nextFloat() * 2.0f - 1.0f, g, 1.2f, lp, bp, hp);
                const float l = bp;
                fr.tick (rnd.nextFloat() * 2.0f - 1.0f, g, 1.2f, lp, bp, hp);
                const float amp = s.riser * 0.35f * t * t;
                write (i, l * amp, bp * amp);
            }
        }

        if (s.reverse > 0.001f)
        {
            const int len = juce::jmin (hit, (int) (sr * 1.6));
            std::vector<float> burstL ((size_t) len), burstR ((size_t) len);
            SVF hl, hr;
            const float g = SVF::gFor (3500.0f * brightness, sr);
            for (int i = 0; i < len; ++i)
            {
                float lp, bp, h1, h2;
                hl.tick (rnd.nextFloat() * 2.0f - 1.0f, g, 1.0f, lp, bp, h1);
                hr.tick (rnd.nextFloat() * 2.0f - 1.0f, g, 1.0f, lp, bp, h2);
                const float env = std::exp (-4.0f * (float) i / (float) len);
                burstL[(size_t) i] = h1 * env;
                burstR[(size_t) i] = h2 * env;
            }
            for (int i = 0; i < len; ++i)   // reversed: swells up and ends at the hit
                write (hit - len + i, burstL[(size_t) (len - 1 - i)] * s.reverse * 0.45f, burstR[(size_t) (len - 1 - i)] * s.reverse * 0.45f);
        }

        if (s.sub > 0.001f)
        {
            const int len = (int) (sr * 1.2);
            double phase = 0.0;
            for (int i = 0; i < len; ++i)
            {
                const float t = (float) i / (float) len;
                const double f = 62.0 * std::pow (30.0 / 62.0, (double) t); // 62 Hz -> 30 Hz
                phase += juce::MathConstants<double>::twoPi * f / sr;
                const float a = std::exp (-3.0f * t) * juce::jmin (1.0f, (float) i / (float) (sr * 0.004));
                const float v = (float) std::sin (phase) * a * s.sub * 0.8f;
                write (hit + i, v, v);
            }
        }

        if (s.impact > 0.001f)
        {
            const int len = (int) (sr * 0.45);
            SVF fl, fr;
            double phase = 0.0;
            for (int i = 0; i < len; ++i)
            {
                const float t = (float) i / (float) len;
                const float g = SVF::gFor ((900.0f * (1.0f - t) + 80.0f) * brightness, sr);
                float lp1, lp2, bp, hp;
                fl.tick (rnd.nextFloat() * 2.0f - 1.0f, g, 1.4f, lp1, bp, hp);
                fr.tick (rnd.nextFloat() * 2.0f - 1.0f, g, 1.4f, lp2, bp, hp);
                phase += juce::MathConstants<double>::twoPi * 52.0 / sr;
                const float env = std::exp (-9.0f * t) * juce::jmin (1.0f, (float) i / (float) (sr * 0.002));
                const float thump = (float) std::sin (phase) * env;
                write (hit + i, (lp1 * 0.6f + thump) * s.impact * 0.7f * 1.0f, (lp2 * 0.6f + thump) * s.impact * 0.7f);
            }
        }
    }

    // ---- Export helpers ----
    inline void resampleBuffer (juce::AudioBuffer<float>& b, double fromSR, double toSR)
    {
        if (b.getNumSamples() == 0 || fromSR <= 0.0 || toSR <= 0.0 || std::abs (fromSR - toSR) < 0.5) return;
        const double ratio = fromSR / toSR;
        const int outLen = juce::jmax (1, (int) std::ceil ((double) b.getNumSamples() / ratio));
        juce::AudioBuffer<float> o (b.getNumChannels(), outLen);
        o.clear();
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
        {
            juce::LagrangeInterpolator li;
            li.process (ratio, b.getReadPointer (ch), o.getWritePointer (ch), outLen);
        }
        b = std::move (o);
    }

    // TPDF dither to `bits` (deterministic for a given seed). Only call when exporting to <= 16 bit.
    inline void ditherTPDF (juce::AudioBuffer<float>& b, int bits, juce::int64 seed = 1234)
    {
        juce::Random r (seed);
        const float lsb = 1.0f / (float) (1 << (bits - 1));
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
        {
            float* d = b.getWritePointer (ch);
            for (int i = 0; i < b.getNumSamples(); ++i)
                d[i] += (r.nextFloat() - r.nextFloat()) * lsb;
        }
    }
}
