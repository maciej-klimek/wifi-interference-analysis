README — WiFi vs Bluetooth Interference Study (technical)
=========================================================

Cel
----
Krótko: zmierzyć wpływ interferencji Bluetooth (model uproszczony) na przepustowość WiFi 2.4GHz.
Repozytorium zawiera: kod symulacji ns-3, skrypt batchowy do wielorunów oraz skrypt do wizualizacji wyników.

Główne pliki i ich rola
------------------------
- `src/bt-wifi-interference-sim.cc` — główny program symulacji (ns-3, C++).
  README — WiFi vs Bluetooth Interference Study (technical)
  =========================================================

  Cel
  ---
  Krótko: zmierzyć wpływ Bluetooth-like interferera na przepustowość WiFi w paśmie 2.4 GHz, z naciskiem na powtarzalny pipeline symulacja -> CSV -> wykresy.

  Główne pliki i ich rola
  -----------------------
  - `src/bt-wifi-interference-sim.cc` — główny program symulacji (ns-3, C++).
    - Tworzy 3 węzły: AP, STA (telefon), BT (słuchawki / jammer).
    - WiFi działa przez `SpectrumWifiPhy` w 2.4 GHz, żeby foreign signal mógł wpłynąć na PHY.
    - AP -> STA: `OnOffHelper` UDP, a odbiór liczy `PacketSink`.
    - BT: hopujący, burstowy interferer na poziomie spektrum, z presetem `s24-liberty4` dla kalibracji.

  - `scripts/run_sweep.sh` — runner bashowy do uruchomień statystycznych.
    - Wejście: liczba powtórzeń (NUM_RUNS) i plik CSV wyjściowy.
    - Dla każdego powtórzenia uruchamia symulację dwukrotnie: BT OFF i BT ON.
    - Aktualnie uruchamia profil bazowy; jeśli chcesz kalibrację pod konkretny sprzęt, użyj `--device-profile` bezpośrednio w binarce.

  - `scripts/plot_results.py` — skrypt do wczytania CSV i wygenerowania wykresów.
    - Wymaga: `matplotlib`, `pandas`, `numpy` (używamy wirtualnego środowiska w repo: `../.venv/bin/python` w Makefile).
    - Generuje: `throughput_comparison.png` i `throughput_distribution.png`.

  - `Makefile` — ułatwiający workflow:
    - `make build` — buduje (CMake + make)
    - `make run-batch NUM_RUNS=N` — uruchamia `scripts/run_sweep.sh` (N powtórzeń)
    - `make plot` — generuje wykresy używając venv (`../.venv/bin/python`)
    - `make all NUM_RUNS=N` — build + run-batch + plot

  Wyniki (format CSV)
  -------------------
  Każdy wiersz ma kolumny:
  ```
  rng_run,bluetooth_enabled,distance_m,simulation_time_s,rx_packets,rx_bytes,throughput_mbps
  ```
  - `throughput_mbps` obliczane jest z `rx_bytes / simulation_time`.
  - `bluetooth_enabled`: 0 = wyłączony, 1 = włączony

  Uruchomienie symulacji z kalibracją pod Samsung S24 + Soundcore Liberty 4
  -------------------------------------------------------------------------
  Poniżej przykład uruchomienia pojedynczego przebiegu z presetem, który najbliżej odwzorowuje realny test:

  ```bash
  ./build/bin/bt-wifi-interference-sim \
    --device-profile=s24-liberty4 \
    --bluetooth-enabled=true \
    --rng-run=1 \
    --simulation-time=5s \
    --distance=10 \
    --output-csv=results/wifi-bluetooth-results.csv
  ```

  W tym presete WiFi jest podbite do 802.11ax na 2.4 GHz z 40 MHz, a BT działa jak bliski, hopujący interferer z niską mocą i burstami.

  Jak interpretować wyniki
  ------------------------
  - Jeżeli zaoferowana szybkość >> możliwość PHY, obserwujemy saturację i rzeczywista `throughput_mbps` będzie niższa.
  - Porównując BT off/on patrz na średnie i odchylenia standardowe z wielu RNG-runów.

  Przykładowy eksperyment — krok po kroku
  ---------------------------------------
  1. Zbuduj:

  ```bash
  make build
  ```

  2. Szybki test w kalibracji pod konkretny telefon/słuchawki:

  ```bash
  ./build/bin/bt-wifi-interference-sim --device-profile=s24-liberty4 --bluetooth-enabled=false --rng-run=1
  ./build/bin/bt-wifi-interference-sim --device-profile=s24-liberty4 --bluetooth-enabled=true --rng-run=1
  ```

  3. Batch sweep i wykresy: użyj `make run-batch` / `make plot` dla baseline albo uruchom własny batch z presetem ręcznie, jeśli zależy Ci na kalibracji pod S24.

  Wskazówki dla kolejnego agenta / dewelopera
  -------------------------------------------
  - Kod symulacji jest już na poziomie spectrum jammera, nie prostego UDP proxy.
  - Jeśli chcesz modelować Bluetooth dokładniej, kolejne sensowne kroki to:
    - dopasowanie mocy i duty cycle do realnych śladów audio,
    - prawdziwszy AFH/FHSS,
    - walidacja przeciw pomiarom z urządzeń referencyjnych.
  - Jeśli planujesz duże numery przebiegów (np. >100), użyj batchowania i rozważ równoległe uruchamianie na klastrze.

  Dalsze kroki / rozszerzenia
  ---------------------------
  - Sweep odległości (np. 5,10,20,30 m)
  - Dalsza kalibracja BT pod pomiary z prawdziwego telefonu i słuchawek
  - Raportowanie dodatkowych metryk: opóźnienie, jitter, utracone pakiety
../.venv/bin/python scripts/plot_results.py --csv results/sweep_results.csv --output-dir results

```
