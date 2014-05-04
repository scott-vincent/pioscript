pioscript
=========

A GPIO Scripting Language for the Raspberry Pi

Watch a video demonstration of pioscript here: http://youtu.be/078fXcP3jSI

Pioscript is a script interpretor for the Raspberry Pi that sits somewhere between ScratchGPIO and Python. It is aimed at primary school children who want to progress from Scratch but aren't yet ready for the complexities of a full programming language such as Python. It is also aimed at people who either find Python a bit daunting or just have no interest in learning a full programming language but wish to explore GPIO on the Pi without investing too much time.

For primary school children I would strongly recommend using the Pibrella add-on board which is fully supported by pioscript. This gives some protection to the Pi as it has some components pre-wired but still exposes some pins for adding up to 4 custom inputs and 4 custom outputs. Using a Pibrella with pioscript means you can specify pins by name, e.g. Button, Amber, InputA without having to worry about pin numbers.

The concept originated from a diorama project and pioscript is ideal for tasks where you want the Pi to run headless and control your latest GPIO invention. Your script could run when the Pi boots up and you could use a GPIO switch to perform a clean shutdown.

Pioscript hides many of the complexities of a full language by allowing complex tasks to be performed in just a few lines of script and allows users to move on from a GUI driven programming environment to a command-line environment using an editor of their choice. I recommend using the nano text editor that comes pre-installed with Raspian.

Advantages over ScratchGPIO
---------------------------
- Speed Of Development - A terminal or remote SSH session can be used to type in a script or paste one in from a web site using any available editor. It can then be parsed and executed using the command line interpretor.
- Subscript Support - Variables can include subscripts so that arrays / lists can be used meaning more intricate programs can be developed if required (see the Simon game in the scripts directory for an example of this where the sequence to memorise gets longer after each turn).

Advantages over Python
----------------------
- Built-In Initialisation - All of the built-in resources such as GPIO, Audio (via SDL_Mixer) and PWM (via ServoBlaster) are initialised automatically and ready for use without a single line of script.
- Hidden Complexity - There are simple one line commands to perform complex operations such as fading out a sound, varying PWM output linearly or sinusoidally and reading an analog input such a photoresistor (see the scripts directory for examples of all of these).

Other Advantages
----------------
- Pibrella Support - There is full built-in support for the Pibrella add-on board which makes it very easy for primary school children to get up and running quickly. They can use easy to remember names for the GPIO inputs/outputs instead of trying to understand pin numbers.
- Play Songs - The play_song command can be used to play a complete song by specifying a sequence of musical note names with optional octave number and duration. If a Pibrella is being used the song will play on its built-in buzzer meaning that speakers do not have to be attached to the Pi.

Let's Go
--------
The only file you need to download to get up and running is the pioscript.tgz file. You can download this file directly on your Pi and install pioscript using the following commands:

    wget https://raw.github.com/scott-vincent/pioscript/master/pioscript.tgz
    tar xvf pioscript.tgz
    cd pioscript
    sudo ./install.sh
    more README
    
See the pioscript_README file for install instructions, full documentation and command reference.
