# Analiz Raporu — Yük Altında Buton Yanıt Süresi

- **Ölçüm tarihi:** 25 Eylül 2026, STM32F407VG Discovery, tek kart.
- **Ham veri:** [S0.csv](../measurements/S0.csv) … [S5.csv](../measurements/S5.csv), pencere sayaçları
  `S<n>_counters.csv`, özet tablo [summary.csv](../measurements/summary.csv).
- **Üretim:** Tablo ve grafiklerin hepsi aynı ham CSV'lerden [analyze.py](analyze.py) ile üretildi.
  Aşağıdaki ek sayılar (faz, dilim, basış aralığı) da aynı CSV'lerden hesaplandı.

## 1. Soru

Telemetri hızı ve CPU yükü arttıkça, buton olayının kart üzerinde gözlenen yanıt süresi
`R = t₄ − t₀` **ne kadar**, **hangi koşulda** ve **hangi aşamada** değişiyor? Deney bütçesi
`R ≤ 20 ms` (yumuşak gerçek-zaman: kaçırma bir hatadır, ölçülür ve raporlanır).

## 2. Yöntem

### 2.1 Sistem ve ayarlar

| Konu | Değer |
|---|---|
| MCU saati | HSE 8 MHz → PLL → SYSCLK 168 MHz; AHB 168, APB1 42, APB2 84 MHz |
| Zaman damgası | TIM2, 32 bit, serbest koşu. APB1 bölücüsü ≠ 1 olduğu için TIM2 saati 2 × 42 = 84 MHz; PSC = 83 → **1 MHz (1 sayım = 1 µs)** |
| FreeRTOS | V11.1.0+ (alt modül, commit `8be86d4`), tick **1 kHz**, heap_4 |
| Görevler | TelemetryTask **3** > ButtonTask **2** > UartTxTask **1** > Idle 0 |
| Kuyruklar | Buton kuyruğu 8 (statik), TX kuyruğu 16 (FIFO) |
| UART | USART2, 115200 8N1; her satır 63 ASCII + LF = 64 bayt → 64 × 10 / 115200 = **5,556 ms** |
| Kesmeler | EXTI0 ve USART2 NVIC önceliği 5 (FromISR sınırı); 4 bit preemption, alt öncelik yok |
| Derleme | IAR EWARM 9.70.4, **Debug** yapılandırması, optimizasyon **Low**, FPU VFPv4 single precision |

### 2.2 Ölçüm noktaları

