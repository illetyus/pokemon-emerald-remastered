# Pokémon Zümrüt Vanilla+ QoL — Master Design

## Amaç

Mevcut Türkçe Pokémon Emerald deneyimini ve Hoenn kimliğini korurken, özellikle günlük oynanıştaki gereksiz sürtünmeleri azaltan kapsamlı bir Vanilla+ sürüm oluşturmak.

Final ROM dosya adı: `pokemon-emerald_v1.gba`.

## Temel ilkeler ve değişmeyecekler

- Game code `BPEE` kalacak.
- Mevcut 128 KiB Emerald `.srm` kayıt yapısı korunacak.
- `SaveBlock1`, `SaveBlock2` ve `PokemonStorage` boyut/offset düzenleri kaydırılmayacak.
- Mevcut item/species/move kimlikleri yeniden numaralandırılmayacak.
- Event ticket, Mystery Gift, özel ada ve legendary içerikleri korunacak.
- Fairy type eklenmeyecek.
- Gen IV fiziksel/özel ayrımı eklenmeyecek.
- Modern move/ability/type chart/stat değişiklikleri yapılmayacak.
- Gym/trainer zorluk overhaul yapılmayacak.
- EXP All hilesi, global catch-rate artışı, wild legendary veya randomizer eklenmeyecek.
- Her faz sonunda eski save ile Continue → Save → reboot → Continue regresyon testi yapılacak.

## Faz 0 — Tabanı sabitleme ve test altyapısı

Amaç: sonraki değişiklikleri güvenli, izole ve hızlı test edilebilir hâle getirmek.

- Repo: `illetyus/pokezumrut-vanillaplus`, branch: `main`.
- Mevcut çalışan değişiklikler korunacak.
- Geliştirme build'lerinde görünür build kimliği kullanılacak (`VPxxx` benzeri).
- Final ROM adı `pokemon-emerald_v1.gba` olacak.
- Ayrı test/debug build sistemi oluşturulacak.
- Test build'de yalnızca geliştirme amacıyla şu kolaylıklar bulunabilecek:
  - haritalara hızlı ışınlanma,
  - badge/flag açma,
  - gerekli itemleri verme,
  - belirli Pokémon/seviye hazırlama,
  - RTC ve event koşullarını hızlı kurma.
- Bu debug araçlarının hiçbiri final ROM'da bulunmayacak.

Başarı ölçütü: eski `.srm` açılır, kayıt alınır, yeniden açılır; market, party, bag, PC ve temel savaş akışı çalışır.

## Faz 1 — Evrim ve market sistemi

### Zaten mevcut ve korunacak

- Kadabra → Alakazam Lv37.
- Machoke → Machamp Lv37.
- Graveler → Golem Lv37.
- Haunter → Gengar Lv37.
- Trade + item evrimleri ilgili item doğrudan kullanılarak yapılır.
- Ultra Top fiyatı 100₽.
- Maks Canlandırıcı fiyatı 200₽.
- Normal Poké Mart'larda standart top stoğu yerine Ultra Top bulunur.
- Bina içinde koşma çalışır.

### Yeni market içeriği

Tüm normal Poké Mart'larda oyunun başından itibaren:

- Ateş Taşı
- Su Taşı
- Yıldırım Taşı
- Yaprak Taşı
- Ay Taşı
- Güneş Taşı
- PP Max

satılacak.

Rare Candy / Nadir Şeker normal marketlere eklenmeyecek.

Lilycove Department Store ve özel dükkânlar, normal Poké Mart değişikliklerine otomatik olarak dahil edilmeyecek.

### Reusable evrim taşları

Yukarıdaki altı evrim taşı, Pokémon üzerinde başarıyla kullanıldığında çantadan eksilmeyecek.

- Bu davranış yalnızca bu altı item ID'sine uygulanacak.
- Diğer tüketilebilir itemlerin davranışı değişmeyecek.
- Save yapısı değişmeyecek.

### Test

- Her taşla en az bir evrim, debug test build ile birkaç dakika içinde doğrulanacak.
- Taş miktarı kullanım sonrasında değişmemeli.
- Trade-item evrimleri ayrıca doğrulanmalı.
- Normal market örnekleri düşük/orta/ileri şehirlerden kontrol edilmeli.
- Eski save ve yeni save ile aynı market stoğu görünmeli.

