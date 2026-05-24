# Sekcja 3. Results: wpływ interferencji Bluetooth na wydajność WiFi 2.4 GHz

Data: 2026-05-24

Autorzy: do uzupełnienia

## Cel sekcji

Ta część raportu pokazuje, jak zmienia się przepustowość WiFi w paśmie 2.4 GHz, gdy obok telefonu działa pobliskie urządzenie Bluetooth modelowane jako burstowy, częstotliwościowo przełączający się interferer. Celem nie jest odtworzenie pełnego stosu Bluetooth, tylko odpowiedź na pytanie praktyczne: czy i o ile spada wydajność po dodaniu zakłóceń zbliżonych do realnych słuchawek TWS.

Wyniki analizujemy dwoma uzupełniającymi się seriami:

1. główny sweep z 20 RNG runami (`rng_run = 1..20`), który zawiera także jeden silny outlier,
2. kontrolny sweep z 11 RNG runami (`rng_run = 145..155`), który sprawdza stabilność modelu w innym fragmencie przestrzeni losowości.

Ta struktura jest ważna, bo sama średnia nie wystarcza do oceny eksperymentu systemowego. Potrzebne są też rozrzut, mediana, przedziały ufności i analiza punktów odstających.

## 3.1 Główna seria: 20 RNG runów

### Dlaczego ten wykres jest potrzebny

Wykres porównawczy pokazuje efekt pierwszego rzędu: średnią przepustowość dla BT off i BT on oraz odchylenie standardowe. To jest najkrótsza odpowiedź na pytanie, czy interferencja ma wpływ na wydajność.

### Figura

- [Porównanie średniej przepustowości](results/bluetooth/figures-sweep-1-20/throughput_comparison.svg)

### Co pokazuje

- BT off utrzymuje bardzo stabilną przepustowość blisko 193 Mbps.
- BT on obniża średnią do około 148 Mbps.
- Spadek średniej wynosi około 23.02%.
- Wariancja po stronie BT on jest dużo większa niż po stronie BT off.

### Liczby

| Scenariusz | n | Średnia [Mbps] | Odch. std. [Mbps] | Mediana [Mbps] | 95% CI dla średniej |
| --- | ---: | ---: | ---: | ---: | ---: |
| BT off | 20 | 192.839 | 0.077 | 192.837 | ±0.034 |
| BT on | 20 | 148.449 | 34.367 | 157.952 | ±15.062 |

### Interpretacja

BT off jest praktycznie płaski, co wskazuje, że sama sieć WiFi i model ruchu wprowadzają niewielką zmienność wyników. W tej serii bazowy przebieg jest więc stabilny względem losowości modelu.

Po włączeniu BT rozrzut rośnie gwałtownie. To sugeruje, że interferencja jest silnie zależna od konkretnego układu hopów i czasów burstów względem ramek WiFi. Innymi słowy, nie chodzi wyłącznie o „średnią moc zakłóceń”, ale o fazowanie w czasie i częstotliwości.

W tej serii występuje też jeden bardzo silny outlier: dla `rng_run = 9` throughput BT on spada do około 0.2212 Mbps. Ten wynik jest reprodukowalny, więc nie jest to błąd zapisu CSV, tylko skrajny przypadek modelu. Dla raportu warto go opisać wprost, bo pokazuje, że nawet przy tej samej konfiguracji średnia nie oddaje całej historii.

### Dodatkowy wykres rozkładu

- [Rozkład wyników per run](results/bluetooth/figures-sweep-1-20/throughput_distribution.svg)

### Co pokazuje

- Po stronie BT off punkty są skupione bardzo ciasno wokół mediany.
- Po stronie BT on widać prawie stały „główny klaster” około 158 Mbps oraz jeden ekstremalny punkt blisko zera.
- Mediana BT on jest wyraźnie wyższa niż średnia, co oznacza, że średnia jest zaniżana przez outlier.

### Wniosek z tego wykresu

Ten wykres pokazuje punkty pomiarowe z każdego runu, a więc ujawnia pełny kształt rozkładu danych w danej serii. Widać na nim, że po stronie BT on większość obserwacji skupia się w wąskim zakresie, przy jednoczesnym wystąpieniu pojedynczego punktu odstającego.

### Wykres parowany

- [Porównanie per run](results/bluetooth/figures-sweep-1-20/throughput_paired_runs.svg)

### Co pokazuje

- Każdy punkt BT off ma swoją parę BT on dla tego samego `rng_run`.
- W większości przypadków BT on jest niżej niż BT off, co potwierdza negatywny wpływ interferencji.
- Jedna para odstaje bardzo mocno, co wzmacnia tezę o wrażliwości modelu na fazę losowości.

### Dlaczego ten wykres jest potrzebny

Sam wykres słupkowy pokazuje wyłącznie średnią. Wykres parowany pozwala porównać wartości BT off i BT on dla tego samego `rng_run`, czyli dla tej samej realizacji losowości.

### Wykres zbieżności

- [Zbieżność średniej i 95% CI](results/bluetooth/figures-sweep-1-20/throughput_convergence.svg)

Ten wykres ma cztery panele: skumulowaną średnią BT off, skumulowaną średnią BT on, skumulowaną stratę throughputu w Mbps oraz skumulowaną redukcję procentową. Dzięki temu nie mieszamy różnych skal na jednej osi, a jednocześnie widać zarówno stabilność estymatora, jak i ewolucję efektu interferencji.

### Czy ma tu sens Jain fairness?

