# Vanilla+ Faz 4 — HM Saha Araçları Tasarımı

## Amaç

HM01–HM08'i savaş hareket slotlarından bağımsız saha araçlarına dönüştürmek. Oyuncu ilgili HM item'ına ve gerekli rozete sahipse, Pokémon'un o HM hareketini öğrenmiş olması gerekmeden Cut, Fly, Surf, Strength, Flash, Rock Smash, Waterfall ve Dive kullanılabilmelidir.

Bu faz, battle moveset verisini değiştirmez. Eski save'de HM öğrenmiş Pokémonların hareketleri aynen kalır; Faz 2 sayesinde bu hareketler normal move replacement akışıyla silinebilir veya değiştirilebilir. Faz 4 saha kullanımı hiçbir Pokémon'a otomatik olarak hareket öğretmez, hareket silmez veya moveset slotu ayırmaz.

## Değişmeyecek sözleşmeler

- `GAME_CODE` yine `BPEE` kalır.
- `SaveBlock1`, `SaveBlock2`, `PokemonStorage`, party Pokémon veri yapısı ve item/species/move ID'leri yeniden düzenlenmez.
- Mevcut 128 KiB save uyumluluğu korunur.
- Vanilla badge progression korunur.
- Story/event flag'leri korunur.
- Event ticket, Mystery Gift ve legendary alanları etkilenmez.
- HM item'ları tüketilmez.
- TM davranışı Faz 2'deki gibi tekrar kullanılabilir kalır.
- HM field animasyonlarının Faz 3'teki kısaltılmış süreleri korunur.
- Link/Union Room gibi vanilla'da field move kullanımının kısıtlı olduğu bağlamlarda yeni bypass oluşturulmaz.

## HM → badge sözleşmesi

Eşleme açık bir tabloyla tutulur; item numarası veya field-move enum sırasından badge türetilmez.

| HM | Hareket | Gerekli rozet |
| --- | --- | --- |
| HM01 | Cut | Badge 1 |
| HM05 | Flash | Badge 2 |
| HM06 | Rock Smash | Badge 3 |
| HM04 | Strength | Badge 4 |
| HM03 | Surf | Badge 5 |
| HM02 | Fly | Badge 6 |
| HM08 | Dive | Badge 7 |
| HM07 | Waterfall | Badge 8 |

Bu açık eşleme zorunludur çünkü HM item sırası ile Emerald badge/field-move sırası aynı değildir.

## Mimari

Faz 4 için HM sahipliği, badge kontrolü, saha bağlamı ve görsel Pokémon aktörü tek bir ortak katmanda toplanır. Bu katman mevcut `party_menu.c` içindeki HM'ye özel sorumluluğu azaltır ve aynı mantığın bag kullanımı, dünya etkileşim scriptleri ve otomatik saha kontrolleri tarafından paylaşılmasını sağlar.

Önerilen sınır:

- `include/vanillaplus_hm.h`
- `src/vanillaplus_hm.c`

Bu modül yalnızca Faz 4 HM saha kullanım sözleşmesini bilir. Save verisi tutmaz ve yeni kalıcı state eklemez.

Ortak API şu sorumlulukları karşılamalıdır:

1. Verilen item'ın HM01–HM08 olup olmadığını belirlemek.
2. HM item → move → badge eşlemesini döndürmek.
3. İlgili HM item'ının çantada bulunup bulunmadığını kontrol etmek.
4. İlgili badge'in alınıp alınmadığını kontrol etmek.
5. HM field animasyonu için güvenli bir party aktörü seçmek.
6. Seçilen HM için mevcut harita/oyuncu durumunun saha kullanımına izin verip vermediğini değerlendirmek.
7. Uygun olduğunda mevcut Emerald field-effect/script akışını başlatmak.

Bu modül yeni bir field-effect sistemi yazmaz; mevcut Cut/Surf/Fly/Flash/Strength/Rock Smash/Dive/Waterfall effect ve scriptlerini mümkün olduğunca yeniden kullanır.

