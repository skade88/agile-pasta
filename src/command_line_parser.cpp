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
    
    if (command == "check" || command == "sanity-check") {
        args.command = CommandLineArgs::Command::SANITY_CHECK;
        
        // Parse --out parameter for sanity check
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "--out" && i + 1 < argc) {
                args.sanity_check_path = argv[++i];
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
    agile-pasta check --out <output_path>

COMMANDS
    help                    Show this help message and usage examples
    transform               Transform PSV data files to CSV format
    check                   Run sanity checks on output configuration files

OPTIONS
    --in <path>            Input directory path (searches recursively for PSV files)
    --out <path>           Output directory path (searches recursively for rule files)
                          For 'check' command: path to validate configuration files

DESCRIPTION
    The transform command processes PSV data files and applies transformation rules
    to generate Excel-compatible CSV output files. 
    
    The check command validates output configuration files to ensure they exist
    and have correct syntax before running transformations.
    
    The program supports:
    
    • Multi-threaded data loading for performance
    • SQL-like operations (SELECT, JOIN, UNION)
    • Global and field-specific transformation rules
    • Progress bars for all operations
    • Cross-platform compatibility (Windows/Linux)
    • Configuration file validation

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
        
        If-else format: GLOBAL|<condition> ? ACCEPT : REJECT|<description>
        Example: GLOBAL|salary >= '75000' ? ACCEPT : REJECT|Only high earners
        
    FIELD rules:
        Applied to transform specific output fields
        Format: FIELD|<output_field>|<expression>|<description>
        Example: FIELD|full_name|first_name + ' ' + last_name|Combine names
        
        If-else format: FIELD|<output_field>|<condition> ? <value1> : <value2>|<description>
        Example: FIELD|status|salary >= '80000' ? 'High' : 'Standard'|Salary tier

EXAMPLES
    # Show help information
    agile-pasta help
    
    # Transform data files
    agile-pasta transform --in /data/input --out /data/output
    
    # Run sanity checks on output configuration
    agile-pasta check --out /data/output
    
    # Windows example
    agile-pasta transform --in C:\Data\Input --out C:\Data\Output

SANITY CHECKS
    The 'check' command validates output configuration files by:
    
    • Verifying that both _Headers.psv and _Rules.psv files exist for each output
    • Validating header file syntax (non-empty, valid PSV format)
    • Validating rule file syntax using the same parser as transform command
    • Reporting detailed status for each configuration pair
    • Providing summary of passed/failed configurations

SUPPORTED OPERATIONS
    • SELECT: Query specific columns from tables
    • JOIN: Combine data from multiple tables (INNER, LEFT, RIGHT, FULL)
    • UNION: Combine rows from multiple tables
    • WHERE: Filter records based on conditions
    • Field transformations using expressions

AUTHORS
    Built with custom ANSI/VT progress bars for enhanced reporting
    Cross-platform C++ implementation

)" << std::endl;
}

void CommandLineParser::print_usage() {
    std::cout << "Usage: agile-pasta help" << std::endl;
    std::cout << "       agile-pasta transform --in <input_path> --out <output_path>" << std::endl;
    std::cout << "       agile-pasta check --out <output_path>" << std::endl;
    std::cout << "Try 'agile-pasta help' for more information." << std::endl;
}