Beş damga da aynı TIM2 sayacından alınır. Farklar işaretsiz 32-bit çıkarmayla (mod 2³²) hesaplanır.
Sayaç yaklaşık 71,6 dakikada bir başa sarar; bir olay bundan kısa sürdüğü sürece sonuç doğrudur
([code-notes](../docs/code-notes.md#zaman-hesapları)).

| Damga | An |
|---|---|
| t₀ | Filtrenin kabul ettiği basış kenarında EXTI0 ISR'sinin ilk işi |
| t₁ | ButtonTask'ta `xQueueReceive` döndüğü an |
| t₂ | TX kuyruğuna `xQueueSend`'den hemen önce |
| t₃ | UartTxTask'ta satır kodlandıktan sonra, `uart_start_tx`'ten hemen önce |
| t₄ | USART2 TC (transmission complete) kesmesinin ilk işi: son stop biti hattan çıkmıştır |

Aşamalar: `t₁ − t₀` görev bekleme, `t₂ − t₁` hazırlama, `t₃ − t₂` TX öncesi bekleme,
`t₄ − t₃` UART + TC. **R bu dört aşamanın toplamıdır.**

Bunlar duvar saati süreleridir. Örneğin `t₂ − t₁` içine kesme ya da preemption girebilir; saf CPU
süresi değildir.

### 2.3 Prosedür

- Senaryo, arayüzün Senaryo panelinden çalışma anında seçildi. Her seçim yeni bir ölçüm penceresi
  açar: kayıt havuzu ve pencere sayaçları sıfırlanır.
- Her senaryoda 5 s ısınmadan sonra **30 basış** yapıldı.
- Basışlar arası süre (ölçülen t₀ farkları):

  | | S0 | S1 | S2 | S3 | S4 | S5 |
  |---|---|---|---|---|---|---|
  | En kısa (s) | 1,51 | 1,59 | 1,50 | 1,56 | 1,48 | 0,53 |
  | Ortanca (s) | 1,66 | 1,83 | 1,67 | 1,65 | 1,81 | 1,81 |

  Şartnamedeki "en az 0,5 s" koşulu her senaryoda sağlandı.
- Ölçüm sırasında hatta yalnızca TEL ve BTN satırları vardı. t₃, t₄ ve olay durumu kartın RAM'indeki
  64 kayıtlık havuzda kapandı; sonuçlar hatta gönderilmedi (probe effect, bkz. 6.3).
- "Ölçümü bitir" komutuyla kart S0'a indi, TX kuyruğu boşaldı; ardından kayıtlar (REC), pencere
  sayaçları (CNT) ve END gönderildi. Arayüz bunları `S<n>.csv` ve `S<n>_counters.csv` olarak kaydetti.

### 2.4 İstatistik

- R ve aşama süreleri **yalnızca `status = ok`** olaylardan hesaplandı.
- `drop`, `tx_error` ve `timeout` olayları ayrı sayıldı. Bunlar "deadline karşılandı" sayılmaz.
- "> 20 ms" sütunu, tamamlanmış (`ok`) ama bütçeyi aşan olayların sayısıdır.

### 2.5 Kalibrasyon

- Ek CPU işi `workload_run()` döngüsüdür.
- İlk ölçüm: 140 000 iterasyon → 8373 µs (duvar saati), yani 0,0598 µs/iterasyon.
- Buradan hesaplanan: **33 440 iterasyon ≈ 2 ms** (S4), **83 600 iterasyon ≈ 5 ms** (S5).
  Kayıt: [workload.h](../firmware/Core/Inc/workload.h).
- Kalibrasyon ve tüm ölçümler aynı yapılandırmada (Debug / Low) yapıldı.
- Her TEL satırı gerçekleşen süreyi (`extra_load_us`) taşır ve arayüzde görünür, ancak CSV'ye
  kaydedilmez (bkz. 7).
- Dolaylı kontrol: en büyük `t₁ − t₀` S4'te 1,93 ms, S5'te 4,67 ms. İşin başında gelen bir basış işin
  neredeyse tamamını bekler; bu değerler ~2 ms ve ~5 ms'lik işle uyumludur.

## 3. Senaryo özeti

### 3.1 Olaylar ve R

| Senaryo | Telemetri (nominal / ölçülen) | Ek iş | Kabul | ok | drop | tx_error | timeout | R min / ort / p95 / maks (ms) | > 20 ms | TX kuyruğu tepe |
|---|---|---|---|---|---|---|---|---|---|---|
| S0 | kapalı / — | — | 30 | 30 | 0 | 0 | 0 | 5,603 / 5,608 / 5,613 / 5,613 | 0 | 1 |
| S1 | 10 / 10,00 Hz | — | 30 | 30 | 0 | 0 | 0 | 5,605 / 6,026 / 9,170 / 11,146 | 0 | 1 |
| S2 | 50 / 50,00 Hz | — | 30 | 30 | 0 | 0 | 0 | 5,605 / 6,672 / 10,303 / 11,190 | 0 | 1 |
| S3 | 100 / 100,01 Hz | — | 30 | 30 | 0 | 0 | 0 | 5,606 / 7,266 / 10,559 / 10,985 | 0 | 1 |
| S4 | 100 / 100,01 Hz | 2 ms | 30 | 30 | 0 | 0 | 0 | 5,605 / 8,216 / 12,719 / 13,128 | 0 | 2 |
| S5 | 100 / 100,01 Hz | 5 ms | 30 | 20 | **10** (hepsi TX kuyruğunda) | 0 | 0 | 29,449 / 113,654 / 164,538 / 165,317 | **20** | **16** |

Ölçülen telemetri hızı, pencere boyunca ölçülen ortalama üretim periyodundan hesaplandı (tablo 3.4).

### 3.2 Aşama ortalamaları (ms, yalnızca `ok`)

| Senaryo | t₁−t₀ görev bekleme | t₂−t₁ hazırlama | t₃−t₂ TX öncesi | t₄−t₃ UART + TC | Toplam = R ort |
|---|---|---|---|---|---|
| S0 | 0,007 | < 0,001 | 0,031 | 5,570 | 5,608 |
| S1 | 0,007 | < 0,001 | 0,449 | 5,570 | 6,026 |
| S2 | 0,007 | < 0,001 | 1,095 | 5,570 | 6,672 |
| S3 | 0,007 | < 0,001 | 1,688 | 5,570 | 7,266 |
| S4 | **0,296** | < 0,001 | **2,349** | 5,570 | 8,216 |
| S5 | **1,551** | < 0,001 | **106,533** | 5,570 | 113,654 |

- `t₂ − t₁` her senaryoda 0,4–0,9 µs.
- `t₄ − t₃` altı senaryonun hepsinde 5569,7–5570,4 µs.
- En büyük değerler:
  - `t₁ − t₀`: S0–S3'te 7–8 µs, S4'te 1,93 ms, S5'te 4,67 ms.
  - `t₃ − t₂`: S0'da 33 µs; S1–S4'te 5,41–5,63 ms, yani bir satır süresi; S5'te 159,7 ms.

### 3.3 Kayıp ve kayıt bütünlüğü

| Senaryo | btn_queue_drop | tx_queue_drop | tx_timeout | tx_start_fail | spurious_tc | pool / droplog taşması | REC alınan / beklenen | Sıra boşluğu | Hatalı satır | repeat (elenen kenar) |
|---|---|---|---|---|---|---|---|---|---|---|
| S0 | 0 | 0 | 0 | 0 | 0 | 0 / 0 | 30 / 30 | 0 | 0 | 21 |
| S1 | 0 | 0 | 0 | 0 | 0 | 0 / 0 | 30 / 30 | 0 | 0 | 35 |
| S2 | 0 | 0 | 0 | 0 | 0 | 0 / 0 | 30 / 30 | 0 | 0 | 74 |
| S3 | 0 | 0 | 0 | 0 | 0 | 0 / 0 | 30 / 30 | 0 | 0 | 135 |
| S4 | 0 | 0 | 0 | 0 | 0 | 0 / 0 | 30 / 30 | 0 | 0 | 50 |
| S5 | 0 | **16** (10 BTN + 6 TEL) | 0 | 0 | 0 | 0 / 0 | 30 / 30 | 0 | 0 | 68 |

`repeat`, 30 ms filtresinin elediği sıçrama kenarlarıdır. Her senaryoda kabul edilen basış sayısı
CSV satır sayısına eşittir (30 = 30); sıçramalar fazladan olay üretmemiştir.

### 3.4 Telemetri üretimi

| Senaryo | Gönderilen TEL | Periyot en kısa / ortalama / en uzun (µs) |
|---|---|---|
| S1 | 676 | 99 077 / 99 998 / 100 000 |
| S2 | 3523 | 19 630 / 19 999 / 20 000 |
| S3 | 6021 | 9 708 / 9 999 / 10 000 |
| S4 | 7723 | 9 539 / 9 999 / 10 000 |
| S5 | 7663 | 9 390 / 9 999 / 10 001 |

- Ortalama ve en uzun periyot nominal değerdedir; S5'te bile telemetri kaçmamıştır.
- En kısa değer yalnızca pencerenin **ilk** periyodudur. Senaryo seçilince görev tick'in ortasında
  uyanır, sonraki uyanış tick sınırına denk gelir; ilk periyot 1 ms'den az kısa çıkar. Bunun karşılığı
  olan uzun bir periyot yoktur (en uzun 10 001 µs).

### 3.5 Hariç tutulan kayıtlar

- R ve aşama istatistiklerinden yalnızca **S5'in 10 `drop` olayı** hariç tutuldu (olay 167, 169–172,
  175–179).
- Bu olayların hepsinde t₀, t₁, t₂ vardır; t₃ ve t₄ yoktur. Cevap satırı t₂'de TX kuyruğu dolu olduğu
  için kuyruğa girememiştir.
- Bu olaylar grafikte ve tabloda ayrıca sayılmış, "deadline karşılandı" sayılmamıştır.
- Başka dışlanan kayıt yoktur: 180 kabul edilen basışın 180'i kayıtlıdır.

## 4. Grafikler

### 4.1 Olay numarası → R

![Olay başına yanıt süresi](plots/r_vs_event.png)

- Birim ms. Senaryo başına `ok` sayısı ve hariç tutulan kayıt sayısı açıklamada.
- Kırmızı kesikli çizgi 20 ms deadline.
- S0–S4'ün bütün noktaları 5,6–13,2 ms bandında.
- S5, ilk basıştan itibaren bütçenin üstünde; basış başına ~10 ms artan bir merdiven çiziyor ve
  ~160–165 ms'de düzleşiyor.

### 4.2 Senaryo → aşamaların ortalama süreleri

![Senaryoya göre aşamaların ortalama süreleri](plots/stages_by_scenario.png)

- Yığılmış sütun, yalnızca `ok` olaylar; her sütunda toplam ve `n` yazılı.
- S5'in 114 ms'lik sütunu ölçeği belirlediği için S0–S4'ün küçük aşama farkları grafikte ince kalıyor.
  Sayısal değerleri tablo 3.2'de.

## 5. Beklenti ve ölçüm karşılaştırması

Ölçümden önce kurulan hipotezler ve sonuçları:

| Beklenti | Hesap | Ölçüm | Sonuç |
|---|---|---|---|
| S0'da R ≈ hat süresi | 5,556 ms | t₄−t₃ = 5,570 ms (+14 µs başlatma ve kesme girişi); R = 5,608 ms | Tuttu |
| S1–S3'te yalnızca t₃−t₂ büyür | Hat doluluğu %5,6 / %27,8 / %55,6 | t₁−t₀ sabit 7 µs; yalnızca t₃−t₂ büyüdü | Tuttu |
| Hattı boş bulan basış sayısı | 30 × (1 − doluluk) = 28,3 / 21,7 / 13,3 | 27 / 21 / 13 | Tuttu |
| S3 ortalama TX bekleme | doluluk × 5,57 / 2 = 1,55 ms | 1,69 ms | Yakın |
| S4'te t₁−t₀ büyür | Ek iş zamanın %20'si; beklenen ort ≈ 0,20 ms | 0,30 ms ort, 1,93 ms maks; 30 basışın 8'i iş penceresinde | Tuttu |
| S4 en kötü durum | 2 + 5,57 + 5,57 = 13,14 ms | 13,13 ms | Tuttu |
| S4'te birikim olmaz | Eşik: 10 − 5,57 = 4,43 ms; 2 ms eşiğin altında | TX tepesi 2, drop yok | Tuttu |
| S5'te doyum | 5 + 5,57 = 10,57 > 10 → periyotta bir satır | TX tepesi 16, 10 BTN drop, merdiven | Tuttu |
| S5 tavanı | 16 × 10 + 5,57 ≈ 166 ms | en büyük R 165,3 ms | Tuttu |

## 6. Sonuç ve yorum

### 6.1 Hangi bileşen değişti?

- **S0:** Hiçbiri. R'nin %99,3'ü hat süresi (`t₄ − t₃`).
- **S1–S3:** Yalnızca `t₃ − t₂` (TX öncesi bekleme): ortalama 0,45 → 1,10 → 1,69 ms. Görev beklemesi
  7 µs'de sabit.
- **S4:** `t₁ − t₀` (görev bekleme, ortalama 0,30 ms) ve `t₃ − t₂` (2,35 ms).
- **S5:** `t₃ − t₂` baskın (ortalama 106,5 ms, en çok 159,7 ms); `t₁ − t₀` 1,55 ms.
- **Değişmeyen:** `t₄ − t₃` altı senaryonun hepsinde 5,570 ms. Hat süresi yük altında değişmiyor;
  değişen, satırın hatta çıkmadan önce ne kadar beklediği.

### 6.2 Neden?

- **S1–S3, kalan servis:**
  - TX kuyruğunun tepe değeri 1, yani butonun önünde birikmiş bir TEL kuyruğu yok.
  - Bekleme, basış anında hatta olan tek satırın bitmesinden ibaret. Hat ne kadar doluysa, basışın
    dolu hatta denk gelme olasılığı o kadar yüksek.
  - En kötü durum bir satır (~5,6 ms), bu yüzden R en çok ~11,2 ms.
- **S4, görev önceliği:**
  - Ek iş en yüksek öncelikli görevde çalışıyor. İş sırasında gelen basışta ButtonTask CPU alamıyor
    (`t₁ − t₀`).
  - İş bitince önce TelemetryTask TEL satırını kuyruğa koyuyor, ButtonTask ondan sonra çalışıyor.
    Buton satırı her seferinde bir TEL'in arkasına düşüyor.
  - Hat üzerinde 2 + 5,57 = 7,57 ms < 10 ms. TEL bittiğinde bir sonraki uyanmaya zaman kalıyor; ikinci
    satır aynı periyotta başlıyor ve borç bir periyotta eriyor.
- **S5, başlatma hakkı:**
  - Periyot içinde 5 + 5,57 = 10,57 ms > 10 ms. Satır 10,57'de bittiğinde yeni periyodun işi sürüyor.
  - UartTxTask (öncelik 1) hazır olsa da CPU'yu alamıyor. Bir sonraki satırı ancak iş bittiğinde
    başlatabiliyor.
  - Başlatma hakkı saniyede 100, yani TEL üretimine eşit: doluluk ρ = 1. Kuyruk bir integratör gibi
    davranıyor; her buton satırı kalıcı bir satır ekliyor, kuyruk erimiyor.
  - Kuyruk 16'ya dolunca:
    - Basış işin içinde geldiyse buton satırı düşüyor.
    - Basış işin dışında geldiyse buton son boş yere giriyor, bir sonraki TEL düşüyor.
  - Hat tek başına yalnızca %56 dolu, CPU işi zamanın %50'si; ikisi ayrı ayrı sığıyor. Darboğaz baud
    ya da "CPU %100" değil; UART'ın boş kaldığı ~4,4 ms'yi kullanacak başlatma hakkının olmaması.

### 6.3 Hangi ölçüm destekliyor?

- **t₄ − t₃ = 5,570 ms sabiti:** Teoriden 14 µs fazla.
  - t₄ DMA'nın "aktarım bitti" ya da TXE kesmesinden alınsaydı değer yaklaşık bir karakter (~87 µs)
    kısa, ~5470 µs çıkardı.
  - Bu sabit hem saat ve baud ayarını hem de t₄'ün son stop bitinden alındığını doğruluyor.
- **S1–S3 hattı boş bulma sayıları:** 27 / 21 / 13, hesaplanan 28,3 / 21,7 / 13,3.
- **S4:**
  - R en çok 13,13 ms; hesaplanan en kötü durum 13,14 ms.
  - t₃ değerleri 10 ms ızgarasına kilitli değil: 30 olay 22 farklı fazda. Hat periyot içinde serbest.
- **S5 faz kilidi:** Çıkabilen 20 butonun t₃ değeri 10 ms'lik sayaçta 8083, 8084 ya da 8086 µs.
  Gönderim yalnızca iş bittiğinde başlayabiliyor.
- **S5 merdiveni:**
  - 151'den 165'e her basış bekleme dilimini (`t₃ − t₂` / 10 ms) tam 1 artırıyor: 2 → 16.
  - Basışlar arasında 1,6–3,5 s (160–350 periyot) olmasına rağmen hiçbir basamak geri inmiyor.
  - Bir önceki S5 denemesinde basışlar ~170 ms arayla gelmişti ve eğri aynıydı (ilk R 24,8 ms,
    tavan ~160–164 ms). O denemenin dosyaları, şartnamedeki 0,5 s kuralı için yapılan bu ölçümle
    değiştirildi. Birikim zamana değil, basış sayısına bağlı.
- **S5 düşme ayrımı:**
  - Düşen 10 butonun hepsinde `t₁ − t₀` = 2,5–4,5 ms: iş penceresinde gelmişler.
  - Tavanda çıkabilenlerde `t₁ − t₀` = 7 µs: iş dışında gelmişler.
  - `tx_queue_drop` = 16 = 10 BTN + 6 TEL. Tavanda çıkabilen 6 buton, 6 TEL satırını düşürmüş.
- **"Algılanmadı" değil:** `accepted` = 30, `btn_queue_drop` = 0 ve bütün olaylarda t₀–t₂ var.
  Düşme algılamada değil, TX kuyruğunda. `host_seq_gaps` = 0: düşen TEL'ler sıra numarası almadan
  düşmüş, hatta hiç çıkmamış.
- **Ölçüm yöntemi, probe effect:** İlk sürüm her basıştan sonra canlı sonuç satırı (BTNR)
  gönderiyordu; S5'te merdiven basamağı +20 ms çıkıyordu. Sonuçlar pencere sonuna alınınca basamak
  +10 ms'ye indi. Ölçümün kendi trafiği ölçülen gecikmeyi iki katına çıkarıyordu.

### 6.4 Fark çıkmayan yerler

- S1 ile S0 arasındaki fark küçük: ortalama R +0,42 ms, 30 basışın 27'si hattı boş buldu.
- Bu bir başarısızlık değil, beklenen sonuç: 10 Hz'de hat zamanın yalnızca %5,6'sında dolu. Etki
  var ama seyrek; en kötü durumda bir satır kadar (11,1 ms).

### 6.5 Ne henüz bilinmiyor?

- **Kanıtlanmış en kötü durum:** 30 örneğin en büyüğü analitik bir worst-case değildir.
- **Eşiğin tam yeri:** S4 (2 ms) ile S5 (5 ms) arasında ölçüm yok. Hesaplanan 4,43 ms'lik eşik
  (örneğin 4 ve 4,5 ms'lik işle) doğrulanmadı.
- **Gerçek ek iş süresi:** Pencere başına `extra_load_us` kaydedilmedi. İş süresi kalibrasyondan ve
  dolaylı olarak en büyük `t₁ − t₀`'dan biliniyor.
- **Toplam CPU kullanımı:** Ölçülmedi (idle sayacı ya da run-time stats yok). %20 / %50 değerleri
  hesaptır.
- **Tasarım alternatifleri:** Denenmedi. DMA, TC kesmesinden bir sonraki satırı doğrudan başlatmak ya
  da UartTxTask'a daha yüksek öncelik vermek, S5'teki başlatma hakkı kısıtını kaldırabilir. Etkileri
  ölçülmedi.
- **Release derleme:** Davranışı ölçülmedi.

## 7. Sınırlamalar

- Gözlenen maksimum, kanıtlanmış en kötü durum (worst-case) değildir.
- t₀ fiziksel basma anı değil, filtrenin kabul ettiği kenarın ISR girişidir. t₃ ilk fiziksel bit
  değildir. t₄'e TC kesmesine giriş gecikmesi dahildir.
- t₂ − t₁ ve kalibrasyon süresi duvar saatidir: kesme ve preemption içerebilir, saf CPU süresi
  değildir.
- Hesaplanan ek CPU payı (U ≈ C × f) ölçülmüş toplam CPU kullanımı değildir.
- Logic analyzer / osiloskopla bağımsız doğrulama yapılmadı. Saati ve baud'u taşıyan dolaylı kanıt,
  t₄ − t₃'teki 14 µs'lik farktır.
- Basışlar elle yapıldı. Periyot içindeki faz kontrol edilmedi; yalnızca basışlar arası süre
  ölçüldü (bölüm 2.3).
- 5 s ısınma CSV'den kesin olarak doğrulanamıyor, çünkü pencere başlangıç anı kaydedilmiyor. Pencere
  süresi (TEL sayısı / hız) ile basış aralığı arasındaki fark her senaryoda 9,7–21 s'dir; bu, 5 s
  ısınmayla tutarlıdır.
- Pencerenin ilk telemetri periyodu 1 ms'den az kısa ölçülür (bölüm 3.4); ortalama ve en uzun periyot
  etkilenmez.
- Tüm sonuçlar tek kart, tek derleme yapılandırması (Debug / Low) ve senaryo başına 30 basışla
  alındı.
