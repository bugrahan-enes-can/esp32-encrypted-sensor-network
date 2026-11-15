# ESP32 Encrypted Sensor Network

Encrypted (AES-128-GCM) wireless temperature and humidity sensor network with two ESP32 boards over ESP-NOW.

## Overview
A sender ESP32 reads temperature and humidity from a DHT22 sensor, encrypts the reading with AES-128-GCM and transmits it wirelessly over ESP-NOW. A receiver ESP32 verifies and decrypts the packet and shows the values on an SSD1306 OLED display.

## Hardware
- 2x ESP32 DevKit V1 (30-pin, CP2102)
- DHT22 temperature and humidity sensor module
- 0.96" SSD1306 128x64 I2C OLED
- Breadboard and jumper wires

## Software
- Arduino IDE 2.x with the ESP32 board package
- Libraries: Adafruit SSD1306, Adafruit GFX, DHT sensor library
- Wokwi for simulation

## Progress
- [x] DHT22 reading (Wokwi simulation)
- [x] AES-128-GCM encrypt / decrypt with authentication tag (mbedTLS, single board)
- [x] OLED output of the decrypted values
- [ ] ESP-NOW sender / receiver on two boards
- [ ] Range and packet-loss measurements
- [ ] Custom PCB (KiCad)

## Repository structure
- `wokwi-aes-oled-test/`: single-board simulation of the sensor, AES-GCM and OLED chain
