#pragma once

class Wave
{
public:
	virtual float get_left_phase() const = 0;
	virtual float get_right_phase() const = 0;

	virtual void stream(unsigned int curr_frame) = 0;
	virtual float get_gain() const = 0;

	virtual ~Wave() = default;

	static int stream(const void* inputBuffer, void* outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void* userData)
	{
		Wave* w = static_cast<Wave*>(userData);
		float* out = static_cast<float*>(outputBuffer);
		(void)inputBuffer; /* Prevent unused variable warning. */

		for (unsigned int i = 0; i < framesPerBuffer; ++i)
		{
			w->stream(i);

			*out++ = w->get_left_phase();
			*out++ = w->get_right_phase();
		}

		return 0;
	}
};

