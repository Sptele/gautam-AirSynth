#pragma once 
class ADSREnvelope
{
	// Attack is described by a peak
	// Decay is described by a percentage of peak
	// Attack is a percentage of total time
	// Decay is percentage of total time
	// Release is percentage of total time
public:
	ADSREnvelope(size_t tableLen, float L, float A, float c_a, float c_d, float c_s, float c_r);
	ADSREnvelope() : tableLen(0) {}
	~ADSREnvelope();

	// deep-copy semantics
	ADSREnvelope(const ADSREnvelope& other);
	ADSREnvelope& operator=(const ADSREnvelope& other);

	void print_table() const;

	float operator[](float index) const;
private:
	float* table;
	const size_t tableLen;

	float L, A, c_a, c_d, c_s, c_r;
};

