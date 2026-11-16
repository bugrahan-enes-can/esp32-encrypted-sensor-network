#include <WiFi.h>
#include <esp_now.h>
#include "mbedtls/gcm.h"
#include "secrets.h"

typedef struct __attribute__((packed)) { float temp; float hum; uint32_t sayac; } Veri;
typedef struct __attribute__((packed)) {
  uint8_t nonce[12];
  uint8_t sifreli[sizeof(Veri)];
  uint8_t tag[16];
} Paket;

uint32_t sonSayac = 0;
bool ilkPaket = true;
uint32_t alinan = 0, kayip = 0, reddedilenTekrar = 0, reddedilenSahte = 0;

void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(Paket)) return;
  int rssi = info->rx_ctrl->rssi;
  Paket p; memcpy(&p, data, sizeof(p));

  Veri v;
  mbedtls_gcm_context gcm; mbedtls_gcm_init(&gcm);
  mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, KEY, 128);
  int r = mbedtls_gcm_auth_decrypt(&gcm, sizeof(Veri), p.nonce, 12, NULL, 0, p.tag, 16, p.sifreli, (uint8_t *)&v);
  mbedtls_gcm_free(&gcm);

  if (r != 0) {
    reddedilenSahte++;
    Serial.println("SAHTE/BOZUK paket reddedildi");
    return;
  }
    if (!ilkPaket && v.sayac <= sonSayac) {
    reddedilenTekrar++;
    Serial.printf("TEKRAR paket reddedildi (sayac %u)\n", v.sayac);
    return;
  }

  if (!ilkPaket) {
    uint32_t aralik = v.sayac - sonSayac;
    if (aralik > 1 && aralik <= 50) kayip += aralik - 1;
  }
  ilkPaket = false;
  sonSayac = v.sayac;
  alinan++;

  float kayipYuzde = 100.0 * kayip / (alinan + kayip);
  Serial.printf("[%lus] #%u  T=%.1fC  N=%.0f%%  RSSI=%d dBm  alinan=%u  kayip=%u (%%%.1f)  tekrar_red=%u\n",
                millis() / 1000, v.sayac, v.temp, v.hum, rssi, alinan, kayip, kayipYuzde, reddedilenTekrar);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) { Serial.println("ESP-NOW baslatilamadi!"); return; }
  esp_now_register_recv_cb(onReceive);
  Serial.println("Alici hazir.");
}

void loop() {}