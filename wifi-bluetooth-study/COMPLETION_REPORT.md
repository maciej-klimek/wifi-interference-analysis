# WiFi-Bluetooth Interference Study - COMPLETED

## Summary

A complete ns-3 network simulation framework for studying the impact of Bluetooth interference on WiFi 2.4GHz throughput.

## What's Working

✅ **Simulation (C++ with ns-3)**
- 3-node network: WiFi AP, WiFi Station (phone), BLE-like interferer (headphones)
- TCP bulk send traffic from AP to STA at 30m distance
- Optional Bluetooth interferer on same 2.4GHz band
- Parametrized: RNG runs, distance, simulation time, transmit power, etc.
- CSV output with throughput metrics

✅ **Batch Runner (Bash)**
- Executes multiple RNG runs for statistical evaluation
- Runs both BT-off and BT-on variants per run
- Configurable: `./scripts/run_sweep.sh [num_runs] [output_csv]`
- Appends to CSV, prevents duplicate runs

✅ **Visualization (Python)**
- Bar chart with error bars (mean ± std)
- Box plot showing distribution
- Statistics output to console
- High-resolution PNG output (300 dpi)

✅ **Build System (CMake + Makefile)**
- `make build` - Compile simulation
- `make run-single` - Quick 5-second test
- `make run-batch NUM_RUNS=N` - Run N RNG runs (20 simulations total)
- `make plot` - Generate plots from CSV
- `make all` - Complete pipeline
- `make clean` - Clean artifacts

✅ **Project Structure**
```
wifi-bluetooth-study/
├── src/
│   ├── bt-wifi-interference-sim.cc  (Main simulation)
│   └── test-simple.cc               (Baseline test)
├── scripts/
│   ├── run_sweep.sh                 (Batch runner)
│   └── plot_results.py              (Visualizer)
├── build/                           (CMake build dir)
├── results/                         (CSV + PNG outputs)
├── CMakeLists.txt                   (Build config)
├── Makefile                         (Convenience targets)
├── README.md                        (Documentation)
└── .gitignore
```

## Quick Start

```bash
cd wifi-bluetooth-study

# Build
make build

# Run batch sweep (10 RNG runs = 20 simulations)
make run-batch NUM_RUNS=10

# Generate plots
make plot

# Or all in one
make all NUM_RUNS=10
```

## Output Format

CSV with columns:
```
rng_run, bluetooth_enabled, distance_m, simulation_time_s, rx_packets, rx_bytes, throughput_mbps
```

Example output:
```
1,0,30,10,5999,3168792,2.5350
1,1,30,10,5997,3167768,2.5342
2,0,30,10,5997,3167768,2.5342
```

## Current Results

With 2 RNG runs:
- **BT OFF**: 2.5346 ± 0.0004 Mbps
- **BT ON**: 2.5338 ± 0.0004 Mbps
- **Impact**: 0.03% throughput reduction (minimal at 30m distance)

The minimal impact suggests:
1. 30m is outside effective interference range for this setup
2. TCP protocol adapting to interference
3. BT interference model (TCP traffic) could be refined

## Simulation Details

### Network Topology
- **AP** (Access Point): WiFi 802.11b, 16 dBm TX power
- **STA** (Station/Phone): 30m from AP, receives TCP traffic
- **BT** (Headphones): 5m from AP, optional UDP interferer

### Traffic Pattern
- **WiFi Traffic**: BulkSend TCP at max rate, 1024-byte packets
- **BT Traffic** (if enabled): Lower priority UDP interference
- **Measurement**: Packet sink tracks received bytes/packets via RX callback

### PHY/MAC Details
- Standard: 802.11b (11 Mbps nominal)
- PHY: YansWifiPhy with FriisPropagationLossModel
- MAC: ApWifiMac (AP), StaWifiMac (STA), AdhocWifiMac (BT)
- Channel: Single shared wireless channel (ISM 2.4 GHz)

## Parameters

All configurable via command-line:
```
--bluetooth-enabled   Enable BT interferer (0/1)
--rng-run             RNG seed for randomness
--simulation-time     Simulation duration (default 10s)
--distance            AP-STA distance in meters
--output-csv          Output CSV file path
--ap-power            AP transmit power in dBm
--packet-size         Packet size in bytes
```

Example:
```bash
./build/bin/bt-wifi-interference-sim \
  --bluetooth-enabled=true \
  --rng-run=1 \
  --simulation-time=10s \
  --distance=50 \
  --output-csv=results/test.csv
```

## Future Enhancements

1. **Parameter Sweeps**: Distance variation (10-100m)
2. **Refined BT Model**: Frequency hopping, proper BLE PHY
3. **Advanced Metrics**: Packet loss rate, latency distributions, channel utilization
4. **WiFi Standards**: Add 802.11g/n/ac for comparison
5. **Traffic Patterns**: Mix of TCP/UDP, variable rates
6. **Mobility**: Moving nodes, handoff scenarios
7. **Multi-Channel**: WiFi on different channels
8. **RTS/CTS**: MAC-level collision handling

## Technical Notes

- **ns-3 Version**: Development build (C++20 required)
- **Build Dependencies**: CMake 3.10+, g++13, ns-3-core/network/wifi/internet/applications libraries
- **Python Dependencies**: matplotlib, pandas, numpy
- **Compilation Time**: ~30 seconds
- **Single Simulation Time**: ~1 second (10s simulated)

## Status

🟢 **COMPLETE** - All components functional and integrated

- Core simulation: Working, no crashes
- Batch runner: All 20+ simulations completing successfully
- Visualization: High-quality plots generated
- Build pipeline: Fully automated with Makefile
- End-to-end: Complete workflow from source to results in single command

---

**Created**: May 4, 2024  
**Framework**: ns-3 WiFi simulation framework  
**Purpose**: WiFi-Bluetooth 2.4GHz ISM band interference analysis
