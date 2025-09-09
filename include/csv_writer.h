#pragma once

#include "query_engine.h"
#include <string>
#include <filesystem>

class CsvWriter {
public:
    // Write query result to CSV file (Excel compatible)
    static bool write_csv(const QueryResult& result, 
                         const std::filesystem::path& output_path);
    
    // Write with progress reporting
    static bool write_csv_with_progress(const QueryResult& result, 
                                       const std::filesystem::path& output_path);

private:
    // Escape CSV field if needed
    static std::string escape_csv_field(const std::string& field);
    
    // Check if field needs quoting
    static bool needs_quoting(const std::string& field);
};