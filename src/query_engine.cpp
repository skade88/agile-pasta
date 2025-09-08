#include "query_engine.h"
#include <algorithm>
#include <regex>
#include <sstream>

QueryEngine::QueryEngine(const Database& db) : database_(db) {}

std::unique_ptr<QueryResult> QueryEngine::select(const std::string& table_name, 
                                                const std::vector<std::string>& columns) {
    const PsvTable* table = database_.get_table(table_name);
    if (!table) {
        return nullptr;
    }
    
    auto result = std::make_unique<QueryResult>();
    
    // Determine columns to select
    if (columns.empty()) {
        // Select all columns
        result->headers = table->headers;
    } else {
        result->headers = columns;
    }
    
    // Copy data
    for (const auto& record : table->records) {
        std::vector<std::string> row;
        
        for (const auto& header : result->headers) {
            auto it = table->header_index.find(header);
            if (it != table->header_index.end() && it->second < record.fields.size()) {
                row.push_back(record.fields[it->second]);
            } else {
                row.push_back("");
            }
        }
        
        result->rows.push_back(row);
    }
    
    return result;
}

std::unique_ptr<QueryResult> QueryEngine::select_where(const std::string& table_name, 
                                                      const std::vector<std::string>& columns,
                                                      const std::string& where_clause) {
    const PsvTable* table = database_.get_table(table_name);
    if (!table) {
        return nullptr;
    }
    
    auto result = std::make_unique<QueryResult>();
    
    // Determine columns to select
    if (columns.empty()) {
        result->headers = table->headers;
    } else {
        result->headers = columns;
    }
    
    // Filter and copy data
    for (const auto& record : table->records) {
        if (evaluate_condition(record, *table, where_clause)) {
            std::vector<std::string> row;
            
            for (const auto& header : result->headers) {
                auto it = table->header_index.find(header);
                if (it != table->header_index.end() && it->second < record.fields.size()) {
                    row.push_back(record.fields[it->second]);
                } else {
                    row.push_back("");
                }
            }
            
            result->rows.push_back(row);
        }
    }
    
    return result;
}

std::unique_ptr<QueryResult> QueryEngine::join(const std::string& left_table,
                                              const std::string& right_table,
                                              const std::string& join_condition,
                                              JoinType join_type) {
    const PsvTable* left = database_.get_table(left_table);
    const PsvTable* right = database_.get_table(right_table);
    
    if (!left || !right) {
        return nullptr;
    }
    
    auto result = std::make_unique<QueryResult>();
    
    // Parse join condition (simplified: assumes "left.field = right.field" format)
    auto condition_parts = parse_join_condition(join_condition);
    if (condition_parts.size() != 2) {
        return nullptr;
    }
    
    std::string left_field = condition_parts[0];
    std::string right_field = condition_parts[1];
    
    // Build headers
    for (const auto& header : left->headers) {
        result->headers.push_back(left_table + "." + header);
    }
    for (const auto& header : right->headers) {
        result->headers.push_back(right_table + "." + header);
    }
    
    // Perform join (simplified implementation for INNER join)
    if (join_type == JoinType::INNER) {
        for (const auto& left_record : left->records) {
            std::string left_value = left->get_field(
                &left_record - &left->records[0], left_field);
            
            for (const auto& right_record : right->records) {
                std::string right_value = right->get_field(
                    &right_record - &right->records[0], right_field);
                
                if (left_value == right_value && !left_value.empty()) {
                    std::vector<std::string> row;
                    
                    // Add left record fields
                    for (const auto& field : left_record.fields) {
                        row.push_back(field);
                    }
                    
                    // Add right record fields
                    for (const auto& field : right_record.fields) {
                        row.push_back(field);
                    }
                    
                    result->rows.push_back(row);
                }
            }
        }
    }
    
    return result;
}

std::unique_ptr<QueryResult> QueryEngine::union_tables(const std::vector<std::string>& table_names) {
    if (table_names.empty()) {
        return nullptr;
    }
    
    auto result = std::make_unique<QueryResult>();
    
    // Use headers from first table
    const PsvTable* first_table = database_.get_table(table_names[0]);
    if (!first_table) {
        return nullptr;
    }
    
    result->headers = first_table->headers;
    
    // Union all tables
    for (const auto& table_name : table_names) {
        const PsvTable* table = database_.get_table(table_name);
        if (!table) continue;
        
        for (const auto& record : table->records) {
            std::vector<std::string> row;
            
            // Map fields to result headers
            for (const auto& header : result->headers) {
                auto it = table->header_index.find(header);
                if (it != table->header_index.end() && it->second < record.fields.size()) {
                    row.push_back(record.fields[it->second]);
                } else {
                    row.push_back("");
                }
            }
            
            result->rows.push_back(row);
        }
    }
    
    return result;
}

bool QueryEngine::evaluate_condition(const PsvRecord& record, 
                                    const PsvTable& table,
                                    const std::string& condition) {
    // Simplified condition evaluation
    // Support basic operations like: field = 'value', field > 'value', etc.
    
    std::regex condition_regex(R"((\w+)\s*(=|!=|>|<|>=|<=)\s*'([^']*)')");
    std::smatch match;
    
    if (std::regex_match(condition, match, condition_regex)) {
        std::string field_name = match[1].str();
        std::string operator_str = match[2].str();
        std::string value = match[3].str();
        
        auto it = table.header_index.find(field_name);
        if (it == table.header_index.end() || it->second >= record.fields.size()) {
            return false;
        }
        
        std::string field_value = record.fields[it->second];
        
        if (operator_str == "=") {
            return field_value == value;
        } else if (operator_str == "!=") {
            return field_value != value;
        } else if (operator_str == ">") {
            return field_value > value;
        } else if (operator_str == "<") {
            return field_value < value;
        } else if (operator_str == ">=") {
            return field_value >= value;
        } else if (operator_str == "<=") {
            return field_value <= value;
        }
    }
    
    return false;
}

std::vector<std::string> QueryEngine::parse_join_condition(const std::string& condition) {
    // Parse condition like "left_table.field = right_table.field" or "field1 = field2"
    
    // Try prefixed format first: "table.field = table.field"
    std::regex prefixed_regex(R"((\w+\.\w+)\s*=\s*(\w+\.\w+))");
    std::smatch match;
    
    if (std::regex_match(condition, match, prefixed_regex)) {
        std::string left_part = match[1].str();
        std::string right_part = match[2].str();
        
        // Extract field names (remove table prefix)
        size_t left_dot = left_part.find('.');
        size_t right_dot = right_part.find('.');
        
        if (left_dot != std::string::npos && right_dot != std::string::npos) {
            return {
                left_part.substr(left_dot + 1),
                right_part.substr(right_dot + 1)
            };
        }
    }
    
    // Try simple format: "field1 = field2"
    std::regex simple_regex(R"((\w+)\s*=\s*(\w+))");
    if (std::regex_match(condition, match, simple_regex)) {
        return {
            match[1].str(),
            match[2].str()
        };
    }
    
    return {};
}