## Faz 2 — TM, Move Tutor ve HM temel QoL

- TM'ler kullanıldığında eksilmeyecek.
- Gen III TM listesi ve Pokémon öğrenme uyumluluğu değişmeyecek.
- Move Tutor'lar tekrar tekrar kullanılabilir olacak.
- Tutor NPC'lerinin tek kullanımlık flag mantığı güvenli şekilde kaldırılacak veya tekrar kullanım sağlayacak şekilde düzenlenecek.
- HM hareketleri normal Move Deleter zorunluluğu olmadan silinebilir olacak.
- HM öğrenme uyumlulukları değişmeyecek.
- Bu fazda HM'lerin field-use zorunluluğu henüz kaldırılmayacak; bu Faz 4'te ele alınacak.

### Test

- Aynı TM iki farklı Pokémon'a öğretilir ve item kalır.
- Aynı tutor art arda iki kez kullanılabilir.
- Surf/Cut/Flash gibi HM hareketleri normal move replacement üzerinden silinebilir.
- Story progression ve badge gating bozulmaz.

## Faz 3 — Overworld ve günlük oynanış QoL

- Running Shoes koşulları gevşetilecek; bina içinde koşma korunacak.
- Repel bittiğinde yeni Repel kullanma sorusu gösterilecek; mevcut Repel varsa hızlı tekrar kullanım sağlanacak.
- Balıkçılık daha az zamanlama baskılı ve daha az tekrar gerektiren hâle getirilecek.
- Low HP beep kapatılacak.
- Overworld poison, Pokémon'u doğrudan 0 HP'ye düşürmeyecek.
- Flash kullanılan karanlık alanlarda görünürlük iyileştirilecek.
- Mach Bike / Acro Bike arasında hızlı geçiş sağlanacak.
- HM field animasyonları kısaltılacak.
- Normal kapı/geçiş akışları korunacak.

### Test

Debug build ile doğrudan ilgili haritalara ışınlanarak Repel, fishing, poison, düşük HP, Flash ve iki bisiklet türü ayrı ayrı test edilecek.

## Faz 4 — HM field-use overhaul

Amaç: Cut, Surf, Strength, Rock Smash, Flash, Fly, Dive ve Waterfall gibi hareketlerin field kullanımı için aktif 4 hareket slotunda bulunma zorunluluğunu mümkün olduğunca kaldırmak.

### Kısıtlar

- HM item'ına sahip olma koşulu korunacak.
- Gerekli badge koşulları vanilla mantığıyla korunacak.
- Battle moveset değişmeyecek.
- Story scriptleri ve progression kontrolleri bozulmayacak.

### Tasarım yönü

Field kullanımı için ilgili HM'yi kullanabilecek uygun party Pokémon'u veya güvenli bir doğrudan HM-item çözümü değerlendirilecek. Uygulama, mevcut script akışına en az yan etki yapan yöntem seçilerek yapılacak.

### Test

Cut, Rock Smash, Strength, Surf, Waterfall, Dive, Fly ve Flash; hareket aktif moveset'te yokken debug build ile tek tek doğrulanacak.

## Faz 5 — Çanta, item yönetimi ve kısayollar

- Çantada sıralama eklenecek.
- İsim/tür/kullanım tipine göre sıralama seçenekleri değerlendirilecek.
- Hızlı auto-sort komutu eklenecek.
- PC item listesi için sıralama sağlanacak.
- Held-item yönetimi kolaylaştırılacak.
- Birden fazla registered item kısayolu hedeflenecek.

### Save güvenliği

- `BAG_ITEMS_COUNT` veya benzeri alanlar körlemesine büyütülmeyecek.
- Save struct içindeki mevcut alanlar kaydırılmayacak.
- Daha büyük çanta ancak save layout bozulmadan uygulanabiliyorsa yapılacak; aksi durumda kapsam dışı bırakılacak.

## Faz 6 — Pokémon Summary ve yönetim ekranları

Summary ekranında:

- IV değerleri,
- EV değerleri,
- nature'ın yükselttiği stat için farklı renk,
- nature'ın düşürdüğü stat için farklı renk,
- Hidden Power tipi,
- move Type / Power / Accuracy / PP,
- mümkünse kısa move açıklaması

gösterilecek.

Ek olarak:

- Move Relearner'a daha kolay erişim,
- menüden nickname değiştirme

