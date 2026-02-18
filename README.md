
# StarLog: Çok Fonksiyonlu, Amatör,Taşınabilir Pentest Cihazı (v3.0)

StarLog; Flipper Zero tarzı cihazların çalışma mantığını anlamak, en düşük maliyetle en ergonomik ve kompakt toolbox'ı oluşturmak için geliştirdiğim bir proje. Sadece bir kod yığını değil; IR sinyallerinden Wi-Fi paket enjeksiyonuna kadar pek çok farklı alanı kapsayan bir öğrenme süreci ve mühendislik denemesi.

## Öne Çıkan Özellikler

### 1. Wi-Fi Pentest & Operasyon
v3.0 ile StarLog artık sadece ağları izlemekle kalmıyor, aktif bir saldırı cihazına dönüşüyor.
* **Deauther (Bağlantı Kesme):** Hedef ağın BSSID ve kanalına odaklanarak `esp_wifi_80211_tx` üzerinden ham paket enjeksiyonu yapar ve bağlantıyı koparır.
* **Beacon Spam:** Çevredeki Wi-Fi listelerini sahte SSID'lerle (MİT Dinleme Aracı, Bobrek Mafyasi, FBI Surveillance Van vb.) doldurarak "troll" ağlar yaratır.
* **Gizli Ağ Tarama:** SSID'si gizlenmiş ağlar dahil çevredeki tüm 2.4GHz yayınlarını listeler.



### 2. IR (Kızılötesi) Ekosistemi
Projenin temel taşı olan IR kısmında, karmaşık protokolleri basit bir klonlayıcıya dönüştürdüm.
* **Sinyal Klonlama:** Herhangi bir kumandadan gelen sinyali yakalayıp EEPROM üzerine kalıcı olarak kaydeder.
* **Hazır Veritabanı:** Samsung, LG, Sony, Vestel gibi 23+ marka ve Epson projeksiyon cihazları için önceden tanımlanmış kontrol kodlarını içerir.
* **Geniş Protokol Desteği:** NEC, RC5, RC6 ve Sony protokollerini kapsayan sinyal işleme yapısı.

### 3. Donanım Kontrolü & Toolbox
* **Servo Kontrolü:** 0-180 derece arası 10'ar derecelik adımlarla hassas servo yönetimi.
* **Güç Çıkışı (GPIO21):** Harici bileşenler (LED, buzzer, lazer vb.) için doğrudan 3.3V kontrolü.
* **Sensör İzleme:** DHT11 ile anlık sıcaklık ve nem takibi. Status bar sayesinde her menüde veriler güncel kalır.
* **Mini Oyunlar:** Kodun içindeki dairesel menü ve offset mantığını test etmek için yazdığım Snake, Pong ve Sayı Tahmin oyunları.

## Teknik Altyapı
StarLog, ESP32 mimarisi ve ST7735 TFT ekran üzerine inşa edildi. v3.0 ile arayüzü daha minimalist ve "carousel" tipi (yatay kayan) bir yapıya çektim.

* **Modüler Kod:** Yeni fonksiyon eklemeyi kolaylaştıran state-machine yapısı.
* **Kalıcı Hafıza:** Renk ayarları ve kaydedilen IR sinyalleri cihaz kapansa dahi silinmez.
* **Düşük Maliyet:** Piyasada satılan pahalı pentest araçlarının sunduğu temel mantığı, standart sensörlerle sunar.

## Kullanılan Donanımlar
* **MCU:** ESP32 DevKit V1
* **Ekran:** ST7735 1.8" TFT
* **Sensörler:** IR Receiver (1838), IR Sender LED, DHT11
* **Kontrol:** 5 Adet Tact Buton (Internal Pull-up)

---

## Gelecek Planları
* **StarLog Internet:** İki cihaz arası bulut tabanlı veri iletimi ve akıllı ev entegrasyonu.
* **Sub-GHz Entegrasyonu:** RF modülleri ile kapı/bariyer kumandalarına yönelik çalışmalar.
* **Gelişmiş Arayüz:** Daha fazla tema seçeneği ve grafiksel veri gösterimi.

---
**Muhammet Ayberk ARSLAN** | *2025*
<br>
<div style="display: flex; flex-wrap: wrap; gap: 10px;">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/1.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/2.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/3.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/4.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/5.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/6.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/7.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/8.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/9.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/10.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/11.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/12.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/13.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/14.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/15.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/16.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/17.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/18.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/19.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/20.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/21.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/22.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/23.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/24.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/25.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/26.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/27.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/28.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/29.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/30.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/31.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/32.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/33.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/34.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/35.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/36.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/37.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/38.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/39.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/40.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/41.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/42.jpg" width="150" alt="text">
<img src="https://raw.githubusercontent.com/ayberkarslan/starlog/refs/heads/main/images/43.jpg" width="150" alt="text">
</div>
