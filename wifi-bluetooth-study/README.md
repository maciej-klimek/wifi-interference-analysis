# WiFi vs Bluetooth Interference Study

This project is a controlled ns-3 experiment for measuring how a nearby Bluetooth-like source changes WiFi throughput in the 2.4 GHz band. The emphasis is on a reproducible simulation pipeline: build the model, run many seeded simulations, write a CSV, and turn that CSV into plots.

## What The Simulation Models

The current simulator has three nodes:

- AP: WiFi transmitter that sends UDP traffic toward the phone.
- Phone / STA: receives the WiFi traffic and is the measured endpoint.
- Bluetooth device: modeled as a nearby spectrum-level interferer on the same 2.4 GHz channel.

The WiFi side uses `SpectrumWifiPhy` so that in-band foreign energy can affect reception at the PHY. The Bluetooth side is not a full Bluetooth stack yet; it is a jammer-style waveform generator that injects energy into the same 2.4 GHz spectrum. That is the part to keep in mind when interpreting the results: it is a physical-layer coexistence experiment, not a protocol-complete BLE coexistence model.

## Pipeline

The workflow is intentionally linear:

1. Build the simulator.
2. Run one or many seeded simulations.
3. Append one row per run to CSV.
4. Plot throughput statistics from the CSV.

That means every figure in `results/` is derived directly from the simulation output file, not from a separate analysis database or hand-edited spreadsheet.

## Repository Layout

```
wifi-bluetooth-study/
├── src/
│   └── bt-wifi-interference-sim.cc   # Main ns-3 simulation
├── scripts/
│   ├── run_sweep.sh                  # BT off/on batch runner
│   └── plot_results.py               # CSV -> plots
├── results/                          # CSV and generated figures
├── CMakeLists.txt                    # Links against the ns-3 build
├── Makefile                          # Build / run / plot shortcuts
└── README.md                         # This file
```

## Build Requirements

You need:

1. A built ns-3 tree in `../ns3/`.
2. Python 3 with `numpy`, `pandas`, and `matplotlib`.
3. A C++20 compiler and CMake.

Typical ns-3 setup:

```bash
cd ../ns3
./ns3 configure --enable-examples
./ns3 build
```

## How The Devices Are Modeled

### WiFi access point and phone

The AP and phone are connected with WiFi using a `SpectrumWifiPhy` channel on 2.4 GHz. The WiFi standard is selectable from the command line with `--wifi-standard`:

- `802.11b`
- `802.11g`
- `802.11n`

The traffic path is simple and explicit:

- `OnOffHelper` generates UDP from AP to phone.
- `PacketSink` on the phone measures received bytes and packets.
- Throughput is computed from received bytes over simulation time.

### Bluetooth interferer

The Bluetooth device is represented as a foreign signal source on the same spectrum channel. It is placed very close to the phone by default and now hops across the 2.4 GHz ISM band using a 1 MHz spectrum model. It can be tuned with:

- `--bt-distance`
- `--bt-power-dbm`
- `--bt-burst-on-ms`
- `--bt-burst-period-ms`
- `--bt-hop-dwell-us`
- `--bt-hop-count`

This model is useful because it actually adds energy into the WiFi receiver’s spectrum path and now changes frequency over time instead of sitting on one fixed carrier. It is still an abstraction, not a faithful BLE link-layer implementation.

Important limitation: real Bluetooth headphones usually behave like a low-power, bursty, adaptive-frequency-hopping device. If the jammer power is set too high, the throughput drop can be much larger than what you would expect from actual headphones. In practice, `--bt-power-dbm` is the easiest knob to use when you want to keep the scenario closer to a realistic headset instead of a strong local interferer.

For calibration against a Samsung S24 + Soundcore Liberty 4 scenario, the simulator includes a preset profile:

- `--device-profile=s24-liberty4`

That preset switches the WiFi side to 802.11ax on 2.4 GHz with a 40 MHz channel, and tunes the Bluetooth side as a nearby TWS headset rather than a strong jammer. The current calibration uses short burst windows, 79 hop channels, 625 us dwell time, a small phone-to-earbud offset, and low transmit power so the interference pattern stays closer to a real earbud link than to a synthetic noise source.

Public product listings for the Liberty 4 family describe it as a Bluetooth 5.3-class true wireless headset with ANC and multipoint support. For this simulator that matters mainly in one way: the headset should be treated as a low-power, bursty, frequency-hopping interferer, not as a continuous transmitter.

What is still not modeled:

- the real BLE/Classic Bluetooth link layer and packet scheduling
- codec-specific behavior such as SBC/AAC/LDAC traffic patterns
- adaptive frequency hopping decisions based on the channel map
- antenna orientation, body shadowing, and hand/head absorption
- the exact coexistence policy inside the phone and earbuds firmware

So the preset is best read as a calibrated interference approximation for an IRL phone + earbuds test, not as a protocol-accurate Liberty 4 emulator.

## Build

```bash
make build
```

Or manually:

