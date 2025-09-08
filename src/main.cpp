#include "command_line_parser.h"
#include "file_scanner.h"
#include "psv_parser.h"
#include "database.h"
#include "query_engine.h"
#include "transformation_engine.h"
#include "csv_writer.h"
#include "progress_manager.h"

#include <iostream>
#include <thread>
#include <future>
#include <chrono>

void load_data_multithreaded(const std::vector<FileInfo>& files, Database& database) {
    std::cout << "\nLoading data files..." << std::endl;
    
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
    
    std::cout << "Loaded " << database.get_total_records() 
              << " total records from " << files.size() << " files." << std::endl;
}

void process_sanity_check(const std::string& output_path) {
    try {
        std::cout << "Running sanity checks on output directory: " << output_path << std::endl;
        
        // Step 1: Scan output files
        auto output_files = FileScanner::scan_output_files(output_path);
        
        if (output_files.empty()) {
            std::cout << "No output configuration files found in: " << output_path << std::endl;
            return;
        }
        
        FileScanner::display_output_structure(output_files);
        
        int total_files = output_files.size();
        int passed_files = 0;
        int failed_files = 0;
        
        std::cout << "\nRunning sanity checks..." << std::endl;
        std::cout << "================================================================================\n";
        
        // Step 2: Validate each output file pair
        for (const auto& output_file : output_files) {
            std::cout << "\nChecking: " << output_file.name_prefix << std::endl;
            bool file_passed = true;
            
            // Check 1: Verify both files exist
            if (!std::filesystem::exists(output_file.headers_path)) {
                std::cout << "  ❌ Headers file missing: " << output_file.headers_path << std::endl;
                file_passed = false;
            } else {
                std::cout << "  ✅ Headers file exists: " << output_file.headers_path << std::endl;
            }
            
            if (!std::filesystem::exists(output_file.rules_path)) {
                std::cout << "  ❌ Rules file missing: " << output_file.rules_path << std::endl;
                file_passed = false;
            } else {
                std::cout << "  ✅ Rules file exists: " << output_file.rules_path << std::endl;
            }
            
            // Check 2: Validate headers file syntax using existing parser
            if (std::filesystem::exists(output_file.headers_path)) {
                try {
                    auto headers = PsvParser::parse_headers(output_file.headers_path);
                    if (headers.empty()) {
                        std::cout << "  ❌ Headers file is empty or invalid: " << output_file.headers_path << std::endl;
                        file_passed = false;
                    } else {
                        std::cout << "  ✅ Headers file syntax valid (" << headers.size() << " headers)" << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cout << "  ❌ Headers file syntax error: " << e.what() << std::endl;
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
                    std::cout << "  ✅ Rules file syntax valid" << std::endl;
                } catch (const std::exception& e) {
                    std::cout << "  ❌ Rules file syntax error: " << e.what() << std::endl;
                    file_passed = false;
                }
            }
            
            if (file_passed) {
                std::cout << "  ✅ Overall: PASSED" << std::endl;
                passed_files++;
            } else {
                std::cout << "  ❌ Overall: FAILED" << std::endl;
                failed_files++;
            }
        }
        
        // Summary
        std::cout << "\n================================================================================\n";
        std::cout << "Sanity check summary:" << std::endl;
        std::cout << "  Total configurations: " << total_files << std::endl;
        std::cout << "  Passed: " << passed_files << std::endl;
        std::cout << "  Failed: " << failed_files << std::endl;
        
        if (failed_files == 0) {
            std::cout << "  🎉 All sanity checks PASSED!" << std::endl;
        } else {
            std::cout << "  ⚠️  Some sanity checks FAILED. Please fix the issues above." << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error during sanity check: " << e.what() << std::endl;
    }
}

void process_transformation(const std::string& input_path, const std::string& output_path) {
    try {
        // Step 1: Scan input files
        std::cout << "Scanning input directory: " << input_path << std::endl;
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
        std::cout << "\nScanning output directory: " << output_path << std::endl;
        auto output_files = FileScanner::scan_output_files(output_path);
        
        if (output_files.empty()) {
            std::cerr << "No output rule files found in: " << output_path << std::endl;
            return;
        }
        
        FileScanner::display_output_structure(output_files);
        
        // Step 4: Process transformations for each output file
        QueryEngine query_engine(database);
        
        for (const auto& output_file : output_files) {
            std::cout << "\nProcessing transformation: " << output_file.name_prefix << std::endl;
            
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
                
                std::cout << "Writing output: " << output_csv_path << std::endl;
                
                if (CsvWriter::write_csv_with_progress(*transformed_data, output_csv_path)) {
                    std::cout << "Successfully wrote " << transformed_data->rows.size() 
                              << " records to " << output_csv_path << std::endl;
                } else {
                    std::cerr << "Failed to write output file: " << output_csv_path << std::endl;
                }
            }
        }
        
        std::cout << "\nTransformation complete!" << std::endl;
        
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