W skrócie: jako główna metryka nie. Jain fairness index mierzy równomierność rozkładu zasobu, a nie niepewność statystyczną średniej. Można go jednak wykorzystać pomocniczo jako wskaźnik, jak nierównomiernie rozkłada się throughput między runami.

$$
J(x) = \frac{\left(\sum_i x_i\right)^2}{n \sum_i x_i^2}
$$

Interpretacja byłaby taka: wartości bliższe 1 oznaczają bardziej równy rozkład throughputu, a niższe większą nierównomierność. W tym projekcie metryka Jain może być dodatkiem, ale nie zastępuje confidence interval ani wykresu zbieżności.

## 3.2 Seria kontrolna: RNG runy 145–155

### Dlaczego ten zestaw jest potrzebny

Ta seria służy jako kontrola stabilności. Chodzi o sprawdzenie, czy wyniki z głównej serii nie są przypadkiem zdominowane przez pechowy fragment przestrzeni losowości.

### Figura

- [Porównanie średniej przepustowości](results/bluetooth/figures-sweep-145-155/throughput_comparison.svg)
- [Rozkład wyników per run](results/bluetooth/figures-sweep-145-155/throughput_distribution.svg)
- [Porównanie per run](results/bluetooth/figures-sweep-145-155/throughput_paired_runs.svg)
- [Zbieżność średniej i 95% CI](results/bluetooth/figures-sweep-145-155/throughput_convergence.svg)

### Liczby

| Scenariusz | n | Średnia [Mbps] | Odch. std. [Mbps] | Mediana [Mbps] | 95% CI dla średniej |
| --- | ---: | ---: | ---: | ---: | ---: |
| BT off | 11 | 192.873 | 0.058 | 192.883 | ±0.034 |
| BT on | 11 | 157.874 | 0.264 | 157.767 | ±0.156 |

### Interpretacja

W tej serii BT on nadal obniża throughput, ale zachowanie jest znacznie bardziej zwarte niż w głównym sweepie. Nie pojawia się prawie zerowy outlier. To ważna obserwacja, bo pokazuje, że model nie jest „zawsze ekstremalny” i że skrajny przypadek z `rng_run = 9` należy traktować jako szczególną, ale dopuszczalną realizację modelu.

### Co to mówi o wiarygodności

Kontrolny sweep pokazuje ten sam kierunek zmiany: BT on obniża przepustowość, a BT off pozostaje stabilny. W porównaniu z główną serią nie występuje ekstremalny punkt odstający.

Wykres zbieżności pokazuje, że po kilku pierwszych próbach estymacja średniej BT on stabilizuje się w okolicach 158 Mbps, a zakres 95% CI szybko się zawęża. To wskazuje, że w kontrolnej serii liczba prób jest już wystarczająca do opisu efektu bez silnej wrażliwości na pojedynczy seed.

## 3.3 Podsumowanie statystyczne

### Główne obserwacje

1. Interferencja Bluetooth obniża przepustowość WiFi w obu badanych seriach.
2. BT off ma bardzo niski rozrzut, więc bazowy system WiFi jest stabilny.
3. BT on ma wyraźnie większy rozrzut, a więc wynik zależy od zbiegu hopów, burstów i ramek WiFi.
4. W głównej serii występuje ciężki outlier, który obniża średnią bardziej niż medianę.
5. Seria kontrolna pokazuje podobny kierunek efektu, ale bez ekstremalnego zjazdu do zera.

### Która metryka jest najuczciwsza

W tym eksperymencie sama średnia jest za słaba. Najlepiej patrzeć jednocześnie na:

- średnią, bo pokazuje przeciętny efekt,
- medianę, bo jest odporna na outliery,
- odchylenie standardowe, bo mówi o stabilności,
- 95% CI, bo pokazuje niepewność estymacji,
- wykres parowany, bo ujawnia wpływ tego samego seed-a na oba warianty.

## 3.4 Co warto jeszcze dodać przed finalnym oddaniem

Żeby sekcja Results była jeszcze mocniejsza, warto rozważyć dodanie kilku rzeczy:

1. **Histogram lub violin plot różnic BT off - BT on**. Pokazałby rozkład bezwzględnej straty przepustowości.
2. **Tabela z minimum, maksimum i medianą** dla każdego scenariusza. To dobrze uzupełnia średnią i CI.
3. **Bootstrapped confidence intervals** dla różnicy średnich. Dałoby to bardziej odporne oszacowanie niepewności niż samo przybliżenie normalne.
4. **Sweep po odległości lub mocy BT**. To byłby już wynik typu sensitivity analysis i bardzo dobrze wyglądałby w raporcie o wydajności systemów.
5. **Zaznaczenie outlierów na wykresie**. Wtedy łatwiej wyjaśnić, dlaczego średnia BT on jest niższa niż mediana.
6. **Wykres zbieżności średniej i 95% CI**. Ten wykres pokazuje, jak zmienia się estymacja wraz z kolejnymi próbami.

## 3.5 Krótki wniosek do tej sekcji

W badanym układzie obecność pobliskiego, burstowego i hopującego interferera Bluetooth wyraźnie pogarsza wydajność WiFi 2.4 GHz. Efekt jest realny statystycznie i widoczny w obu seriach, ale jego skala zależy od seed-a i może mieć ciężki ogon. Dlatego w dalszej części raportu trzeba omawiać nie tylko średnią, ale też rozrzut i odporność wyników.

> Uwaga organizacyjna: ten dokument obejmuje wyłącznie eksperyment symulacyjny ns-3. Część o stanowisku fizycznym oraz osobny eksperyment z zakłócaniem mikrofalówką będą opisane w odrębnych sekcjach raportu.