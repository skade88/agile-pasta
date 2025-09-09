#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <map>
#include <functional>

struct PsvRecord {
    std::vector<std::string> fields;
};

struct PsvTable {
    std::string name;
    std::vector<std::string> headers;
    std::vector<PsvRecord> records;
    std::filesystem::path source_file;
    
    // Index for fast lookups
    std::map<std::string, size_t> header_index;
    
    void build_header_index();
    std::string get_field(size_t record_idx, const std::string& header) const;
};

class PsvParser {
public:
    // Parse a PSV file with its headers
    static std::unique_ptr<PsvTable> parse_file(const std::filesystem::path& data_path, 
                                               const std::filesystem::path& headers_path);
    
    // Parse a PSV file with progress callback
    static std::unique_ptr<PsvTable> parse_file(const std::filesystem::path& data_path, 
                                               const std::filesystem::path& headers_path,
                                               std::function<void(size_t)> progress_callback);
    
    // Parse headers file
    static std::vector<std::string> parse_headers(const std::filesystem::path& headers_path);
    
    // Parse data file with progress reporting
    static std::vector<PsvRecord> parse_data(const std::filesystem::path& data_path, 
                                            size_t& total_records);
                                            
    // Parse data file with callback for progress reporting
    static std::vector<PsvRecord> parse_data_with_callback(const std::filesystem::path& data_path, 
                                                          size_t& total_records,
                                                          std::function<void(size_t)> progress_callback);

private:
    static std::vector<std::string> split_psv_line(const std::string& line);
    static std::string trim(const std::string& str);
};