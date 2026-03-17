# LCD_Controller - STM32 LCD Control System

## 📋 Proje Özeti

LCD_Controller, STM32H747 dual-core mikrodenetleyici üzerinde çalışan, denizcilik uygulamaları için geliştirilmiş bir LCD kontrol ve NMEA veri işleme sistemidir. Proje, ILI9488 TFT LCD ekran ve ILITEK dokunmatik ekran sürücülerini sıfırdan geliştirilmiş, LVGL 8.3 grafik kütüphanesi ile modern bir kullanıcı arayüzü sunmaktadır.

## 🎯 Proje Özellikleri

### Donanım Özellikleri
- **Mikrodenetleyici**: STM32H747XI (Dual-core: CM4 + CM7)
- **LCD Ekran**: ILI9488 TFT (SPI4 üzerinden)
- **Dokunmatik Ekran**: ILITEK Touch Controller (I2C4 üzerinden)
- **EEPROM**: AT24MAC402 (I2C3 üzerinden)
- **QSPI Flash**: SST26VF032B
- **Ethernet**: LAN8742 PHY (LWIP stack ile)
- **UART**: NMEA veri alışverişi için (UART5)

### Yazılım Özellikleri
- **RTOS**: FreeRTOS (CMSIS-OS wrapper ile)
- **Grafik Kütüphanesi**: LVGL 8.3.11
- **UI Tasarım Aracı**: SquareLine Studio
- **Ağ Protokolü**: LWIP (Ethernet backend)
- **JSON Kütüphanesi**: cJSON

## 🏗️ Mimari Yapı

### FreeRTOS Task Yapısı

#### CM7 Core (Ana İşlemci)
1. **Display_TaskFunction** (Priority: osPriorityNormal)
   - LVGL timer handler'ı çalıştırır
   - Ekran güncellemelerini yönetir
   - Screen7'den gelen UART verilerini işler
   - LCD mutex ile korunur

2. **Touch_TaskFunction** (Priority: osPriorityNormal)
   - Dokunmatik ekran verilerini okur
   - LVGL touch input interface'ine bağlanır
   - Touch mutex ile korunur

3. **UART_Receive_TaskFunction** (Priority: osPriorityNormal)
   - UART5'ten gelen NMEA verilerini okur
   - Verileri Screen7QueueHandle kuyruğuna gönderir
   - Screen7'de gösterim için hazırlar

4. **EEPROM_TestTask** (Priority: osPriorityLow)
   - AT24MAC402 EEPROM test işlemlerini yürütür
   - Non-volatile veri saklama işlemleri

5. **LogTaskHandler** (Priority: osPriorityLow)
   - Sistem log mesajlarını yönetir
   - UART üzerinden debug çıktısı

### Mutex ve Queue Yapısı
- **lcdMutexHandle**: LCD ekran erişimi için mutex
- **touchMutexHandle**: Dokunmatik ekran erişimi için mutex
- **Screen7QueueHandle**: UART verilerini Screen7'ye aktarmak için queue

## 📁 Proje Yapısı

```
LCD_Controller/
├── CM7/                          # CM7 Core (Ana İşlemci)
│   ├── Core/
│   │   ├── Inc/
│   │   │   ├── DISPLAY_INC/      # LCD ve Touch header dosyaları
│   │   │   │   ├── ili9488.h
│   │   │   │   └── ilitek_touch.h
│   │   │   ├── EEPROM_INC/       # EEPROM driver
│   │   │   ├── ExtFLASH_INC/      # QSPI Flash driver
│   │   │   ├── ETH_INC/          # Ethernet backend
│   │   │   ├── freertos_custom.h # FreeRTOS özel yapılandırma
│   │   │   ├── tasktest.h        # Task fonksiyon prototipleri
│   │   │   └── ui_custom.h       # UI özel fonksiyonlar
│   │   └── Src/
│   │       ├── DISPLAY_SRC/
│   │       │   ├── ili9488.c     # ILI9488 LCD driver (sıfırdan geliştirilmiş)
│   │       │   └── ilitek_touch.c # ILITEK touch driver (sıfırdan geliştirilmiş)
│   │       ├── EEPROM_SRC/
│   │       ├── ExtFLASH_SRC/
│   │       ├── ETH_SRC/
│   │       ├── freertos_custom.c  # FreeRTOS task oluşturma
│   │       ├── tasktest.c        # Tüm task implementasyonları
│   │       └── ui_custom.c       # UI özel fonksiyonlar ve event handler'lar
│   └── LVGL_Library/
│       ├── lvgl/                 # LVGL 8.3.11 kütüphanesi
│       └── ui/                   # SquareLine Studio'dan üretilen UI
│           └── screens/
│               ├── ui_Screen7.c  # UART receive ekranı
│               └── ui_Screen8.c  # UART transmit/ayarlar ekranı
├── CM4/                          # CM4 Core (İkincil işlemci)
└── Drivers/                      # STM32 HAL ve BSP sürücüleri
```

