#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <Preferences.h>
#include "mbedtls/gcm.h"
#include "secrets.h"

#define DHTPIN 4
#define DHTTYPE DHT22
#define REPLAY_TEST true

uint8_t aliciMac[] = {0xE0, 0x8C, 0xFE, 0x36, 0x6E, 0xB8};

typedef struct __attribute__((packed)) { float temp; float hum; uint32_t sayac; } Veri;
typedef struct __attribute__((packed)) {
  uint8_t nonce[12];
  uint8_t sifreli[sizeof(Veri)];
  uint8_t tag[16];
} Paket;

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(128, 64, &Wire, -1);
Preferences prefs;
uint32_t sayac = 0, kaydedilen = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  dht.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { Serial.println("OLED bulunamadi!"); while (true); }
  display.setTextColor(SSD1306_WHITE);

  prefs.begin("sayac", false);
  sayac = prefs.getUInt("son", 0) + 200;
  kaydedilen = sayac;
  prefs.putUInt("son", sayac);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) { Serial.println("ESP-NOW baslatilamadi!"); return; }
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, aliciMac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) { Serial.println("Alici eklenemedi!"); return; }
}
void loop() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);

  if (isnan(t) || isnan(h)) {
    display.println("Sensor");
    display.println("okunamadi");
  } else {
    sayac++;
    Veri v = {t, h, sayac};
    Paket p;
    esp_fill_random(p.nonce, 12);

    mbedtls_gcm_context gcm; mbedtls_gcm_init(&gcm);
    mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, KEY, 128);
    mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, sizeof(Veri), p.nonce, 12, NULL, 0,
                              (uint8_t *)&v, p.sifreli, 16, p.tag);
    mbedtls_gcm_free(&gcm);

    esp_err_t sonuc = esp_now_send(aliciMac, (uint8_t *)&p, sizeof(p));
    if (REPLAY_TEST && sayac % 5 == 0) {
      delay(50);
      esp_now_send(aliciMac, (uint8_t *)&p, sizeof(p));
      Serial.println("(replay testi: ayni paket tekrar gonderildi)");
    }

    if (sayac - kaydedilen >= 100) {
      prefs.putUInt("son", sayac);
      kaydedilen = sayac;
    }

    display.print("T: "); display.print(t, 1); display.println(" C");
    display.print("N: "); display.print(h, 0); display.println(" %");
    display.setTextSize(1);
    display.printf("Paket #%u %s\n", sayac, sonuc == ESP_OK ? "OK" : "HATA");
    Serial.printf("Gonderildi #%u\n", sayac);
  }

  display.display();
  delay(2000);
}