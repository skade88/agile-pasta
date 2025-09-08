#include "psv_parser.h"
#include "progress_manager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

void PsvTable::build_header_index() {
    header_index.clear();
    for (size_t i = 0; i < headers.size(); ++i) {
        header_index[headers[i]] = i;
    }
}

std::string PsvTable::get_field(size_t record_idx, const std::string& header) const {
    if (record_idx >= records.size()) {
        return "";
    }
    
    auto it = header_index.find(header);
    if (it == header_index.end()) {
        return "";
    }
    
    size_t field_idx = it->second;
    if (field_idx >= records[record_idx].fields.size()) {
        return "";
    }
    
    return records[record_idx].fields[field_idx];
}

std::unique_ptr<PsvTable> PsvParser::parse_file(const std::filesystem::path& data_path, 
                                               const std::filesystem::path& headers_path) {
    auto table = std::make_unique<PsvTable>();
    
    // Parse headers first
    table->headers = parse_headers(headers_path);
    
    // Parse data
    size_t total_records = 0;
    table->records = parse_data(data_path, total_records);
    
    // Set metadata
    table->source_file = data_path;
    table->name = data_path.stem().string();
    
    // Build index for fast lookups
    table->build_header_index();
    
    return table;
}

std::vector<std::string> PsvParser::parse_headers(const std::filesystem::path& headers_path) {
    std::ifstream file(headers_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open headers file: " + headers_path.string());
    }
    
    std::string line;
    if (!std::getline(file, line)) {
        throw std::runtime_error("Headers file is empty: " + headers_path.string());
    }
    
    return split_psv_line(line);
}

std::vector<PsvRecord> PsvParser::parse_data(const std::filesystem::path& data_path, 
                                            size_t& total_records) {
    std::ifstream file(data_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open data file: " + data_path.string());
    }
    
    // Count lines first for progress reporting
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<PsvRecord> records;
    std::string line;
    size_t current_pos = 0;
    total_records = 0;
    
    // Create progress bar
    auto progress = ProgressManager::create_file_progress(
        data_path.filename().string(), file_size);
    
    while (std::getline(file, line)) {
        line = trim(line);
        if (!line.empty()) {
            PsvRecord record;
            record.fields = split_psv_line(line);
            records.push_back(record);
            total_records++;
        }
        
        // Update progress
        current_pos = file.tellg();
        if (current_pos != std::string::npos) {
            ProgressManager::update_progress(*progress, current_pos);
        }
    }
    
    ProgressManager::complete_progress(*progress);
    return records;
}

std::vector<std::string> PsvParser::split_psv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string field;
    
    while (std::getline(ss, field, '|')) {
        fields.push_back(trim(field));
    }
    
    return fields;
}

std::string PsvParser::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}