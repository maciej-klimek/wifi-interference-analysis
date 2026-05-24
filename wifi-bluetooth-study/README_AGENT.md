# Agent Notes

Krótki stan projektu do szybkiego powrotu.

## O co tu chodzi
To jest projekt o wpływie interferencji w paśmie 2.4 GHz na wydajność WiFi. W repo są dwa główne eksperymenty symulacyjne w ns-3:
- Bluetooth/WiFi interference study
- Microwave/WiFi interference study

Każdy eksperyment ma trzy warstwy:
- symulacja C++ w `src/`
- batch runner w `scripts/`
- plotter w `scripts/`

Wyniki trafiają do `results/` i są rozdzielone na osobne katalogi dla Bluetooth i mikrofalówki.

## Cel pracy
- Porównać wpływ różnych źródeł zakłóceń na throughput WiFi.
- Utrzymać wyniki w formie powtarzalnych sweepów po `rng_run`.
- Mieć wykresy i CSV gotowe do raportu.

## Forma raportu
- Pliki wynikowe i wykresy są podstawą do sekcji raportu.
- `REPORT_RESULTS.md` i `REPORT_EVALUATION_SETUP.md` zawierają wersje robocze opisu.
- Nie dopisuj do nich odniesień do PDF-a o realnym eksperymencie mikrofalowym; był tylko referencją roboczą.

## Jak myśleć o danych
- BT jest teraz ustawione na zakres `rng_run 145..165`.
- Mikrofalówka ma model pulsacyjny, bo model ciągły dawał zbyt ekstremalne tłumienie.
- Jeśli wynik wygląda zbyt skrajnie, najpierw sprawdź moc, duty cycle i okres emisji.

## Zalecenia dla agenta
- Najpierw sprawdź, czy zmiana dotyczy BT czy mikrofalówki.
- Przed edycją pliku sprawdź, czy są aktualne wyniki w `results/`.
- Jeśli zmieniasz model symulacji, od razu uruchom build i jeden krótki run.
- Jeśli zmieniasz batch albo plot, porównaj ścieżki z `Makefile`.
- Usuwaj stare artefakty tylko wtedy, gdy nowy zakres danych już jest wygenerowany.

## Co tu jest
- `src/bt-wifi-interference-sim.cc` - symulacja Bluetooth/WiFi.
- `src/microwave-interference-sim.cc` - symulacja mikrofalówki.
- `scripts/run_bluetooth_sweep.sh` - batch dla BT.
- `scripts/plot_bluetooth_results.py` - wykresy dla BT.
- `scripts/run_microwave_sweep.sh` - batch dla mikrofalówki.
- `scripts/plot_microwave_results.py` - wykresy dla mikrofalówki.
- `results/bluetooth/` - aktualne dane BT.
- `results/microwave/` - aktualne dane mikrofalówka.

## Aktualny stan
- BT workflow jest ustawiony na zakres `rng_run 145..165`.
- Obecny plik BT CSV: `results/bluetooth/s24-liberty4-sweep-145-165.csv`.
- Stare BT CSV-y zostały usunięte z repo.
- Mikrofalówka działa jako model pulsacyjny, nie ciągły.
- Domyślne wyniki mikrofalówki są w `results/microwave/microwave-sweep.csv`.

## Najważniejsze komendy
- `make build`
- `make run-bt-batch NUM_RUNS=21 START_RUN=145`
- `make plot-bt`
- `make run-microwave-batch NUM_RUNS=3`
- `make plot-microwave`
- `make clean`

## Uwaga
- `BWS__Projekt2.pdf` jest lokalnie w `.gitignore` i nie powinien wracać do repo.
- Jeśli trzeba wrócić do raportu, patrz: `REPORT_RESULTS.md` i `REPORT_EVALUATION_SETUP.md`.
- Jeśli trzeba wrócić do nowych danych, najpierw sprawdź `results/bluetooth/` i `results/microwave/`.
