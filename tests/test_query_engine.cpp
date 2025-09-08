#include <gtest/gtest.h>
#include "query_engine.h"
#include "database.h"
#include <memory>

class QueryEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        database.clear();
        
        // Create test employees table
        auto employees = std::make_unique<PsvTable>();
        employees->name = "employees";
        employees->headers = {"id", "name", "age", "dept_id", "salary"};
        
        PsvRecord emp1; emp1.fields = {"1", "John Doe", "30", "10", "75000"};
        PsvRecord emp2; emp2.fields = {"2", "Jane Smith", "25", "20", "65000"};
        PsvRecord emp3; emp3.fields = {"3", "Bob Johnson", "35", "10", "85000"};
        PsvRecord emp4; emp4.fields = {"4", "Alice Brown", "28", "30", "70000"};
        
        employees->records = {emp1, emp2, emp3, emp4};
        employees->build_header_index();
        
        // Create test departments table
        auto departments = std::make_unique<PsvTable>();
        departments->name = "departments";
        departments->headers = {"id", "name", "location"};
        
        PsvRecord dept1; dept1.fields = {"10", "Engineering", "Building A"};
        PsvRecord dept2; dept2.fields = {"20", "Marketing", "Building B"};
        PsvRecord dept3; dept3.fields = {"30", "Sales", "Building C"};
        
        departments->records = {dept1, dept2, dept3};
        departments->build_header_index();
        
        database.load_table(std::move(employees));
        database.load_table(std::move(departments));
        
        query_engine = std::make_unique<QueryEngine>(database);
    }

    void TearDown() override {
        query_engine.reset();
        database.clear();
    }

    Database database;
    std::unique_ptr<QueryEngine> query_engine;
};

// Test basic SELECT operations
TEST_F(QueryEngineTest, SelectAllColumns) {
    auto result = query_engine->select("employees");
    
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->headers.size(), 5);
    EXPECT_EQ(result->headers[0], "id");
    EXPECT_EQ(result->headers[1], "name");
    EXPECT_EQ(result->headers[2], "age");
    EXPECT_EQ(result->headers[3], "dept_id");
    EXPECT_EQ(result->headers[4], "salary");
    
    ASSERT_EQ(result->rows.size(), 4);
    EXPECT_EQ(result->rows[0][0], "1");
    EXPECT_EQ(result->rows[0][1], "John Doe");
    EXPECT_EQ(result->rows[1][1], "Jane Smith");
}

TEST_F(QueryEngineTest, SelectSpecificColumns) {
    auto result = query_engine->select("employees", {"name", "salary"});
    
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->headers.size(), 2);
    EXPECT_EQ(result->headers[0], "name");
    EXPECT_EQ(result->headers[1], "salary");
    
    ASSERT_EQ(result->rows.size(), 4);
    EXPECT_EQ(result->rows[0][0], "John Doe");
    EXPECT_EQ(result->rows[0][1], "75000");
    EXPECT_EQ(result->rows[1][0], "Jane Smith");
    EXPECT_EQ(result->rows[1][1], "65000");
}

TEST_F(QueryEngineTest, SelectNonExistentTable) {
    auto result = query_engine->select("nonexistent");
    
    EXPECT_EQ(result, nullptr);
}

TEST_F(QueryEngineTest, SelectWithNonExistentColumn) {
    auto result = query_engine->select("employees", {"name", "nonexistent", "salary"});
    
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->headers.size(), 3);
    EXPECT_EQ(result->headers[0], "name");
    EXPECT_EQ(result->headers[1], "nonexistent");
    EXPECT_EQ(result->headers[2], "salary");
    
    ASSERT_EQ(result->rows.size(), 4);
    EXPECT_EQ(result->rows[0][0], "John Doe");
    EXPECT_EQ(result->rows[0][1], ""); // Empty for non-existent column
    EXPECT_EQ(result->rows[0][2], "75000");
}

// Test SELECT with WHERE clause
TEST_F(QueryEngineTest, SelectWhereNumericCondition) {
    auto result = query_engine->select_where("employees", {"name", "salary"}, "salary >= '70000'");
    
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->headers.size(), 2);
    
    // Should return John Doe (75000), Bob Johnson (85000), Alice Brown (70000)
    ASSERT_EQ(result->rows.size(), 3);
    
    // Check that all returned salaries are >= 70000
    for (const auto& row : result->rows) {
        int salary = std::stoi(row[1]);
        EXPECT_GE(salary, 70000);
    }
}

TEST_F(QueryEngineTest, SelectWhereStringCondition) {
    auto result = query_engine->select_where("employees", {"name", "age"}, "name = 'John Doe'");
    
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->rows.size(), 1);
    EXPECT_EQ(result->rows[0][0], "John Doe");
    EXPECT_EQ(result->rows[0][1], "30");
}

TEST_F(QueryEngineTest, SelectWhereNoMatches) {
    auto result = query_engine->select_where("employees", {"name"}, "name = 'NonExistentEmployee'");
    
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->rows.size(), 0);
}

TEST_F(QueryEngineTest, SelectWhereNonExistentTable) {
    auto result = query_engine->select_where("nonexistent", {"name"}, "age > '25'");
    
    EXPECT_EQ(result, nullptr);
}

// Test UNION operations
TEST_F(QueryEngineTest, UnionCompatibleTables) {
    // Create a second employees table with same structure
    auto employees2 = std::make_unique<PsvTable>();
    employees2->name = "employees2";
    employees2->headers = {"id", "name", "age", "dept_id", "salary"};
    
    PsvRecord emp5; emp5.fields = {"5", "Charlie Green", "40", "20", "80000"};
    employees2->records = {emp5};
    employees2->build_header_index();
    
    database.load_table(std::move(employees2));
    
    auto result = query_engine->union_tables({"employees", "employees2"});
    
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->headers.size(), 5);
    EXPECT_EQ(result->rows.size(), 5); // 4 + 1
    
    // Last row should be Charlie Green
    EXPECT_EQ(result->rows[4][1], "Charlie Green");
}

TEST_F(QueryEngineTest, UnionWithNonExistentTable) {
    auto result = query_engine->union_tables({"employees", "nonexistent"});
    
    // Should still work with just the valid table
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->rows.size(), 4);
}

TEST_F(QueryEngineTest, UnionEmptyTableList) {
    auto result = query_engine->union_tables({});
    
    EXPECT_EQ(result, nullptr);
}

// Test JOIN operations  
TEST_F(QueryEngineTest, InnerJoin) {
    auto result = query_engine->join("employees", "departments", "dept_id = id", JoinType::INNER);
    
    ASSERT_NE(result, nullptr);
    EXPECT_GT(result->headers.size(), 5); // Should have columns from both tables
    
    // Should have matches for dept_id 10, 20, 30
    EXPECT_GT(result->rows.size(), 0);
    
    // All employees should have matching departments
    for (const auto& row : result->rows) {
        EXPECT_FALSE(row.empty());
    }
}

TEST_F(QueryEngineTest, JoinNonExistentTable) {
    auto result = query_engine->join("employees", "nonexistent", "dept_id = id");
    
    EXPECT_EQ(result, nullptr);
}

TEST_F(QueryEngineTest, JoinWithInvalidCondition) {
    auto result = query_engine->join("employees", "departments", "");
    
    // Should handle gracefully - implementation dependent
    // At minimum should not crash
    EXPECT_TRUE(result == nullptr || result->rows.empty());
}