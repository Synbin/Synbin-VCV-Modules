#include "plugin.hpp"

template <typename T>
T expCurve(T x)
{
	return (3 + x * (-13 + 5 * x)) / (3 + 2 * x);
}

template <int OVERSAMPLE, int QUALITY, typename T>
struct LowFrequencyOscillator
{
	T phase = 0.f;
	T freq;
	const T pulseWidth = .5f;
	const T syncDirection = 1.f;

	dsp::TRCFilter<T> plsFilter;

	T triValue = 0.f;
	T sqrValue = 0.f;
	T plsValue = 0.f;

	void setPitch(T pitch)
	{
		freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitch + 30) / 1073741824;
	}

	void process(float deltaTime)
	{
		// Advance phase
		T deltaPhase = simd::clamp(freq * deltaTime, 1e-6f, 0.35f);
		phase += deltaPhase;

		// Wrap phase
		phase -= simd::floor(phase);

		// Tri
		triValue = tri(phase);

		// Square
		sqrValue = sqr(phase);

		// Pulse
		plsValue = pls(phase, deltaTime);

	}

	// Triangle wave ------------------------------------------------
	T tri(T phase) {
		T v;
		T x = phase + 0.25f;
		x -= simd::trunc(x);
		T halfX = (x >= 0.5f);
		x *= 2;
		x -= simd::trunc(x);
		v = expCurve(x) * simd::ifelse(halfX, 1.f, -1.f);
		return v;
	}

	T tri()
	{
		return triValue;
	}

	// Square wave --------------------------------------------------
	T sqr(T phase) {
		T v = phase < pulseWidth ? 1.f : -1.f;
		return v;
	}

	T sqr() {
		return sqrValue;
	}

	// Pulse wave ---------------------------------------------------
	T pls(T phase, T deltaTime) {
		T v = phase < pulseWidth ? 1.f : -1.f;
		plsFilter.setCutoffFreq(20.f * deltaTime);
		plsFilter.process(v);
		v = plsFilter.highpass() * 0.95f;
		return v;
	}

	T pls()
	{
		return plsValue;
	}

	// Rate LED -----------------------------------------------------
	T light()
	{
		return simd::sin(2 * T(M_PI) * phase);
	}

};
