if [ `id -u` -ne 0 ]
then
  echo "Please run this script as root"
  exit
fi

if [ ! -d Third-Party ]
then
  echo Third-party directory not found
  exit
fi
cd Third-Party

echo -- Pre-req: WiringPi
cd wiringPi/wiringPi
make install
cd ../gpio
make install
cd ../devLib
make install
cd ../..

echo -- Pre-req: SDL-Mixer
apt-get install libsdl1.2-dev libsdl-mixer1.2-dev

echo -- Pre-req: ServoBlaster
cd ServoBlaster
cp servod /usr/local/bin
chown root /usr/local/bin/servod
chmod 4755 /usr/local/bin/servod
cd ..

echo -- PioScript
cd ..
cp pioscript /usr/local/bin
chown root /usr/local/bin/pioscript
chmod 4755 /usr/local/bin/pioscript
echo -- Done
