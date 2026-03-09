# AirSynth

How can I make using a synthesizer intuitive for the average user, using Computer Vision? AirSynth answers this question by returning to our hands: simply pinch, close, and move your hands to control a variable amount of constructive waveforms (additive synthesis)!

Elementary controls for the synthesizer are operated with your hands, and sound playback operates on a timed loop. Sounds will Attack, Decay, Sustain, and Release before restarting. This lets you better understand the chord you've created!

For more understanding of the timeline, follow this [presentation](https://docs.google.com/presentation/d/1sDdlktXYAuvMxP4pl61lq2GmVgRPhMfZiHRXvumgJyQ/edit?usp=sharing).

Built by Gautam Khajuria (CCS Computing '29) at UCSB for Harlan Kringen's CMPTGCS 1L class!

## Building

This is a long build process due to the various C++ and Python dependencies. Just stick with it, and all will be well!

You can build in either `Debug` or `Release`. It should work either way. There is no internal difference in `AirSynth`'s code; however, dependencies likely optimize in Release builds

### Python Build Instructions
* Install `Python 3.10` from the Python website. Make sure that the executable is added to PATH
* Download and open the project folder in Visual Studio. Open the developer terminal.
* Make sure you are at the root directory of the project (`AirSynth/`) and not a subdirectory like (`AirSynth/AirSynth`) (confusing, huh?)
* Run `pip install -r requirements.txt` to install the necessary Python dependencies

### PortAudio Installation
AirSynth depends on `PortAudio` to play audio. The repo _does not_ include this dependency; you need to install it manually
* Click on this link to install the recommended stable PortAudio zip (`.tgz`): [PortAudio v19](https://files.portaudio.com/archives/pa_stable_v190700_20210406.tgz). You MUST install version `v19` for the project to work
* Once the tarball is installed, extract only the subdirectory named `portaudio`. This subdirectory contains the _source code_, not the build files. You need to manually build the project using Visual Studio. Follow this [guide](https://files.portaudio.com/docs/v19-doxydocs/compile_windows.html) and include only the `PA_USE_WASAPI` flag.
* Once portaudio has been built, move it into `ABS_PATH/AirSynth`
* Verify that your installation is correct by ensuring that the file structure is:
  ```
  AirSynth/
    ...
    portaudio/
      Release/
      portaudio/
    ...
  ```

### Visual Studio Build Settings
Now, you're going to have to mess with Visual Studio's build settings. Don't be scared, however! If I can do it, anyone can!
* First, find out where `Python 3.10` is located on your computer. This process varies by device; for example, on the Windows command prompt, you can type `where python` to get the directory
* We are going to call this location `PYTHON_DIR` from now on, so whenever you see `PYTHON_DIR`, just replace it with your Python 3.10 directory
* For AirSynth to bind C++ code into Python, it requires the location of several `include/` directories (which are where our dependencies store their code)
* Assuming that `nanobind` was installed at `PYTHON_DIR/lib/site-packages/nanobind`, take note of (preferably on a big sheet of paper) these directories (which tell AirSynth where to find the code for python, nanobind, and nanobind's dependency `robin_map`):
  1. `PYTHON_DIR/lib/site-packages/nanobind/include`
  2. `PYTHON_DIR/include`
  3. `PYTHON_DIR/lib/site-packages/nanobind/src`
  4. `PYTHON_DIR/lib/site-packages/nanobind/ext/robin_map`
* Back in Visual Studio, click the little Arrow next to `Local ____ Debugger`; click `AirSynth Debug Properties`. Then click `C/C++ -> General -> Additional Include Directories`, and add the four directories listed above as separate entries. Click `Ok`.
* In the same menu, add the `portaudio` dependency by adding (assuming the absolute path to the parent directory of the projet is called `ABS_PATH`) `ABS_PATH/AirSynth/portaudio/portaudio/include`
* Next, in the same Properties Page, click `Linker -> General -> Additional Library Dependencies` and add `PYTHON_DIR/libs` and `ABS_PATH/AirSynth/portaudio/release`. Click `Ok`
* Then, inside of the `Linker` subheading, click `Input -> Additional Dependencies`. Add `portaudio_x64.lib` and `python310.lib`.
* Additionally, click `C/C++ -> Code Generation -> Runtime Library`, and ensure it has either `/MD` or `/MDd`
* Click `Apply` and then `Ok` to close the window
* Finally, you need to locate the `portaudio_x64.dll` from within `ABS_PATH/portaudio/release`; **copy** this `.dll` and place it in `ABS_PATH/AirSynth/x64/Debug/` (or `/Release/` for `Release` builds). If this directory doesn't exist, attempt to build the project (it's okay if this fails, as it should still make the directory)

### Command Settings
To run the project, you need to do two things:
1. Build the C++ into a `.pyd` library
2. Run `vision.py` using Python

Luckily, Visual Studio can manage all of this, but you need to change some build settings:
* Open the `Project Properties` (little arrow next to the debugger button), and click `General -> Configuration Type`; click the current value (likely `.exe`) and use the dropdown menu to set it to `Dynamic Library (.dll)`
* While you're here, ensure that AirSynth uses the `C++17 Language Standard` (`/std:c++17`) and that the `Platform Toolset` is set to `v143`
* Then, click `Debugging -> Command`, and set the command to be `PYTHON_DIR/python.exe`. Furthermore, set `Command Arguments` to `vision.py`
* Finally, click `Advanced -> Target File Extension`, and set it to `.pyd` (which is what `nanobind` looks for)

### Running
If everything above was done correctly, you just need to click the green arrow at the top (or press `Shift-F5`) to spin the python terminal that will run the code!

## Program Operation
The additive synthesizer currently has elementary controls for various elementary functions:
* **Add a Wave**: Close your right hand into a fist (in view of the camera). The dot on your hand will turn cyan, and text indicating the frequency (in `hz`) and the gain (in `%`) will appear
  - Move your hand up/down to increase/decrease the frequency
  - Move your hand right/left to increase/decrease the gain
  - The increment calculation works so that:
      - if your hand is in the bottom/left half of the screen when you close it, the increment will be large (in the 10s) so that you can quickly scale up to the max (`2093hz`)
      - if your hand is in the other half when you close it, the increment will be small so that you can fine-tune lower frequencies
      - The frequency defaults to `440hz` (an `A4` - the standard tuning pitch), and the gain defaults to `25%`
  - Once you have the frequency/gain how you like it, open your fist. The dot on your fist will flash yellow, and a new sound will spawn
* **Pause/Resume Playback**: With your left hand, simply close and reopen your hand to toggle playback. Toggling playback will synchronize current sounds, which are not automatically synchronized when created
* **Change Length Duration**: To change how long notes play before restarting, "pinch" your pinky and thumb (bring them together). Similar to adjusting frequency, move your pinched hand up or down to adjust the frequency, which is shown in seconds on the screen.
  - To apply the change, release your hand. This will immediately cut playback and restart with the new length
  - **NOTE:** This is the most finicky part of the project, and jitters may occur.
* **End the Program**: press the `q` key to close the window

## Contributions

### Libraries
* **PortAudio**: All low-level audio output, including opening & spinning an audio stream, controlling the raw frequency of the speakers, and safely terminating the stream
* **NanoBind**: Transpiling & Binding the C++ synthesizer code (including PortAudio operation) into Python modules
* **OpenCV**: Getting camera input, displaying the live camera feed with text/circles added, and any internal modifications
* **MediaPipe**: based on OpenCV input, getting the coordinates of each hand's landmarks ([21 points per hand](https://ai.google.dev/edge/mediapipe/solutions/vision/hand_landmarker))

### LLM Usage
* Github Copilot & Google AI Overview was used minimally as a guide for unfamiliar complex things (Thread-Safety/Atomics, Debounce logic, etc.)
* They usually provided outlines, guides, and code mockups that I then wrote out manually in code
* Copilot did, on occasion, generate workable code that made considerable improvements to existing code. In this case, I made sure to ask Copilot to understand how this logic worked (i.e. PinchState)

### Theory
* I learned about the ADSR Envelope with this [Berklee Video](https://www.youtube.com/watch?v=0YeT9Gr-sJA). However, the ADSR implementation was [modeled](https://www.desmos.com/calculator/z9mylxstri) and written by me
* The synth was developed after reading this [tutorial by Giovanni De Poli](https://www.researchgate.net/profile/Giovanni-De-Poli/publication/245122776_A_Tutorial_on_Digital_Sound_Synthesis_Techniques/links/5460a5f50cf295b56162786e/A-Tutorial-on-Digital-Sound-Synthesis-Techniques.pdf) and other sources
  - However, the equations given in this guide had to be customized to better fit my model

Most other code was written by me, using logic that I designed and implemented.

## CSIL Notice
This project is roughly 1.30GB with dependencies and roughly 1.15GB without. Thus, given only a 4GiB quota on CSIL (which is not adjustable), and in consideration with future classes/assignments, it is **not possible** to host this project on CSIL.

Instead, it is better to just clone this repo, unpack it, and load it into VS. Furthermore, file systems work differently on Linux than Windows, so this guide may not be compatible.

## Copyright
<p>&copy; 2026 AirSynth. Licensed under <a href="https://www.gnu.org/licenses/agpl-3.0.en.html" target="_blank" rel="noopener noreferrer">Affero General Public License</a>.</p>