## Görsel field aktörü

Vanilla field-effect kodlarının bir bölümü `GetCursorSelectionMonId()` üzerinden party menu imlecini kullanarak Pokémon sprite'ı, nickname'i veya effect argümanını seçiyor. Faz 4'te HM kullanımı party menu'den gelmeyebileceği için bu artık güvenli bir kaynak değildir.

HM saha kullanımı için `GetHmFieldActorPartyId()` benzeri ortak bir seçim yapılır:

- party'deki ilk gerçek, Egg olmayan Pokémon seçilir;
- fainted olması saha aracı kullanımını engellemez;
- tür, HM learnset uyumluluğu veya moveset kontrol edilmez;
- normal oynanışta geçerli bir aktör bulunamaması beklenmez; yine de bulunamazsa kullanım güvenli biçimde reddedilir ve `PARTY_SIZE` gibi geçersiz indeks field-effect koduna verilmez.

Bu Pokémon yalnızca görsel/teknik field-effect aktörüdür. HM'yi bildiği, HM'yi öğrenebildiği veya HM'yi kullanan Pokémon olduğu anlamına gelmez.

## Bag davranışı

TM'ler mevcut `ItemUseOutOfBattle_TMHM` öğrenme akışını kullanmaya devam eder.

HM01–HM08 ise Faz 4'te saha aracı olarak davranır:

- HM item seçildiğinde party teach ekranı açılmaz;
- item tüketilmez;
- önce badge koşulu kontrol edilir;
- sonra mevcut saha bağlamı kontrol edilir;
- uygunsa ilgili mevcut field-effect/script başlatılır;
- uygun değilse kullanıcıya normal 'burada kullanılamaz' benzeri mesaj gösterilir.

Bunun için HM item kayıtlarının field-use tipi/fonksiyonu TM'lerden ayrılabilir. Amaç `src/data/items.h` içinde HM'leri doğrudan field kullanım yoluna yönlendirmek; TM01–TM50'nin mevcut tipini ve öğrenme fonksiyonunu değiştirmemektir.

Bu karar bilinçlidir: yeni saha sistemi HM item'ını battle move öğretme aracı olarak kullanmaz. Eski save'de öğrenilmiş HM hareketleri korunur ve battle mekanikleri değişmez.

## Party menu davranışı

HM saha kullanımı artık moveset'e bağlı olmadığı için Pokémon seçim menüsünde Cut/Flash/Rock Smash/Strength/Surf/Fly/Dive/Waterfall seçeneklerinin çıkması gerekli değildir.

`SetPartyMonFieldSelectionActions` içinde HM01–HM08'e karşılık gelen learned moves, saha aksiyonu olarak eklenmemelidir. Dig, Teleport, Secret Power, Milk Drink, Soft-Boiled ve Sweet Scent gibi HM olmayan field move davranışları mevcut şekilde devam eder.

Bu sayede aynı HM için iki farklı otorite oluşmaz: HM field-use otoritesi çanta + badge + saha bağlamıdır.

## Dünya etkileşimleri

### Cut

Ağaç/çalı etkileşimi:

- Badge 1 korunur.
- `checkpartymove MOVE_CUT` kaldırılır.
- HM01'in çantada bulunması şart olur.
- Yes/No ve mevcut Cut field-effect akışı korunur.

### Rock Smash

Kırılabilir kaya etkileşimi:

- Badge 3 korunur.
- `checkpartymove MOVE_ROCK_SMASH` kaldırılır.
- HM06 çantada olmalıdır.
- Rusturf Tunnel state ve Rock Smash wild encounter davranışları korunur.

### Strength

Boulder etkileşimi:

