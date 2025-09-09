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
        return nullptr; // Handle gracefully when no headers are loaded
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
    
    // Check if we have any field rules that reference input fields
    bool has_input_field_references = false;
    for (const auto& rule : rules_) {
        if (rule.type == TransformationRule::RuleType::FIELD) {
            // Check if the rule condition references any input field
            // This includes direct field references, complex expressions, and if-else conditions
            for (const auto& table_name : table_names) {
                const PsvTable* table = database_.get_table(table_name);
                if (!table) continue;
                
                // Check if rule.condition contains any field name (with word boundaries)
                for (const auto& header : table->headers) {
                    // Use word boundary search like in field replacement
                    size_t pos = 0;
                    bool found_reference = false;
                    while ((pos = rule.condition.find(header, pos)) != std::string::npos) {
                        // Make sure it's a whole word (including underscores as part of identifiers)
                        bool is_start_ok = (pos == 0 || (!std::isalnum(rule.condition[pos-1]) && rule.condition[pos-1] != '_'));
                        bool is_end_ok = (pos + header.length() == rule.condition.length() || 
                                         (!std::isalnum(rule.condition[pos + header.length()]) && rule.condition[pos + header.length()] != '_'));
                        
                        if (is_start_ok && is_end_ok) {
                            found_reference = true;
                            break;
                        }
                        pos += header.length();
                    }
                    
                    if (found_reference) {
                        has_input_field_references = true;
                        break;
                    }
                }
                if (has_input_field_references) break;
            }
            if (has_input_field_references) break;
        }
    }
    
    // If we have input field references, find a suitable source table
    if (has_input_field_references) {
        // Try to find a table that contains most of the fields we need
        for (const auto& table_name : table_names) {
            const PsvTable* table = database_.get_table(table_name);
            if (!table) continue;
            
            // Count how many of our required fields are in this table
            int field_matches = 0;
            for (const auto& rule : rules_) {
                if (rule.type == TransformationRule::RuleType::FIELD) {
                    // Check if rule.condition contains any field name from this table
                    for (const auto& header : table->headers) {
                        // Use the same word boundary logic as above
                        size_t pos = 0;
                        bool found_reference = false;
                        while ((pos = rule.condition.find(header, pos)) != std::string::npos) {
                            bool is_start_ok = (pos == 0 || (!std::isalnum(rule.condition[pos-1]) && rule.condition[pos-1] != '_'));
                            bool is_end_ok = (pos + header.length() == rule.condition.length() || 
                                             (!std::isalnum(rule.condition[pos + header.length()]) && rule.condition[pos + header.length()] != '_'));
                            
                            if (is_start_ok && is_end_ok) {
                                found_reference = true;
                                break;
                            }
                            pos += header.length();
                        }
                        
                        if (found_reference) {
                            field_matches++;
                            break; // Count each rule only once per table
                        }
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
    } else {
        // All field rules are static (no input field references)
        // Create a single empty row to process static rules
        source_data = std::make_unique<QueryResult>();
        source_data->headers = {};
        source_data->rows = {{}};  // Single empty row
        source_headers = {};
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
    
    // Check for unmapped output fields and warn
    std::vector<std::string> unmapped_fields;
    for (const auto& output_header : output_headers_) {
        bool has_rule = false;
        bool has_direct_mapping = false;
        
        // Check if there's a FIELD rule for this output header
        for (const auto& rule : rules_) {
            if (rule.type == TransformationRule::RuleType::FIELD && 
                rule.target_field == output_header) {
                has_rule = true;
                break;
            }
        }
        
        // Check if there's a direct header mapping
        if (!has_rule) {
            auto it = std::find(source_headers.begin(), source_headers.end(), output_header);
            if (it != source_headers.end()) {
                has_direct_mapping = true;
            }
        }
        
        // If neither rule nor direct mapping exists, this field will be empty
        if (!has_rule && !has_direct_mapping) {
            unmapped_fields.push_back(output_header);
        }
    }
    
    // Warn about unmapped fields
    if (!unmapped_fields.empty()) {
        std::cerr << "Warning: The following output fields have no transformation rules and no matching input headers:" << std::endl;
        for (const auto& field : unmapped_fields) {
            std::cerr << "  - " << field << " (will be empty)" << std::endl;
        }
        std::cerr << "Consider adding FIELD transformation rules for these fields." << std::endl;
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
    
    // Check for if-else (ternary) syntax: condition ? value1 : value2
    std::regex ternary_regex(R"((.+?)\s*\?\s*(.+?)\s*:\s*(.+))");
    std::smatch ternary_match;
    
    if (std::regex_match(expression, ternary_match, ternary_regex)) {
        std::string condition_part = ternary_match[1].str();
        std::string true_value = ternary_match[2].str();
        std::string false_value = ternary_match[3].str();
        
        // Trim whitespace from all parts
        condition_part.erase(0, condition_part.find_first_not_of(" \t"));
        condition_part.erase(condition_part.find_last_not_of(" \t") + 1);
        true_value.erase(0, true_value.find_first_not_of(" \t"));
        true_value.erase(true_value.find_last_not_of(" \t") + 1);
        false_value.erase(0, false_value.find_first_not_of(" \t"));
        false_value.erase(false_value.find_last_not_of(" \t") + 1);
        
        // Evaluate the condition part
        bool condition_result = evaluate_simple_condition(condition_part, input_row, input_headers);
        
        // Apply transformation to the chosen value
        std::string chosen_value = condition_result ? true_value : false_value;
        
        // Remove quotes from chosen value if present
        if ((chosen_value.front() == '"' && chosen_value.back() == '"') ||
            (chosen_value.front() == '\'' && chosen_value.back() == '\'')) {
            chosen_value = chosen_value.substr(1, chosen_value.length() - 2);
        }
        
        return chosen_value;
    }
    
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
    
    // Extract and preserve quoted string literals first
    // Handle both double quotes (new syntax) and single quotes (backward compatibility)
    std::vector<std::pair<std::string, std::string>> string_literals;
    std::regex double_quote_regex("\"([^\"]*)\"");
    std::regex single_quote_regex("'([^']*)'");
    std::smatch match;
    int literal_counter = 0;
    
    // First handle double quotes (preferred)
    while (std::regex_search(result_expression, match, double_quote_regex)) {
        std::string placeholder = "__STRING_LITERAL_" + std::to_string(literal_counter++) + "__";
        string_literals.push_back(std::make_pair(placeholder, match[1].str()));
        result_expression = std::regex_replace(result_expression, double_quote_regex, placeholder, std::regex_constants::format_first_only);
    }
    
    // Then handle single quotes (for backward compatibility)
    while (std::regex_search(result_expression, match, single_quote_regex)) {
        std::string placeholder = "__STRING_LITERAL_" + std::to_string(literal_counter++) + "__";
        string_literals.push_back(std::make_pair(placeholder, match[1].str()));
        result_expression = std::regex_replace(result_expression, single_quote_regex, placeholder, std::regex_constants::format_first_only);
    }
    
    // Replace field references with actual values
    for (size_t i = 0; i < input_headers.size() && i < input_row.size(); ++i) {
        const std::string& header = input_headers[i];
        const std::string& value = input_row[i];
        
        // Replace all occurrences of the header name with the value
        size_t pos = 0;
        while ((pos = result_expression.find(header, pos)) != std::string::npos) {
            // Make sure it's a whole word and not part of a placeholder (including underscores as part of identifiers)
            bool is_start_ok = (pos == 0 || (!std::isalnum(result_expression[pos-1]) && result_expression[pos-1] != '_'));
            bool is_end_ok = (pos + header.length() == result_expression.length() || 
                             (!std::isalnum(result_expression[pos + header.length()]) && result_expression[pos + header.length()] != '_'));
            
            // Also check it's not part of a string literal placeholder
            bool is_in_placeholder = false;
            for (const auto& literal : string_literals) {
                size_t placeholder_pos = result_expression.find(literal.first);
                if (placeholder_pos != std::string::npos &&
                    pos >= placeholder_pos && 
                    pos < placeholder_pos + literal.first.length()) {
                    is_in_placeholder = true;
                    break;
                }
            }
            
            if (is_start_ok && is_end_ok && !is_in_placeholder) {
                result_expression.replace(pos, header.length(), value);
                pos += value.length();
            } else {
                pos += header.length();
            }
        }
    }
    
    // Restore string literals
    for (const auto& literal : string_literals) {
        size_t pos = result_expression.find(literal.first);
        if (pos != std::string::npos) {
            result_expression.replace(pos, literal.first.length(), literal.second);
        }
    }
    
    // Handle simple expressions
    if (result_expression.find(" + ") != std::string::npos) {
        // String concatenation - be more careful about preserving spaces in quotes
        std::stringstream ss(result_expression);
        std::string part;
        std::string result;
        
        while (std::getline(ss, part, '+')) {
            // Check if this part is all whitespace but not empty
            bool is_whitespace_only = !part.empty() && 
                                      std::all_of(part.begin(), part.end(), [](char c) { return std::isspace(c); });
            
            if (is_whitespace_only) {
                // This might be a space literal, preserve it as a single space
                result += " ";
            } else {
                // Normal trimming for non-whitespace parts
                part.erase(0, part.find_first_not_of(" \t"));
                part.erase(part.find_last_not_of(" \t") + 1);
                
                // Don't process empty parts (this can happen with multiple + operators)
                if (!part.empty()) {
                    result += part;
                }
            }
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
    } else if (result_expression.find("TITLE(") == 0) {
        // TITLE function - convert to title case (first letter of each word uppercase, rest lowercase)
        size_t start = result_expression.find('(') + 1;
        size_t end = result_expression.find(')', start);
        if (end != std::string::npos) {
            std::string value = result_expression.substr(start, end - start);
            
            // Convert entire string to lowercase first
            std::transform(value.begin(), value.end(), value.begin(), ::tolower);
            
            // Make first character uppercase if it's alphabetic
            if (!value.empty() && std::isalpha(value[0])) {
                value[0] = std::toupper(value[0]);
            }
            
            // Make the first character after each space uppercase
            for (size_t i = 1; i < value.length(); ++i) {
                if (value[i-1] == ' ' && std::isalpha(value[i])) {
                    value[i] = std::toupper(value[i]);
                }
            }
            
            return value;
        }
    }
    
    // If no special handling, return the expression result as-is
    return result_expression;
}

bool TransformationEngine::evaluate_rule_condition(const std::string& condition,
                                                  const std::vector<std::string>& row,
                                                  const std::vector<std::string>& headers) {
    // Check for if-else (ternary) syntax: condition ? ACCEPT : REJECT
    std::regex ternary_regex(R"((.+?)\s*\?\s*(ACCEPT|REJECT)\s*:\s*(ACCEPT|REJECT))");
    std::smatch ternary_match;
    
    if (std::regex_match(condition, ternary_match, ternary_regex)) {
        std::string condition_part = ternary_match[1].str();
        std::string true_value = ternary_match[2].str();
        std::string false_value = ternary_match[3].str();
        
        // Trim whitespace from condition part
        condition_part.erase(0, condition_part.find_first_not_of(" \t"));
        condition_part.erase(condition_part.find_last_not_of(" \t") + 1);
        
        // Evaluate the condition part
        bool condition_result = evaluate_simple_condition(condition_part, row, headers);
        
        // Return true if the result is ACCEPT, false if REJECT
        std::string result_value = condition_result ? true_value : false_value;
        return result_value == "ACCEPT";
    }
    
    // Fall back to simple condition evaluation for backward compatibility
    return evaluate_simple_condition(condition, row, headers);
}

bool TransformationEngine::evaluate_simple_condition(const std::string& condition,
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
        
        // Try numeric comparison first
        bool is_numeric = true;
        double field_num = 0.0, value_num = 0.0;
        
        try {
            field_num = std::stod(field_value);
            value_num = std::stod(value);
        } catch (...) {
            is_numeric = false;
        }
        
        if (is_numeric) {
            // Numeric comparison
            if (operator_str == "=") {
                return field_num == value_num;
            } else if (operator_str == "!=") {
                return field_num != value_num;
            } else if (operator_str == ">") {
                return field_num > value_num;
            } else if (operator_str == "<") {
                return field_num < value_num;
            } else if (operator_str == ">=") {
                return field_num >= value_num;
            } else if (operator_str == "<=") {
                return field_num <= value_num;
            }
        } else {
            // String comparison
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
    }
    
    return false;
}