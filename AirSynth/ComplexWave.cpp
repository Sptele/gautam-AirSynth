
#include "ComplexWave.h"

ComplexWave::ComplexWave(float gain) : gain(gain)
{
}

ComplexWave::~ComplexWave()
{
	waves.clear();
}

void ComplexWave::add(const std::unique_ptr<Wave>& ptr)
{
	//waves.push_back(std::move(ptr));
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

