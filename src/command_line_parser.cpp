#include "command_line_parser.h"
#include "ansi_output.h"
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
    AnsiOutput::header("Agile Pasta - Data Transformation Tool");
    AnsiOutput::separator();
    AnsiOutput::plain("");
    AnsiOutput::plain("A high-performance, multi-threaded command-line application for transforming");
    AnsiOutput::plain("pipe-separated value (PSV) data files into Excel-compatible CSV format using");
    AnsiOutput::plain("configurable transformation rules.");
    AnsiOutput::plain("");
    AnsiOutput::styled("SYNOPSIS", AnsiOutput::Color::yellow, AnsiOutput::Style::bold);
    AnsiOutput::plain("    agile-pasta help");
    AnsiOutput::plain("    agile-pasta transform --in <input_path> --out <output_path>");
    AnsiOutput::plain("    agile-pasta check --out <output_path>");
    AnsiOutput::plain("");
    AnsiOutput::styled("COMMANDS", AnsiOutput::Color::yellow, AnsiOutput::Style::bold);
    AnsiOutput::styled("    help", AnsiOutput::Color::cyan);
    AnsiOutput::plain("                    Show this help message and usage examples");
    AnsiOutput::styled("    transform", AnsiOutput::Color::cyan);
    AnsiOutput::plain("               Transform PSV data files to CSV format");
    AnsiOutput::styled("    check", AnsiOutput::Color::cyan);
    AnsiOutput::plain("                   Run sanity checks on output configuration files");
    AnsiOutput::plain("");
    AnsiOutput::styled("OPTIONS", AnsiOutput::Color::yellow, AnsiOutput::Style::bold);
    AnsiOutput::styled("    --in <path>", AnsiOutput::Color::cyan);
    AnsiOutput::plain("            Input directory path (searches recursively for PSV files)");
    AnsiOutput::styled("    --out <path>", AnsiOutput::Color::cyan);
    AnsiOutput::plain("           Output directory path (searches recursively for rule files)");
    AnsiOutput::plain("                          For 'check' command: path to validate configuration files");
    AnsiOutput::plain("");
    AnsiOutput::styled("DESCRIPTION", AnsiOutput::Color::yellow, AnsiOutput::Style::bold);
    AnsiOutput::plain("    The transform command processes PSV data files and applies transformation rules");
    AnsiOutput::plain("    to generate Excel-compatible CSV output files.");
    AnsiOutput::plain("");
    AnsiOutput::plain("    The check command validates output configuration files to ensure they exist");
    AnsiOutput::plain("    and have correct syntax before running transformations.");
    AnsiOutput::plain("");
    AnsiOutput::plain("    The program supports:");
    AnsiOutput::plain("");
    AnsiOutput::plain("    • Multi-threaded data loading for performance");
    AnsiOutput::plain("    • SQL-like operations (SELECT, JOIN, UNION)");
    AnsiOutput::plain("    • Global and field-specific transformation rules");
    AnsiOutput::plain("    • Progress bars for all operations");
    AnsiOutput::plain("    • Cross-platform compatibility (Windows/Linux)");
    AnsiOutput::plain("    • Configuration file validation");
    AnsiOutput::plain("");
    AnsiOutput::styled("INPUT FILES", AnsiOutput::Color::yellow, AnsiOutput::Style::bold);
    AnsiOutput::plain("    The --in directory should contain pairs of PSV files:");
    AnsiOutput::plain("");
    AnsiOutput::plain("    • Data files: iFile_A.psv, iFile_B.psv, etc.");
    AnsiOutput::plain("        Contains the actual data records with pipe-separated values");
    AnsiOutput::plain("");
    AnsiOutput::plain("    • Header files: iFile_A_Headers.psv, iFile_B_Headers.psv, etc.");
    AnsiOutput::plain("        Contains column names for the corresponding data files");
    AnsiOutput::plain("");
    AnsiOutput::plain("    Example data file (employees.psv):");
    AnsiOutput::styled("        1001|John Doe|Software Engineer|2023-01-15|85000", AnsiOutput::Color::green);
    AnsiOutput::styled("        1002|Jane Smith|Data Analyst|2023-02-01|75000", AnsiOutput::Color::green);
    AnsiOutput::plain("");
    AnsiOutput::plain("    Example header file (employees_Headers.psv):");
    AnsiOutput::styled("        emp_id|name|position|hire_date|salary", AnsiOutput::Color::green);
    AnsiOutput::plain("");
    AnsiOutput::styled("OUTPUT FILES", AnsiOutput::Color::yellow, AnsiOutput::Style::bold);
    AnsiOutput::plain("    The --out directory should contain pairs of configuration files:");
    AnsiOutput::plain("");
    AnsiOutput::plain("    • Header files: oFile_A_Headers.psv, oFile_B_Headers.psv, etc.");
    AnsiOutput::plain("        Defines the column names for the output CSV files");
    AnsiOutput::plain("");
    AnsiOutput::plain("    • Rule files: oFile_A_Rules.psv, oFile_B_Rules.psv, etc.");
    AnsiOutput::plain("        Contains transformation rules in plain text format");
    AnsiOutput::plain("");
    AnsiOutput::plain("    Example output header file (summary_Headers.psv):");
    AnsiOutput::styled("        employee_name|department|annual_salary", AnsiOutput::Color::green);
    AnsiOutput::plain("");
    AnsiOutput::plain("    Example rule file (summary_Rules.psv):");
    AnsiOutput::styled("        GLOBAL|hire_date >= '2023-01-01'|Only employees hired in 2023 or later", AnsiOutput::Color::green);
    AnsiOutput::styled("        FIELD|annual_salary|salary * 12|Convert monthly to annual salary", AnsiOutput::Color::green);
    AnsiOutput::styled("        FIELD|employee_name|UPPER(name)|Convert name to uppercase", AnsiOutput::Color::green);
    AnsiOutput::plain("");
    AnsiOutput::styled("RULE FORMAT", AnsiOutput::Color::yellow, AnsiOutput::Style::bold);
    AnsiOutput::plain("    Transformation rules support two types:");
    AnsiOutput::plain("");
    AnsiOutput::styled("    GLOBAL rules:", AnsiOutput::Color::cyan, AnsiOutput::Style::bold);
    AnsiOutput::plain("        Applied to filter all records before field transformations");
    AnsiOutput::plain("        Format: GLOBAL|<condition>|<description>");
    AnsiOutput::plain("        Example: GLOBAL|department = 'Engineering'|Only engineering employees");
    AnsiOutput::plain("");
    AnsiOutput::plain("        If-else format: GLOBAL|<condition> ? ACCEPT : REJECT|<description>");
    AnsiOutput::plain("        Example: GLOBAL|salary >= '75000' ? ACCEPT : REJECT|Only high earners");
    AnsiOutput::plain("");
    AnsiOutput::styled("    FIELD rules:", AnsiOutput::Color::cyan, AnsiOutput::Style::bold);
    AnsiOutput::plain("        Applied to transform specific output fields");
    AnsiOutput::plain("        Format: FIELD|<output_field>|<expression>|<description>");
    AnsiOutput::plain("        Example: FIELD|full_name|first_name + ' ' + last_name|Combine names");
    AnsiOutput::plain("");
    AnsiOutput::plain("        If-else format: FIELD|<output_field>|<condition> ? <value1> : <value2>|<description>");
    AnsiOutput::plain("        Example: FIELD|status|salary >= '80000' ? 'High' : 'Standard'|Salary tier");
    AnsiOutput::plain("");
    AnsiOutput::styled("EXAMPLES", AnsiOutput::Color::yellow, AnsiOutput::Style::bold);
    AnsiOutput::plain("    # Show help information");
    AnsiOutput::styled("    agile-pasta help", AnsiOutput::Color::green);
    AnsiOutput::plain("");
    AnsiOutput::plain("    # Transform data files");
    AnsiOutput::styled("    agile-pasta transform --in /data/input --out /data/output", AnsiOutput::Color::green);
    AnsiOutput::plain("");
    AnsiOutput::plain("    # Run sanity checks on output configuration");
    AnsiOutput::styled("    agile-pasta check --out /data/output", AnsiOutput::Color::green);
    AnsiOutput::plain("");
    AnsiOutput::plain("    # Windows example");
    AnsiOutput::styled("    agile-pasta transform --in C:\\Data\\Input --out C:\\Data\\Output", AnsiOutput::Color::green);
    AnsiOutput::plain("");
    AnsiOutput::styled("SANITY CHECKS", AnsiOutput::Color::yellow, AnsiOutput::Style::bold);
    AnsiOutput::plain("    The 'check' command validates output configuration files by:");
    AnsiOutput::plain("");
    AnsiOutput::plain("    • Verifying that both _Headers.psv and _Rules.psv files exist for each output");
    AnsiOutput::plain("    • Validating header file syntax (non-empty, valid PSV format)");
    AnsiOutput::plain("    • Validating rule file syntax using the same parser as transform command");
    AnsiOutput::plain("    • Reporting detailed status for each configuration pair");
    AnsiOutput::plain("    • Providing summary of passed/failed configurations");
    AnsiOutput::plain("");
    AnsiOutput::styled("SUPPORTED OPERATIONS", AnsiOutput::Color::yellow, AnsiOutput::Style::bold);
    AnsiOutput::plain("    • SELECT: Query specific columns from tables");
    AnsiOutput::plain("    • JOIN: Combine data from multiple tables (INNER, LEFT, RIGHT, FULL)");
    AnsiOutput::plain("    • UNION: Combine rows from multiple tables");
    AnsiOutput::plain("    • WHERE: Filter records based on conditions");
    AnsiOutput::plain("    • Field transformations using expressions");
    AnsiOutput::plain("");
    AnsiOutput::styled("AUTHORS", AnsiOutput::Color::yellow, AnsiOutput::Style::bold);
    AnsiOutput::styled("    Built with custom ANSI/VT progress bars for enhanced reporting", AnsiOutput::Color::cyan);
    AnsiOutput::styled("    Cross-platform C++ implementation", AnsiOutput::Color::cyan);
    AnsiOutput::plain("");
}

void CommandLineParser::print_usage() {
    AnsiOutput::plain("Usage: agile-pasta help");
    AnsiOutput::plain("       agile-pasta transform --in <input_path> --out <output_path>");
    AnsiOutput::plain("       agile-pasta check --out <output_path>");
    AnsiOutput::info("Try 'agile-pasta help' for more information.");
}