#pragma once

#include "database.h"
#include "query_engine.h"
#include <string>
#include <vector>
#include <filesystem>

struct TransformationRule {
    enum class RuleType {
        GLOBAL,     // Applied to all fields
        FIELD       // Applied to specific field
    };
    
    RuleType type;
    std::string field_name;  // For FIELD type rules
    std::string condition;   // Rule condition/expression
    std::string target_field; // Output field name
};

class TransformationEngine {
public:
    explicit TransformationEngine(const Database& db, QueryEngine& query_engine);
    
    // Load transformation rules from file
    void load_rules(const std::filesystem::path& rules_path);
    
    // Load output headers
    void load_output_headers(const std::filesystem::path& headers_path);
    
    // Transform data according to rules
    std::unique_ptr<QueryResult> transform_data();
    
    // Get output headers
    const std::vector<std::string>& get_output_headers() const;

private:
    const Database& database_;
    QueryEngine& query_engine_;
    std::vector<TransformationRule> rules_;
    std::vector<std::string> output_headers_;
    
    // Parse rule from text line
    TransformationRule parse_rule(const std::string& rule_text);
    
    // Apply rule to a data row
    std::string apply_rule(const TransformationRule& rule, 
                          const std::vector<std::string>& input_row,
                          const std::vector<std::string>& input_headers);
    
    // Evaluate rule condition
    bool evaluate_rule_condition(const std::string& condition,
                               const std::vector<std::string>& row,
                               const std::vector<std::string>& headers);
};