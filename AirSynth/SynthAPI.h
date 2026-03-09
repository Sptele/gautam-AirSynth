#pragma once

#include <nanobind/nanobind.h>
#include <iostream>

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
    void add_waveform_series(float freq, float gain);

	void start();
	void stop();
private:
	// We want a master ComplexWave to store ALL subwaveforms
	ComplexWave master;
	ADSREnvelope adsr_template;

	PaStream* stream;

	void log(const std::string& log) const;
};

namespace nb = nanobind;

// This is GNARLY
NB_MODULE(AirSynth, m)
{
	nb::class_<SynthAPI>(m, "SynthAPI")
		.def(nb::init<>())
		.def("add_waveform_series", &SynthAPI::add_waveform_series)
		.def("start", &SynthAPI::start)
		.def("stop", &SynthAPI::stop);
}

