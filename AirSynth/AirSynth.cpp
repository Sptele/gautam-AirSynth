#include <complex>
#include <cmath>
#include <iostream>

#define SAMPLE_RATE (44100)
#define TABLE_LEN (256)

#include "ADSREnvelope.h"
#include "ComplexWave.h"
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


    ADSREnvelope env(TABLE_LEN, length, amp, .1f, .01f, .3f, .025f);
    //env.print_table();

    ComplexWave wave(0.3f);

    float gains[4] = { 1, 0.1f, 0.2f, 0.5f };

    for (int i = 0; i < 4; ++i)
    {
        std::unique_ptr<Wave> temp = std::make_unique<Sine>(env, freq * (i + 1), TABLE_LEN, length, gains[i]);

            wave.add(temp);
    }


    err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, SAMPLE_RATE, TABLE_LEN, ComplexWave::stream, &wave);

    err = Pa_StartStream(stream);

    Pa_Sleep(length * 1000);

    err = Pa_StopStream(stream);
    err = Pa_CloseStream(stream);

    Pa_Terminate();

    return 0;
}