#pragma once
#include <portaudio.h>

#include <complex>
#include <iostream>

#include "ADSREnvelope.h"

#define SAMPLE_RATE (44100)
#define FREQ 440 // A = 440hz
#define AMPLITUDE 100
#define PI 3.14159265358979323846

class Sine
{
public:
    Sine(ADSREnvelope& amp_envelope, float freq, size_t tableLength, float length);
    ~Sine();

    void print_table() const;

    float interpolate(float i) const;

    static int stream(const void* inputBuffer, void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData);

    void mono(float value);
    void monoOffsetStereo(float value, float offset);

    float get_length() const { return length;  }
private:
    float* table;
    const size_t tableLen;

    float freq;
    ADSREnvelope& amp;
    float left_phase;
    float right_phase;
    float t_phase;
    float amp_phase;
    float length;
};

