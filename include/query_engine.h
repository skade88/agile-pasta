#pragma once

#include "database.h"
#include "psv_parser.h"
#include <string>
#include <vector>
#include <memory>

struct QueryResult {
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
};

enum class JoinType {
    INNER,
    LEFT,
    RIGHT,
    FULL
};

class QueryEngine {
public:
    explicit QueryEngine(const Database& db);
    
    // Execute SELECT query
    std::unique_ptr<QueryResult> select(const std::string& table_name, 
                                       const std::vector<std::string>& columns = {});
    
    // Execute SELECT with WHERE clause
    std::unique_ptr<QueryResult> select_where(const std::string& table_name, 
                                             const std::vector<std::string>& columns,
                                             const std::string& where_clause);
    
    // Execute JOIN query
    std::unique_ptr<QueryResult> join(const std::string& left_table,
                                     const std::string& right_table,
                                     const std::string& join_condition,
                                     JoinType join_type = JoinType::INNER);
    
    // Execute UNION query
    std::unique_ptr<QueryResult> union_tables(const std::vector<std::string>& table_names);
    
private:
    const Database& database_;
    
    // Helper methods
    bool evaluate_condition(const PsvRecord& record, 
                          const PsvTable& table,
                          const std::string& condition);
    
    std::vector<std::string> parse_join_condition(const std::string& condition);
};