## 🔧 Teknolojiler ve Kullanım Alanları

### 1. LVGL (Light and Versatile Graphics Library) 8.3.11
**Neden Kullanıldı:**
- Embedded sistemler için optimize edilmiş, hafif grafik kütüphanesi
- Modern UI bileşenleri (butonlar, textarea, label, vb.)
- Düşük bellek kullanımı ve yüksek performans
- SquareLine Studio ile görsel tasarım desteği

**Nerede Kullanıldı:**
- Tüm kullanıcı arayüzü ekranları
- Screen7: UART'tan gelen NMEA verilerini gösterim
- Screen8: NMEA veri gönderme ve ayarlar ekranı
- `CM7/LVGL_Library/lvgl/` ve `CM7/LVGL_Library/ui/` dizinlerinde

### 2. FreeRTOS (CMSIS-OS Wrapper)
**Neden Kullanıldı:**
- Gerçek zamanlı görev yönetimi
- Task, mutex ve queue senkronizasyonu
- Çoklu görev ortamında kaynak paylaşımı
- Öncelik tabanlı görev zamanlama

**Nerede Kullanıldı:**
- `freertos_custom.c`: Task oluşturma ve mutex/queue yönetimi
- `tasktest.c`: Tüm task fonksiyonları
- Display, Touch, UART, EEPROM ve Log task'ları
- Timer'lar: UART otomatik gönderim için (Screen8)

### 3. SPI İletişimi (SPI4)
**Neden Kullanıldı:**
- ILI9488 LCD controller ile yüksek hızlı veri aktarımı
- Frame buffer verilerinin hızlı aktarımı
- DMA desteği ile CPU yükünü azaltma

**Nerede Kullanıldı:**
- `ili9488.c`: LCD komut ve veri gönderimi
- `spi_write()`: Düşük seviye SPI yazma fonksiyonu
- `lcd_write_command()`: LCD komut gönderimi
- `tft_flush()`: LVGL frame buffer'ını LCD'ye aktarma

### 4. I2C İletişimi (I2C4, I2C3)
**Neden Kullanıldı:**
- ILITEK touch controller ile haberleşme (I2C4)
- AT24MAC402 EEPROM ile haberleşme (I2C3)
- Düşük pin sayısı ile çoklu cihaz kontrolü

**Nerede Kullanıldı:**
- `ilitek_touch.c`: Touch verilerini okuma
- `eeprom_at24mac402.c`: EEPROM okuma/yazma işlemleri
- Touch koordinatlarını LVGL'e aktarma

### 5. UART İletişimi (UART5)
**Neden Kullanıldı:**
- NMEA 0183 protokolü ile denizcilik verileri alışverişi
- Debug ve log mesajları için seri çıktı
- Düşük maliyetli seri iletişim

**Nerede Kullanıldı:**
- `tasktest.c`: `UART_Receive_TaskFunction` - NMEA veri alma
- `ui_custom.c`: `uart_transmit_timer_callback` - NMEA veri gönderme
- Screen7: Gelen verileri textarea'da gösterme
- Screen8: Kullanıcı tarafından girilen NMEA cümlelerini gönderme

### 6. SquareLine Studio
**Neden Kullanıldı:**
- Görsel UI tasarımı
- LVGL kod üretimi
- Hızlı prototipleme

**Nerede Kullanıldı:**
- `CM7/LVGL_Library/ui/`: Tüm UI ekranları
- `ui_Screen7.c/h`: UART receive ekranı
- `ui_Screen8.c/h`: UART transmit ekranı
- `ui_custom.c`: SquareLine'dan üretilen kodlara özel fonksiyonlar eklendi

### 7. Custom UI Extensions (`ui_custom.c`)
**Neden Kullanıldı:**
- SquareLine Studio'dan üretilen kodlara özel mantık ekleme
- Event handler'lar (buton tıklamaları, timer'lar)
- UART veri gönderme/alma işlemleri
- FreeRTOS timer entegrasyonu

