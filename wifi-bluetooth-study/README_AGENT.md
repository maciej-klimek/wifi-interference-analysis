README — WiFi vs Bluetooth Interference Study (technical)
=========================================================

Cel
----
Krótko: zmierzyć wpływ interferencji Bluetooth (model uproszczony) na przepustowość WiFi 2.4GHz.
Repozytorium zawiera: kod symulacji ns-3, skrypt batchowy do wielorunów oraz skrypt do wizualizacji wyników.

Główne pliki i ich rola
------------------------
- `src/bt-wifi-interference-sim.cc` — główny program symulacji (ns-3, C++).
  - Tworzy 3 węzły: AP, STA (telefon), BT (słuchawki).
  - Konfiguruje PHY/MAC (802.11b, YansWifiPhy, FriisPropagationLossModel).
  - Konfiguruje ruch aplikacyjny:
    - AP -> STA: `OnOffHelper` używający `ns3::UdpSocketFactory` (UDP), parametryzowany parametrem `--data-rate`.
    - BT interferer (opcjonalny): `OnOffHelper` UDP wysyłający na inny port (utrudnia odbiór WiFi poprzez wspólny kanał).
  - Mierzy przyjęte bajty/pakiety w `PacketSink` i zapisuje wiersz do pliku CSV.
  - Kluczowe argumenty (CommandLine):
    - `--bluetooth-enabled` (bool): włącza interferer (domyślnie false)
    - `--rng-run` (uint32): seed / numer powtórzenia
    - `--simulation-time` (Time): czas symulacji (np. `10s`)
    - `--distance` (double): odległość AP–STA w metrach
    - `--output-csv` (string): ścieżka do pliku wynikowego CSV
    - `--data-rate` (string): oferta AP (np. `150Mbps`)
    - `--bt-data-rate` (string): oferta BT (np. `500Kbps`)

- `scripts/run_sweep.sh` — runner bashowy do uruchomień statystycznych.
  - Wejście: liczba powtórzeń (NUM_RUNS) i plik CSV wyjściowy.
  - Dla każdego powtórzenia uruchamia symulację dwukrotnie: BT OFF i BT ON.
  - Piszemy wyniki do jednego CSV (append).
  - Przykład: `bash scripts/run_sweep.sh 10 results/sweep_results.csv`

- `scripts/plot_results.py` — skrypt do wczytania CSV i wygenerowania wykresów.
  - Wymaga: `matplotlib`, `pandas`, `numpy` (używamy wirtualnego środowiska w repo: `../.venv/bin/python` w Makefile).
  - Generuje: `throughput_comparison.png` (bar + error bars) i `throughput_distribution.png` (boxplot).
  - Użycie: `python scripts/plot_results.py --csv results/sweep_results.csv --output-dir results`

- `Makefile` — ułatwiający workflow:
  - `make build` — buduje (CMake + make)
  - `make run-batch NUM_RUNS=N` — uruchamia `scripts/run_sweep.sh` (N powtórzeń)
  - `make plot` — generuje wykresy używając venv (`../.venv/bin/python`)
  - `make all NUM_RUNS=N` — build + run-batch + plot

Wyniki (format CSV)
--------------------
Każdy wiersz ma kolumny:
```
rng_run,bluetooth_enabled,distance_m,simulation_time_s,rx_packets,rx_bytes,throughput_mbps
```
- `throughput_mbps` obliczane jest z `rx_bytes / simulation_time`.
- `bluetooth_enabled`: 0 = wyłączony, 1 = włączony

Uruchomienie symulacji z ~150 Mbps oferty
-----------------------------------------
Poniżej przykład uruchomienia pojedynczego przebiegu z AP oferującym 150 Mbps:

```bash
cd wifi-bluetooth-study
# build jeśli nie zbudowane
make build
# uruchomienie jednej symulacji, AP wysyła UDP z ofertą 150Mbps
./build/bin/bt-wifi-interference-sim \
  --bluetooth-enabled=false \
  --rng-run=1 \
  --simulation-time=5s \
  --distance=30 \
  --data-rate=150Mbps \
  --output-csv=results/wifi-bluetooth-results.csv
```

Uwaga: `--data-rate` to oferowana szybkość (aplikacyjna). Fizyczna przepustowość ograniczona jest przez: standard WiFi (802.11b nominalnie do 11 Mbps), odległość, model propagacji, kolizje i mechanizmy MAC — więc zaoferowanie 150Mbps nie oznacza, że zostanie odebrane 150Mbps.

Jak interpretować wyniki
-------------------------
- Jeżeli zaoferowana szybkość >> możliwość PHY, obserwujemy saturację i rzeczywista `throughput_mbps` będzie niższa.
- Porównując BT off/on patrz na średnie i odchylenia standardowe z wielu RNG-runów.

Przykładowy eksperyment — krok po kroku
--------------------------------------
1. Zbuduj:

```bash
make build
```

2. Szybki test (pojedynczy przebieg, niska długość):

```bash
make run-single
```

3. Batch sweep (statystyka):

```bash
make run-batch NUM_RUNS=20
```

4. Wygeneruj wykresy:

```bash
make plot
# lub ręcznie (venv):
../.venv/bin/python scripts/plot_results.py --csv results/sweep_results.csv --output-dir results
```

5. Otwórz `results/throughput_comparison.png` i `results/throughput_distribution.png`.

Wskazówki dla kolejnego agenta / dewelopera
------------------------------------------
- Kod symulacji jest celowo prosty: AP i BT działają jako zwykłe generatory UDP, co upraszcza analizę wpływu oferty interferenta.
- Jeśli chcesz modelować Bluetooth dokładniej, rozważ:
  - modelowanie częstotliwości (FHSS) i krótkich okresów transmisji,
  - implementację burstów zamiast ciągłego OnOff,
  - bardziej wierne oddanie BLE (protokół link layer) — to wymagało by rozbudowy modułu fizycznego lub użycia innego symulatora.
- Jeśli planujesz duże numery przebiegów (np. >100), użyj batchowania i rozważ równoległe uruchamianie na klastrze.

Dalsze kroki / rozszerzenia
---------------------------
- Dodanie parametru kanału WiFi i skanowania kanałów
- Sweep odległości (np. 5,10,20,30 m)
- Zmiana standardu WiFi (802.11g/n) do porównań
- Raportowanie dodatkowych metryk: opóźnienie, jitter, utracone pakiety

Kontakt / notatki
------------------
Pliki projektu: `/home/martin/Repos/wifi-interference-analysis/wifi-bluetooth-study/`

Jeśli chcesz, mogę:
- uruchomić batch z `--data-rate=150Mbps` i przygotować wykresy,
- dodać automatyczny sweep po odległości i ofercie BT,
- zaimplementować UDP rate-limiter lub kontrolę pakietów dla bardziej realistycznego modelu BT.

Plan dla kolejnego agenta (priorytetowe zadania):
1. Implementować bardziej realistyczny model Bluetooth (docelowo):
  - zaimplementować FHSS-like behavior lub użyć modułu Spectrum do modelowania zakłóceń na poziomie spektrum,
  - modelować krótkie bursty reklamacyjne i połączeniowe zgodne z BLE (timing, duty-cycle),
  - uwzględnić różne profile mocy i adaptacyjne zachowanie (burst power, duty-cycle variability),
  - porównać wpływ raw FHSS vs. prostych burstów na przepustowość WiFi.
2. Dodać eksperymenty porównawcze: 802.11g vs 802.11n vs 802.11ac (jeśli chcesz testować 5GHz też),
3. Zautomatyzować parametryzację i walidację wyników (skrypty do analizy statystycznej, CI dla eksperymentów).