- Badge 4 korunur.
- `FLAG_SYS_USE_STRENGTH` oturum davranışı korunur.
- İlk aktivasyonda HM04 çantada olmalıdır.
- `checkpartymove MOVE_STRENGTH` kaldırılır.
- Boulder push mekanikleri değişmez.

### Surf

Surf kullanım şartı:

- Badge 5 + HM03 + surfable water.
- `PartyHasMonWithSurf()` saha yetkisi olarak kullanılmaz.
- Surf blob, follower davranışı, warp ve encounter mantığı korunur.
- HM03 moveset'te olmasa da normal su kenarı kullanım akışı çalışır.

### Fly

Fly kullanım şartı:

- Badge 6 + HM02.
- Vanilla Fly'ın izin verdiği harita koşulları korunur.
- Region map/hedef seçimi mevcut kod üzerinden yürür.
- HM02 bilen Pokémon aranmaz.

### Flash

Flash kullanım şartı:

- Badge 2 + HM05.
- Karanlık mağara kontrolü ve `FLAG_SYS_USE_FLASH` korunur.
- Faz 3'teki tam görünürlük davranışı korunur.
- Registeel Braille puzzle gibi Flash'a özel story/puzzle yolu kaybolmaz; aynı HM sahipliği/badge sözleşmesinden erişilebilir kalır.

### Dive

Dive kullanım şartı:

- Badge 7 + HM08.
- Yüzeyden dalış ve underwater surface akışlarının ikisi de HM08 sahipliği kontrol eder.
- `TrySetDiveWarp` ve map-type kontrolleri korunur.
- Script içindeki `checkpartymove MOVE_DIVE` kaldırılır.

### Waterfall

Waterfall kullanım şartı:

- Badge 8 + HM07.
- Oyuncunun doğru yönde surf ediyor olması ve waterfall metatile kontrolleri korunur.
- `checkpartymove MOVE_WATERFALL` kaldırılır.
- `field_control_avatar.c` içindeki erken seçim de HM07 sahipliğini dikkate alır; yalnız badge ile script başlatılmaz.

## Hata ve mesaj davranışı

Üç farklı başarısızlık sınıfı korunur:

1. HM item yok.
2. Gerekli badge yok.
3. HM var ve badge var ama mevcut konum/harita uygun değil.

Story progression açısından 1 ve 2 kesinlikle bypass edilmemelidir. Mesajların bire bir vanilla metni olması zorunlu değildir; Türkçe ROM'un mevcut metin stili kullanılmalıdır. Ancak yanlış nedenle başarı verilmemesi daha önemlidir.

Dünya üzerindeki ağaç/kaya/boulder gibi doğrudan etkileşimlerde mevcut doğal çevre mesajları mümkün olduğunca korunur. Bag'den yanlış yerde kullanımda genel 'burada kullanılamaz' mesajı yeterlidir.

## Beklenen dosya etkileri

Uygulama planı kesin dosya listesini repo incelemesinden sonra kilitleyecek, ancak Faz 4'ün beklenen değişim alanları şunlardır:

- yeni `include/vanillaplus_hm.h`
- yeni `src/vanillaplus_hm.c`
- `src/data/items.h`
- `src/item_use.c`
- `include/item_use.h`
- `src/party_menu.c`
- `src/data/party_menu.h` yalnız ihtiyaç varsa
- `data/scripts/field_move_scripts.inc`
- `src/field_control_avatar.c`
- HM effect dosyaları: `src/fldeff_cut.c`, `src/fldeff_flash.c`, `src/fldeff_rocksmash.c`, `src/fldeff_strength.c` ve gerekli Surf/Fly/Dive/Waterfall callback alanları
- ilgili header/export dosyaları
- `tools/vanillaplus_phase04_verify.py`
- `tools/phase_version_contract_test.py`
- `.github/workflows/build.yml`
- `Makefile`
- Continue ekran build marker metni

Alakasız refactor yapılmaz.

## Build kimliği

