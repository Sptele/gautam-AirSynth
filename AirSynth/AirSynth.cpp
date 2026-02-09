#include <complex>
#include <cmath>
#include <iostream>

#define SAMPLE_RATE (44100)
#define AMPLITUDE 50

#include "ADSREnvelope.h"
#include "portaudio.h"
#include "Sine.h"


int main(int argv, char** argc) {
    if (argv != 4)
    {
        std::cerr << "Usage: " << argc[0] << " <amp> <freq> <length>" << std::endl;
        exit(1);
    }

    double amp = std::stoi(argc[1]);
    double freq = std::stoi(argc[2]);
    double length = std::stoi(argc[3]);

    Pa_Initialize();

    PaStream* stream;
    PaError err;


    ADSREnvelope env(256, length, amp, .01f, .01f, .3f, .1f);
    //env.print_table();


    Sine wave(env, freq, 256, length);
    //wave.print_table();


    err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, SAMPLE_RATE, 256, Sine::stream, &wave);

    err = Pa_StartStream(stream);

    Pa_Sleep(length * 1000);

    err = Pa_StopStream(stream);
    err = Pa_CloseStream(stream);

    Pa_Terminate();

    return 0;
}