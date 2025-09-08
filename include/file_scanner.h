#pragma once

#include <string>
#include <vector>
#include <filesystem>

struct FileInfo {
    std::filesystem::path path;
    std::filesystem::path headers_path;
    size_t size_bytes;
    std::string name_prefix;
};

struct OutputFileInfo {
    std::filesystem::path headers_path;
    std::filesystem::path rules_path;
    std::string name_prefix;
};

class FileScanner {
public:
    // Scan for input PSV files and their headers
    static std::vector<FileInfo> scan_input_files(const std::string& root_path);
    
    // Scan for output rule files
    static std::vector<OutputFileInfo> scan_output_files(const std::string& root_path);
    
    // Display found files with sizes
    static void display_file_structure(const std::vector<FileInfo>& files);
    static void display_output_structure(const std::vector<OutputFileInfo>& files);

private:
    static std::string format_file_size(size_t bytes);
};