Faz 3 şu anda build marker `004` sahibidir. Faz 4 tamamlandığında release/test/Continue marker'ları senkron şekilde bir sonraki değere ilerletilir:

- `ZUMRUT VP005`
- `ZUMRUT T005`
- `OYUNCU V+005`

Eski faz verifier'ları daha yeni marker'ı kabul edecek şekilde version-agnostic kalmalıdır. Faz 4 verifier'ı exact `005` marker'ının sahibi olur ve `phase_version_contract_test.py` ownership zincirini Faz 4'e genişletir.

## Otomatik doğrulama

Yeni `tools/vanillaplus_phase04_verify.py` en az şu kontratları kaynak seviyesinde doğrular:

- Sekiz HM için açık item/move/badge eşlemesi mevcut.
- HM saha yetkisi `checkpartymove` veya `PartyHasMonWithSurf` gibi learned-move şartına bağlı değil.
- Cut/Rock Smash/Strength/Waterfall/Dive scriptlerinde ilgili `checkpartymove` kontrolleri kalmamış.
- HM bag kayıtları TM teach yolundan ayrılmış.
- TM01–TM50 hâlâ `ItemUseOutOfBattle_TMHM` ile öğretilebilir.
- HM field actor seçimi party-menu cursor'una zorunlu olarak bağlı değil.
- Badge kontrolleri sekiz HM için korunuyor.
- Faz 3 Flash görünürlüğü, Repel, fishing, poison, bike ve low-HP kontratları bozulmamış.
- Faz 0–1, Faz 2 ve Faz 3 verifier'ları hâlâ CI'da çalışıyor.
- Faz 4 verifier'ı CI'a eklenmiş.
- Build marker `005` üç yerde senkron.
- Save struct veya bag kapasite layout sabitleri Faz 4 tarafından değiştirilmemiş.

## Build ve regresyon testi

Kod tamamlandıktan sonra:

1. Phase version contract testi.
2. Faz 0–1 verifier.
3. Faz 2 verifier.
4. Faz 3 verifier.
5. Faz 4 verifier.
6. Release ROM build.
7. Test ROM build.
8. Eski save ile Continue → Save → reboot → Continue.

## Cihaz/emülatör smoke test matrisi

Her HM, hareket hiçbir party Pokémonunun aktif moveset'inde yokken test edilir.

- Cut: HM01 + Badge 1 ile ağaç kesilir; HM01 yokken veya badge yokken kesilmez.
- Flash: HM05 + Badge 2 ile karanlık alan aydınlanır; Registeel puzzle yolu ayrıca kontrol edilir.
- Rock Smash: HM06 + Badge 3 ile kaya kırılır; Rusturf/encounter yan etkileri çalışır.
- Strength: HM04 + Badge 4 ile Strength aktive edilir ve boulder itilir.
- Surf: HM03 + Badge 5 ile surfable water'da Surf başlar.
- Fly: HM02 + Badge 6 ile izin verilen haritada region map/hedef seçimi açılır.
- Dive: HM08 + Badge 7 ile uygun dive tile'da dalış ve underwater yüzeye çıkış çalışır.
- Waterfall: HM07 + Badge 8 ile doğru waterfall koşulunda tırmanılır.

Her HM için ayrıca yanlış badge, eksik HM ve yanlış harita/konum negatif testi yapılır.

## Başarı ölçütü

Faz 4 tamamlanmış sayılır ancak aşağıdakilerin tamamı doğruysa:

- Sekiz HM'nin hiçbiri saha kullanımı için learned move gerektirmez.
- İlgili HM item'ı olmadan kullanım mümkün değildir.
- Gerekli badge olmadan kullanım mümkün değildir.
- Battle moveset otomatik olarak değiştirilmez.
- Eski save açılır ve yeniden kaydedilebilir.
- Story progression bypass edilmez.
- Faz 0–3 regresyon kontratları geçer.
- Release ve test ROM build'leri başarılıdır.
