# ESP32 Homekit Thermometer

Simple esp32-powered Homekit thermometer.  I wrote very little of
this.  It's the temperature_sensor example from
https://github.com/maximkulkin/esp-homekit-demo, modified to build for
the esp32.

## Building

Update `main/config.h` with the details for your Wi-Fi network.  Then:

```bash
git submodule update --init --recursive
docker build -t esp32-build .
docker run --rm -v $PWD:/project -w /project esp32-build make menuconfig
docker run --rm -v $PWD:/project -w /project esp32-build make all
esptool.py --chip esp32 --port /dev/tty.usbserial-0001 --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x1000 build/bootloader/bootloader.bin 0x10000 build/esp32_homekit_thermometer.bin 0x8000 build/partitions_singleapp.bin
```

## Generate New Password and Setup ID

The app comes with a baked-in password and setup code.  Here's how you
can generate a new one.  Update `main/config.h` with these values.

```bash
sudo pip3 install pillow
git submodule update --init --recursive
PASSWORD=$(LC_ALL=C tr -dc '0-9' </dev/urandom | fold -w 8 | head -n 1 | sed 's/\(...\)\(..\)\(...\)/\1-\2-\3/')
SETUP_ID=$(LC_ALL=C tr -dc '0-9A-Z' </dev/urandom | fold -w 4 | head -n 1)
python ./components/homekit/tools/gen_qrcode 9 $PASSWORD $SETUP_ID qr.png
echo "Password: $PASSWORD"
echo "Setup ID: $SETUP_ID"
```
