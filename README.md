# Agile Pasta - Data Transformation Tool

A high-performance, multi-threaded C++ command-line application for transforming pipe-separated value (PSV) data files into Excel-compatible CSV format using configurable transformation rules.

## Features

- **Cross-platform compatibility**: Compiles on Windows (Visual Studio 2022) and Linux (Ubuntu/Debian)
- **Multi-threaded data loading**: Efficient parallel processing for large datasets
- **SQL-like operations**: Support for SELECT, JOIN, UNION, and WHERE operations
- **Flexible transformation rules**: Global filters and field-specific transformations
- **Progress reporting**: Real-time progress bars for all operations
- **Memory efficient**: Optimized for processing large datasets
- **Excel-compatible output**: Generates properly formatted CSV files

## Build Requirements

### Windows (Visual Studio 2022)
- Visual Studio 2022 with C++ tools
- CMake 3.16 or later
- Git

### Linux (Ubuntu/Debian)
- GCC 7+ or Clang 6+
- CMake 3.16 or later
- Git
- Build essentials

```bash
sudo apt update
sudo apt install build-essential cmake git
```

## Getting Started

### 1. Clone the Repository

```bash
# Clone with submodules
git clone --recursive https://github.com/skade88/agile-pasta.git
cd agile-pasta

# If you forgot --recursive, initialize submodules:
git submodule update --init --recursive
```

### 2. Build on Linux (Ubuntu/Debian)

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### 3. Build on Windows (Visual Studio 2022)

```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

### 4. Run the Application

```bash
# Show help
./build/bin/agile-pasta help

# Transform data
./build/bin/agile-pasta transform --in /path/to/input --out /path/to/output
```

## Usage

### Command Line Interface

```bash
agile-pasta help                                    # Show help
agile-pasta transform --in <input> --out <output>  # Transform data
```

### Input File Structure

The `--in` directory should contain pairs of PSV files:

#### Data Files
- Format: `filename.psv`
- Contains pipe-separated values
- Example: `employees.psv`

```
1001|John Doe|Software Engineer|2023-01-15|85000|Engineering
1002|Jane Smith|Data Analyst|2023-02-01|75000|Analytics
```

#### Header Files
- Format: `filename_Headers.psv`
- Contains column names for the corresponding data file
- Example: `employees_Headers.psv`

```
emp_id|name|position|hire_date|salary|department
```

### Output Configuration Files

The `--out` directory should contain pairs of configuration files:

#### Output Header Files
- Format: `outputname_Headers.psv`
- Defines column names for the output CSV
- Example: `employee_summary_Headers.psv`

```
employee_name|annual_salary|department_name|years_since_hire
```

#### Transformation Rules Files
- Format: `outputname_Rules.psv`
- Contains transformation rules
- Example: `employee_summary_Rules.psv`

```
GLOBAL|hire_date >= '2023-01-01'|Only employees hired in 2023 or later
FIELD|employee_name|name|Copy employee name
FIELD|annual_salary|salary * 12|Convert monthly to annual salary
FIELD|department_name|UPPER(department)|Convert department to uppercase
FIELD|full_name|first_name + " " + last_name|Combine first and last name
```

## Transformation Rules

### Global Rules
Apply filters to all records before field transformations:

```
GLOBAL|<condition>|<description>
```

Examples:
```
GLOBAL|department = 'Engineering'|Only engineering employees
GLOBAL|salary >= '75000'|Only employees with salary >= 75000
GLOBAL|hire_date >= '2023-01-01'|Only recent hires
```

### Field Rules
Transform specific output fields:

```
FIELD|<output_field>|<expression>|<description>
```

Examples:
```
FIELD|full_name|first_name + " " + last_name|Combine names with space
FIELD|annual_salary|salary * 12|Convert monthly to annual salary
FIELD|upper_name|UPPER(name)|Convert name to uppercase
FIELD|lower_dept|LOWER(department)|Convert department to lowercase
FIELD|greeting|"Hello, " + name + "!"|Add greeting with punctuation
```

### Supported Operations

#### Comparison Operators
- `=` (equals)
- `!=` (not equals)
- `>` (greater than)
- `<` (less than)
- `>=` (greater than or equal)
- `<=` (less than or equal)

#### Functions
- `UPPER(field)` - Convert to uppercase
- `LOWER(field)` - Convert to lowercase
- `field1 + field2` - String concatenation
- `field * number` - Numeric multiplication

#### String Literals
- Use double quotes for string literals: `"literal text"`
- Single quotes also supported for backward compatibility: `'text'`
- Examples:
  - `"Hello, " + name + "!"` - Add greeting with punctuation
  - `first_name + " " + last_name` - Combine names with space
  - `"ID: " + emp_id` - Add prefix to field

## Example Project Structure

```
project/
├── input/
│   ├── employees.psv
│   ├── employees_Headers.psv
│   ├── departments.psv
│   └── departments_Headers.psv
└── output/
    ├── summary_Headers.psv
    └── summary_Rules.psv
```

## Performance Features

- **Multi-threaded loading**: Automatically uses available CPU cores
- **Memory-efficient parsing**: Streams large files without loading entirely into memory
- **Progress reporting**: Real-time progress bars for all operations
- **Optimized queries**: Efficient in-memory indexing for fast lookups

## Using as a Template

To use this project as a template for your own data transformation tool:

1. Fork or download the repository
2. Ensure the indicators submodule is included:
   ```bash
   git submodule add https://github.com/p-ranav/indicators.git external/indicators
   ```
3. Modify the transformation rules and logic as needed
4. Update CMakeLists.txt if adding new source files

## Dependencies

- **indicators**: Progress bar library (included as submodule)
- **C++17 standard library**: filesystem, thread, regex support

## Troubleshooting

### Build Issues

**Linux**: If you get filesystem linking errors:
```bash
# For older GCC versions
sudo apt install libstdc++fs-dev
```

**Windows**: Ensure Visual Studio 2022 is properly installed with C++ tools.

### Runtime Issues

**No files found**: Check that PSV files have matching header files with `_Headers.psv` suffix.

**No output**: Verify that global rules aren't filtering out all records.

**Progress bar display**: Progress bars work best in terminals that support ANSI escape sequences.

## License

This project uses open-source software and is provided as-is for educational and commercial use.

## Contributing

Feel free to submit issues and pull requests. This project demonstrates:
- Modern C++ practices
- Cross-platform development
- Multi-threaded programming
- File processing and data transformation
- Command-line application development