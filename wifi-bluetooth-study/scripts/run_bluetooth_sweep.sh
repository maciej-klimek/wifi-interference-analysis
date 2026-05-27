#!/bin/bash

###############################################################################
# WiFi-Bluetooth Interference Study - Batch Runner
#
# This script runs the simulation multiple times with different RNG seeds
# and collects results into a CSV file for statistical analysis.
#
# Usage:
#   ./scripts/run_bluetooth_sweep.sh [num_runs] [output_csv] [device_profile] [start_run]
# Environment overrides are also supported, for example:
#   NUM_RUNS=10 OUTPUT_CSV=results/bluetooth/s24-liberty4-sweep-145-165.csv ./scripts/run_bluetooth_sweep.sh
#
# Example:
#   ./scripts/run_sweep.sh 10 results/sweep_results.csv
###############################################################################

set -e

# Configuration
NUM_RUNS=${NUM_RUNS:-${1:-10}}
OUTPUT_CSV=${OUTPUT_CSV:-${2:-results/bluetooth/s24-liberty4-sweep-145-165.csv}}
DEVICE_PROFILE=${DEVICE_PROFILE:-${3:-s24-liberty4}}
START_RUN=${START_RUN:-${4:-1}}
SIMULATION_TIME=${SIMULATION_TIME:-10s}
DISTANCE=${DISTANCE:-3}
SIM_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NS3_DIR="${SIM_DIR}/../ns3"
BUILD_DIR="${SIM_DIR}/build"
BIN_DIR="${BUILD_DIR}/bin"
EXECUTABLE="${BIN_DIR}/bt-wifi-interference-sim"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}WiFi-Bluetooth Interference Study - Batch Runner${NC}"
echo "======================================================"
echo "Number of runs: $NUM_RUNS"
echo "Output CSV: $OUTPUT_CSV"
echo "Device profile: $DEVICE_PROFILE"
echo "Start RNG run: $START_RUN"
echo "Simulation time: $SIMULATION_TIME"
echo "Distance: $DISTANCE m"
echo "Simulation dir: $SIM_DIR"
echo "NS3 dir: $NS3_DIR"
echo ""

# Check if ns3 is configured and built
if [ ! -d "${NS3_DIR}/build" ]; then
    echo -e "${RED}Error: ns3 not built at ${NS3_DIR}/build${NC}"
    echo "Please build ns3 first:"
    echo "  cd $NS3_DIR"
    echo "  ./ns3 configure --enable-examples"
    echo "  ./ns3 build"
    exit 1
fi

# Build the simulation if not already built
if [ ! -f "${EXECUTABLE}" ]; then
    echo -e "${YELLOW}Building simulation...${NC}"
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    cmake -DPython3_EXECUTABLE=$(which python3) ..
    make
    echo -e "${GREEN}Build successful!${NC}"
    echo ""
fi

# Create output directory
mkdir -p "$(dirname "$OUTPUT_CSV")"

# Clear CSV file if it exists
rm -f "${OUTPUT_CSV}"

# Run simulations
echo -e "${YELLOW}Running simulations...${NC}"
echo ""

END_RUN=$((START_RUN + NUM_RUNS - 1))

for run in $(seq "${START_RUN}" "${END_RUN}"); do
    CURRENT_INDEX=$((run - START_RUN + 1))
    echo -ne "Run ${run} (${CURRENT_INDEX}/${NUM_RUNS})... "

    # Run without Bluetooth
    echo -n "[BT off]"
    ${EXECUTABLE} \
        --device-profile="${DEVICE_PROFILE}" \
        --bluetooth-enabled=false \
        --rng-run=$run \
        --simulation-time="${SIMULATION_TIME}" \
        --distance="${DISTANCE}" \
        --output-csv="${OUTPUT_CSV}" \
        > /dev/null 2>&1
    BT_OFF_ROW=$(tail -n 1 "${OUTPUT_CSV}")

    echo -n " [BT on]"
    # Run with Bluetooth
    ${EXECUTABLE} \
        --device-profile="${DEVICE_PROFILE}" \
        --bluetooth-enabled=true \
        --rng-run=$run \
        --simulation-time="${SIMULATION_TIME}" \
        --distance="${DISTANCE}" \
        --output-csv="${OUTPUT_CSV}" \
        > /dev/null 2>&1
    BT_ON_ROW=$(tail -n 1 "${OUTPUT_CSV}")

    BT_OFF_TPUT=$(echo "${BT_OFF_ROW}" | awk -F',' '{print $7}')
    BT_ON_TPUT=$(echo "${BT_ON_ROW}" | awk -F',' '{print $7}')
    echo -e " ${GREEN}✓${NC} off=${BT_OFF_TPUT} Mbps on=${BT_ON_TPUT} Mbps"
done

echo ""
echo -e "${GREEN}All simulations completed!${NC}"
echo "Results saved to: $OUTPUT_CSV"
echo ""

# Show summary
if [ -f "${OUTPUT_CSV}" ]; then
    echo "CSV Preview (first 5 lines):"
    head -6 "${OUTPUT_CSV}" | column -t -s','
    echo ""
    echo "Total runs: $(tail -n +2 "${OUTPUT_CSV}" | wc -l)"
fi
