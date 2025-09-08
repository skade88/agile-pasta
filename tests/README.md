# Unit Tests for Agile Pasta

This directory contains comprehensive unit tests for the Agile Pasta data transformation tool.

## Test Structure

The test suite is organized into the following test files:

### Core Component Tests
- `test_command_line_parser.cpp` - Tests command-line argument parsing
- `test_psv_parser.cpp` - Tests PSV file parsing functionality
- `test_database.cpp` - Tests in-memory database operations
- `test_query_engine.cpp` - Tests query operations (SELECT, WHERE, JOIN, UNION)
- `test_transformation_engine.cpp` - Tests data transformation rules
- `test_csv_writer.cpp` - Tests CSV output generation
- `test_file_scanner.cpp` - Tests input/output file discovery

### Integration Tests
- `test_integration.cpp` - End-to-end workflow testing

## Building and Running Tests

### Prerequisites
- CMake 3.16 or later
- Google Test (automatically installed via package manager)
- GCC 7+ or compatible C++ compiler

### Building
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Running Tests
```bash
# Run all tests
./agile-pasta-tests

# Run specific test suite
./agile-pasta-tests --gtest_filter="CommandLineParserTest.*"

# Run specific test
./agile-pasta-tests --gtest_filter="PsvParserTest.ParseValidHeaders"

# Run tests with verbose output
./agile-pasta-tests --gtest_filter="*" --verbose

# Using CTest
cd build
ctest --verbose
```

## Test Coverage

The test suite covers:

### Command Line Parser (12 tests)
- Help command parsing
- Transform command with parameters
- Sanity check command
- Error handling for invalid arguments

### PSV Parser (12 tests)
- Header file parsing
- Data file parsing with various formats
- Empty file handling
- Error conditions
- Complete file parsing workflow

### Database (12 tests)
- Table loading and storage
- Data retrieval
- Multiple table management
- Error handling

### Query Engine (12 tests)
- SELECT operations
- WHERE clause filtering
- JOIN operations
- UNION operations
- Error handling

### Transformation Engine (12 tests)
- Rule loading and parsing
- Data transformation
- Global filtering
- Field transformations
- String functions
- Error handling

### CSV Writer (12 tests)
- Basic CSV output
- Special character handling
- Empty data handling
- File I/O operations

### File Scanner (13 tests)
- Input file discovery
- Output file discovery
- Directory scanning
- Error handling

### Integration Tests (7 tests)
- Complete data pipeline
- End-to-end workflows
- Error handling integration
- Command line integration

## Test Results Summary

Total Tests: 92
- CommandLineParser: 12 tests (all passing)
- PsvParser: 12 tests 
- Database: 12 tests (all passing)
- QueryEngine: 12 tests
- TransformationEngine: 12 tests
- CsvWriter: 12 tests (all passing)
- FileScanner: 13 tests
- Integration: 7 tests

## Known Test Limitations

Some tests expect specific behavior that may differ from the actual implementation:
- WHERE clause evaluation uses string comparison, not numeric
- Some components handle errors gracefully rather than throwing exceptions
- Transformation engine may have implementation-specific behavior

## Adding New Tests

To add new tests:

1. Create test functions following the existing patterns
2. Use the Google Test framework macros (TEST_F, EXPECT_EQ, etc.)
3. Follow the naming convention: `ComponentTest.SpecificTestCase`
4. Update CMakeLists.txt if adding new test files
5. Rebuild and run tests to verify

## Continuous Integration

Tests can be integrated into CI/CD pipelines:
```bash
# Build and test in CI
mkdir build && cd build
cmake .. && make -j$(nproc)
./agile-pasta-tests --gtest_output=xml:test_results.xml
```

The test suite provides comprehensive coverage of the core functionality and helps ensure code quality and reliability.