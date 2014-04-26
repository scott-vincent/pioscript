pioscript
=========

GPIO Scripting Language for Raspberry Pi

Pioscript is a script interpretor for the Raspberry Pi that sits somewhere between ScratchGPIO and Python. It is aimed at primary school children who want to progress from Scratch but aren't yet ready for the complexities of a full programming language such as Python.

Pioscript hides many of the complexities of a full language by allowing complex tasks to be performed in just a few lines of script and allows users to move on from a GUI driven programming environment to a command-line environment using an editor of their choice.

Advantages over ScratchGPIO
---------------------------
- Speed Of Development - A terminal or remote SSH session can be used to type in a script or paste one in from a web site using any available editor. It can then be parsed and executed using the command line interpretor.
- Subscript Support - Variables can include subscripts so that arrays / lists can be used meaning more intricate programs can be developed if required (see the Simon game in the scripts directory for an example of this where the sequence to memorise gets longer after each turn).

Advantages over Python
----------------------
- Built-In Initialisation - All of the built-in resources such as GPIO, Audio (via SSL-Mixer) and PWM (via ServoBlaster) are initialised automatically and ready for use with a single line of script.
- Hidden Complexity - There are simple one line commands to perform complex operations such as fading out a sound, varying PWM output linearly or sinusoidally and reading an analog input such a photoresistor (see the scripts directory for examples of all of these).

Other Advantages
----------------
- Pibrella Support - There is full built-in support for the Pibrella add-on board which makes it very easy for primary school children to get up and running quickly. They can use easy to remember names for the GPIO inputs/outputs instead of trying to understand pin numbers.
- Play Songs - The play_song command can be used to play a complete song by specifying a sequence of musical note names with optional octave number and duration. If a Pibrella is being used the song will play on its built-in buzzer meaning that speakers do not have to be attached to the Pi.

Let's Go
--------
The only file you need to download to get up and running is the pioscript.tar file.
See the pioscript_README file for install instructions, full documentation and command reference.
