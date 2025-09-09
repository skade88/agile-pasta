#include "file_scanner.h"
#include "ansi_output.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <regex>

std::vector<FileInfo> FileScanner::scan_input_files(const std::string& root_path) {
    std::vector<FileInfo> files;
    
    try {
        std::filesystem::path root(root_path);
        
        if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
            throw std::runtime_error("Input path does not exist or is not a directory: " + root_path);
        }
        
        // Use recursive iterator to find all .psv files
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
            if (!entry.is_regular_file()) continue;
            
            std::string filename = entry.path().filename().string();
            
            // Look for data files (not header files)
            if (filename.length() >= 4 && filename.substr(filename.length() - 4) == ".psv" && 
                !(filename.length() >= 12 && filename.substr(filename.length() - 12) == "_Headers.psv")) {
                // Extract prefix (everything before .psv)
                std::string prefix = filename.substr(0, filename.length() - 4);
                
                // Look for corresponding header file
                std::filesystem::path headers_path = entry.path().parent_path() / (prefix + "_Headers.psv");
                
                if (std::filesystem::exists(headers_path)) {
                    FileInfo info;
                    info.path = entry.path();
                    info.headers_path = headers_path;
                    info.size_bytes = std::filesystem::file_size(entry.path());
                    info.name_prefix = prefix;
                    
                    files.push_back(info);
                }
            }
        }
        
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("Filesystem error while scanning input files: " + std::string(e.what()));
    }
    
    return files;
}

std::vector<OutputFileInfo> FileScanner::scan_output_files(const std::string& root_path) {
    std::vector<OutputFileInfo> files;
    
    try {
        std::filesystem::path root(root_path);
        
        if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
            throw std::runtime_error("Output path does not exist or is not a directory: " + root_path);
        }
        
        // Use recursive iterator to find header files
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
            if (!entry.is_regular_file()) continue;
            
            std::string filename = entry.path().filename().string();
            
            // Look for header files
            if (filename.length() >= 12 && filename.substr(filename.length() - 12) == "_Headers.psv") {
                // Extract prefix (everything before _Headers.psv)
                std::string prefix = filename.substr(0, filename.length() - 12);
                
                // Look for corresponding rule file
                std::filesystem::path rules_path = entry.path().parent_path() / (prefix + "_Rules.psv");
                
                if (std::filesystem::exists(rules_path)) {
                    OutputFileInfo info;
                    info.headers_path = entry.path();
                    info.rules_path = rules_path;
                    info.name_prefix = prefix;
                    
                    files.push_back(info);
                }
            }
        }
        
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("Filesystem error while scanning output files: " + std::string(e.what()));
    }
    
    return files;
}

void FileScanner::display_file_structure(const std::vector<FileInfo>& files) {
    AnsiOutput::success("\nFound " + std::to_string(files.size()) + " input file pairs:");
    AnsiOutput::separator(80, '-');
    
    for (const auto& file : files) {
        AnsiOutput::info("Data file:   " + file.path.string() + " (" + format_file_size(file.size_bytes) + ")");
        AnsiOutput::plain("Header file: " + file.headers_path.string());
        AnsiOutput::plain("Prefix:      " + file.name_prefix);
        AnsiOutput::plain("");
    }
}

void FileScanner::display_output_structure(const std::vector<OutputFileInfo>& files) {
    AnsiOutput::success("\nFound " + std::to_string(files.size()) + " output configuration pairs:");
    AnsiOutput::separator(80, '-');
    
    for (const auto& file : files) {
        AnsiOutput::info("Headers file: " + file.headers_path.string());
        AnsiOutput::plain("Rules file:   " + file.rules_path.string());
        AnsiOutput::plain("Output name:  " + file.name_prefix + ".csv");
        AnsiOutput::plain("");
    }
}

std::string FileScanner::format_file_size(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size << " " << units[unit_index];
    return oss.str();
}