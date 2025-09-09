#include "command_line_parser.h"
#include "file_scanner.h"
#include "psv_parser.h"
#include "database.h"
#include "query_engine.h"
#include "transformation_engine.h"
#include "csv_writer.h"
#include "progress_manager.h"
#include "ansi_output.h"

#include <iostream>
#include <thread>
#include <future>
#include <chrono>

void load_data_multithreaded(const std::vector<FileInfo>& files, Database& database) {
    AnsiOutput::info("\nLoading data files...");
    
    std::vector<std::future<std::unique_ptr<PsvTable>>> futures;
    
    for (const auto& file : files) {
        auto future = std::async(std::launch::async, [&file]() {
            auto progress = ProgressManager::create_file_progress(
                file.path.filename().string(), file.size_bytes);
            
            auto table = PsvParser::parse_file(file.path, file.headers_path);
            
            ProgressManager::complete_progress(*progress);
            return table;
        });
        
        futures.push_back(std::move(future));
    }
    
    // Collect results
    for (auto& future : futures) {
        auto table = future.get();
        if (table) {
            database.load_table(std::move(table));
        }
    }
    
    AnsiOutput::success("Loaded " + std::to_string(database.get_total_records()) + 
                     " total records from " + std::to_string(files.size()) + " files.");
}

