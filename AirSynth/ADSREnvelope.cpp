#include "ADSREnvelope.h"

#include <iostream>

ADSREnvelope::ADSREnvelope(size_t tableLen, float L, float A, float c_a, float c_d, float c_s, float c_r)
: tableLen(tableLen), L(L), A(A), c_a(c_a), c_d(c_d), c_s(c_s), c_r(c_r)
{
	this->table = new float[tableLen];

	// To fractions
	float a = c_a * L;
	float d = c_d * L;
	float s = c_s * A;
	float r_t = c_r * L;
	float s_t = L - a - d - r_t;

	for (int i = 0; i < tableLen; ++i)
	{
		// We can sample by increasing our multipying our index by totalTime / tableLen
		// This calculation makes sense with this visualization:
		// (Desmos) https://www.desmos.com/calculator/z9mylxstri
		float x = i * L / tableLen;

		if (x < a)
		{
			table[i] = (A / a) * x;
		} else if (x < a + d)
		{
			table[i] = A - (A - s) / d * (x - a);
		} else if (x < a + d + s_t)
		{
			table[i] = s;
		} else if (x < L)
		{
			table[i] = s - (s / r_t) * (x - (a + d + s_t));
		}
	}

}

ADSREnvelope::~ADSREnvelope()
{
	delete[] this->table;
}

// Copy constructor: deep copies the dynamically allocated table and copies parameters.
ADSREnvelope::ADSREnvelope(const ADSREnvelope& other)
	: tableLen(other.tableLen),
	L(other.L), A(other.A), c_a(other.c_a), c_d(other.c_d), c_s(other.c_s), c_r(other.c_r)
{
	this->table = new float[tableLen];
	std::copy(other.table, other.table + tableLen, this->table);
}

// Copy assignment: only allowed when table lengths match because tableLen is const.
// Performs deep copy of table contents and copies other parameters.
// Throws std::invalid_argument if lengths differ.
ADSREnvelope& ADSREnvelope::operator=(const ADSREnvelope& other)
{
	if (this == &other) return *this;

	if (other.tableLen != this->tableLen)
	{
		throw std::invalid_argument("ADSREnvelope::operator=: cannot assign envelopes with differing tableLen (tableLen is const).");
	}

	// copy scalar parameters
	this->L = other.L;
	this->A = other.A;
	this->c_a = other.c_a;
	this->c_d = other.c_d;
	this->c_s = other.c_s;
	this->c_r = other.c_r;

	// deep copy table contents
	std::copy(other.table, other.table + tableLen, this->table);

	return *this;
}

void ADSREnvelope::print_table() const
{
	for (size_t i = 0; i < tableLen; ++i)
	{
		std::cout << "Envelope: " << table[i] << std::endl;
	}
}

float ADSREnvelope::operator[](float i) const
{
	int lower = std::floor(i);

	if (lower == i) return i; // If i is integer, just return it to avoid computation

	int higher = std::ceil(i);

	if (lower < 0) return table[0];
	if (higher >= tableLen) return table[tableLen - 1];

	return (table[lower] + table[higher]) / 2;
}

 