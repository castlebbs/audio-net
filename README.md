Audio-net: my connected music device 
======================

I love music, I love embedded systems. This is my prototype of an internet and Bluetooth connected music device. I really need a better name :-)

![devices](https://user-images.githubusercontent.com/381306/220328512-69c132dd-37b7-4bed-97ab-9b6f81f3f24d.jpg)

The hardware used supports, and has been tested to do the following:
* Audio inputs can be looped over, recorded to SD card, uploaded to the cloud. Audio input can be a codec input or a Bluetooth A2DP source.
* Mixing of multiple sound inputs.
* Wi-Fi connection to stream audio (to the cloud, or from the cloud).
* A2DP Bluetooth source and sinks to transmit or receive audio.
* SD card to store locally recording or cache audio from the internet.
* Assign controls from the foot switches to various functions (including BLE MIDI CC).

This version of the code checked-in on Github is a simple looper, which records the codec input (44.1kHz/16-bit) to SD card, and loops over it. This functions as a simple instrument looper.

## Hardware used

I experimented with audio on a variety of hardware. The most important criteria were the microcontroler and the audio codec. I also wanted to use a microcontroller, which can run FreeRTOS as I was more familiar with this embedded OS. Another criterion was that the components I chose had to be affordable. They also had to be popular enough that I can use one of the several PCB manufacturing and assembly online services. This last criterion greatly limited the options.

To experiment initially, I purchased a number of dev kits, which allow me to focus on software without having to worry about hardware design right away.
I obtained the following kits:
* NXP LPC55S69-EVK (Dual core Arm Cortex-M33 at 150 MHz, Audio codec with line in and line out).I also purchased a MIKROE-3542 Wi-Fi/BLE extension.
* ESP32-LyraT (Espressif Dual Core ESP32, ES8388 Audio Codec with line in, line out and mic)
* AiThinker Audio Kit (Ai-Thinker/Espressif Module ESP32-A1S, which include a ESP32 and an audio codec AC101)
* LilyGo Taudio (Espressif Module ESP32-WROVER, audio codec WM8978)

Some audio codecs I considered: [TI TLV320AIC3262](https://www.ti.com/product/TLV320AIC3262), [Analog Devices ADAU1961](https://www.analog.com/media/en/technical-documentation/data-sheets/ADAU1961.pdf)

I ended-up using mostly the [Ai-Thinker Audio kit](https://docs.ai-thinker.com/en/audio_development_board_esp32-audio-kit) with its integrated AC101 codec. It worked well for my needs and my requirements. It is by far the cheapest dev kit (I think I had found it for $11). The AC101 audio codec sounded better to my ears than the others (no measurement done). Some people have reported interference when Wi-Fi is used. One issue with the AC101 is that it was not supported by the Espressif SDKs. I could find some drivers online, but none worked correctly. I had to dive into the [AC101 datasheet](http://www.x-powers.com/en.php/Info/down/id/96) and fix the driver. My version of the driver is [here](https://github.com/castlebbs/audio-net/blob/main/components/my_board/my_codec_driver/new_codec.c).

![inside](https://user-images.githubusercontent.com/381306/220346351-a60a9cb2-3ce9-4278-b9b8-15323a14caf1.jpg)

Currently only line in and line out are connected, as well as the foot switches. The device is powered by a Lithium-ion battery.

### Software
If you want to read the code, [start here](https://github.com/castlebbs/audio-net/blob/main/main/main.c).
I used the ESP-IDF framework from Espressif. I didn't use the Espressif's audio framework [ESP-ADF](https://github.com/espressif/esp-adf). This is because I couldn't get the performance I needed. I believe (at least at the time) ESP-ADF uses inefficient ring buffers, and also uses them where they are not needed. I ended up using a different implementation for ring buffers, and directly addressing i2s for accessing the audio codec, instead of using Espressif API. I also leveraged more from the FreeRTOS API. Ultimately, this also led to better use of the i2s DMA support from the microcontroller and much better performance. My implementation of codec communication: [i2s_writer](https://github.com/castlebbs/audio-net/blob/main/main/i2s_writer.c) [i2s_reader](https://github.com/castlebbs/audio-net/blob/main/main/i2s_reader.c)

## Future ideas
Some ideas I had for it:
* Online collaborations device to play with other musicians (e.g. integration with Jamulus. No need for computer or audio interface.)
* Advanced looper, which would allow sharing loops real-time with other musicians.
* Leverage microphone and use voice recognition to control the device by voice (e.g. leveraging Amazon transcribe)
* Automatically save music to online DAWs or online drives e.g. Dropbox.
* Educational tool, e.g. with speech recognition you say, play a "ii-V-I backing track and record me"
* Some light DSP, e.g. reverb, but accounting for the limited RAM available.
* Allow some switches to be configured as midi controls. (Could be used as a wireless midi controller.)
* UI using LCD, and/or mobile app via Bluetooth or Wi-Fi.
