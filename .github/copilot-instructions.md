# Agile Pasta - Data Transformation Tool

Always reference these instructions first and fallback to search or bash commands only when you encounter unexpected information that does not match the info here.

## Working Effectively

- **Bootstrap the repository:**
  - `git submodule update --init --recursive` -- initializes indicators library submodule (required)
  - `mkdir build && cd build`
  - `cmake ..` -- takes less than 1 second
  - `make -j$(nproc)` -- takes 12-15 seconds. NEVER CANCEL. Set timeout to 30+ seconds.

- **Run the application:**
  - ALWAYS run the bootstrapping steps first before using the application
  - Show help: `./build/bin/agile-pasta help`
  - Validate configuration: `./build/bin/agile-pasta check --out examples/output`
  - Transform data: `./build/bin/agile-pasta transform --in examples/input --out examples/output`

- **Test data generation:**
  - Generate test dataset: `python3 scripts/generate_large_dataset.py [size]`
  - Default generates 2M records if no size specified
  - For testing, use smaller sizes like 1000-10000 records

## Validation

- **ALWAYS run these validation steps after making changes:**
  1. Build the application: `cd build && make -j$(nproc)`
  2. Test help command: `./bin/agile-pasta help`
  3. Test sanity check: `./bin/agile-pasta check --out ../examples/output`
  4. Test transformation: `./bin/agile-pasta transform --in ../examples/input --out ../examples/output`
  5. Verify CSV files are generated in examples/output/

- **Validation scenarios to test:**
  - Configuration validation: All sanity checks should pass
  - Data transformation: Should process input PSV files and generate CSV output
  - Large dataset handling: Generate 1000+ records and verify processing works
  - Progress bars: Should display during file loading and transformation

- **Expected results:**
  - Sanity check: "🎉 All sanity checks PASSED!"
  - Transform: "Transformation complete!" with CSV files created
  - Check CSV output: Files should have proper headers and transformed data

## Dependencies

- **Required system packages:**
  - CMake 3.16 or later (build system)
  - GCC 7+ or Clang 6+ (C++ compiler with C++17 support)
  - Git (for submodules)
  - Build tools (make, g++)

- **External dependencies:**
  - indicators library (included as git submodule at external/indicators)
  - C++17 standard library with filesystem, thread, regex support

- **Installation on Ubuntu/Debian:**
  ```bash
  sudo apt update
  sudo apt install build-essential cmake git
  ```

## Build Timing and Critical Warnings

- **CMake configuration:** < 1 second
- **Build process:** 12-15 seconds. NEVER CANCEL. Set timeout to 30+ seconds.
- **Application execution:** < 1 second for small datasets, a few seconds for large datasets
- **Large dataset generation:** 1000 records takes < 1 second, 2M records takes ~30 seconds

## Common Tasks

### Repo Structure
```
.
├── README.md                    # Main documentation
├── CMakeLists.txt              # Build configuration
├── src/                        # C++ source files (9 files)
├── include/                    # C++ header files (8 files)
├── examples/
│   ├── input/                  # PSV data files with headers
│   └── output/                 # Configuration files (headers + rules)
├── scripts/
│   └── generate_large_dataset.py  # Test data generator
└── external/
    └── indicators/             # Progress bar library (submodule)
```

### Source Files
```
src/main.cpp                    # Entry point and main commands
src/command_line_parser.cpp     # CLI argument parsing
src/file_scanner.cpp           # File discovery and validation
src/psv_parser.cpp             # PSV file parsing
src/database.cpp               # In-memory data storage
src/query_engine.cpp           # SQL-like operations
src/transformation_engine.cpp  # Rule processing and transformation
src/csv_writer.cpp             # CSV output generation
src/progress_manager.cpp       # Progress bar management
```

### Example Usage Outputs
```bash
# Help command shows comprehensive documentation
./build/bin/agile-pasta help

# Check command validates all configuration files
./build/bin/agile-pasta check --out examples/output
# Output: "🎉 All sanity checks PASSED!"

# Transform processes data files using rules
./build/bin/agile-pasta transform --in examples/input --out examples/output
# Output: "Transformation complete!" + CSV files created
```

### Working with Data Files

- **Input format:** PSV files with matching header files
  - Data files: `filename.psv` (pipe-separated values)
  - Header files: `filename_Headers.psv` (column names)

- **Output format:** Configuration pairs for CSV generation
  - Header files: `outputname_Headers.psv` (output column definitions)
  - Rules files: `outputname_Rules.psv` (transformation rules)

- **Rule syntax:**
  - Global filters: `GLOBAL|condition|description`
  - Field transforms: `FIELD|output_field|expression|description`

## Troubleshooting

### Build Issues

- **Submodule not found:** Run `git submodule update --init --recursive`
- **CMake not found:** Install with `sudo apt install cmake` on Ubuntu/Debian
- **Compiler errors:** Ensure GCC 7+ or C++17 support is available
- **Linking errors on older GCC:** The build automatically handles filesystem linking

### Runtime Issues

- **No files found:** Ensure PSV files have matching `_Headers.psv` files
- **No output:** Check that global rules aren't filtering out all records
- **Progress bar issues:** Works best in terminals with ANSI escape sequence support
- **Large dataset memory:** Application is optimized for large datasets with multi-threading

### Development Workflow

- **Always build and test changes:** The application has no formal unit tests, so functional testing is critical
- **Use examples directory:** Contains working test cases for validation
- **Test with different dataset sizes:** Use the dataset generator for performance testing
- **Validate CSV output:** Check that generated files have correct headers and data transformation

## Key Project Features

- Cross-platform C++ application (Windows/Linux)
- Multi-threaded data processing for performance
- SQL-like operations: SELECT, JOIN, UNION, WHERE
- Real-time progress reporting with indicators library
- Memory-efficient processing of large datasets
- Excel-compatible CSV output format
- Comprehensive command-line interface with validation