# AirSynth

How can I make using a synthesizer intuitive for the average user, using Computer Vision? AirSynth answers this question by returning to our hands: simply pinch, close, and move your hands to control a variable amount of constructive waveforms (additive synthesis)!

## Building

This is a long build process due to the various C++ and Python dependencies. Just stick with it and all will be well:
* Install `Python 3.10` from the Python website. Make sure that the executable is added to PATH
* Download and open the project folder in Visual Studio. Open the developer terminal.
* Make sure you are at the root directory of the project (`AirSynth/`) and not a subdirectory like (`AirSynth/AirSynth`) (confusing, huh?)
* Run `pip install -r requirements.txt` to install the necessary python dependencies
* Now you're going to have to mess with Visual Studio's build settings. Don't be scared, however! If *I* could do it, then anyone can!
* First, find out where python 3.10 is located on your computer. This process varies by device; for example, on the windows command prompt, you can type `where python` to get the directory
* We are going to call this location `PYTHON_DIR` from now, so whenever you see `PYTHON_DIR`, just replace it with your python 3.10 directory
* In order for AirSynth to bind C++ code into Python, it requires the location of several `include/` directories (which is where our dependencies store their code)
* Assuming that `nanobind` was installed at `lib\site-packages\nanobind`, take note of (preferably on a big sheet of paper) these directories (which tell AirSynth where to find the code for python, nanobind, and nanobind's dependency robin_map):
  1. `PYTHON_DIR/lib/site-packages/nanobind/include`
  2. `PYTHON_DIR/include`
  3. `PYTHON_DIR/lib/site-packages/nanobind/src`
  4. `PYTHON_DIR/lib/site-packages/nanobind/ext/robin_map`
* Back in Visual Studio, click the little Arrow next to `Local ____ Debugger`; click `AirSynth Debug Properties`. Then click `C/C++ -> General -> Additional Include Directories`, and add the four directories listed above as separate entries. Click `Ok`.
* In the same menu, add the `portaudio` dependency by adding (assuming the absolute path to the parent directory of the projet is called `ABS_PATH`) `ABS_PATH/AirSynth/portaudio/portaudio/include`
* Next, in the same Properties Page, click `Linker -> General -> Additional Library Dependencies` and add `PYTHON_DIR/libs` and `ABS_PATH/AirSynth/portaudio/release`. Click `Ok`
* Then, inside of the `Linker` subheading, click `Input -> Additional Dependencies`. Add `portaudio_x64.lib` and `python310.lib`.
* Click `Apply` and then `Ok` to close the window
* Finally, you need to locate the `portaudio_x64.dll` from within `ABS_PATH/portaudio/release`; copy this `.dll` and place it in `ABS_PATH/AirSynth/x64/Debug/`. If this directory doesn't exist, attempt to build the project (it's okay if this fails, as it should still make the directory)
* You should be all set to build the project!
