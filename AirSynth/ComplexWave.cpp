
#include "ComplexWave.h"

void ComplexWave::add(ADSREnvelope& amp_envelope, float freq, size_t tableLength, float length, float gain)
{
	waves.emplace_back(amp_envelope, freq, tableLength, length, gain);
}

float ComplexWave::sum_left_phases() const
{
	float sum = 0;
	for (const Sine& s : waves)
	{
		sum += s.left_phase;
	}

	return sum;
}

float ComplexWave::sum_right_phases() const
{
	float sum = 0;
	for (const Sine& s : waves)
	{
		sum += s.right_phase;
	}

	return sum;
}

ComplexWave::ComplexWave(float gain) : gain(gain)
{
}

int ComplexWave::stream(const void* inputBuffer, void* outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void* userData)
{
	ComplexWave* cs = static_cast<ComplexWave*>(userData);
	float* out = static_cast<float*>(outputBuffer);
	(void)inputBuffer; /* Prevent unused variable warning. */

	

	for (unsigned int i = 0; i < framesPerBuffer; ++i)
	{

		for (Sine& s : cs->waves)
		{
			float sam_i = s.freq / SAMPLE_RATE * s.tableLen;

			s.t_phase += sam_i;

			s.mono(s.gain * s.amp[s.amp_phase] * s.interpolate(s.t_phase));

			s.t_phase = std::fmod(s.t_phase, s.tableLen);
			s.amp_phase = std::fmod(s.amp_phase + static_cast<float>(s.tableLen) / (s.length * SAMPLE_RATE), s.tableLen);
		}


		*out++ = cs->gain * cs->sum_left_phases();
		*out++ = cs->gain * cs->sum_right_phases();
	}

	return 0;
}
