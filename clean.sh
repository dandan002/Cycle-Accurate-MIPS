# Specify the test folder containing the .bin files and cache config
test_folder="./test"  # Current directory
cache_config_name="cache_config"

make clean
if [ $? -ne 0 ]; then
    echo "Error: make clean failed"
    exit 1
fi

# Delete all .out files in the folder
echo "Cleaning up .out files..."
find "$test_folder" -type f -name "*.out" -exec rm -f {} \;

# Clean up all .bin and .elf files in the folder
echo "Cleaning up .bin and .elf files..."
find "$test_folder" -type f -name "*.bin" -exec rm -f {} \;
find "$test_folder" -type f -name "*.elf" -exec rm -f {} \;

echo "All tasks completed!"