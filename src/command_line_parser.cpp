#include "command_line_parser.h"
#include <iostream>
#include <string>

CommandLineArgs CommandLineParser::parse(int argc, char* argv[]) {
    CommandLineArgs args;
    
    if (argc < 2) {
        args.show_help = true;
        args.command = CommandLineArgs::Command::HELP;
        return args;
    }
    
    std::string command = argv[1];
    
    if (command == "help" || command == "--help" || command == "-h") {
        args.command = CommandLineArgs::Command::HELP;
        return args;
    }
    
    if (command == "transform") {
        args.command = CommandLineArgs::Command::TRANSFORM;
        
        // Parse --in and --out parameters
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "--in" && i + 1 < argc) {
                args.input_path = argv[++i];
            } else if (arg == "--out" && i + 1 < argc) {
                args.output_path = argv[++i];
            } else {
                // Unknown parameter
                args.command = CommandLineArgs::Command::INVALID;
                return args;
            }
        }
        
        return args;
    }
    
    args.command = CommandLineArgs::Command::INVALID;
    return args;
}

void CommandLineParser::print_help() {
    std::cout << R"(
Agile Pasta - Data Transformation Tool
=====================================

A high-performance, multi-threaded command-line application for transforming
pipe-separated value (PSV) data files into Excel-compatible CSV format using
configurable transformation rules.

SYNOPSIS
    agile-pasta help
    agile-pasta transform --in <input_path> --out <output_path>

COMMANDS
    help                    Show this help message and usage examples
    transform               Transform PSV data files to CSV format

OPTIONS
    --in <path>            Input directory path (searches recursively for PSV files)
    --out <path>           Output directory path (searches recursively for rule files)

DESCRIPTION
    The transform command processes PSV data files and applies transformation rules
    to generate Excel-compatible CSV output files. The program supports:
    
    • Multi-threaded data loading for performance
    • SQL-like operations (SELECT, JOIN, UNION)
    • Global and field-specific transformation rules
    • Progress bars for all operations
    • Cross-platform compatibility (Windows/Linux)

INPUT FILES
    The --in directory should contain pairs of PSV files:
    
    • Data files: iFile_A.psv, iFile_B.psv, etc.
        Contains the actual data records with pipe-separated values
        
    • Header files: iFile_A_Headers.psv, iFile_B_Headers.psv, etc.
        Contains column names for the corresponding data files

    Example data file (employees.psv):
        1001|John Doe|Software Engineer|2023-01-15|85000
        1002|Jane Smith|Data Analyst|2023-02-01|75000
        
    Example header file (employees_Headers.psv):
        emp_id|name|position|hire_date|salary

OUTPUT FILES
    The --out directory should contain pairs of configuration files:
    
    • Header files: oFile_A_Headers.psv, oFile_B_Headers.psv, etc.
        Defines the column names for the output CSV files
        
    • Rule files: oFile_A_Rules.psv, oFile_B_Rules.psv, etc.
        Contains transformation rules in plain text format

    Example output header file (summary_Headers.psv):
        employee_name|department|annual_salary
        
    Example rule file (summary_Rules.psv):
        GLOBAL|hire_date >= '2023-01-01'|Only employees hired in 2023 or later
        FIELD|annual_salary|salary * 12|Convert monthly to annual salary
        FIELD|employee_name|UPPER(name)|Convert name to uppercase

RULE FORMAT
    Transformation rules support two types:
    
    GLOBAL rules:
        Applied to filter all records before field transformations
        Format: GLOBAL|<condition>|<description>
        Example: GLOBAL|department = 'Engineering'|Only engineering employees
        
    FIELD rules:
        Applied to transform specific output fields
        Format: FIELD|<output_field>|<expression>|<description>
        Example: FIELD|full_name|first_name + ' ' + last_name|Combine names

EXAMPLES
    # Show help information
    agile-pasta help
    
    # Transform data files
    agile-pasta transform --in /data/input --out /data/output
    
    # Windows example
    agile-pasta transform --in C:\Data\Input --out C:\Data\Output

SUPPORTED OPERATIONS
    • SELECT: Query specific columns from tables
    • JOIN: Combine data from multiple tables (INNER, LEFT, RIGHT, FULL)
    • UNION: Combine rows from multiple tables
    • WHERE: Filter records based on conditions
    • Field transformations using expressions

AUTHORS
    Built with indicators library for progress reporting
    Cross-platform C++ implementation

)" << std::endl;
}

void CommandLineParser::print_usage() {
    std::cout << "Usage: agile-pasta help" << std::endl;
    std::cout << "       agile-pasta transform --in <input_path> --out <output_path>" << std::endl;
    std::cout << "Try 'agile-pasta help' for more information." << std::endl;
}