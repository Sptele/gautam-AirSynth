#include "SynthAPI.h"

void SynthAPI::add_waveform_series(float freq, float gain, float length)
{
#define HEIGHT 8
	float gains[HEIGHT] = { 1, 0.8, 0.6, 0.4, 0.3, 0.2, 0.1, 0.05 };

	ComplexWave to_add(gain);
	to_add.generate_harmonic_series(freq, HEIGHT, this->adsr_template, TABLE_LEN, length, gains);

	this->master.add_complex(to_add);

	this->log("Added Series!");
}

void SynthAPI::start()
{
	if (stream != nullptr) return;

	this->log("Opening Stream!");

	PaError err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, SAMPLE_RATE, TABLE_LEN, Wave::stream, &master);

	if (err != paNoError)
	{
		log(std::string("Pa_OpenDefaultStream failed: ") + Pa_GetErrorText(err));
		stream = nullptr;
		return;
	}

	err = Pa_StartStream(stream);

	if (err != paNoError)
	{
		log(std::string("Pa_StartStream failed: ") + Pa_GetErrorText(err));
		Pa_CloseStream(stream);
		stream = nullptr;
		return;
	}
}

void SynthAPI::stop()
{
	if (stream == nullptr) return;

	this->log("Ending Stream! Goodbye :P");

	Pa_StopStream(stream);
	Pa_CloseStream(stream);
	stream = nullptr;
}

void SynthAPI::rebuild_with_new_length(float new_length_sec)
{
	// Store length, indicate busy
	rebuild_pending_length.store(new_length_sec, std::memory_order_relaxed);
	rebuild_pending.store(true, std::memory_order_release);
	start_rebuild_worker();
}

void SynthAPI::start_rebuild_worker()
{
	if (rebuild_worker.joinable())
		return;

	rebuild_worker = std::thread([this]()
		{
			for (;;)
			{
				if (rebuild_shutdown.load(std::memory_order_acquire)) return;

				if (!rebuild_pending.load(std::memory_order_acquire))
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(2));
					continue;
				}

				rebuild_pending.store(false, std::memory_order_release);
				const float new_len = rebuild_pending_length.load(std::memory_order_relaxed);

				master.rebuild_length(new_len);
			}
		}
	);
}


void SynthAPI::log(const std::string& log) const
{
	std::cout << "[SYNTH] " << log << std::endl;
}
