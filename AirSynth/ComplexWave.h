#pragma once

#include <functional>
#include <vector>
#include <memory>

#include "Sine.h"

class ComplexWave : public Wave
{
public:
	ComplexWave(float gain);
	~ComplexWave() override = default;
	ComplexWave(const ComplexWave& o);

	std::unique_ptr<Wave> clone() const override;

	void add(const std::unique_ptr<Wave>& ptr) = delete;
	void add_sine(ADSREnvelope& amp_envelope, float freq, size_t tableLen, float length, float gain);
	void add_complex(const ComplexWave& o);

	void generate_harmonic_series(float freq, float height, ADSREnvelope& amp_envelope, size_t tableLen, float length, float gain[]);
	float get_left_phase() const override;
	float get_right_phase() const override;
	float get_gain() const override { return gain; }

	std::vector<std::unique_ptr<Wave>>& synth() { return *std::atomic_load(&waves); }

	void stream(unsigned int curr_frame) override;

	static int stream(const void* inputBuffer, void* outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void* userData);
private:

	std::shared_ptr<std::vector<std::unique_ptr<Wave>>> waves;
	float gain;
};

