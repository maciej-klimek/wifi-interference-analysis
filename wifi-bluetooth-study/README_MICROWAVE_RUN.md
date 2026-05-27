# Quick Run: Microwave Simulation (ns-3)

Krótka instrukcja uruchamiania symulacji mikrofalówki i generowania wykresów.
Zakładam, że jesteś w katalogu `wifi-bluetooth-study` (repo root: one level up).

**Ważne:** korzystaj z wirtualnego środowiska, żeby mieć stabilne zależności.

## 1) Aktywacja venv
Jeśli masz venv w repo root (jak w projekcie), aktywuj go przy wejściu do katalogu:

```bash
# będąc w katalogu wifi-bluetooth-study
source ../.venv/bin/activate
```

Jeśli `../.venv` nie istnieje na nowym komputerze — utwórz je i zainstaluj zależności z `requirements.txt` (plik znajduje się w tym folderze `wifi-bluetooth-study`):

```bash
# będąc w katalogu wifi-bluetooth-study
# utwórz venv w katalogu repo root (../.venv)
python3 -m venv ../.venv
source ../.venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt
```

Alternatywnie, jeśli chcesz wywołać Pythona bez aktywacji:

```bash
../.venv/bin/python scripts/plot_microwave_results.py --csv results/microwave/microwave-sweep.csv
```

## 2) Build (CMake)
Zbuduj wykonywalne pliki (BT i microwave):

```bash
# będąc w katalogu wifi-bluetooth-study
make build
# lub (ręcznie)
# mkdir -p build && cd build && cmake .. && make
```

Spodziewane pliki: `build/bin/microwave-interference-sim` i `build/bin/bt-wifi-interference-sim`.

## 3) Szybki test (pojedynczy run)
Uruchom krótki test pojedynczy, aby sprawdzić poprawność działania:

```bash
# przykład jednego runu
./build/bin/microwave-interference-sim --rng-run=1 --output-csv=/tmp/mw-test.csv
# sprawdź podsumowanie
head -n 5 /tmp/mw-test.csv
```

Jeśli chcesz zmodyfikować parametry (moc, okres, duty-cycle):

```bash
./build/bin/microwave-interference-sim --rng-run=1 --mw-power-dbm=-10 --mw-period-s=0.05 --mw-duty-cycle=0.35 --output-csv=/tmp/mw-test.csv
```

## 4) Batch sweep (wiele RNG runów)
Przykład uruchomienia batcha (tworzy `results/microwave/microwave-sweep.csv`):

```bash
# uruchom 10 runów i zapisz do results/microwave/
NUM_RUNS=10 OUTPUT_CSV=results/microwave/microwave-sweep.csv ./scripts/run_microwave_sweep.sh

# lub używając Makefile (domyślnie START_RUN=1):
make run-microwave-batch NUM_RUNS=10
```

Możesz nadpisać parametry emisji z env:

```bash
# przykład z zmienioną mocą i duty cycle
NUM_RUNS=10 MW_POWER_DBM=-10 MW_PERIOD_S=0.05 MW_DUTY_CYCLE=0.35 OUTPUT_CSV=results/microwave/microwave-sweep.csv ./scripts/run_microwave_sweep.sh
```

## 5) Generowanie wykresów
W virtualenv (zalecane) uruchom plotter:

```bash
# będąc w katalogu wifi-bluetooth-study
./scripts/plot_microwave_results.py results/microwave/microwave-sweep.csv
# lub explicit
../.venv/bin/python scripts/plot_microwave_results.py --csv results/microwave/microwave-sweep.csv --output-dir results/microwave
```

Wynik: `results/microwave/throughput_time_series.(png|svg)` i `throughput_phase_summary.(png|svg)`.

## 6) Przydatne wskazówki
- Jeśli widzisz totalny blackout podczas `microwave-on`, sprawdź `--mw-power-dbm`, `--mw-period-s`, `--mw-duty-cycle` i pozycję (`--microwave-x-m`, `--microwave-y-m`).
- Przed kasowaniem starych CSV zawsze wygeneruj nowy batch i porównaj.
- Makefile ma targety: `make build`, `make run-microwave-batch`, `make plot-microwave`.

## 7) Szybkie komendy (skróty)

```bash
# build
make build

# run small batch
NUM_RUNS=3 OUTPUT_CSV=results/microwave/microwave-sweep.csv ./scripts/run_microwave_sweep.sh

# plot
./scripts/plot_microwave_results.py results/microwave/microwave-sweep.csv
```

---
Plik utworzony jako szybka referencja — trzymany obok [README_AGENT.md].
