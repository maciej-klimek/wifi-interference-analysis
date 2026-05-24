#!/bin/bash

###############################################################################
# WiFi-Microwave Interference Study - Batch Runner
#
# This script runs the microwave simulation multiple times with different RNG
# seeds and collects results into a CSV file for statistical analysis.
#
# Usage:
#   ./scripts/run_microwave_sweep.sh [num_runs] [output_csv] [device_profile] [start_run]
# Environment overrides are also supported, for example:
#   NUM_RUNS=10 OUTPUT_CSV=results/microwave/microwave-sweep.csv ./scripts/run_microwave_sweep.sh
#
# Example:
#   ./scripts/run_microwave_sweep.sh 10 results/microwave/microwave-sweep.csv kitchen-microwave 1
###############################################################################

set -e

NUM_RUNS=${NUM_RUNS:-${1:-10}}
OUTPUT_CSV=${OUTPUT_CSV:-${2:-results/microwave/microwave-sweep.csv}}
DEVICE_PROFILE=${DEVICE_PROFILE:-${3:-kitchen-microwave}}
START_RUN=${START_RUN:-${4:-1}}
SIMULATION_TIME=${SIMULATION_TIME:-12s}
BIN_WIDTH=${BIN_WIDTH:-100ms}
MICROWAVE_ENABLED=${MICROWAVE_ENABLED:-true}
MW_ON_START=${MW_ON_START:-4.0}
MW_ON_STOP=${MW_ON_STOP:-8.0}
MW_POWER_DBM=${MW_POWER_DBM:-0.0}
MW_CENTER_FREQ_MHZ=${MW_CENTER_FREQ_MHZ:-2450.0}
MW_BANDWIDTH_MHZ=${MW_BANDWIDTH_MHZ:-20.0}
SIM_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NS3_DIR="${SIM_DIR}/../ns3"
BUILD_DIR="${SIM_DIR}/build"
BIN_DIR="${BUILD_DIR}/bin"
EXECUTABLE="${BIN_DIR}/microwave-interference-sim"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}WiFi-Microwave Interference Study - Batch Runner${NC}"
echo "========================================================"
echo "Number of runs: $NUM_RUNS"
echo "Output CSV: $OUTPUT_CSV"
echo "Device profile: $DEVICE_PROFILE"
echo "Start RNG run: $START_RUN"
echo "Simulation time: $SIMULATION_TIME"
echo "Bin width: $BIN_WIDTH"
echo "Microwave enabled: $MICROWAVE_ENABLED"
echo "Microwave window: ${MW_ON_START}s .. ${MW_ON_STOP}s"
echo "Microwave power: ${MW_POWER_DBM} dBm"
echo "Microwave center/bandwidth: ${MW_CENTER_FREQ_MHZ} MHz / ${MW_BANDWIDTH_MHZ} MHz"
echo "Simulation dir: $SIM_DIR"
echo "NS3 dir: $NS3_DIR"
echo ""

if [ ! -d "${NS3_DIR}/build" ]; then
    echo -e "${RED}Error: ns3 not built at ${NS3_DIR}/build${NC}"
    echo "Please build ns3 first:"
    echo "  cd $NS3_DIR"
    echo "  ./ns3 configure --enable-examples"
    echo "  ./ns3 build"
    exit 1
fi

if [ ! -f "${EXECUTABLE}" ]; then
    echo -e "${YELLOW}Building simulation...${NC}"
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    cmake -DPython3_EXECUTABLE=$(which python3) ..
    make
    echo -e "${GREEN}Build successful!${NC}"
    echo ""
fi

mkdir -p "$(dirname "$OUTPUT_CSV")"
rm -f "${OUTPUT_CSV}"

echo -e "${YELLOW}Running simulations...${NC}"
echo ""

END_RUN=$((START_RUN + NUM_RUNS - 1))

for run in $(seq "${START_RUN}" "${END_RUN}"); do
    CURRENT_INDEX=$((run - START_RUN + 1))
    echo -ne "Run ${run} (${CURRENT_INDEX}/${NUM_RUNS})... "
    "${EXECUTABLE}" \
        --device-profile="${DEVICE_PROFILE}" \
        --microwave-enabled="${MICROWAVE_ENABLED}" \
        --rng-run="$run" \
        --simulation-time="${SIMULATION_TIME}" \
        --bin-width="${BIN_WIDTH}" \
        --mw-on-start-s="${MW_ON_START}" \
        --mw-on-stop-s="${MW_ON_STOP}" \
        --mw-power-dbm="${MW_POWER_DBM}" \
        --mw-center-frequency-mhz="${MW_CENTER_FREQ_MHZ}" \
        --mw-bandwidth-mhz="${MW_BANDWIDTH_MHZ}" \
        --output-csv="${OUTPUT_CSV}"
    echo -e "${GREEN}✓${NC}"
done

echo ""
echo -e "${GREEN}All simulations completed!${NC}"
echo "Results saved to: $OUTPUT_CSV"
echo ""

if [ -f "${OUTPUT_CSV}" ]; then
    echo "CSV Preview (first 5 lines):"
    head -6 "${OUTPUT_CSV}" | column -t -s','
    echo ""
    echo "Total rows: $(tail -n +2 "${OUTPUT_CSV}" | wc -l)"
fi
