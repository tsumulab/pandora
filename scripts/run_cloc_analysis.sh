#!/bin/bash

# Exit immediately if a command fails.
set -e

# Resolve the repository root from this script's location and run from there,
# so backends/, the workload dirs, and Logs/ resolve regardless of the caller's directory.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT_DIR"

# Validate command-line arguments.
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <backend_name> <workload_name>"
    echo "Example: $0 tinystm kmeans"
    exit 1
fi

readonly BACKEND_NAME="$1"
readonly WORKLOAD_NAME="$2"

# Verify required tools.
command -v unifdef >/dev/null || {
    echo "[Error] 'unifdef' not found."
    exit 1
}

command -v cloc >/dev/null || {
    echo "[Error] 'cloc' not found."
    exit 1
}

# Select workload-specific source files.
case "$WORKLOAD_NAME" in
    "kmeans"|"kmeans_low"|"kmeans_high")
        TARGET_FILES=("normal.c" "normal.h")
        ;;
    "genome")
        TARGET_FILES=("table.c" "table.h" "sequencer.c" "sequencer.h")
        ;;
    "ssca2")
        TARGET_FILES=("computeGraph.c" "computeGraph.h")
        ;;
    "intruder")
        TARGET_FILES=("intruder.c" "decoder.c" "decoder.h" "stream.c" "stream.h")
        ;;
    "vacation"|"vacation_low"|"vacation_high")
        TARGET_FILES=("customer.c" "customer.h" "reservation.c" "reservation.h" "client.c" "client.h" "manager.c" "manager.h")
        ;;
    "labyrinth")
        TARGET_FILES=("grid.c" "grid.h" "router.cpp" "router.h" )
        ;;
    "yada")
        TARGET_FILES=("yada.c" "region.c" "region.h" "mesh.c" "mesh.h" "element.c" "element.h")
        ;;
    *)
        TARGET_FILES=("${WORKLOAD_NAME}.c")
        ;;
esac

# Create output directory.
OUTPUT_DIR="Logs/cloc_results"
mkdir -p "$OUTPUT_DIR"

RESULT_FILE="${OUTPUT_DIR}/${WORKLOAD_NAME}_${BACKEND_NAME}_cloc.txt"

TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT

echo "=== LOC Analysis ==="
echo "Backend : $BACKEND_NAME"
echo "Workload: $WORKLOAD_NAME"

DEFINED_MACROS=()
UNDEFINED_MACROS=()

# Extract macro definitions from a Makefile.
extract_macros() {
    local makefile="$1"

    [ -f "$makefile" ] || return

    while read -r flag; do
        case "$flag" in
            -D*) DEFINED_MACROS+=("${flag#-D}") ;;
            -U*) UNDEFINED_MACROS+=("${flag#-U}") ;;
        esac
    done < <(
        grep -v '^\s*#' "$makefile" |
        grep -oE '\-[DU][A-Za-z0-9_]+' || true
    )
}

extract_macros "backends/${BACKEND_NAME}/Makefile"
extract_macros "${WORKLOAD_NAME}/Defines.common.mk"

# Adjust backend-specific macro definitions.
if [[ "$BACKEND_NAME" == "fine-lock" || "$BACKEND_NAME" == "finelock" ]]; then
    DEFINED_MACROS+=("FINE_LOCK" "LOCK")
    UNDEFINED_MACROS+=("STM" "TM")
else
    DEFINED_MACROS+=("STM" "TM")
    UNDEFINED_MACROS+=("FINE_LOCK")
fi

DEFINED_MACROS+=(
    "THREAD_SWITCHING"
    "USE_TCMALLOC"
    "LIBFIBRE"
)

UNDEFINED_MACROS+=("USE_TLMALLOC")

# Remove duplicate entries.
array_unique() {
    echo "$@" | tr ' ' '\n' | sort -u | tr '\n' ' '
}

echo "Scanning conditional compilation macros..."

ALL_SRC_FILES=()

for file in "${TARGET_FILES[@]}"; do
    [ -f "${WORKLOAD_NAME}/${file}" ] &&
        ALL_SRC_FILES+=("${WORKLOAD_NAME}/${file}")
done

[ -f "backends/${BACKEND_NAME}/tm.h" ] &&
    ALL_SRC_FILES+=("backends/${BACKEND_NAME}/tm.h")

DETECTED_MACROS=$(
grep -hE '^\s*#\s*(ifdef|ifndef|if\s+defined|elif\s+defined)' \
"${ALL_SRC_FILES[@]}" 2>/dev/null |
sed -E 's/^\s*#\s*(ifdef|ifndef|if|elif)//g' |
sed -E 's/defined|\(|\)|!|&&|\|\|//g' |
tr ' ' '\n' |
grep -v '^$' |
sort -u
)

FINAL_UNIFDEF_FLAGS=""

UNIQUE_DEFINES=$(array_unique "${DEFINED_MACROS[@]}")

