#pragma once
#include <portaudio.h>

#include <complex>
#include <iostream>

#define SAMPLE_RATE (44100)
#define FREQ 440 // A = 440hz
#define AMPLITUDE 100
#define PI 3.14159265358979323846

class Sine
{
public:
    Sine(float amplitude, float freq, size_t tableLength);
    ~Sine();

    void print_table() const;

    float get_inter(float i) const;

    static int stream(const void* inputBuffer, void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData);
private:
    float* table;
    const size_t tableLen;

    float freq;
    float amp;
    float left_phase;
    float right_phase;
};

