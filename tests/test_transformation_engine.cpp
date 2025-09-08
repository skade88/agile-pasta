#include <gtest/gtest.h>
#include "transformation_engine.h"
#include "database.h"
#include "query_engine.h"
#include <filesystem>
#include <fstream>

class TransformationEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory
        test_dir = std::filesystem::temp_directory_path() / "transformation_tests";
        std::filesystem::create_directories(test_dir);
        
        // Setup test database
        database.clear();
        
        auto employees = std::make_unique<PsvTable>();
        employees->name = "employees";
        employees->headers = {"id", "first_name", "last_name", "age", "salary", "department"};
        
        PsvRecord emp1; emp1.fields = {"1", "John", "Doe", "30", "75000", "engineering"};
        PsvRecord emp2; emp2.fields = {"2", "Jane", "Smith", "25", "65000", "marketing"};
        PsvRecord emp3; emp3.fields = {"3", "Bob", "Johnson", "35", "85000", "engineering"};
        
        employees->records = {emp1, emp2, emp3};
        employees->build_header_index();
        
        database.load_table(std::move(employees));
        query_engine = std::make_unique<QueryEngine>(database);
        transformation_engine = std::make_unique<TransformationEngine>(database, *query_engine);
    }

    void TearDown() override {
        transformation_engine.reset();
        query_engine.reset();
        database.clear();
        
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }

    void createTestFile(const std::string& filename, const std::string& content) {
        std::ofstream file(test_dir / filename);
        file << content;
        file.close();
    }

    std::filesystem::path test_dir;
    Database database;
    std::unique_ptr<QueryEngine> query_engine;
    std::unique_ptr<TransformationEngine> transformation_engine;
};

// Test loading output headers
TEST_F(TransformationEngineTest, LoadOutputHeaders) {
    createTestFile("output_headers.psv", "employee_name|annual_salary|department_name");
    
    transformation_engine->load_output_headers(test_dir / "output_headers.psv");
    
    auto headers = transformation_engine->get_output_headers();
    ASSERT_EQ(headers.size(), 3);
    EXPECT_EQ(headers[0], "employee_name");
    EXPECT_EQ(headers[1], "annual_salary");
    EXPECT_EQ(headers[2], "department_name");
}

TEST_F(TransformationEngineTest, LoadEmptyHeaders) {
    createTestFile("empty_headers.psv", "");
    
    transformation_engine->load_output_headers(test_dir / "empty_headers.psv");
    
    auto headers = transformation_engine->get_output_headers();
    EXPECT_TRUE(headers.empty());
}

TEST_F(TransformationEngineTest, LoadNonExistentHeaders) {
    EXPECT_THROW(
        transformation_engine->load_output_headers(test_dir / "nonexistent.psv"),
        std::exception
    );
}

// Test loading transformation rules
TEST_F(TransformationEngineTest, LoadValidRules) {
    createTestFile("rules.psv", 
        "GLOBAL|salary >= '70000'|Only high earners\n"
        "FIELD|employee_name|first_name + \" \" + last_name|Combine names\n"
        "FIELD|annual_salary|salary|Copy salary\n"
        "FIELD|department_name|UPPER(department)|Uppercase department");
    
    // Should not throw
    EXPECT_NO_THROW(
        transformation_engine->load_rules(test_dir / "rules.psv")
    );
}

TEST_F(TransformationEngineTest, LoadRulesWithInvalidSyntax) {
    createTestFile("invalid_rules.psv", 
        "INVALID|rule|syntax\n"
        "FIELD|name|value"); // Missing description
    
    // Should handle gracefully by ignoring invalid rules
    EXPECT_NO_THROW(
        transformation_engine->load_rules(test_dir / "invalid_rules.psv")
    );
}

TEST_F(TransformationEngineTest, LoadNonExistentRules) {
    EXPECT_THROW(
        transformation_engine->load_rules(test_dir / "nonexistent.psv"),
        std::exception
    );
}

