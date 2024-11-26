#!/bin/bash

# Default values
default_bin_name="fib"
default_cache_config="cache_config"

# Initialize variables
bin_name="$default_bin_name"
cache_config_name="$default_cache_config"
test_folder="./test"

# Function to display usage
usage() {
    echo "Usage: $0 [-b binary_name] [-c cache_config]"
    echo "  -b: Binary name (default: $default_bin_name)"
    echo "  -c: Cache config name (default: $default_cache_config)"
    exit 1
}

# Parse command-line options
while getopts ":b:c:h" opt; do
    case ${opt} in
        b )
            bin_name=$OPTARG
            ;;
        c )
            cache_config_name=$OPTARG
            ;;
        h )
            usage
            ;;
        \? )
            echo "Invalid option: $OPTARG" 1>&2
            usage
            ;;
        : )
            echo "Invalid option: $OPTARG requires an argument" 1>&2
            usage
            ;;
    esac
done
shift $((OPTIND -1))

# Prepare full paths
bin_path="${test_folder}/${bin_name}.bin"
cache_config_path="${test_folder}/${cache_config_name}.txt"
output_file="SUMMARY_${bin_name}"

echo "Running with:"
echo "  Binary: $bin_path"
echo "  Cache Config: $cache_config_path"
echo "  Output File: $output_file"
echo ""
echo "============= SIM FUNCT ================"

# Run sim_funct command
echo "Running sim_funct for ${bin_name}.bin..."
sim_funct "${test_folder}/${bin_name}.bin" .

echo ""
echo ""
echo "============= SIM CYCLE ================"

# Run sim_cycle command
echo "Running sim_cycle for ${bin_name}.bin..."
sim_cycle "${test_folder}/${bin_name}.bin" "${test_folder}/${cache_config_name}.txt"

echo ""
echo "========= COMPLETED RUN ==============="

# File patterns
out_files=(
    "${test_folder}/${bin_name}_cycle_sim_stats.out"
    "${test_folder}/${bin_name}_cycle_reg_state.out"
    "${test_folder}/${bin_name}_cycle_mem_state.out"
    "${test_folder}/${bin_name}_cycle_pipe_state.out"
)

# Clear or create the output file with a summary section
echo "COMPARISON RESULTS: ${bin_name}.bin" > "$output_file"
echo "==================================" >> "$output_file"
echo "" >> "$output_file"
echo "SUMMARY OF DIFFERENCES" >> "$output_file"
echo "---------------------" >> "$output_file"

# Track total differences
total_different_lines=0

# Compare each file with its corresponding reference file
for out_file in "${out_files[@]}"; do
    ref_file="${out_file/.out/.ref}"
    
    if [[ -f "$out_file" && -f "$ref_file" ]]; then
        # Count the number of different lines
        diff_count=$(diff "$out_file" "$ref_file" | wc -l)
        file_name=$(basename "$out_file")
        
        # Add to summary
        echo "${file_name}: ${diff_count} different lines" >> "$output_file"
        
        # Accumulate total differences
        total_different_lines=$((total_different_lines + diff_count))
    else
        echo "Error: $out_file or $ref_file is missing." >> "$output_file"
    fi
done

# Add total differences to summary
echo "---------------------" >> "$output_file"
echo "TOTAL DIFFERENT LINES: ${total_different_lines}" >> "$output_file"
echo "==================================" >> "$output_file"
echo "" >> "$output_file"

# Now append the detailed diffs
for out_file in "${out_files[@]}"; do
    ref_file="${out_file/.out/.ref}"
    
    if [[ -f "$out_file" && -f "$ref_file" ]]; then
        echo "Comparing $out_file with $ref_file:" >> "$output_file"
        diff "$out_file" "$ref_file" >> "$output_file"
        echo "----------------------------------" >> "$output_file"
    fi
done