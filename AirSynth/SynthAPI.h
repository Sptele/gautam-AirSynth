#pragma once

#include <nanobind/nanobind.h>
#include <iostream>
#include <thread>

#include "ComplexWave.h"

/**
 * API Requirements for the Synth

The Synth must provide:
- a function to create and add new waveforms
    - These waveforms show be a harmonic series, specifically
- etc...

 */

#define TABLE_LEN 256
#define LENGTH 3.0
#define AMP 10.0

class SynthAPI
{
public:
	SynthAPI() : master(0.25f), adsr_template(TABLE_LEN, LENGTH, AMP, 0.1f, 0.1f, 0.75f, 0.1f), stream()
	{
		Pa_Initialize();
	}
	~SynthAPI()
	{
		rebuild_shutdown.store(true, std::memory_order_release);
		rebuild_pending.store(true, std::memory_order_release);

		if (rebuild_worker.joinable()) rebuild_worker.join();

		stop();
		Pa_Terminate();
	}

    void add_waveform_series(float freq, float gain, float length);

	void start();
	void stop();

	void rebuild_with_new_length(float new_length_sec);

private:
	// We want a master ComplexWave to store ALL subwaveforms
	ComplexWave master;
	ADSREnvelope adsr_template;

	PaStream* stream;

	std::atomic<bool> rebuild_pending{ false };
	std::atomic<float> rebuild_pending_length{ static_cast<float>(LENGTH) };
	std::atomic<bool> rebuild_shutdown{ false };
	std::thread rebuild_worker;

	void start_rebuild_worker();

	void log(const std::string& log) const;
};

namespace nb = nanobind;

// This is GNARLY
NB_MODULE(AirSynth, m)
{
	nb::class_<SynthAPI>(m, "SynthAPI")
		.def(nb::init<>())
		.def("add_waveform_series", &SynthAPI::add_waveform_series)
		.def("rebuild_with_new_length", &SynthAPI::rebuild_with_new_length)
		.def("start", &SynthAPI::start)
		.def("stop", &SynthAPI::stop);
}

