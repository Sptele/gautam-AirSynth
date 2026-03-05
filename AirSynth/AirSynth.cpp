#include <complex>
#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#define SAMPLE_RATE (44100)
#define TABLE_LEN (256)
#define NUM_ARGS 5

#include <windows.h> // for GetModuleHandle/GetModuleFileName

#include "ADSREnvelope.h"
#include "ComplexWave.h"
#include "portaudio.h"
#include "Sine.h"


int main(int argv, char** argc)
{
    // 0 = open default camera.
    int deviceID = 0;
    // 0 = autodetect default API.
    int apiID = cv::CAP_ANY;

    cv::VideoCapture cap;
    // Open selected camera using selected API.
    cap.open(deviceID, apiID);

    // Check if we succeeded in opening the camera
    if (!cap.isOpened()) {
        std::cerr << "ERROR! Unable to open camera\n";
        return -1;
    }

    cv::Mat frame;
    cv::namedWindow("Live Camera Feed", cv::WINDOW_AUTOSIZE); // Create a window

    // Continuous loop to read and display frames
    while (true) {
        // Read a new frame from video stream
        cap >> frame;

        // Check if frame is empty
        if (frame.empty()) {
            std::cerr << "ERROR! Blank frame grabbed\n";
            break;
        }

        // Show the frame in the "Live Camera Feed" window
        cv::imshow("Live Camera Feed", frame);

        // Wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
        if (cv::waitKey(30) == 27) {
            std::cout << "Esc key is pressed by user. Stopping video.\n";
            break;
        }
    }

    // Release the camera and destroy all windows (optional, but good practice)
    cap.release();
    cv::destroyAllWindows();

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
	ADSREnvelope env(TABLE_LEN, length, amp, .1f, .1f, .3f, .25f);

	Pa_Initialize();

	PaStream* stream;
	PaError err;

	ComplexWave A(0.3f);

	A.generate_harmonic_series(freq, HEIGHT, env, TABLE_LEN, length, gains);

	ComplexWave B(0.4f);

	B.generate_harmonic_series(freq * 1.1892f, HEIGHT, env, TABLE_LEN, length, gains);

	ComplexWave C(0.3f);

	C.generate_harmonic_series(freq * 1.5f, HEIGHT, env, TABLE_LEN, length, gains);

	ComplexWave wave(1);
	wave.add_complex(A);
	wave.add_complex(B);
	wave.add_complex(C);


	


	err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, SAMPLE_RATE, TABLE_LEN, Wave::stream, &wave);

	err = Pa_StartStream(stream);

	Pa_Sleep(length * 1000);

	err = Pa_StopStream(stream);
	err = Pa_CloseStream(stream);

	Pa_Terminate();

	return 0;
}
