# AI Kullanım Notu

**Araç:** Claude (Anthropic), Claude Code masaüstü uygulaması üzerinden.

**Özet:** AI'ı kod, arayüz, analiz betiği, belgeler ve sunum için kullandım. Kapsamı ve kararları ben
belirledim; ölçümleri kartta ben aldım. AI'ın her önerisini kartta, derlemede ya da ham veride
gördüğüm sonuçla kontrol ettim.

## Hangi işlerde destek aldım?

- **Proje iskeleti:** Dizin yapısı, IAR projesi (IAR 9.70.4'ün ürettiği boş projeden türetildi),
  FreeRTOS-Kernel'in git alt modülü olarak eklenmesi, `FreeRTOSConfig.h` ve CMSIS dosyalarının
  seçilmesi.
- **Firmware:**
  - Register seviyesi sürücüler: RCC, GPIO, EXTI, TIM2, USART2, ADC.
  - Üç görev, kuyruklar, buton ISR'si ve sıçrama filtresi.
  - Kayıt havuzu, UART TC tabanlı t₄ ölçümü, ASCII 64 baytlık mesaj kodlayıcısı.
  - Arayüzden senaryo seçme komutu, kart sıcaklığı telemetrisi.
- **Web Serial arayüzü:** Canlı TEL/BTN gösterimi, senaryo paneli, gecikme dağılımı paneli, ölçüm
  sonu döküm, CSV kaydı, kayıp ve bütünlük sayaçları.
- **Analiz:** `analyze.py` betiği; ölçüm verisindeki tutarsızlıkların taranması.
- **Belgeler:** README, `setup.md`, `code-notes.md` ve `report.md` (tablolar ve yorum benim ölçüm
  verimden dolduruldu).
- **Açıklama:** Sistemi, S5'teki birikimi, UART TC ile t₄ ölçümünü ve FreeRTOS ayarlarını bana kod
  üzerinden anlatması.
- **Sunum:** İskeletini benim verdiğim PowerPoint sunumu ve slayt slayt konuşma metni.

## AI'ı nasıl yönlendirdim?

1. **Kapsamı ben belirledim.** İlk mesajda kartı (STM32F407VG Discovery), üç görevi ve önceliklerini,
   t₀–t₄ ölçüm noktalarını, UART ve mesaj biçimini, S0–S5 senaryolarını, dizin yapısını ve arayüzden
   beklentilerimi verdim. "Önce temeli kuralım, eklenecekleri sonra söyleyeceğim" diyerek işi adım
   adım ilerlettim.

2. **İçerik kararlarını ben verdim.** Telemetri verisinin ne olduğunu sordum ve kartın sıcaklığı
   olmasını istedim.

3. **Kartta gördüğümü geri bildirdim.** İlk S0 denemesinde bir basışta iki olay geldiğini (basma ve
   bırakma) ve telemetrinin gelmediğini ekran görüntüsüyle bildirdim. "1 basışta 1 okuma olması
   gerekmez mi?" sorum buton filtresinin düzeltilmesine yol açtı.

4. **Kendi tasarım önerilerimi getirdim.** Ders slaytlarındaki iki yöntemi önerdim:

   - Buton ISR'si tasarımı: yalnızca basış kenarı, son kabulden 30 ms, sayaçlar, kuyruk kurulum
     sırası.
   - Slayt 04'teki ölçüm kaydı yöntemi: sonuçları kayıt havuzunda tutmak.

   Her ikisinde de AI'dan önce değerlendirme istedim, neyin kalıp neyin değişeceğini gerekçesiyle
   dinledim, sonra onay verdim ("evet devam edelim", "olur").

5. **Arayüz isteklerini ben tanımladım.**

   - Gecikme hesabının slayttaki gibi aşamalara bölünerek gösterilmesi.
   - Senaryoların (S0–S5) yeniden derlemeden arayüzden seçilebilmesi.

6. **Şartnameye göre denetlettim.** Şartname dosyasını (`odev-01.html`) verip ne kadarının
   karşılandığını sordum. Eksikleri giderirken kapsamı "sadece yazılımsal eksikleri giderelim" diye
   sınırladım.

7. **Ne zaman kod değişeceğini ben kontrol ettim.** Anlamak istediğim yerlerde "kodda değişiklik
   yapma", "sadece öğret", "sadece cevap ver" diyerek AI'ın dosyalara dokunmasını engelledim:

   - sistemin baştan sona anlatımı ve görsel ağaç,
   - UART TC ve t₄ ölçümünün kod üzerinde anlatımı,
   - `configMAX_PRIORITIES`'in nerede olduğu,
   - kodun event-driven olup olmadığı.

   Bir kez de web'den araştırmaya başlamasını durdurup doğrudan cevap istedim.

8. **Veriyi denetlettim ve gerekince yeniden ölçtüm.** "Bir gariplik var mı?" diye ham veriyi
   kontrol ettirdim. Çıkan üç sorun:

   - S1'de 29 olay (en az 30 gerekiyordu),
   - basışlar arasının 0,5 s'den kısa olması,
   - CSV'deki `tx_drop` durum adının şartnamedeki `drop` ile uyuşmaması.

   Durum adını düzelttirdim ve altı senaryoyu şartnamedeki prosedürle yeniden ölçtüm.

9. **Anlamadığım yeri sordum.** S5'te R'nin neden sürekli artıp bir yerde sabitlendiğini ve
   sonuçların neden basış anında değil ölçüm sonunda geldiğini sordum. Bunları sunumda kendim
   anlatabilmek için açıklattım.

10. **Sunumun iskeletini ben verdim.** A1–A25 ve E1–E16 başlıklarıyla iskeleti ben verdim, AI slaytları
    ve konuşma metnini hazırladı. AI iskeletteki sayısal iddiaları ölçüm verisiyle karşılaştırdı;
    uymayan bir ifadeyi bana bildirip düzeltti (aşağıda).

11. **Teslimi ben yönettim.** Eksik listesini çıkarttım, rapor ve kurulum belgesini doldurttum.
    Commit ve GitHub'a yükleme benim onayımla yapılır.

## Üretilen kodu nasıl kontrol ettim?

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
