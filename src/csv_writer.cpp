#include "csv_writer.h"
#include "progress_manager.h"
#include <fstream>
#include <sstream>
#include <algorithm>

bool CsvWriter::write_csv(const QueryResult& result, const std::filesystem::path& output_path) {
    std::ofstream file(output_path);
    if (!file.is_open()) {
        return false;
    }
    
    // Write headers
    for (size_t i = 0; i < result.headers.size(); ++i) {
        if (i > 0) file << ",";
        file << escape_csv_field(result.headers[i]);
    }
    file << "\n";
    
    // Write data rows
    for (const auto& row : result.rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0) file << ",";
            file << escape_csv_field(row[i]);
        }
        file << "\n";
    }
    
    return file.good();
}

bool CsvWriter::write_csv_with_progress(const QueryResult& result, 
                                       const std::filesystem::path& output_path) {
    std::ofstream file(output_path);
    if (!file.is_open()) {
        return false;
    }
    
    // Create progress bar
    auto progress = ProgressManager::create_processing_progress(
        "Writing " + output_path.filename().string(), result.rows.size() + 1);
    
    // Write headers
    for (size_t i = 0; i < result.headers.size(); ++i) {
        if (i > 0) file << ",";
        file << escape_csv_field(result.headers[i]);
    }
    file << "\n";
    
    ProgressManager::update_progress(*progress, 1);
    
    // Write data rows
    for (size_t row_idx = 0; row_idx < result.rows.size(); ++row_idx) {
        const auto& row = result.rows[row_idx];
        
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0) file << ",";
            file << escape_csv_field(row[i]);
        }
        file << "\n";
        
        // Update progress periodically
        if (row_idx % 100 == 0 || row_idx == result.rows.size() - 1) {
            ProgressManager::update_progress(*progress, row_idx + 2);
        }
    }
    
    ProgressManager::complete_progress(*progress);
    return file.good();
}

std::string CsvWriter::escape_csv_field(const std::string& field) {
    if (needs_quoting(field)) {
        std::string escaped = "\"";
        for (char c : field) {
            if (c == '"') {
                escaped += "\"\""; // Escape quotes by doubling them
            } else {
                escaped += c;
            }
        }
        escaped += "\"";
        return escaped;
    }
    return field;
}

bool CsvWriter::needs_quoting(const std::string& field) {
    // Quote if contains comma, quote, newline, or starts/ends with whitespace
    return field.find(',') != std::string::npos ||
           field.find('"') != std::string::npos ||
           field.find('\n') != std::string::npos ||
           field.find('\r') != std::string::npos ||
           (!field.empty() && (std::isspace(field.front()) || std::isspace(field.back())));
}