```bash
mkdir -p build
cd build
cmake ..
make
```

## Run A Single Simulation

Bluetooth off:

```bash
./build/bin/bt-wifi-interference-sim \
  --bluetooth-enabled=false \
  --rng-run=1 \
  --simulation-time=5s \
  --distance=10 \
  --wifi-standard=802.11n \
  --data-rate=100Mbps
```

Bluetooth on:

```bash
./build/bin/bt-wifi-interference-sim \
  --bluetooth-enabled=true \
  --rng-run=1 \
  --simulation-time=5s \
  --distance=10 \
  --wifi-standard=802.11n \
  --data-rate=100Mbps \
  --bt-burst-on-ms=2 \
  --bt-burst-period-ms=50 \
  --bt-power-dbm=0 \
  --bt-distance=0.0
```

## Batch Sweep

Run paired BT off / BT on experiments across multiple RNG seeds:

```bash
bash scripts/run_bluetooth_sweep.sh 20 results/bluetooth/s24-liberty4-sweep.csv
```

This generates 40 rows total for `NUM_RUNS=20`: one BT-off and one BT-on row per seed.

## Plotting

Generate the figures from the CSV:

```bash
python3 scripts/plot_bluetooth_results.py --csv results/bluetooth/s24-liberty4-sweep.csv --output-dir results/bluetooth
```

The plotting script currently writes:

- `results/bluetooth/throughput_comparison.png`
- `results/bluetooth/throughput_distribution.png`

The microwave experiment uses the same structure under `results/microwave/` with `scripts/run_microwave_sweep.sh` and `scripts/plot_microwave_results.py`.

The first is a bar chart with error bars. The second is a boxplot of throughput distribution for BT off vs on.

## Command-Line Parameters

Current simulation flags:

- `--bluetooth-enabled`: turn the BT interferer on or off.
- `--rng-run`: select the RNG run / seed.
- `--simulation-time`: total duration, for example `5s`.
- `--distance`: AP-to-phone distance in meters.
- `--output-csv`: path to the output CSV.
- `--data-rate`: offered WiFi application rate, for example `100Mbps`.
- `--wifi-standard`: `802.11b`, `802.11g`, or `802.11n`.
- `--bt-burst-on-ms`: BT jammer ON time per burst.
- `--bt-burst-period-ms`: BT jammer period.
- `--bt-distance`: BT device distance from the phone.
- `--bt-power-dbm`: BT jammer power in dBm. Keep this low if you want a headphone-like scenario.
- `--bt-hop-dwell-us`: dwell time for each Bluetooth hop.
- `--bt-hop-count`: number of hop channels across the 2.4 GHz ISM band.
- `--device-profile`: optional preset such as `s24-liberty4`.
- `--wifi-channel-width-mhz`: WiFi channel width, used by the WiFi preset and 2.4 GHz calibration runs.

The point of these flags is to separate the radio assumptions from the traffic assumptions. `--data-rate` controls offered load, while `--wifi-standard` controls the PHY capability.

## CSV Output

The main results file contains one row per run:

```csv
rng_run,bluetooth_enabled,distance_m,simulation_time_s,rx_packets,rx_bytes,throughput_mbps
```

Column meanings:

- `rng_run`: RNG seed used for the run.
- `bluetooth_enabled`: `0` for BT off, `1` for BT on.
- `distance_m`: AP-to-phone distance.
- `simulation_time_s`: length of the simulation.
- `rx_packets`: packets received by the phone.
- `rx_bytes`: total bytes received.
- `throughput_mbps`: received throughput computed from `rx_bytes`.

## Typical Workflow

1. Build once.
2. Run a single BT off / BT on pair to confirm the model behaves as expected.
3. Run a batch sweep when you want statistics.
4. Plot the CSV and compare mean throughput and spread.

Example:

```bash
make build
./build/bin/bt-wifi-interference-sim --bluetooth-enabled=false --rng-run=1
./build/bin/bt-wifi-interference-sim --bluetooth-enabled=true --rng-run=1
bash scripts/run_sweep.sh 20 results/sweep_results.csv
python3 scripts/plot_results.py --csv results/sweep_results.csv --output-dir results
```

## Notes

- The current BT model is still a controlled jammer approximation, not a protocol-accurate BLE stack.
- The current defaults are meant to resemble a nearby headset only loosely; `--bt-power-dbm` is the main realism knob.
- If your real-world test shows a much larger throughput drop, that does not necessarily mean the code is wrong. It usually means the jammer settings are too aggressive relative to a typical Bluetooth headset.
- The design goal is reproducibility and interpretability first, realism second.
- The next natural step is to calibrate the BT settings against a measured headset trace or a packet-level Bluetooth model.

## References

- ns-3 WiFi module: https://www.nsnam.org/docs/models/html/wifi.html
- ns-3 Spectrum module: https://www.nsnam.org/docs/models/html/spectrum.html
- Bluetooth SIG: https://www.bluetooth.com/specifications/specs/
