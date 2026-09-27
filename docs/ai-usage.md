
- **Derleme:**
  - Firmware IAR 9.70.4 ile derlendi, uyarılar sıfırlandı.
  - İlk derlemedeki 4 uyarıdan ikisi `volatile` okuma sırasına dair gerçek tanımsız davranıştı.
  - Her değişiklikten sonra derleme 0 hata / 0 uyarı ile tekrarlandı.
- **Mesaj kodlayıcı:**
  - `protocol.c` bilgisayarda gcc ile, her satır türü için en kötü durum değerleriyle test edildi
    (en uzun satır 60 bayt ≤ 63).
  - Arayüz, firmware kodlayıcısının ürettiği satırlarla test edildi: parçalı okuma, yarım satır,
    bozuk satır, sayaç sarması, durum adlarının CSV'ye `drop` olarak yazılması.
- **Analiz betiği:** Önce boş ve sentetik veriyle denendi (yalnızca geçici klasörde, depoya
  konmadı). Sonra gerçek ölçümlerle çalıştırıldı.
- **Kart üzerinde:**
  - İlk S0'da her basış iki olay üretti. Ardışık olaylar arasındaki 60–280 ms bırakma sıçramasını
    gösterdi ve filtre düzeltildi. Yeni ölçümlerde uzun aralıklardan sonra "eş" olay gelmediği
    görüldü.
  - Satır süresi `t₄ − t₃` = 5570 µs ölçüldü (teorik 5556 µs). Bu, t₄'ün UART TC'den geldiğini ve
    saat / baud ayarının doğru olduğunu gösterdi.
  - İlk S5'te `extra_load_us` = 8373 µs görüldü (hedef 5000); iş yükü bu ölçüme göre kalibre
    edildi.
  - Telemetri periyotları ölçüldü: ortalama 99 998 / 19 999 / 9 999 µs.
- **Veri bütünlüğü:** Her senaryoda kabul edilen basış sayısı CSV satır sayısına, kartın gönderdiği
  kayıt sayısı alınan kayıt sayısına eşit; sıra boşluğu ve hatalı satır yok.
- **Rapor ve sunum:** Rapordaki ve sunumdaki sayılar ham CSV'lerden yeniden hesaplanıp
  `summary.csv` ile karşılaştırıldı. Sunumun slaytları resme çevrilip tek tek kontrol edildi.
- **Belgeleme:** Sıcaklık sensörü kalibrasyon adresleri CMSIS başlığında olmadığı için
  datasheet'ten (DS8626) alındı. Sıcaklık bir termometreyle bağımsız olarak doğrulanmadı.

## Hangi önerileri değiştirdim ve neden?

- **Buton filtresi:**
  - Slayttaki "yalnızca basış kenarı + son kabulden 30 ms" önerim bırakma sıçramasını çözmüyordu;
    bunu kartta gözledim.
  - Birleşik bir tasarımda anlaştık: 30 ms pencere, sayaçlar, ilk kenar kabulü ve kuyruk kurulum
    sırası korundu; iki kenar ve basılı/bırakılmış durumu eklendi.
- **Ölçüm kaydı:**
  - Slayt 04'ten önerdiğim yöntem HAL/DMA tabanlıydı; register seviyesi kesme sürücüsüne uyarlandı.
  - Havuzun tek sahibi UartTxTask yapıldı; tampon kayda konmadı (4 KB tasarruf).
  - Havuz dolunca eski kayıt ezilmiyor.
- **Canlı sonuç satırı kaldırıldı:** İlk sürüm her basıştan sonra sonucu (BTNR) canlı gönderiyordu.
  S5 verisi bunun ölçümü bozduğunu gösterdi (birikim basış başına +20 ms). Sonuçlar ölçüm penceresi
  sonunda dökülüyor; basamak +10 ms'ye indi.
- **Mesaj biçimi:** İlk sürüm ikili (binary) 64 bayttı. Şartnamedeki ASCII 63 + LF biçimine
  çevrildi.
- **Durum adları:** Kart kaybın nerede olduğunu `tx_drop` / `btn_drop` diye ayırıyordu. CSV'de
  şartnamedeki `drop` değerine çevrildi; nerede düştüğü, hangi zamanların dolu olduğundan
  anlaşılıyor.
- **Ölçümler:** İlk veri setinde S1'de 29 olay vardı ve basışlar çoğunlukla 0,5 s'den sık gelmişti.
  Sonuçlar değişmese de şartnameye uymak için altı senaryoyu yeniden ölçtüm.
- **Bilinçli sapmalar:** README'deki "Şartnameden sapmalar ve eklemeler" tablosunda gerekçeleriyle
  listelendi.

### AI'ın yanlış söyleyip düzeltilen yerleri

- **IAR menüsü:** IAR'da yapılandırma değiştirme menüsünün yerini önce yanlış tarif etti; sorunca
  düzeltti (Workspace penceresindeki açılır liste).
- **Event-driven cevabı:** Sıcaklık okumasının her periyotta ADC'yi yoklayarak beklediğini söyledi.
  Sunumu hazırlarken kodla karşılaştırınca yanlış olduğu çıktı: okuma beklemiyor, bekleyen fonksiyon
  yalnızca açılışta çalışıyor.
- **Sunum iskeleti:** "Her basış t₃ − t₂'ye tam 10,0 ms ekler" ifadesi veriyle karşılaştırılınca tam
  doğru çıkmadı. Doğrusu: her basış bekleme dilimini tam 1 artırıyor (2 → 16). Slayt buna göre
  düzeltildi.

Bu örnekler, AI'ın söylediğini koda ve veriye bakmadan kabul etmediğimi gösteriyor.

## Neler AI ile yapılmadı?

- **Ölçüm verisi üretilmedi.** `measurements/` altındaki CSV'lerin hepsi benim kartta aldığım gerçek
  ölçümler. Sentetik veri gerçek sonuç gibi sunulmadı.
- Kablolama, karta yükleme, butona basma ve arayüzle ölçüm alma işlerini ben yaptım.
- Senaryoların ve sunum iskeletinin çerçevesi, hangi önerinin kabul edileceği ve teslim kararları
  bana aitti.
- Anlatım videosunu kendim kaydediyorum; AI kullanılmadı.
