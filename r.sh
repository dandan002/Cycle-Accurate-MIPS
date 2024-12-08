#!/bin/bash

# Exit immediately if a command exits with a non-zero status
# set -e

# Specify the test folder containing the .bin files and cache config
test_folder="./test"  # Current directory
cache_config_name="cache_config"

make clean
if [ $? -ne 0 ]; then
    echo "Error: make clean failed"
    exit 1
fi

make
if [ $? -ne 0 ]; then
    echo "Error: make failed"
    exit 1
fi

# Step 1: Delete all .out files in the folder
echo "Cleaning up .out files..."
find "$test_folder" -type f -name "*.out" -exec rm -f {} \;

# Step 2: Iterate over each .bin file and run the commands
for bin_file in "$test_folder"/*.bin; do
    # Extract the base name of the .bin file (without path and extension)
    bin_name=$(basename "$bin_file" .bin)

    # Prepare full paths
    bin_path="${test_folder}/${bin_name}.bin"
    cache_config_path="${test_folder}/${cache_config_name}.txt"

    echo ""
    echo "--------------------------------------"
    echo "Running tests for: ${bin_name}"
    echo "--------------------------------------"

    echo "Running with:"
    echo "  Binary: $bin_path"
    echo "  Cache Config: $cache_config_path"
    echo ""
    echo "============= SIM FUNCT ================"

    # Run sim_funct command
    echo "Running sim_funct for ${bin_name}.bin..."
    sim_funct "$bin_path" .

    echo ""
    echo ""
    echo "============= SIM CYCLE ================"

    # Run sim_cycle command
    echo "Running sim_cycle for ${bin_name}.bin..."
    sim_cycle "$bin_path" "$cache_config_path"

    echo ""
    echo "========= COMPLETED RUN ==============="
done

# Step 3: Compare .student and .out files and create .summary files
echo ""
echo "Generating summaries for .student vs .out files..."
for student_file in "$test_folder"/*.student; do
    # Extract the base name of the .student file
    file_name=$(basename "$student_file" .student)

    # Corresponding .out file
    out_file="${test_folder}/${file_name}.out"

    # Check if .out file exists
    if [ -f "$out_file" ]; then
        # Create a .summary file with the diff output
        summary_file="${test_folder}/${file_name}.summary"
        echo "Comparing $student_file and $out_file..."
        diff "$student_file" "$out_file" > "$summary_file"

        if [ -s "$summary_file" ]; then
            echo "Differences found. Summary saved to $summary_file."
        else
            echo "No differences found. Removing empty summary file."
            rm "$summary_file"
        fi
    else
        echo "Warning: Corresponding .out file for $student_file not found."
    fi
done

# Clean up all .bin and .elf files in the folder
echo "Cleaning up .bin and .elf files..."
find "$test_folder" -type f -name "*.bin" -exec rm -f {} \;
find "$test_folder" -type f -name "*.elf" -exec rm -f {} \;

echo "All tasks completed!"