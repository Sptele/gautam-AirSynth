#pragma once
#include <portaudio.h>

#include <complex>
#include <iostream>

#include "ADSREnvelope.h"
#include "Wave.h"

#define SAMPLE_RATE (44100)
#define FREQ 440 // A = 440hz
#define AMPLITUDE 100
#define PI 3.14159265358979323846

class Sine : public Wave
{
public:
    Sine(ADSREnvelope& amp_envelope, float freq, size_t tableLen, float length, float gain);
    ~Sine() override;
    // deep-copy semantics
    Sine(const Sine& other);
    Sine& operator=(const Sine& other);

    void print_table() const;

    float interpolate(float i) const;

    static int stream(const void* inputBuffer, void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData);

    void mono(float value);
    void monoOffsetStereo(float value, float offset);

    float get_length() const { return length; }

    float get_gain() const override { return gain; }
    float get_left_phase() const override { return left_phase; }
    float get_right_phase() const override { return right_phase; }

    void stream(unsigned int curr_frame) override;
private:
    float* table;
    const size_t tableLen;

    ADSREnvelope& amp;
    float freq;
    float length;
    float gain;

    float left_phase;
    float right_phase;
    float t_phase;
    float amp_phase;
};

