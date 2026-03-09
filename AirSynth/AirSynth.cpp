#include <complex>
#include <iostream>
#include <fstream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>


#define SAMPLE_RATE (44100)
#define TABLE_LEN (256)
#define NUM_ARGS 5

#include "ADSREnvelope.h"
#include "ComplexWave.h"
#include "portaudio.h"
#include "Sine.h"


int main(int argv, char** argc)
{

    if (argv != NUM_ARGS)
    {
        std::cerr << "Usage: " << argc[0] << " <amp> <freq> <length>" << std::endl;
        exit(1);
    }

    float amp = std::stof(argc[1]);
    float freq = std::stof(argc[2]);
    float length = std::stof(argc[3]);


#define HEIGHT 8

    float gains[HEIGHT] = { 1, 0.8, 0.6, 0.4, 0.3, 0.2, 0.1, 0.05 };
    ADSREnvelope env(TABLE_LEN, length, amp, 0, 0, 1, 0);

    Pa_Initialize();

    PaStream* stream;
    PaError err;

    ComplexWave A(0.3f);

    A.generate_harmonic_series(freq, HEIGHT, env, TABLE_LEN, length, gains);

    ComplexWave B(0.4f);

    B.generate_harmonic_series(freq * 1.1892f, HEIGHT, env, TABLE_LEN, length, gains);

    ComplexWave C(0.3f);

    C.generate_harmonic_series(freq * 1.5f, HEIGHT, env, TABLE_LEN, length, gains);

    ComplexWave wave(0.01f);
    wave.add_complex(A);
    wave.add_complex(B);
    wave.add_complex(C);

    err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, SAMPLE_RATE, TABLE_LEN, Wave::stream, &wave);


    err = Pa_StartStream(stream);

    err = Pa_StopStream(stream);
    err = Pa_CloseStream(stream);

    Pa_Terminate();



	return 0;
}
