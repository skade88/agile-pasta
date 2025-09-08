#pragma once

#include "psv_parser.h"
#include <memory>
#include <unordered_map>
#include <string>

class Database {
public:
    // Load all tables from parsed data
    void load_table(std::unique_ptr<PsvTable> table);
    
    // Get table by name
    const PsvTable* get_table(const std::string& name) const;
    
    // Get all table names
    std::vector<std::string> get_table_names() const;
    
    // Get total record count across all tables
    size_t get_total_records() const;
    
    // Clear all data
    void clear();

private:
    std::unordered_map<std::string, std::unique_ptr<PsvTable>> tables_;
};