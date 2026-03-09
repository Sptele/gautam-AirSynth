#include "SynthAPI.h"

void SynthAPI::add_waveform_series(float freq, float gain)
{
#define HEIGHT 8
	float gains[HEIGHT] = { 1, 0.8, 0.6, 0.4, 0.3, 0.2, 0.1, 0.05 };

	ComplexWave to_add(gain);
	to_add.generate_harmonic_series(freq, HEIGHT, this->adsr_template, TABLE_LEN, LENGTH, gains);

	this->master.add_complex(to_add);

	this->log("Added Series!");
}

void SynthAPI::start()
{
	this->log("Opening Stream!");

	Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, SAMPLE_RATE, TABLE_LEN, Wave::stream, &master);

	Pa_StartStream(stream);
}

void SynthAPI::stop()
{
	this->log("Ending Stream! Goodbye :P");

	Pa_StopStream(stream);
	Pa_CloseStream(stream);
}

void SynthAPI::log(const std::string& log) const
{
	std::cout << "[SYNTH] " << log << std::endl;
}