// Test complete transformation workflow
TEST_F(TransformationEngineTest, TransformDataWithGlobalFilter) {
    createTestFile("headers.psv", "employee_name|annual_salary");
    createTestFile("rules.psv", 
        "GLOBAL|salary >= '70000'|Only high earners\n"
        "FIELD|employee_name|first_name + \" \" + last_name|Combine names\n"
        "FIELD|annual_salary|salary|Copy salary");
    
    transformation_engine->load_output_headers(test_dir / "headers.psv");
    transformation_engine->load_rules(test_dir / "rules.psv");
    
    auto result = transformation_engine->transform_data();
    
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->headers.size(), 2);
    EXPECT_EQ(result->headers[0], "employee_name");
    EXPECT_EQ(result->headers[1], "annual_salary");
    
    // Should only return John Doe (75000) and Bob Johnson (85000)
    ASSERT_EQ(result->rows.size(), 2);
    
    // Check first result (order might vary)
    bool found_john = false, found_bob = false;
    for (const auto& row : result->rows) {
        if (row[0] == "John Doe") {
            found_john = true;
            EXPECT_EQ(row[1], "75000");
        } else if (row[0] == "Bob Johnson") {
            found_bob = true;
            EXPECT_EQ(row[1], "85000");
        }
    }
    EXPECT_TRUE(found_john);
    EXPECT_TRUE(found_bob);
}

TEST_F(TransformationEngineTest, TransformDataWithStringFunctions) {
    createTestFile("headers.psv", "upper_name|lower_dept|title_name");
    createTestFile("rules.psv", 
        "FIELD|upper_name|UPPER(first_name)|Uppercase first name\n"
        "FIELD|lower_dept|LOWER(department)|Lowercase department\n"
        "FIELD|title_name|TITLE(last_name)|Title case last name");
    
    transformation_engine->load_output_headers(test_dir / "headers.psv");
    transformation_engine->load_rules(test_dir / "rules.psv");
    
    auto result = transformation_engine->transform_data();
    
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->rows.size(), 3); // No global filter, all records
    
    // Check first record transformations
    EXPECT_EQ(result->rows[0][0], "JOHN");        // UPPER(first_name)
    EXPECT_EQ(result->rows[0][1], "engineering"); // LOWER(department)
    EXPECT_EQ(result->rows[0][2], "Doe");         // TITLE(last_name)
}

TEST_F(TransformationEngineTest, TransformDataWithMissingField) {
    createTestFile("headers.psv", "employee_name|nonexistent_field");
    createTestFile("rules.psv", 
        "FIELD|employee_name|first_name + \" \" + last_name|Combine names");
    // No rule for nonexistent_field
    
    transformation_engine->load_output_headers(test_dir / "headers.psv");
    transformation_engine->load_rules(test_dir / "rules.psv");
    
    auto result = transformation_engine->transform_data();
    
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->rows.size(), 3);
    
    // Check that missing field gets empty value
    for (const auto& row : result->rows) {
        EXPECT_FALSE(row[0].empty()); // employee_name should have value
        EXPECT_TRUE(row[1].empty());  // nonexistent_field should be empty
    }
}

TEST_F(TransformationEngineTest, TransformDataNoHeaders) {
    createTestFile("rules.psv", 
        "FIELD|name|first_name|Copy first name");
    
    transformation_engine->load_rules(test_dir / "rules.psv");
    
    // Should handle gracefully when no headers are loaded
    auto result = transformation_engine->transform_data();
    
    // Implementation dependent - might return null or empty result
    EXPECT_TRUE(result == nullptr || result->rows.empty());
}

TEST_F(TransformationEngineTest, TransformDataNoRules) {
    createTestFile("headers.psv", "employee_name");
    
    transformation_engine->load_output_headers(test_dir / "headers.psv");
    
    // Should handle gracefully when no rules are loaded
    auto result = transformation_engine->transform_data();
    
    // Should still work, but fields might be empty
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->headers.size(), 1);
}