for macro in $UNIQUE_DEFINES; do
    FINAL_UNIFDEF_FLAGS+=" -D${macro}"
done

for macro in $DETECTED_MACROS; do
    if ! echo " $UNIQUE_DEFINES " | grep -q " $macro "; then
        FINAL_UNIFDEF_FLAGS+=" -U${macro}"
    fi
done

echo "Running unifdef..."

PROCESSED_FILES=()

# Process workload source files.
for file in "${TARGET_FILES[@]}"; do
    SRC="${WORKLOAD_NAME}/${file}"

    if [ -f "$SRC" ]; then
        OUT="${TMP_DIR}/${file}"

        # Preserve #if 0 blocks.
        unifdef -k $FINAL_UNIFDEF_FLAGS "$SRC" > "$OUT" || true

        #######################################################################
        # intruder: count only processPackets()
        #######################################################################
        if [[ "$WORKLOAD_NAME" == "intruder" && "$file" == "intruder.c" ]]; then
            echo " -> Extracting only processPackets() from intruder.c..."

            awk '
                /processPackets[ \t]*\(/ {
                    in_func = 1
                    if (prev_line ~ /^[ \t]*void[ \t]*$/)
                        print prev_line
                }

                in_func {
                    print
                    open_b  += gsub(/\{/,"{")
                    close_b += gsub(/\}/,"}")

                    if (open_b > 0 && open_b == close_b)
                        exit
                }

                { prev_line = $0 }
            ' "$OUT" > "${TMP_DIR}/intruder_processPackets_only.c"

            OUT="${TMP_DIR}/intruder_processPackets_only.c"
        fi

        #######################################################################
        # yada: count only process()
        #######################################################################
        if [[ "$WORKLOAD_NAME" == "yada" && "$file" == "yada.c" ]]; then
            echo " -> Extracting only process() from yada.c..."

            awk '
                /process[ \t]*\(/ {
                    if (prev_line ~ /^[ \t]*void[ \t]*$/ || $0 ~ /void[ \t]+process/) {
                        in_func = 1
                        if (prev_line ~ /^[ \t]*void[ \t]*$/)
                            print prev_line
                    }
                }

                in_func {
                    print
                    open_b  += gsub(/\{/,"{")
                    close_b += gsub(/\}/,"}")

                    if (open_b > 0 && open_b == close_b)
                        exit
                }

                { prev_line = $0 }
            ' "$OUT" > "${TMP_DIR}/yada_process_only.c"

            OUT="${TMP_DIR}/yada_process_only.c"
        fi

        PROCESSED_FILES+=("$OUT")
    fi
done

# Process backend header (tm.h).
TM_HEADER="backends/${BACKEND_NAME}/tm.h"

if [ -f "$TM_HEADER" ]; then
    OUT="${TMP_DIR}/backend_tm.h"
    unifdef -k $FINAL_UNIFDEF_FLAGS "$TM_HEADER" > "$OUT" || true
    PROCESSED_FILES+=("$OUT")
fi

echo "Counting lines of code..."

if [ ${#PROCESSED_FILES[@]} -eq 0 ]; then
    echo "[Error] No valid files found."
    exit 1
fi

# ==============================================================================
# LOC including backend tm.h
# ==============================================================================

echo "=== LOC (including backend tm.h) ===" | tee "$RESULT_FILE"

WITH_OUTPUT=$(cloc --by-file "${PROCESSED_FILES[@]}")

echo "$WITH_OUTPUT" | tee -a "$RESULT_FILE"

TOTAL_WITH_TM=$(echo "$WITH_OUTPUT" | awk '/^SUM:/{print $4}')
TM_CODE=$(echo "$WITH_OUTPUT" | awk '/backend_tm\.h/{print $4}')

echo "" | tee -a "$RESULT_FILE"
echo "TOTAL_LOC_WITH_TM=$TOTAL_WITH_TM" | tee -a "$RESULT_FILE"

# ==============================================================================
# LOC excluding backend tm.h
# ==============================================================================

TOTAL_WITHOUT_TM=$((TOTAL_WITH_TM - TM_CODE))

echo "" | tee -a "$RESULT_FILE"
echo "=== LOC (excluding backend tm.h) ===" | tee -a "$RESULT_FILE"

SOURCE_FILES=()

for file in "${PROCESSED_FILES[@]}"; do
    if [[ "$(basename "$file")" != "backend_tm.h" ]]; then
        SOURCE_FILES+=("$file")
    fi
done

WITHOUT_OUTPUT=$(cloc --by-file "${SOURCE_FILES[@]}")

echo "$WITHOUT_OUTPUT" | tee -a "$RESULT_FILE"

echo "" | tee -a "$RESULT_FILE"
echo "TOTAL_LOC_WITHOUT_TM=$TOTAL_WITHOUT_TM" | tee -a "$RESULT_FILE"

echo
echo "Result saved to: $RESULT_FILE"