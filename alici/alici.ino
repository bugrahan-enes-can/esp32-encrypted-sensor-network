#include <WiFi.h>
#include <esp_now.h>
#include "mbedtls/gcm.h"                   // AES-GCM şifreleme kütüphanesi (ESP32'de hazır geliyor)

// Gizli anahtar: İKİ KARTTA AYNI OLMALI. Kendi anahtarınla değiştirebilirsin (16 bayt).
const uint8_t KEY[16] = {0};   // Gerçek anahtarı buraya koyma, repoya gitmesin

typedef struct __attribute__((packed)) { float temp; float hum; } Veri;   // Asıl veri (8 bayt)
typedef struct __attribute__((packed)) {
  uint8_t nonce[12];                       // Her paket için tek seferlik rastgele sayı (aynı veri bile her seferinde farklı şifrelensin)
  uint8_t sifreli[sizeof(Veri)];           // Şifrelenmiş veri
  uint8_t tag[16];                         // Doğrulama etiketi (paket değişirse tutmaz)
} Paket;

void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(Paket)) return;        // Boyut tutmuyorsa yok say
  Paket p; memcpy(&p, data, sizeof(p));

  Serial.print("Sifreli veri (hex): ");    // Havadan gelen ham halini göster, okunamaz olduğunu gör
  for (int i = 0; i < sizeof(Veri); i++) Serial.printf("%02X ", p.sifreli[i]);
  Serial.println();

  Veri v;
  mbedtls_gcm_context gcm; mbedtls_gcm_init(&gcm);
  mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, KEY, 128);   // Anahtarı yükle (128 bit)
  int r = mbedtls_gcm_auth_decrypt(&gcm, sizeof(Veri), p.nonce, 12, NULL, 0,
                                   p.tag, 16, p.sifreli, (uint8_t *)&v);  // Tag'i doğrula + çöz
  mbedtls_gcm_free(&gcm);

  if (r != 0) { Serial.println("Dogrulama BASARISIZ, paket reddedildi"); return; }
  Serial.printf("Cozuldu -> Sicaklik: %.1f C, Nem: %.0f %%\n", v.temp, v.hum);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) { Serial.println("ESP-NOW baslatilamadi!"); return; }
  esp_now_register_recv_cb(onReceive);
  Serial.println("Alici hazir, sifreli veri bekleniyor...");
}

void loop() {}