**Nerede Kullanıldı:**
- `ui_custom_Screen7_init()`: Screen7 başlangıç ayarları
- `ui_custom_Screen8_init()`: Screen8 başlangıç ayarları
- `start_transmit_event()`: UART gönderim başlatma
- `stop_transmit_event()`: UART gönderim durdurma
- `uart_transmit_timer_callback()`: Periyodik NMEA gönderimi

### 8. EEPROM (AT24MAC402)
**Neden Kullanıldı:**
- Non-volatile veri saklama
- Sistem ayarlarını kalıcı olarak saklama
- MAC adresi ve cihaz bilgileri

**Nerede Kullanıldı:**
- `EEPROM_SRC/eeprom_at24mac402.c`
- `tasktest.c`: `EEPROM_TestTask` - Test ve doğrulama

### 9. QSPI Flash (SST26VF032B)
**Neden Kullanıldı:**
- Büyük veri saklama (32 Mbit)
- Firmware güncellemeleri
- Log verileri saklama

**Nerede Kullanıldı:**
- `ExtFLASH_SRC/sst26vf032b.c`
- Bootloader entegrasyonu

### 10. Ethernet (LWIP)
**Neden Kullanıldı:**
- Web tabanlı kontrol ve izleme
- CGI backend ile dinamik içerik
- Uzaktan erişim imkanı

**Nerede Kullanıldı:**
- `ETH_SRC/ethernet_backend.c`
- LWIP stack konfigürasyonu
- HTTP server ve CGI handler'lar

## 🚀 Kullanım

### Screen7 - UART Receive Ekranı
- UART5'ten gelen NMEA verilerini gerçek zamanlı gösterir
- Veriler `Screen7QueueHandle` kuyruğu üzerinden aktarılır
- `ui_tareceive` textarea bileşeninde gösterilir

### Screen8 - UART Transmit/Ayarlar Ekranı
- **NMEA Sentence Textbox** (`ui_tanmea`): Gönderilecek NMEA cümlesi
- **Transmit Interval Textbox** (`ui_tainterval`): Gönderim aralığı (saniye cinsinden)
- **Start Button**: Otomatik gönderimi başlatır
  - FreeRTOS auto-reload timer oluşturur
  - Belirtilen aralıkta NMEA cümlesini gönderir
- **Stop Button**: Otomatik gönderimi durdurur

### Örnek Kullanım Senaryosu
1. Screen8'e geçiş yapın
2. NMEA cümlesini `ui_tanmea` textbox'ına girin (örn: `$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47`)
3. Gönderim aralığını `ui_tainterval` textbox'ına girin (örn: `5` saniye)
4. "Start" butonuna tıklayın
5. Sistem her 5 saniyede bir NMEA cümlesini UART5 üzerinden gönderecektir
6. "Stop" butonuna tıklayarak gönderimi durdurun

## 🔍 Önemli Dosyalar

### Display Driver (`ili9488.c`)
- **`spi_write()`**: SPI4 üzerinden tek byte veri gönderimi
- **`lcd_write_command()`**: LCD komut gönderimi (CS, DC pin kontrolü ile)
- **`lcd_init_command_list()`**: ILI9488 başlangıç komut dizisi
- **`lcd_init()`**: LCD başlatma fonksiyonu
- **`Display_init()`**: LVGL display driver kaydı
- **`tft_flush()`**: LVGL frame buffer'ını LCD'ye aktarma (DMA destekli)

### Touch Driver (`ilitek_touch.c`)
- **`Touch_LVGL_Init()`**: LVGL touch input interface kaydı
- I2C4 üzerinden touch koordinatlarını okuma
- LVGL event sistemine entegrasyon

### Task Management (`tasktest.c`)
- **`Display_TaskFunction()`**: Ana display task'ı
  - LVGL timer handler çağrısı
  - Screen7 veri işleme
  - LCD mutex koruması
- **`Touch_TaskFunction()`**: Touch input task'ı
- **`UART_Receive_TaskFunction()`**: UART veri alma task'ı
- **`Screen7_ProcessInboundData()`**: Queue'dan veri okuma ve UI güncelleme

### FreeRTOS Configuration (`freertos_custom.c`)
- **`MainProcessFreertosInit()`**: Tüm task'ları, mutex'leri ve queue'ları oluşturur
- Stack size ve priority ayarları
- Error handling

## 🛠️ Geliştirme Notları

