
#include "ComplexWave.h"

ComplexWave::ComplexWave(float gain) : gain(gain)
{
	waves = std::make_shared<std::vector<std::unique_ptr<Wave>>>();
}


ComplexWave::ComplexWave(const ComplexWave& o) : gain(o.gain)
{
	waves = std::make_shared<std::vector<std::unique_ptr<Wave>>>();

	auto rdr = std::atomic_load(&o.waves);
	auto copy = std::make_shared<std::vector<std::unique_ptr<Wave>>>();
	for (const auto& w : *rdr)
	{
		copy->push_back(w->clone());
	}

	std::atomic_store(&waves, copy);
}

std::unique_ptr<Wave> ComplexWave::clone() const
{
	return std::make_unique<ComplexWave>(*this);
}

void ComplexWave::add_sine(ADSREnvelope& amp_envelope, float freq, size_t tableLen, float length, float gain)
{
	auto curr = std::atomic_load(&waves);

	auto copy = std::make_shared<std::vector<std::unique_ptr<Wave>>>();
	for (auto& w : *curr)
	{
		copy->push_back(w->clone());
	}

	copy->push_back(std::make_unique<Sine>(amp_envelope, freq, tableLen, length, gain));

	std::atomic_store(&waves, copy);
}

void ComplexWave::add_complex(const ComplexWave& o)
{
	auto curr = std::atomic_load(&waves);

	auto copy = std::make_shared<std::vector<std::unique_ptr<Wave>>>();
	for (auto& w : *curr)
	{
		copy->push_back(w->clone());
	}

	copy->push_back(o.clone());

	std::atomic_store(&waves, copy);
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

	auto rdr = std::atomic_load(&waves);
	for (const std::unique_ptr<Wave>& s : *rdr)
	{
		sum += s->get_left_phase();
	}

	return get_gain() * sum;
}

float ComplexWave::get_right_phase() const
{
	float sum = 0;

	auto rdr = std::atomic_load(&waves);

	for (const std::unique_ptr<Wave>& s : *rdr)
	{
		sum += s->get_right_phase();
	}

	return get_gain() * sum;
}

void ComplexWave::stream(unsigned int curr_frame)
{
	auto rdr = std::atomic_load(&waves);

	for (const std::unique_ptr<Wave>& s : *rdr)
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
		auto rdr = std::atomic_load(&cs->waves);

		for (std::unique_ptr<Wave>& s : *rdr)
		{
			s->stream(i);
		}

		*out++ = cs->get_left_phase();
		*out++ = cs->get_right_phase();
	}

	return 0;
}

