#!/bin/bash

# Continue execution even if an error occurs.
set +e

# Resolve the repository root from this script's location and run from there,
# so Logs/ resolves and the per-pair analysis script is found regardless of the caller's dir.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT_DIR"

# Analysis script (sibling of this script)
SINGLE_SCRIPT="$SCRIPT_DIR/run_cloc_analysis.sh"

# Transactional Memory backends
BACKENDS=(
    "tinystm"
    "fine-lock"
    "coarse-lock"
)

# Benchmark workloads
WORKLOADS=(
    "kmeans"
    "genome"
    "ssca2"
    "intruder"
    "vacation"
    "labyrinth"
    "yada"
)

# Check that the analysis script exists.
if [ ! -f "$SINGLE_SCRIPT" ]; then
    echo "[Error] Analysis script '$SINGLE_SCRIPT' not found."
    echo "Please verify the file name and execution permission (chmod +x $SINGLE_SCRIPT)."
    exit 1
fi

# Output directories
LOG_ROOT="Logs"
SUMMARY_DIR="${LOG_ROOT}/cloc_summary_logs"
RESULT_DIR="${LOG_ROOT}/cloc_results"

mkdir -p "$SUMMARY_DIR"
mkdir -p "$RESULT_DIR"

TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
MASTER_LOG="${SUMMARY_DIR}/master_summary_${TIMESTAMP}.log"

echo "==================================================" | tee "$MASTER_LOG"
echo " PANDORA Benchmarks Multi-Backend LOC Analysis" | tee -a "$MASTER_LOG"
echo " Date: $(date)" | tee -a "$MASTER_LOG"
echo " Backends: ${BACKENDS[*]}" | tee -a "$MASTER_LOG"
echo "==================================================" | tee -a "$MASTER_LOG"
echo | tee -a "$MASTER_LOG"

# Run analysis for each backend/workload pair.
for backend in "${BACKENDS[@]}"; do
    echo "==================================================" | tee -a "$MASTER_LOG"
    echo " Backend: $backend" | tee -a "$MASTER_LOG"
    echo "==================================================" | tee -a "$MASTER_LOG"

    for workload in "${WORKLOADS[@]}"; do
        echo "--------------------------------------------------" | tee -a "$MASTER_LOG"
        echo " Workload: $workload" | tee -a "$MASTER_LOG"
        echo "--------------------------------------------------" | tee -a "$MASTER_LOG"

        bash "$SINGLE_SCRIPT" "$backend" "$workload" 2>&1 | tee -a "$MASTER_LOG"
        echo | tee -a "$MASTER_LOG"
    done
done

###############################################################################
# LOC summary (including backend tm.h)
###############################################################################

echo "==================================================" | tee -a "$MASTER_LOG"
echo " LOC Summary (including backend tm.h)" | tee -a "$MASTER_LOG"
echo "==================================================" | tee -a "$MASTER_LOG"

HEADER=$(printf "%-15s" "Workload")
for backend in "${BACKENDS[@]}"; do
    HEADER+=" | $(printf "%-12s" "$backend")"
done

echo "$HEADER" | tee -a "$MASTER_LOG"
echo "-----------------------------------------------------------------------------------" | tee -a "$MASTER_LOG"

for workload in "${WORKLOADS[@]}"; do

    ROW=$(printf "%-15s" "$workload")

    for backend in "${BACKENDS[@]}"; do

        if [[ "$workload" == "yada" && "$backend" == "fine-lock" ]]; then
            ROW+=" | $(printf "%-12s" "N/A")"
            continue
        fi

        RESULT_FILE="${RESULT_DIR}/${workload}_${backend}_cloc.txt"

        if [ -f "$RESULT_FILE" ]; then
            CODE_LOC=$(grep "^TOTAL_LOC_WITH_TM=" "$RESULT_FILE" \
                | cut -d= -f2)
            ROW+=" | $(printf "%-12s" "${CODE_LOC:-N/A}")"
        else
            ROW+=" | $(printf "%-12s" "ERROR")"
        fi

    done

    echo "$ROW" | tee -a "$MASTER_LOG"

done

###############################################################################
# LOC summary (excluding backend tm.h)
###############################################################################

