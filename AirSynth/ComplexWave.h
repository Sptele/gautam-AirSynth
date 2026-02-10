#pragma once

#include <functional>
#include <vector>
#include <memory>

#include "Sine.h"

class ComplexWave : public Wave
{
public:
	ComplexWave(float gain);
	~ComplexWave() override;

	void add(const std::unique_ptr<Wave>& ptr) = delete;
	void add_sine(ADSREnvelope& amp_envelope, float freq, size_t tableLen, float length, float gain);
	void add_all_sine(const std::vector<Sine>& vec);
	float get_left_phase() const override;
	float get_right_phase() const override;
	float get_gain() const override { return gain; }

	std::vector<std::unique_ptr<Wave>>& synth() { return waves; }

	void stream(unsigned int curr_frame) override;

	static int stream(const void* inputBuffer, void* outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void* userData);
private:

	std::vector<std::unique_ptr<Wave>> waves;
	float gain;
};

