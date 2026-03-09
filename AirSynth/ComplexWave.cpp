
#include "ComplexWave.h"

ComplexWave::ComplexWave(float gain) : gain(gain)
{
}

ComplexWave::~ComplexWave()
{
	waves.clear();
}

ComplexWave::ComplexWave(const ComplexWave& o) : gain(o.gain)
{
	for (const auto& w : o.waves)
	{
		synth().push_back(
			w->clone()
		);
	}
}

std::unique_ptr<Wave> ComplexWave::clone() const
{
	return std::make_unique<ComplexWave>(*this);
}

void ComplexWave::add_sine(ADSREnvelope& amp_envelope, float freq, size_t tableLen, float length, float gain)
{
	synth().push_back(std::make_unique<Sine>(amp_envelope, freq, tableLen, length, gain));
}

void ComplexWave::add_complex(const ComplexWave& o)
{
	synth().push_back(std::make_unique<ComplexWave>(o));
}

void ComplexWave::generate_harmonic_series(float freq, float height, ADSREnvelope& amp_envelope, size_t tableLen, float length, float gain[])
{
	for (int i = 1; i <= height; ++i)
	{
		add_sine(amp_envelope, freq * i, tableLen, length, gain[i-1]);
	}
}

float ComplexWave::get_left_phase() const
{
	float sum = 0;
	for (const std::unique_ptr<Wave>& s : waves)
	{
		sum += s->get_left_phase();
	}

	return get_gain() * sum;
}

float ComplexWave::get_right_phase() const
{
	float sum = 0;
	for (const std::unique_ptr<Wave>& s : waves)
	{
		sum += s->get_right_phase();
	}

	return get_gain() * sum;
}

void ComplexWave::stream(unsigned int curr_frame)
{
	for (const std::unique_ptr<Wave>& s : this->waves)
	{
		s->stream(curr_frame);
	}
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

		for (std::unique_ptr<Wave>& s : cs->waves)
		{
			s->stream(i);
		}

		*out++ = cs->get_left_phase();
		*out++ = cs->get_right_phase();
	}

	return 0;
}

