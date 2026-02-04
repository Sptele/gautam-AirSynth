#include "Sine.h"

Sine::Sine(float amplitude, float freq, size_t tableLen) : tableLen(tableLen), amp(amplitude), freq(freq), left_phase(0), right_phase(0)
{
	table = new float[this->tableLen];

	// Generate table
	for (size_t i = 0; i < this->tableLen; ++i)
	{
		// High Precision will generate a better table
		table[i] = std::sin(i * PI/64);
	}
}

Sine::~Sine()
{
	delete[] table;
}

void Sine::print_table() const
{
	for (size_t i = 0; i < tableLen && i < 1000 /* Hard cap */; ++i)
	{
		std::cout << table[i] << std::endl;

		if (i >= 999)
		{
			std::cout << "... [" << tableLen - 1000 << " elements hidden] ..." << std::endl;

			break;
		}
	}

}

float Sine::get_inter(float i) const
{
	int lower = std::floor(i);
	int higher = std::ceil(i);

	return (table[lower] + table[higher]) / 2;
}

int Sine::stream(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	Sine* s = static_cast<Sine*>(userData);
	float* out = static_cast<float*>(outputBuffer);
	(void)inputBuffer; /* Prevent unused variable warning. */

	float phase = 0;

	for (unsigned int i = 0; i < framesPerBuffer; ++i)
	{
		*out++ = s->left_phase;
		*out++ = s->right_phase;

		double sam_i = (static_cast<double>(s->tableLen) / SAMPLE_RATE) * s->freq;

		phase = std::fmod(phase + sam_i, s->tableLen);

		s->left_phase = s->get_inter(phase);
		s->right_phase = s->get_inter(phase);

	}

	return 0;

}
