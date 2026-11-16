# ESP32 Encrypted Wireless Sensor Link

![Setup](docs/setup.jpg)

Two ESP32 boards exchange temperature and humidity readings over ESP-NOW. Every packet is encrypted and authenticated with AES-128-GCM, and the receiver rejects replayed or tampered packets. The receiver also reports signal strength (RSSI) and packet loss.

## What it does

- **Sender:** reads a DHT22 sensor, shows the values on an SSD1306 OLED, encrypts them and transmits every 2 seconds.
- **Receiver:** decrypts and verifies each packet, rejects invalid or replayed ones, and prints the reading, RSSI and running packet-loss statistics to the serial monitor.

```
DHT22 --> [ESP32 sender] --(ESP-NOW, AES-GCM)--> [ESP32 receiver] --> Serial Monitor
              |
            OLED
```

## Security design

| Feature | How it is done |
|---|---|
| Confidentiality | AES-128 in GCM mode (mbedTLS, built into the ESP32 Arduino core) |
| Integrity / authenticity | 16-byte GCM authentication tag; packets with a wrong tag are dropped |
| Nonce | Fresh random 12-byte nonce for every packet (`esp_fill_random`) |
| Replay protection | Monotonic packet counter inside the encrypted payload; the receiver rejects any counter that is not greater than the last accepted one |
| Counter persistence | Sender stores the counter in flash (`Preferences`) and skips ahead after a reboot, so a restart does not make valid packets look old |
| Key handling | Key lives in `secrets.h`, which is git-ignored; `secrets.example.h` shows the format |

Example receiver output:

```
[25s] #290  T=28.7C  N=58%  RSSI=-33 dBm  alinan=13  kayip=0 (%0.0)  tekrar_red=2
TEKRAR paket reddedildi (sayac 290)
```

(Log messages are in Turkish: `alinan` = received, `kayip` = lost, `tekrar_red` = replays rejected.)

## Hardware

- 2x ESP32 DevKit (30-pin, CP2102)
- DHT22 module
- SSD1306 0.96" I2C OLED (128x64)
- Breadboard and jumper wires

### Wiring (sender)

| Part | Pin | ESP32 |
|---|---|---|
| OLED | SDA | GPIO21 |
| OLED | SCL | GPIO22 |
| OLED | VCC / GND | 3V3 / GND |
| DHT22 | DATA | GPIO4 |
| DHT22 | VCC / GND | 3V3 / GND |

The receiver needs only a USB connection.

## Theoretical range estimate (calculated, not measured)

This is a link-budget calculation, not a measurement. It shows what the datasheet numbers predict and will be compared with real measurements later.

**Inputs**

| Parameter | Value | Source |
|---|---|---|
| TX power (802.11b) | 19.5 dBm (typ) | ESP32 datasheet, Table 5-6 |
| RX sensitivity (11b, 1 Mbps) | -98 dBm (typ) | ESP32 datasheet, Table 5-6 |
| ESP-NOW default rate | 1 Mbps | ESP-IDF ESP-NOW docs |
| Frequency | 2412 MHz (channel 1) | assumption: unconnected STA default |
| Antenna gain (each side) | 0 dBi | assumption |
| Fade margin | 10 dB | assumption |

**Model:** log-distance path loss, `PL(d) = PL(1 m) + 10·n·log10(d) + wall losses`, with `PL(1 m) = 20·log10(f_MHz) - 27.55 = 40.1 dB` and `RSSI = Ptx + Gtx + Grx - PL`.
Path-loss exponent `n` and wall loss (8 dB per wall) are assumed typical values, not measured for this building.

**Predicted RSSI (dBm)**

| Distance | Free space (n=2) | Indoor, open (n=3) | Indoor, 2 walls (n=3, 16 dB) |
|---|---|---|---|
| 1 m | -20.6 | -20.6 | -36.6 |
| 2 m | -26.6 | -29.6 | -45.6 |
| 5 m | -34.6 | -41.6 | -57.6 |
| 10 m | -40.6 | -50.6 | -66.6 |
| 20 m | -46.6 | -59.6 | -75.6 |
| 50 m | -54.6 | -71.6 | -87.6 |
| 100 m | -60.6 | -80.6 | -96.6 |

**Predicted maximum range**

| Scenario | Down to sensitivity (-98 dBm) | With 10 dB fade margin |
|---|---|---|
| Free space | ~7.4 km | ~2.3 km |
| Indoor, open | ~380 m | ~175 m |
| Indoor, 2 walls | ~110 m | ~50 m |

**Caveats:** the datasheet numbers are for the chip, not for a dev-board PCB antenna, so real boards usually reach less. The free-space row is a theoretical upper bound that needs line of sight and no reflections. For reference, the two boards side by side on the desk reported about -33 dBm, which is lower than the model's 1 m prediction, so the model is optimistic at close range and should be calibrated with real data.

## Range test

Not measured yet. The table below is a template for the real measurements.

| Location | Obstacles | Avg RSSI (dBm) | Packet loss (%) |
|---|---|---|---|
| Same room, 1 m | none | - | - |
| Same room, far corner | none | - | - |
| Next room | 1 wall | - | - |
| Two rooms away | 2 walls | - | - |

## Build and run

1. Install the Arduino IDE and the ESP32 board package. Select **ESP32 Dev Module**.
2. Install libraries: `Adafruit SSD1306`, `Adafruit GFX Library`, `DHT sensor library`.
3. In both `alici/` and `gonderici/`, copy `secrets.example.h` to `secrets.h` and put the **same** random 16-byte key in both.
4. Put the receiver's MAC address into `aliciMac[]` in `gonderici/gonderici.ino`. (Print it with `WiFi.macAddress()`.)
5. Upload `alici/alici.ino` to the receiver and `gonderici/gonderici.ino` to the sender, then open the receiver's serial monitor at 115200 baud.

`REPLAY_TEST true` in the sender sends every 5th packet twice, so you can watch the receiver reject the duplicate.

## Limitations

- The key is shared manually; there is no key exchange or rotation.
- Replay state is held in RAM on the receiver, so a receiver reboot accepts the next packet regardless of its counter.
- ESP-NOW itself is used without its built-in encryption; security comes from the application-layer AES-GCM above.
- Range has not been measured yet; see the theoretical estimate above.

## Repository layout

```
alici/                receiver sketch
gonderici/            sender sketch
wokwi-aes-oled-test/  initial AES-GCM + OLED simulation (Wokwi)
docs/                 photos
```

## Author

Buğrahan Enes Can, Electronics and Communication Engineering