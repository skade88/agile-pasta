#include "transformation_engine.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <stdexcept>
#include <iostream>
#include <algorithm>

TransformationEngine::TransformationEngine(const Database& db, QueryEngine& query_engine)
    : database_(db), query_engine_(query_engine) {}

void TransformationEngine::load_rules(const std::filesystem::path& rules_path) {
    std::ifstream file(rules_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open rules file: " + rules_path.string());
    }
    
    rules_.clear();
    std::string line;
    
    while (std::getline(file, line)) {
        line = line.substr(0, line.find('#')); // Remove comments
        if (line.empty()) continue;
        
        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (!line.empty()) {
            try {
                auto rule = parse_rule(line);
                rules_.push_back(rule);
            } catch (const std::exception& e) {
                // Skip invalid rules but log the error
                std::cerr << "Warning: Invalid rule ignored: " << line << " (" << e.what() << ")" << std::endl;
            }
        }
    }
}

void TransformationEngine::load_output_headers(const std::filesystem::path& headers_path) {
    std::ifstream file(headers_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open output headers file: " + headers_path.string());
    }
    
    std::string line;
    if (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string header;
        
        output_headers_.clear();
        while (std::getline(ss, header, '|')) {
            // Remove leading/trailing whitespace
            header.erase(0, header.find_first_not_of(" \t"));
            header.erase(header.find_last_not_of(" \t") + 1);
            
            if (!header.empty()) {
                output_headers_.push_back(header);
            }
        }
    }
}

std::unique_ptr<QueryResult> TransformationEngine::transform_data() {
    if (output_headers_.empty()) {
        throw std::runtime_error("No output headers loaded");
    }
    
    auto result = std::make_unique<QueryResult>();
    result->headers = output_headers_;
    
    // Get all table names
    auto table_names = database_.get_table_names();
    if (table_names.empty()) {
        return result; // Empty result
    }
    
    // For now, use only the first table that has all required input fields
    // In a more sophisticated implementation, this could use JOIN operations
    std::unique_ptr<QueryResult> source_data;
    std::vector<std::string> source_headers;
    
    // Try to find a table that contains most of the fields we need
    for (const auto& table_name : table_names) {
        const PsvTable* table = database_.get_table(table_name);
        if (!table) continue;
        
        // Count how many of our required fields are in this table
        int field_matches = 0;
        for (const auto& rule : rules_) {
            if (rule.type == TransformationRule::RuleType::FIELD) {
                if (std::find(table->headers.begin(), table->headers.end(), rule.condition) != table->headers.end()) {
                    field_matches++;
                }
            }
        }
        
        // Use this table if it has the most matches
        if (field_matches > 0) {
            source_data = query_engine_.select(table_name);
            source_headers = table->headers;
            break; // Use first matching table for now
        }
    }
    
    if (!source_data) {
        return result; // No suitable source data
    }
    
    // Apply global rules (filtering)
    std::vector<std::vector<std::string>> filtered_rows;
    
    for (const auto& row : source_data->rows) {
        bool passes_global_filters = true;
        
        for (const auto& rule : rules_) {
            if (rule.type == TransformationRule::RuleType::GLOBAL) {
                if (!evaluate_rule_condition(rule.condition, row, source_headers)) {
                    passes_global_filters = false;
                    break;
                }
            }
        }
        
        if (passes_global_filters) {
            filtered_rows.push_back(row);
        }
    }
    
    // Apply field transformations
    for (const auto& input_row : filtered_rows) {
        std::vector<std::string> output_row;
        
        for (const auto& output_header : output_headers_) {
            std::string output_value;
            
            // Look for field-specific rule
            bool found_rule = false;
            for (const auto& rule : rules_) {
                if (rule.type == TransformationRule::RuleType::FIELD && 
                    rule.target_field == output_header) {
                    
                    output_value = apply_rule(rule, input_row, source_headers);
                    found_rule = true;
                    break;
                }
            }
            
            // If no rule found, try to map directly from input
            if (!found_rule) {
                auto it = std::find(source_headers.begin(), source_headers.end(), output_header);
                if (it != source_headers.end()) {
                    size_t index = std::distance(source_headers.begin(), it);
                    if (index < input_row.size()) {
                        output_value = input_row[index];
                    }
                }
            }
            
            output_row.push_back(output_value);
        }
        
        result->rows.push_back(output_row);
    }
    
    return result;
}

const std::vector<std::string>& TransformationEngine::get_output_headers() const {
    return output_headers_;
}

