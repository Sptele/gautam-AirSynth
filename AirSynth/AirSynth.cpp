#include <complex>
#include <iostream>

#define SAMPLE_RATE (44100)
#define TABLE_LEN (256)

#include "ADSREnvelope.h"
#include "ComplexWave.h"
#include "portaudio.h"
#include "Sine.h"

static std::vector<Sine> generate_harmonic_series(float root_freq, float a, float b, int numSeries, ADSREnvelope& env, size_t tableLen, float length, float gain[])
{
	std::vector<Sine> v(numSeries);

	for (int i = 0; i < numSeries; ++i)
	{
		Sine temp(env, root_freq * (a * i + b), tableLen, length, gain[i]);
		v.push_back(temp);
	}

	return v;
	
}

int main(int argv, char** argc)
{
	if (argv != 4)
	{
		std::cerr << "Usage: " << argc[0] << " <amp> <freq> <length>" << std::endl;
		exit(1);
	}

	float amp = std::stof(argc[1]);
	float freq = std::stof(argc[2]);
	float length = std::stof(argc[3]);

	float gains[4] = { 1, 0.1f, 0.2f, 0.5f };
	ADSREnvelope env(TABLE_LEN, length, amp, .1f, .1f, .3f, .25f);

	Pa_Initialize();

	PaStream* stream;
	PaError err;

	ComplexWave wave(0.3f);

	wave.add_all_sine(generate_harmonic_series(freq, 2, 1, 4, env, TABLE_LEN, length, gains));
	wave.add_all_sine(generate_harmonic_series(freq*1.25, 2, 1, 4, env, TABLE_LEN, length, gains));

	


	err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, SAMPLE_RATE, TABLE_LEN, Wave::stream, &wave);

	err = Pa_StartStream(stream);

	Pa_Sleep(length * 1000);

	err = Pa_StopStream(stream);
	err = Pa_CloseStream(stream);

	Pa_Terminate();

	return 0;
}
