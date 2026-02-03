#include <complex>
#include <cmath>
#include <iostream>

#define PI 3.14159265358979323846
#define SAMPLE_RATE (44100)
#define FREQ 440 // A = 440hz
#define AMPLITUDE 10

#include "portaudio.h"

// generate with the sampling_inc? idkjh
static double table[] = {
    0,
    0.5,
    0.707107,
    0.866025,
    1,
    0.866025,
    0.707107,
    0.5,
    0,
    -0.5,
    -0.707107,
    -0.866025,
    -1,
    -0.866025,
    -0.707107,
    -0.5
};

static int table_len = 16;

typedef struct
{
    int pa_i;
    float left_phase;
    float right_phase;
}
paTestData;

static int patestCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    /* Cast data passed through stream to our structure. */
    paTestData* data = (paTestData*)userData;
    float* out = (float*)outputBuffer;
    unsigned int i;
    (void)inputBuffer; /* Prevent unused variable warning. */

    int phase = 0;

    for (i = 0; i < framesPerBuffer; i++)
    {
        *out++ = data->left_phase;  /* left */
        *out++ = data->right_phase;  /* right */
        ///* Generate simple sawtooth phaser that ranges between -1.0 and 1.0. */
        //data->left_phase += 0.01f;
        ///* When signal reaches top, drop back down. */
        //if (data->left_phase >= 1.0f) data->left_phase -= 2.0f;
        ///* higher pitch so we can distinguish left and right. */
        //data->right_phase += 0.03f;
        //if (data->right_phase >= 1.0f) data->right_phase -= 2.0f;
        double sampling_inc = static_cast<double>(table_len) / SAMPLE_RATE * FREQ;
        std::cout << ">> " << sampling_inc << std::endl;
        phase = std::fmod(phase + sampling_inc, table_len);

    	data->left_phase = AMPLITUDE * table[phase];
        data->right_phase = AMPLITUDE * table[phase];

        std::cout << data->pa_i << ": (" << data->left_phase << ", " << data->right_phase << ")" << std::endl;

        ++data->pa_i;

        if (data->pa_i == 20) data->pa_i = 0;
    }
    return 0;
}

int main() {
    Pa_Initialize();

    static paTestData data{ 0, 0, 0 };

    PaStream* stream;
    PaError err;

    err = Pa_OpenDefaultStream(
		&stream,
        0,          /* no input channels */
        2,          /* stereo output */
        paFloat32,  /* 32 bit floating point output */
        SAMPLE_RATE,
        256,        /* frames per buffer, i.e. the number
                           of sample frames that PortAudio will
                           request from the callback. Many apps
                           may want to use
                           paFramesPerBufferUnspecified, which
                           tells PortAudio to pick the best,
                           possibly changing, buffer size.*/
        patestCallback, /* this is your callback function */
        &data); /*This is a pointer that will be passed to
                           your callback*/

    err = Pa_StartStream(stream);

    Pa_Sleep(10 * 1000);

    err = Pa_StopStream(stream);
    err = Pa_CloseStream(stream);

    Pa_Terminate();
    return 0;
}