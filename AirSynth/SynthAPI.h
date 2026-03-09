#pragma once

#include <nanobind/nanobind.h>
#include <iostream>

/**
 * API Requirements for the Synth

The Synth must provide:
- a function to create and add new waveforms
    - These waveforms show be a harmonic series, specifically
- etc...

 */
class SynthAPI
{
public:
	SynthAPI() {}
    void add_waveform_series(float freq, float gain);
};

namespace nb = nanobind;


NB_MODULE(AirSynth, m)
{
	nb::class_<SynthAPI>(m, "SynthAPI")
		.def(nb::init<>())
		.def("add_waveform_series", &SynthAPI::add_waveform_series);
}