echo "" | tee -a "$MASTER_LOG"
echo "==================================================" | tee -a "$MASTER_LOG"
echo " LOC Summary (excluding backend tm.h)" | tee -a "$MASTER_LOG"
echo "==================================================" | tee -a "$MASTER_LOG"

HEADER=$(printf "%-15s" "Workload")
for backend in "${BACKENDS[@]}"; do
    HEADER+=" | $(printf "%-12s" "$backend")"
done

echo "$HEADER" | tee -a "$MASTER_LOG"
echo "-----------------------------------------------------------------------------------" | tee -a "$MASTER_LOG"

for workload in "${WORKLOADS[@]}"; do

    ROW=$(printf "%-15s" "$workload")

    for backend in "${BACKENDS[@]}"; do

        if [[ "$workload" == "yada" && "$backend" == "fine-lock" ]]; then
            ROW+=" | $(printf "%-12s" "N/A")"
            continue
        fi

        RESULT_FILE="${RESULT_DIR}/${workload}_${backend}_cloc.txt"

        if [ -f "$RESULT_FILE" ]; then
            CODE_LOC=$(grep "^TOTAL_LOC_WITHOUT_TM=" "$RESULT_FILE" \
                | cut -d= -f2)
            ROW+=" | $(printf "%-12s" "${CODE_LOC:-N/A}")"
        else
            ROW+=" | $(printf "%-12s" "ERROR")"
        fi

    done

    echo "$ROW" | tee -a "$MASTER_LOG"

done

###############################################################################
# LOC ratio (excluding backend tm.h)
###############################################################################

echo "" | tee -a "$MASTER_LOG"
echo "==================================================" | tee -a "$MASTER_LOG"
echo " LOC Ratio (excluding backend tm.h)" | tee -a "$MASTER_LOG"
echo "==================================================" | tee -a "$MASTER_LOG"

printf "%-15s | %-15s | %-15s\n" \
    "Workload" \
    "fine/tinystm" \
    "coarse/tinystm" | tee -a "$MASTER_LOG"

echo "----------------------------------------------------------------" | tee -a "$MASTER_LOG"

for workload in "${WORKLOADS[@]}"; do

    TINY_FILE="${RESULT_DIR}/${workload}_tinystm_cloc.txt"
    FINE_FILE="${RESULT_DIR}/${workload}_fine-lock_cloc.txt"
    COARSE_FILE="${RESULT_DIR}/${workload}_coarse-lock_cloc.txt"

    tiny=""
    fine=""
    coarse=""

    if [ -f "$TINY_FILE" ]; then
        tiny=$(grep "^TOTAL_LOC_WITHOUT_TM=" "$TINY_FILE" \
            | cut -d= -f2)
    fi

    if [ -f "$FINE_FILE" ]; then
        fine=$(grep "^TOTAL_LOC_WITHOUT_TM=" "$FINE_FILE" \
            | cut -d= -f2)
    fi

    if [ -f "$COARSE_FILE" ]; then
        coarse=$(grep "^TOTAL_LOC_WITHOUT_TM=" "$COARSE_FILE" \
            | cut -d= -f2)
    fi

    if [[ -z "$tiny" || "$tiny" == "0" ]]; then
        fine_ratio="N/A"
        coarse_ratio="N/A"
    else

        if [[ "$workload" == "yada" ]]; then
            fine_ratio="N/A"
        elif [[ -n "$fine" ]]; then
            fine_ratio=$(awk "BEGIN { printf \"%.3f\", $fine / $tiny }")
        else
            fine_ratio="N/A"
        fi

        if [[ -n "$coarse" ]]; then
            coarse_ratio=$(awk "BEGIN { printf \"%.3f\", $coarse / $tiny }")
        else
            coarse_ratio="N/A"
        fi

    fi

    printf "%-15s | %-15s | %-15s\n" \
        "$workload" \
        "$fine_ratio" \
        "$coarse_ratio" | tee -a "$MASTER_LOG"

done

echo "" | tee -a "$MASTER_LOG"
echo "==================================================" | tee -a "$MASTER_LOG"
echo "Summary log saved to: $MASTER_LOG" | tee -a "$MASTER_LOG"