TransformationRule TransformationEngine::parse_rule(const std::string& rule_text) {
    // Expected format: GLOBAL|condition|description
    // or: FIELD|target_field|expression|description
    
    std::vector<std::string> parts;
    std::stringstream ss(rule_text);
    std::string part;
    
    while (std::getline(ss, part, '|')) {
        // Trim whitespace
        part.erase(0, part.find_first_not_of(" \t"));
        part.erase(part.find_last_not_of(" \t") + 1);
        parts.push_back(part);
    }
    
    if (parts.size() < 3) {
        throw std::runtime_error("Invalid rule format: " + rule_text);
    }
    
    TransformationRule rule;
    
    if (parts[0] == "GLOBAL") {
        rule.type = TransformationRule::RuleType::GLOBAL;
        rule.condition = parts[1];
        // parts[2] is description (ignored for now)
    } else if (parts[0] == "FIELD") {
        rule.type = TransformationRule::RuleType::FIELD;
        rule.target_field = parts[1];
        rule.condition = parts[2]; // Actually the expression for field rules
        // parts[3] is description (ignored for now)
    } else {
        throw std::runtime_error("Unknown rule type: " + parts[0]);
    }
    
    return rule;
}

std::string TransformationEngine::apply_rule(const TransformationRule& rule, 
                                           const std::vector<std::string>& input_row,
                                           const std::vector<std::string>& input_headers) {
    std::string expression = rule.condition;
    
    // First, check if the expression is just a field name
    auto field_it = std::find(input_headers.begin(), input_headers.end(), expression);
    if (field_it != input_headers.end()) {
        size_t field_index = std::distance(input_headers.begin(), field_it);
        if (field_index < input_row.size()) {
            return input_row[field_index];
        }
    }
    
    // If not a simple field reference, process as an expression
    std::string result_expression = expression;
    
    // Replace field references with actual values
    for (size_t i = 0; i < input_headers.size() && i < input_row.size(); ++i) {
        const std::string& header = input_headers[i];
        const std::string& value = input_row[i];
        
        // Replace all occurrences of the header name with the value
        size_t pos = 0;
        while ((pos = result_expression.find(header, pos)) != std::string::npos) {
            // Make sure it's a whole word
            bool is_start_ok = (pos == 0 || !std::isalnum(result_expression[pos-1]));
            bool is_end_ok = (pos + header.length() == result_expression.length() || 
                             !std::isalnum(result_expression[pos + header.length()]));
            
            if (is_start_ok && is_end_ok) {
                result_expression.replace(pos, header.length(), value);
                pos += value.length();
            } else {
                pos += header.length();
            }
        }
    }
    
    // Handle simple expressions
    if (result_expression.find(" + ") != std::string::npos) {
        // String concatenation
        std::stringstream ss(result_expression);
        std::string part;
        std::string result;
        
        while (std::getline(ss, part, '+')) {
            // Trim whitespace and quotes
            part.erase(0, part.find_first_not_of(" \t'\""));
            part.erase(part.find_last_not_of(" \t'\"") + 1);
            result += part;
        }
        
        return result;
    } else if (result_expression.find(" * ") != std::string::npos) {
        // Numeric multiplication (simplified)
        std::stringstream ss(result_expression);
        std::string left, op, right;
        ss >> left >> op >> right;
        
        try {
            double left_val = std::stod(left);
            double right_val = std::stod(right);
            return std::to_string(left_val * right_val);
        } catch (...) {
            return result_expression; // Return as-is if can't evaluate
        }
    } else if (result_expression.find("UPPER(") == 0) {
        // UPPER function
        size_t start = result_expression.find('(') + 1;
        size_t end = result_expression.find(')', start);
        if (end != std::string::npos) {
            std::string value = result_expression.substr(start, end - start);
            std::transform(value.begin(), value.end(), value.begin(), ::toupper);
            return value;
        }
    } else if (result_expression.find("LOWER(") == 0) {
        // LOWER function
        size_t start = result_expression.find('(') + 1;
        size_t end = result_expression.find(')', start);
        if (end != std::string::npos) {
            std::string value = result_expression.substr(start, end - start);
            std::transform(value.begin(), value.end(), value.begin(), ::tolower);
            return value;
        }
    }
    
    // If no special handling, return the expression result as-is
    return result_expression;
}

bool TransformationEngine::evaluate_rule_condition(const std::string& condition,
                                                  const std::vector<std::string>& row,
                                                  const std::vector<std::string>& headers) {
    // Simplified condition evaluation
    // Support: field = 'value', field != 'value', field > 'value', etc.
    
    std::regex condition_regex(R"((\w+)\s*(=|!=|>|<|>=|<=)\s*'([^']*)')");
    std::smatch match;
    
    if (std::regex_match(condition, match, condition_regex)) {
        std::string field_name = match[1].str();
        std::string operator_str = match[2].str();
        std::string value = match[3].str();
        
        // Find field in headers
        auto it = std::find(headers.begin(), headers.end(), field_name);
        if (it == headers.end()) {
            return false;
        }
        
        size_t field_index = std::distance(headers.begin(), it);
        if (field_index >= row.size()) {
            return false;
        }
        
        std::string field_value = row[field_index];
        
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