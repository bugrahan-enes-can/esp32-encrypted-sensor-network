#include "DHT.h"
#include "mbedtls/gcm.h"
#include <Wire.h>                // I2C haberleşmesi için
#include <Adafruit_GFX.h>        // Ekrana çizim/yazı temel kütüphanesi
#include <Adafruit_SSD1306.h>    // Bizim OLED'in sürücüsü

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// 128x64 piksel ekran, I2C üzerinden, reset pini yok (-1)
Adafruit_SSD1306 display(128, 64, &Wire, -1);

const uint8_t KEY[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                         0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};

void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin(21, 22);  // SDA=21, SCL=22: I2C hattını başlat
  // 0x3C: bu OLED'lerin standart I2C adresi (ESP32 ekranı bu adresle çağırıyor)
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED bulunamadi");
    while (true);  // Ekran yoksa dur, boşuna çalışmasın
  }
  display.clearDisplay();
  display.display();
}

void loop() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) { Serial.println("Okuma hatasi"); delay(2000); return; }

  // --- Gönderici tarafı: metne çevir ve şifrele ---
  char msg[32];
  snprintf(msg, sizeof(msg), "T=%.1f;H=%.1f", t, h);
  size_t len = strlen(msg);

  uint8_t iv[12];
  esp_fill_random(iv, sizeof(iv));
  uint8_t cipher[32], tag[16];
  mbedtls_gcm_context ctx;
  mbedtls_gcm_init(&ctx);
  mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, KEY, 128);
  mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT, len, iv, sizeof(iv),
                            NULL, 0, (const uint8_t*)msg, cipher, sizeof(tag), tag);

  // --- Alıcı tarafı: çöz, doğrula ---
  uint8_t plain[32];
  int rc = mbedtls_gcm_auth_decrypt(&ctx, len, iv, sizeof(iv), NULL, 0,
                                    tag, sizeof(tag), cipher, plain);
  plain[len] = 0;
  mbedtls_gcm_free(&ctx);

  // --- Çözülen metinden sayıları geri çıkar ---
  float rt = 0, rh = 0;
  sscanf((char*)plain, "T=%f;H=%f", &rt, &rh);  // "T=24.0;H=40.0" içinden iki sayıyı al

  // --- OLED'e yaz ---
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(rc == 0 ? "Sifre cozuldu: OK" : "HATA: etiket tutmadi");
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.printf("%.1f C", rt);
  display.setCursor(0, 44);
  display.printf("%.1f %%", rh);
  display.display();   // Çizdiklerimizi ekrana gerçekten gönder

  Serial.printf("Cozulen: %s (kod %d)\n", plain, rc);
  delay(2000);
}