sağlanacak.

Pokémon veri formatı değişmeyecek; mevcut veriler yalnızca okunacak ve arayüzde gösterilecek.

## Faz 7 — Metin, menü ve animasyon hızı

- Mevcut Fast'tan daha hızlı metin seçeneği veya eşdeğer hızlı metin davranışı.
- Menü açılış/kapanış beklemeleri azaltılacak.
- Gereksiz frame gecikmeleri azaltılacak.
- HM animasyonları daha kısa olacak.
- Pokémon Center gibi sık tekrarlanan uzun sekanslarda kontrollü kısaltma değerlendirilecek.
- Battle sisteminin mekanik timing mantığına dokunulmayacak.

## Faz 8 — RTC ve zaman sistemi

- Oyun içinden güvenli RTC reset/fix yolu sağlanacak.
- Emulator RTC ile uyumluluk korunacak.
- Berry growth ve daily event sistemleri korunacak.
- Shoal Cave tide, Mirage Island ve diğer time-based özellikler bozulmayacak.
- Eski save'in zaman alanları migrate edilmeden kullanılmaya devam edecek.

## Faz 9 — Wild encounter yeniden dengelemesi

Amaç: Hoenn kimliğini korurken version/trade bağımlılığını azaltmak ve Gen I/II erişimini kontrollü biçimde artırmak.

### Hedef ağırlıklar

Grass:
`16 / 14 / 13 / 12 / 11 / 10 / 8 / 7 / 5 / 4`

Surf / Rock Smash / Super Rod:
`25 / 22 / 20 / 18 / 15`

Good Rod:
`40 / 35 / 25`

Old Rod:
`60 / 40`

### İçerik kuralları

- Version/trade türleri çoğunlukla %7–12.
- Daha değerli nadir türler %4–7.
- Evolved türler %4–6.
- Biyom uyumu korunacak.
- Wild legendary olmayacak.
- Randomizer mantığı olmayacak.
- Hoenn türleri ana kimlik olarak kalacak.
- Feebas erişimi daha insancıl hâle getirilecek.
- Gen I/II türleri kontrollü dağıtılacak.

Gerekirse encounter selection kodu bu ağırlıkları destekleyecek şekilde düzenlenecek.

## Faz 10 — PC ve Pokémon yönetimi ileri QoL

- PC box sıralama.
- Species / level / type gibi kriterler.
- Held item görünürlüğü ve yönetimi.
- Box ↔ party işlemlerini daha az menü ile yapma.
- Box organizasyon kolaylıkları.
- PokémonStorage binary yapısı değişmeyecek.

## Faz 11 — Final regresyon ve release

Eski ve yeni save üzerinde en az şu sistemler doğrulanacak:

- Continue / Save / reboot / Continue
- Party
- PC
- Bag
- Pokédex
- story flags
- badge flags
- event tickets
- Mystery Gift
- legendary/event alanları
- RTC
- marts
- evolution
- TMs
- tutors
- HMs
- fishing
- repel
- poison
- bikes
- Flash
- summary

Final çıktıda:

- `pokemon-emerald_v1.gba`
- SHA-1
- SHA-256
- CRC32
- değişiklik listesi
- test edilen save sürümleri
- bilinen sınırlamalar

sunulacak.

## Geliştirme sırası

Önerilen sıra:

`0 → 1 → 2 → 3 → 4 → 8 → 5 → 6 → 7 → 9 → 10 → 11`

HM overhaul ve RTC diğer QoL sistemlerinden izole tutulacak. Her faz tek başına derlenebilir ve test edilebilir durumda bitirilecek.

## Test stratejisi

Normal oynanışta saatlerce ilerleme gerektiren testler kabul edilmeyecek.

Her faz için debug/test build kullanılacak ve gerekli koşullar hızlıca hazırlanacak. Test build yalnızca doğrulama içindir ve final ROM'a debug menüsü, warp kolaylığı, ücretsiz item, badge/flag hilesi veya benzeri geliştirme aracı taşınmayacaktır.

Her fazın kabul kriteri üç parçalıdır:

1. İlgili özelliğin debug test build ile hızlı fonksiyonel testi.
2. Mevcut eski `.srm` ile save uyumluluk testi.
3. Release build'de debug araçlarının bulunmadığının doğrulanması.
