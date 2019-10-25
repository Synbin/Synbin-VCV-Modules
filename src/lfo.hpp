struct LFO
{
	float phase = 0.f;
	float freq;
	const float pulseWidth = .5f;
	const float syncDirection = 1.f;

	dsp::TRCFilter<float> plsFilter;

	float triValue = 0.f;
	float sqrValue = 0.f;
	float plsValue = 0.f;

	void setPitch(float pitch)
	{
		freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitch + 30) / 1073741824;
	}

	void process(float deltaTime)
	{
		// Advance phase
		float deltaPhase = simd::clamp(freq * deltaTime, 1e-6f, 0.35f);
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
	float tri(float phase) {
		float v;
		float x = phase + 0.25f;
		x -= simd::trunc(x);
		float halfX = (x >= 0.5f);
		x *= 2;
		x -= simd::trunc(x);
		v = expCurve(x) * simd::ifelse(halfX, 1.f, -1.f);
		return v;
	}

	float tri()
	{
		return triValue;
	}

	float expCurve(float x)
	{
		return (3 + x * (-13 + 5 * x)) / (3 + 2 * x);
	}

	// Square wave --------------------------------------------------
	float sqr(float phase) {
		float v = phase < pulseWidth ? 1.f : -1.f;
		return v;
	}

	float sqr() {
		return sqrValue;
	}

	// Pulse wave ---------------------------------------------------
	float pls(float phase, float deltaTime) {
		float v = phase < pulseWidth ? 1.f : -1.f;
		plsFilter.setCutoffFreq(20.f * deltaTime);
		plsFilter.process(v);
		v = plsFilter.highpass() * 0.95f;
		return v;
	}

	float pls()
	{
		return plsValue;
	}

	// Rate LED -----------------------------------------------------
	float light()
	{
		return simd::sin(2 * float(M_PI) * phase);
	}

};
