#!/bin/bash

# Resolve the repository root from this script's location (this script lives in Logs/).
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Transactional Memory or locks backends
types=("tinystm" "tl2" "norec" "swisstm" "rtm" "coarse-lock" "fine-lock")

# Benchmark workloads
workloads=("genome" "yada" "intruder" "vacation" "kmeans" "ssca2" "labyrinth")

# Benchmark options
benchmark_options=(-b)

for tm in "${types[@]}"; do
    for workload in "${workloads[@]}"; do
        for option in "${benchmark_options[@]}"; do
            echo "Building: TM=${tm}, Workload=${workload}, Option=${option}"

            "$ROOT_DIR/scripts/sbuilder.sh" "$tm" "$workload" "$option"
        done
    done
done

echo "All builds completed."
