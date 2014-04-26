USAGE="Usage: resetAudio.sh [hdmi|analog]"
if [ $# -ne 1 ]
then
  echo $USAGE
  exit
elif [ "$1" = hdmi -o "$1" = HDMI ]
then
  # Set audio to HDMI output
  amixer cset numid=3 2
elif [ "$1" = analog -o "$1" = ANALOG ]
then
  # Set audio to analog output
  amixer cset numid=3 1
else
  echo $USAGE
  exit
fi

# Set audio volume to 0dB
amixer set PCM 0

# Type alsamixer at the command line to check