TEST_F(TransformationEngineTest, TransformDataWithNumericOperations) {
    createTestFile("headers.psv", "monthly_salary|double_age");
    createTestFile("rules.psv", 
        "FIELD|monthly_salary|salary / 12|Convert to monthly\n"
        "FIELD|double_age|age * 2|Double the age");
    
    transformation_engine->load_output_headers(test_dir / "headers.psv");
    transformation_engine->load_rules(test_dir / "rules.psv");
    
    auto result = transformation_engine->transform_data();
    
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->rows.size(), 3);
    
    // Check numeric calculations (implementation dependent)
    // This test verifies the engine can handle numeric expressions
    for (const auto& row : result->rows) {
        EXPECT_FALSE(row[0].empty()); // Should have some value for monthly_salary
        EXPECT_FALSE(row[1].empty()); // Should have some value for double_age
    }
}

// Test if-else functionality for field rules
TEST_F(TransformationEngineTest, TransformDataWithFieldIfElse) {
    createTestFile("headers.psv", "salary_category|dept_status");
    createTestFile("rules.psv", 
        "FIELD|salary_category|salary >= '75000' ? 'High' : 'Low'|Categorize salary\n"
        "FIELD|dept_status|department = 'engineering' ? 'Tech' : 'Non-Tech'|Categorize department");
    
    transformation_engine->load_output_headers(test_dir / "headers.psv");
    transformation_engine->load_rules(test_dir / "rules.psv");
    
    auto result = transformation_engine->transform_data();
    
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->rows.size(), 3); // All 3 records should be included
    
    // Check if-else results
    // John: salary=75000, dept=engineering -> "High", "Tech"
    // Jane: salary=65000, dept=marketing -> "Low", "Non-Tech"  
    // Bob: salary=85000, dept=engineering -> "High", "Tech"
    
    bool found_high_tech = false, found_low_nontech = false;
    for (const auto& row : result->rows) {
        if (row[0] == "High" && row[1] == "Tech") {
            found_high_tech = true;
        } else if (row[0] == "Low" && row[1] == "Non-Tech") {
            found_low_nontech = true;
        }
    }
    EXPECT_TRUE(found_high_tech);
    EXPECT_TRUE(found_low_nontech);
}

// Test if-else functionality for global rules
TEST_F(TransformationEngineTest, TransformDataWithGlobalIfElse) {
    createTestFile("headers.psv", "employee_name|salary");
    createTestFile("rules.psv", 
        "GLOBAL|salary >= '75000' ? ACCEPT : REJECT|Filter high earners\n"
        "FIELD|employee_name|first_name + \" \" + last_name|Combine names\n"
        "FIELD|salary|salary|Copy salary");
    
    transformation_engine->load_output_headers(test_dir / "headers.psv");
    transformation_engine->load_rules(test_dir / "rules.psv");
    
    auto result = transformation_engine->transform_data();
    
    ASSERT_NE(result, nullptr);
    
    // Should only return John Doe (75000) and Bob Johnson (85000)
    // Jane Smith (65000) should be filtered out
    ASSERT_EQ(result->rows.size(), 2);
    
    bool found_john = false, found_bob = false;
    for (const auto& row : result->rows) {
        if (row[0] == "John Doe") {
            found_john = true;
            EXPECT_EQ(row[1], "75000");
        } else if (row[0] == "Bob Johnson") {
            found_bob = true;
            EXPECT_EQ(row[1], "85000");
        }
    }
    EXPECT_TRUE(found_john);
    EXPECT_TRUE(found_bob);
}

// Test numeric comparison in conditions
TEST_F(TransformationEngineTest, TransformDataWithNumericComparison) {
    createTestFile("headers.psv", "employee_name");
    createTestFile("rules.psv", 
        "GLOBAL|salary >= '80000'|Only salaries >= 80000\n"
        "FIELD|employee_name|first_name + \" \" + last_name|Combine names");
    
    transformation_engine->load_output_headers(test_dir / "headers.psv");
    transformation_engine->load_rules(test_dir / "rules.psv");
    
    auto result = transformation_engine->transform_data();
    
    ASSERT_NE(result, nullptr);
    
    // Should only return Bob Johnson (85000)
    // John Doe (75000) and Jane Smith (65000) should be filtered out
    ASSERT_EQ(result->rows.size(), 1);
    EXPECT_EQ(result->rows[0][0], "Bob Johnson");
}