### Custom Driver Geliştirme
Proje, ILI9488 ve ILITEK sürücülerini sıfırdan geliştirmiştir:
- SPI protokolü manuel olarak implement edilmiştir
- LCD register komutları datasheet'e göre yazılmıştır
- Touch controller I2C protokolü implement edilmiştir
- LVGL display ve input driver interface'leri entegre edilmiştir

### UI Customization
SquareLine Studio'dan üretilen kodlara özel fonksiyonlar `ui_custom.c` dosyasında:
- Event handler'lar
- Timer callback'leri
- UART entegrasyonu
- Queue-based veri aktarımı

### FreeRTOS Best Practices
- Mutex kullanımı: LCD ve touch erişimi korunur
- Queue kullanımı: Inter-task iletişim için
- Timer kullanımı: Periyodik görevler için (UART transmit)
- Task priority: Kritik görevler yüksek öncelikli

## 📊 Sistem Mimarisi

```
┌─────────────────────────────────────────────────────────┐
│                    STM32H747 (CM7)                      │
│                                                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │   Display    │  │    Touch     │  │    UART      │ │
│  │    Task      │  │    Task      │  │ Receive Task │ │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘ │
│         │                  │                 │         │
│         └──────────────────┼─────────────────┘         │
│                            │                           │
│                    ┌───────▼────────┐                  │
│                    │  LVGL Engine   │                  │
│                    └───────┬────────┘                  │
│                            │                           │
│  ┌─────────────────────────┼─────────────────────────┐ │
│  │         UI Screens (SquareLine Studio)           │ │
│  │  Screen7: UART Receive  │  Screen8: UART Transmit│ │
│  └─────────────────────────┼─────────────────────────┘ │
│                            │                           │
│  ┌─────────────────────────┼─────────────────────────┐ │
│  │              Custom UI Extensions                 │ │
│  │         (ui_custom.c - Event Handlers)            │ │
│  └───────────────────────────────────────────────────┘ │
│                                                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │   EEPROM     │  │   QSPI Flash  │  │   Ethernet   │ │
│  │    Task      │  │               │  │   (LWIP)     │ │
│  └──────────────┘  └───────────────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────┘
         │                  │                 │
         ▼                  ▼                 ▼
    ┌────────┐        ┌────────┐        ┌────────┐
    │ AT24MAC │        │ SST26  │        │ LAN8742│
    │  402    │        │ VF032B │        │  PHY   │
    └────────┘        └────────┘        └────────┘
```

## 🔌 Pin Bağlantıları

### ILI9488 LCD (SPI4)
- **SPI4_SCK**: SPI Clock
- **SPI4_MOSI**: SPI Data
- **LCD_CS**: Chip Select
- **LCD_DC**: Data/Command
- **LCD_RST**: Reset

### ILITEK Touch (I2C4)
- **I2C4_SCL**: I2C Clock
- **I2C4_SDA**: I2C Data
- **Touch_IRQ**: Interrupt (opsiyonel)

### UART5 (NMEA)
- **UART5_TX**: Transmit
- **UART5_RX**: Receive

## 📝 Lisans

Bu proje özel bir geliştirme projesidir. Tüm kod sıfırdan geliştirilmiştir.

## 👨‍💻 Geliştirici Notları

- **ILI9488 Driver**: Datasheet'e göre sıfırdan implement edilmiştir
- **ILITEK Touch Driver**: I2C protokolü ve touch koordinat hesaplama sıfırdan yazılmıştır
- **LVGL Integration**: Display ve input driver interface'leri custom olarak entegre edilmiştir
- **FreeRTOS Architecture**: Task, mutex ve queue yapısı optimize edilmiştir
- **UI Design**: SquareLine Studio ile tasarlanmış, custom event handler'lar eklenmiştir

## 🎓 Öğrenilen Teknolojiler

Bu proje aşağıdaki teknolojilerin derinlemesine anlaşılmasını sağlamıştır:
- Embedded C programlama
- STM32 HAL kütüphanesi
- SPI ve I2C protokolleri
- FreeRTOS gerçek zamanlı işletim sistemi
- LVGL grafik kütüphanesi
- Touch screen driver geliştirme
- LCD controller register yönetimi
- Queue ve mutex senkronizasyonu
- Timer-based periyodik görevler
- UART seri iletişim protokolü
- NMEA 0183 denizcilik protokolü

---

**Proje Durumu**: ✅ Aktif Geliştirme
**Son Güncelleme**: 2025

