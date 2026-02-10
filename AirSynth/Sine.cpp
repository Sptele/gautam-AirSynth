#include "Sine.h"

Sine::Sine(ADSREnvelope& amplitude, float freq, size_t tableLen, float length, float gain)
: tableLen(tableLen), amp(amplitude), freq(freq), length(length), gain(gain), left_phase(0), right_phase(0), t_phase(0), amp_phase(0)
{
	table = new float[this->tableLen];

	const double m_PI = std::acos(-1.);

	// Generate table
	for (int i = 0; i < this->tableLen; ++i)
	{
		// We want to sample n partitions of [0, 2pi]
		// partition = 2pi/n

		// In this case, i = 2pi / n. We can still apply freq * i


		// Asin(2*pi*freq*i)
		table[i] = std::sin(2 * m_PI * static_cast<float>(i)/ this->tableLen);
	}
}

Sine::~Sine()
{
	delete[] table;
}

// Copy constructor: deep copies the table and copies scalar state.
// The amp reference is bound to the same envelope as the source.
Sine::Sine(const Sine& other)
	: tableLen(other.tableLen),
	freq(other.freq),
	amp(other.amp),
	left_phase(other.left_phase),
	right_phase(other.right_phase),
	t_phase(other.t_phase),
	amp_phase(other.amp_phase),
	length(other.length),
	gain(other.gain)
{
	this->table = new float[this->tableLen];
	std::copy(other.table, other.table + this->tableLen, this->table);
}

// Copy assignment: only allowed when table lengths match and amp reference is the same.
// Throws std::invalid_argument if lengths differ or amp differs (reference cannot be reseated).
Sine& Sine::operator=(const Sine& other)
{
	if (this == &other) return *this;

	if (other.tableLen != this->tableLen)
	{
		throw std::invalid_argument("Sine::operator=: cannot assign Sine objects with differing tableLen (tableLen is const).");
	}

	if (&other.amp != &this->amp)
	{
		throw std::invalid_argument("Sine::operator=: cannot assign Sine objects with different amp references.");
	}

	// copy scalar parameters
	this->freq = other.freq;
	this->left_phase = other.left_phase;
	this->right_phase = other.right_phase;
	this->t_phase = other.t_phase;
	this->amp_phase = other.amp_phase;
	this->length = other.length;
	this->gain = other.gain;

	// deep copy table contents
	std::copy(other.table, other.table + this->tableLen, this->table);

	return *this;
}

void Sine::print_table() const
{
	for (size_t i = 0; i < tableLen && i < 1000 /* Hard cap */; ++i)
	{
		std::cout << table[i] << std::endl;

		if (i > 999)
		{
			std::cout << "... [" << tableLen - 1000 << " elements hidden] ..." << std::endl;

			break;
		}
	}

}

float Sine::interpolate(float i) const
{

	int lower = std::floor(i);

	if (lower == i) return i; // If i is integer, just return it to avoid computation

	int higher = std::ceil(i);

	if (lower < 0) return table[0];
	if (higher >= tableLen) return table[tableLen - 1];

	return (table[lower] + table[higher]) / 2;
}

int Sine::stream(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	Sine* s = static_cast<Sine*>(userData);
	float* out = static_cast<float*>(outputBuffer);
	(void)inputBuffer; /* Prevent unused variable warning. */

	float sam_i = s->freq / SAMPLE_RATE * s->tableLen;

	for (unsigned int i = 0; i < framesPerBuffer; ++i)
	{
		s->t_phase += sam_i;

		s->mono(s->amp[s->amp_phase] * s->interpolate(s->t_phase));

		s->t_phase = std::fmod(s->t_phase, s->tableLen);
		s->amp_phase = std::fmod(s->amp_phase + static_cast<float>(s->tableLen) / (s->length * SAMPLE_RATE), s->tableLen);

		*out++ = s->left_phase;
		*out++ = s->right_phase;
	}

	return 0;

}

void Sine::mono(float value)
{
	left_phase = value;
	right_phase = value;
}

void Sine::monoOffsetStereo(float value, float offset)
{
	left_phase = value;
	right_phase = offset * value;
}

void Sine::stream(unsigned int curr_frame)
{
	float sam_i = freq / SAMPLE_RATE * tableLen;

	t_phase += sam_i;

	mono(get_gain() * amp[amp_phase] * interpolate(t_phase));

	t_phase = std::fmod(t_phase, tableLen);
	amp_phase = std::fmod(amp_phase + static_cast<float>(tableLen) / (length * SAMPLE_RATE), tableLen);
}
