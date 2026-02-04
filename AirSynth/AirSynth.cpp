#include <complex>
#include <cmath>
#include <iostream>

#define SAMPLE_RATE (44100)
#define AMPLITUDE 50

#include "portaudio.h"
#include "Sine.h"


int main(int argv, char** argc) {
    if (argv != 3)
    {
        std::cerr << "Usage: " << argc[0] << "<amp> <freq>" << std::endl;
        exit(1);
    }

    double amp = std::stoi(argc[1]);
    double freq = std::stoi(argc[2]);

    Pa_Initialize();

    PaStream* stream;
    PaError err;

    Sine wave(amp, freq, 100'000);
    wave.print_table();

    err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, SAMPLE_RATE, 256, Sine::stream, &wave);

    err = Pa_StartStream(stream);

    Pa_Sleep(10 * 1000);

    err = Pa_StopStream(stream);
    err = Pa_CloseStream(stream);

    Pa_Terminate();

    return 0;
}