void process_sanity_check(const std::string& output_path) {
    try {
        AnsiOutput::info("Running sanity checks on output directory: " + output_path);
        
        // Step 1: Scan output files
        auto output_files = FileScanner::scan_output_files(output_path);
        
        if (output_files.empty()) {
            AnsiOutput::warning("No output configuration files found in: " + output_path);
            return;
        }
        
        FileScanner::display_output_structure(output_files);
        
        int total_files = output_files.size();
        int passed_files = 0;
        int failed_files = 0;
        
        AnsiOutput::plain("");
        AnsiOutput::info("Running sanity checks...");
        AnsiOutput::separator();
        
        // Step 2: Validate each output file pair
        for (const auto& output_file : output_files) {
            AnsiOutput::plain("");
            AnsiOutput::header("Checking: " + output_file.name_prefix);
            bool file_passed = true;
            
            // Check 1: Verify both files exist
            if (!std::filesystem::exists(output_file.headers_path)) {
                AnsiOutput::error("  ❌ Headers file missing: " + output_file.headers_path.string());
                file_passed = false;
            } else {
                AnsiOutput::success("  ✅ Headers file exists: " + output_file.headers_path.string());
            }
            
            if (!std::filesystem::exists(output_file.rules_path)) {
                AnsiOutput::error("  ❌ Rules file missing: " + output_file.rules_path.string());
                file_passed = false;
            } else {
                AnsiOutput::success("  ✅ Rules file exists: " + output_file.rules_path.string());
            }
            
            // Check 2: Validate headers file syntax using existing parser
            if (std::filesystem::exists(output_file.headers_path)) {
                try {
                    auto headers = PsvParser::parse_headers(output_file.headers_path);
                    if (headers.empty()) {
                        AnsiOutput::error("  ❌ Headers file is empty or invalid: " + output_file.headers_path.string());
                        file_passed = false;
                    } else {
                        AnsiOutput::success("  ✅ Headers file syntax valid (" + std::to_string(headers.size()) + " headers)");
                    }
                } catch (const std::exception& e) {
                    AnsiOutput::error("  ❌ Headers file syntax error: " + std::string(e.what()));
                    file_passed = false;
                }
            }
            
            // Check 3: Validate rules file syntax using existing parser
            if (std::filesystem::exists(output_file.rules_path)) {
                try {
                    // Create a dummy database and query engine for validation
                    Database dummy_db;
                    QueryEngine dummy_query(dummy_db);
                    TransformationEngine temp_engine(dummy_db, dummy_query);
                    
                    temp_engine.load_rules(output_file.rules_path);
                    AnsiOutput::success("  ✅ Rules file syntax valid");
                } catch (const std::exception& e) {
                    AnsiOutput::error("  ❌ Rules file syntax error: " + std::string(e.what()));
                    file_passed = false;
                }
            }
            
            if (file_passed) {
                AnsiOutput::success("  ✅ Overall: PASSED");
                passed_files++;
            } else {
                AnsiOutput::error("  ❌ Overall: FAILED");
                failed_files++;
            }
        }
        
        // Summary
        AnsiOutput::plain("");
        AnsiOutput::separator();
        AnsiOutput::header("Sanity check summary:");
        AnsiOutput::plain("  Total configurations: " + std::to_string(total_files));
        AnsiOutput::plain("  Passed: " + std::to_string(passed_files));
        AnsiOutput::plain("  Failed: " + std::to_string(failed_files));
        
        if (failed_files == 0) {
            AnsiOutput::success("  🎉 All sanity checks PASSED!");
        } else {
            AnsiOutput::warning("  ⚠️  Some sanity checks FAILED. Please fix the issues above.");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error during sanity check: " << e.what() << std::endl;
    }
}

void process_transformation(const std::string& input_path, const std::string& output_path) {
    try {
        // Step 1: Scan input files
        AnsiOutput::info("Scanning input directory: " + input_path);
        auto input_files = FileScanner::scan_input_files(input_path);
        
        if (input_files.empty()) {
            std::cerr << "No input PSV files found in: " << input_path << std::endl;
            return;
        }
        
        FileScanner::display_file_structure(input_files);
        
        // Step 2: Load data into database
        Database database;
        load_data_multithreaded(input_files, database);
        
        // Step 3: Scan output files for transformation rules
        AnsiOutput::info("\nScanning output directory: " + output_path);
        auto output_files = FileScanner::scan_output_files(output_path);
        
        if (output_files.empty()) {
            std::cerr << "No output rule files found in: " << output_path << std::endl;
            return;
        }
        
        FileScanner::display_output_structure(output_files);
        
        // Step 4: Process transformations for each output file
        QueryEngine query_engine(database);
        
        for (const auto& output_file : output_files) {
            AnsiOutput::header("\nProcessing transformation: " + output_file.name_prefix);
            
            TransformationEngine transform_engine(database, query_engine);
            
            // Load transformation rules and headers
            transform_engine.load_output_headers(output_file.headers_path);
            transform_engine.load_rules(output_file.rules_path);
            
            // Transform data
            auto transformed_data = transform_engine.transform_data();
            
            if (transformed_data) {
                // Write CSV output
                auto output_csv_path = output_file.headers_path.parent_path() / 
                                     (output_file.name_prefix + ".csv");
                
                AnsiOutput::info("Writing output: " + output_csv_path.string());
                
                if (CsvWriter::write_csv_with_progress(*transformed_data, output_csv_path)) {
                    AnsiOutput::success("Successfully wrote " + std::to_string(transformed_data->rows.size()) + 
                                       " records to " + output_csv_path.string());
                } else {
                    std::cerr << "Failed to write output file: " << output_csv_path << std::endl;
                }
            }
        }
        
        AnsiOutput::success("\nTransformation complete!");
        
    } catch (const std::exception& e) {
        std::cerr << "Error during transformation: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    try {
        auto args = CommandLineParser::parse(argc, argv);
        
        switch (args.command) {
            case CommandLineArgs::Command::HELP:
                CommandLineParser::print_help();
                return 0;
                
            case CommandLineArgs::Command::TRANSFORM:
                if (args.input_path.empty() || args.output_path.empty()) {
                    std::cerr << "Error: Both --in and --out paths are required for transform command." << std::endl;
                    CommandLineParser::print_usage();
                    return 1;
                }
                process_transformation(args.input_path, args.output_path);
                return 0;
                
            case CommandLineArgs::Command::SANITY_CHECK:
                if (args.sanity_check_path.empty()) {
                    std::cerr << "Error: --out path is required for check command." << std::endl;
                    CommandLineParser::print_usage();
                    return 1;
                }
                process_sanity_check(args.sanity_check_path);
                return 0;
                
            case CommandLineArgs::Command::INVALID:
            default:
                std::cerr << "Error: Invalid command or arguments." << std::endl;
                CommandLineParser::print_usage();
                return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}