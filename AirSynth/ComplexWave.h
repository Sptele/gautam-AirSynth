#pragma once
#include <vector>

#include "Sine.h"

class ComplexWave
{
public:
	void add(ADSREnvelope& amp_envelope, float freq, size_t tableLength, float length, float gain);
	float sum_left_phases() const;
	float sum_right_phases() const;

	ComplexWave(float gain);


	static int stream(const void* inputBuffer, void* outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void* userData);
private:

	std::vector<Sine> waves;
	float gain;
};

