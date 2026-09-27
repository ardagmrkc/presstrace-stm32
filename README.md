# Yük Altında Buton Yanıt Süresi Analizi (STM32F407VG + FreeRTOS)

Bu proje, STM32F407VG Discovery kartı üzerinde FreeRTOS ile çalışan 3 görevli bir
sistemde, artan telemetri yükü ve ek CPU işi altında **buton-tepki gecikmesinin**
(`R = t4 - t0`) nasıl değiştiğini ölçer, kaydeder ve analiz eder.

Deney bütçesi: **R ≤ 20 ms** (yumuşak gerçek-zaman hedefi).

## 1. Kart, bağlantılar ve araç sürümleri

| Bileşen | Detay |
|---|---|
| Kart | STM32F407VG Discovery (MCU: STM32F407VGT6, Cortex-M4F, 168 MHz) |
| Buton | Kart üzerindeki kullanıcı butonu **B1 (mavi)** — PA0, aktif-HIGH, EXTI0 |
| LED (opsiyonel gözlem) | PD12 (yeşil) — ButtonTask'in olayı işlediğini görsel olarak gösterir |
| UART | USART2 — **PA2 = TX**, **PA3 = RX** (kart üzerinde USB-UART köprüsü yok; harici USB-TTL adaptör gerekir, bkz. [docs/setup.md](docs/setup.md)) |
| Zaman damgası kaynağı | TIM2 (32-bit, 1 MHz'e böl → 1 µs çözünürlük, serbest sayaç) |
| Telemetri verisi | Dahili sıcaklık sensörü (ADC1 kanal 16), fabrika kalibrasyonu + VREFINT ile VDDA düzeltmesi (bkz. [docs/code-notes.md](docs/code-notes.md#telemetri-verisi-kart-sıcaklığı)) |
| IDE / Toolchain | IAR Embedded Workbench for ARM **9.70.4** (komut satırı derleyicisi `iarbuild` ile de doğrulandı) |
| RTOS | [FreeRTOS Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel), **V11.1.0+** (`main` dalı, commit `8be86d4`, 2026-08-26), port `portable/IAR/ARM_CM4F`, `heap_4` |
| CMSIS | ARM CMSIS-Core 5.3 (Cortex-M4) + ST CMSIS STM32F4 Device paketi 2.6.11 (bkz. [docs/code-notes.md](docs/code-notes.md#üçüncü-parti-bileşenler)) |
| Arayüz | Tarayıcı tabanlı, Web Serial API (`interface/`); kurulum gerektirmez, Chrome/Edge |
| Analiz | Python 3.11, pandas, matplotlib (`analysis/analyze.py`) |

Tam pin haritası, güç/toprak bağlantıları ve USB-TTL kablolama şeması için
[docs/setup.md](docs/setup.md) dosyasına bakın.

## 2. Derleme, yükleme ve arayüzü başlatma

Kısa özet — ayrıntılı adımlar [docs/setup.md](docs/setup.md) içinde:

1. Alt modülleri çek: `git submodule update --init --recursive`
2. IAR EWARM'da `firmware/EWARM/odev_1.eww` çalışma alanını açın. Kaynak dosyalar,
   include yolları, tanımlar, linker dosyası ve ST-LINK önceden ayarlıdır.
3. **Options → General Options → Target → Device** altında **ST STM32F407VG** seçin
   (tek elle yapılan ayar; bkz. [docs/setup.md](docs/setup.md#4-iar-projesi-oluşturma)).
4. Derleyin (F7), ST-LINK üzerinden karta yükleyin (Ctrl+D).
5. Harici USB-TTL adaptörü PA2/PA3/GND'ye bağlayın.
6. `interface/index.html` dosyasını Chrome veya Edge ile açın, seri portu
   seçip **Bağlan**'a basın.

## 3. Senaryo seçimi ve ölçüm adımları

Senaryolar `S0`…`S5` olarak tanımlıdır ve telemetri hızı + ek CPU yükü
kombinasyonlarını temsil eder:

| ID | Telemetri | Ek hesaplama/aktivasyon | Hedef |
|---|---|---|---|
| S0 | Kapalı | Yok | Referans (yüksüz) yanıt süresi |
| S1 | 10 Hz (100 ms) | Yok | Düşük telemetri sıklığı |
| S2 | 50 Hz (20 ms) | Yok | Orta telemetri sıklığı |
| S3 | 100 Hz (10 ms) | Yok | Yüksek telemetri sıklığı |
| S4 | 100 Hz (10 ms) | ~2 ms CPU işi / periyot | Ek CPU yükü |
| S5 | 100 Hz (10 ms) | ~5 ms CPU işi / periyot | Daha yüksek CPU yükü |

Senaryo, yeniden derlemeden **arayüzün Senaryo panelinden** seçilir. Kart komutu
periyot sınırında uygular ve onaylar (bkz. [docs/code-notes.md](docs/code-notes.md#senaryo-komutu)).
`main.h` içindeki `ACTIVE_SCENARIO` yalnızca açılış senaryosudur. Bunun için
USB-TTL adaptörün **TX** ucu kartın **PA3** pinine bağlı olmalıdır. Her senaryo için:

1. **Senaryoyu seçin.** Düğme, kart onayladığında vurgulanır. Liste, kartın kayıt havuzu ve
   tüm pencere sayaçları sıfırlanır; yeni ölçüm penceresi başlar.
2. **Ayarları doğrulayın ve 5 saniye ısınma bekleyin.** Telemetri panelinde gerçek periyot
   (10/20/100 ms) ve `extra_load_us` hedefle uyumlu olmalı (S4 ≈ 2000 µs, S5 ≈ 5000 µs).
3. **En az 30 basış yapın.** Aralarında en az 0,5 s olsun ve basış zamanlarını değiştirin. Kart
   havuzu 64 olay tutar. Her basış canlı görünür, ama t₃/t₄ ve durum hattı meşgul etmemek için
   kartta kalır.
4. **"Ölçümü bitir ve kayıtları al"**'a basın. Kart telemetriyi durdurur (S0), TX'i
   tamamlar ve kayıtları (`REC`), pencere sayaçlarını (`CNT`) ve `END`'i gönderir. Özet
   satırında kayıt bütünlüğünü kontrol edin.
5. **"CSV Kaydet"**'e basın. İki dosya iner, ikisini de `measurements/` altına koyun:
   `S<n>.csv` (olaylar) ve `S<n>_counters.csv` (pencere sayaçları).
6. Altı senaryo bitince `analysis/` içinde `python analyze.py` çalıştırın. `summary.csv` ve iki
   grafik aynı ham veriden üretilir (bkz. [analysis/report.md](analysis/report.md)).

## 4. Timer ve FreeRTOS ayarları

| Ayar | Değer |
|---|---|
| MCU saati | HSE 8 MHz → PLL → SYSCLK **168 MHz**; AHB 168, APB1 42, APB2 84 MHz |
| RTOS tick | `configTICK_RATE_HZ` = **1000** (1 ms), preemptive, `configMAX_PRIORITIES` = 5 |
| Zaman damgası | TIM2, 32-bit serbest sayaç, 84 MHz / 84 = **1 MHz** (1 µs); ~71,6 dakikada sarar, farklar mod 2³² |
| Görev öncelikleri | `TelemetryTask (3) > ButtonTask (2) > UartTxTask (1)`; Idle = 0 |
| Kuyruklar | Buton: **8** olay (statik). TX: **16** mesaj, FIFO; UART'a yalnızca `UartTxTask` dokunur |
| UART | USART2, **115200 8N1**, IT ile gönderim (TXE/TC); görev TC'ye kadar bloklanır (en fazla 50 ms) |
| Mesaj biçimi | ASCII, boşlukla 63 bayt + LF = **64 bayt** (bkz. [docs/code-notes.md](docs/code-notes.md#paket-protokolü)) |
| Kesme öncelikleri | EXTI0 ve USART2 = 5 (`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`), gruplama 4 bit preemption |
| Buton filtresi | 30 ms tekrar-kenar penceresi |
| Kayıt | 64 olaylık havuz + 32 olaylık düşen olay kaydı, taşma sayaçlarıyla |
| Deadline / zaman aşımı | `R = t4 − t0 ≤ 20 ms`; deney zaman aşımı 1 s |
| Derleme | IAR **Debug** yapılandırması (optimizasyon *Low*); S4/S5 iş yükü bu ayarla kalibre edildi (`workload.h`) |

## 4.1 Şartnameden sapmalar ve eklemeler

| Konu | Şartname | Bu projede | Neden |
|---|---|---|---|
| Kart | STM32L476RG ya da eşdeğeri | STM32F407VG Discovery | Şartname başka kartlara izin veriyor (UART, buton, µs zaman kaynağı var) |
| Buton kenarı | Yalnızca basış kenarı, ilk kabulden itibaren 30 ms | İki kenar dinlenir; 30 ms sessizlik + basılı/bırakılmış durumu | İlk S0 ölçümünde bırakma sıçraması ikinci olay üretti: ardışık olaylar arası 60–280 ms, yani basılı tutma süresi. Yalnızca basış kenarıyla 30 ms bunu çözmez (ayrıntı: [code-notes.md](docs/code-notes.md#buton-isrsi-kısa-isr-kopyalanan-olay-kontrol-edilen-sonuç)) |
| Senaryo seçimi | Derleme ayarıyla olabilir; ek komut zorunlu değil | Arayüzden, 5 baytlık komutla | Yeniden derlemeden senaryo değiştirmek için. Komut yalnızca kullanıcı seçince gider, ölçüm sırasında hatta komut trafiği yok |
| TX zaman aşımı | Deney zaman aşımı 1 s | TC bekleme süresi 50 ms, R ≥ 1 s ise `timeout` | 64 bayt 5,56 ms sürer; kaçan TC görevi 1 s kilitlemesin |
| Sayaç dosyası | `S0.csv … S5.csv`, `summary.csv` | Ayrıca `S<n>_counters.csv` | Pencere sayaçları (kuyruk tepe, gerçek telemetri hızı, taşmalar) olay satırı değildir; olay CSV'si şartname biçiminde kalır |

## 4.2 Bilinen sınırlamalar

- t₀ fiziksel basma anı değil, filtrenin kabul ettiği kenarın ISR girişindeki zamanıdır.
- t₃ ilk fiziksel bit değildir. t₄'e TC kesmesine giriş gecikmesi dahildir.
- Gözlenen maksimum, kanıtlanmış en kötü durum (worst-case) değildir.
- Logic analyzer / osiloskop ile bağımsız doğrulama yapılmadı.

## 5. Ham veri, grafik ve rapor bağlantıları

- Ham CSV ölçümleri: [measurements/](measurements/) (`S0.csv`…`S5.csv`, `S<n>_counters.csv`, `summary.csv`)
- Analiz betiği: [analysis/analyze.py](analysis/analyze.py)
- Analiz raporu: [analysis/report.md](analysis/report.md)
- Grafikler: [analysis/plots/](analysis/plots/): `r_vs_event.png` (olay → R, 20 ms çizgisi),
  `stages_by_scenario.png` (senaryo → aşama süreleri, yığılmış)
- Kurulum detayları: [docs/setup.md](docs/setup.md)
- Kod notları / tasarım kararları: [docs/code-notes.md](docs/code-notes.md)
- AI kullanım notu: [docs/ai-usage.md](docs/ai-usage.md)

## Dizin yapısı

```
freertos-bootcamp/
└── hafta-01/
    ├── README.md
    ├── firmware/          # IAR EWARM projesi, uygulama + sürücü kaynak kodu
    ├── interface/         # Web Serial tabanlı izleme/kayıt arayüzü
    ├── measurements/      # Ham CSV ölçümleri (S0..S5) ve özet
    ├── analysis/          # Analiz betiği, rapor ve grafikler
    └── docs/              # Kurulum, kod notları, AI kullanım notu
```

> **Durum:** Firmware IAR 9.70.4 ile uyarısız derleniyor. Mesaj kodlayıcısı bilgisayarda en kötü
> durum değerleriyle, arayüz de firmware'in ürettiği satırlarla test edildi. `measurements/`
> içindeki CSV'ler şu an yalnızca başlık satırı içeriyor; gerçek ölçümler kart üzerinde alınacak.
