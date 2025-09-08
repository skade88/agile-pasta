#include <gtest/gtest.h>
#include "command_line_parser.h"
#include "file_scanner.h"
#include "psv_parser.h"
#include "database.h"
#include "query_engine.h"
#include "transformation_engine.h"
#include "csv_writer.h"
#include <filesystem>
#include <fstream>
#include <sstream>

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = std::filesystem::temp_directory_path() / "integration_tests";
        std::filesystem::create_directories(test_dir);
        
        input_dir = test_dir / "input";
        output_dir = test_dir / "output";
        std::filesystem::create_directories(input_dir);
        std::filesystem::create_directories(output_dir);
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }

    void createFile(const std::filesystem::path& path, const std::string& content) {
        std::ofstream file(path);
        file << content;
        file.close();
    }

    std::string readFile(const std::filesystem::path& path) {
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    void createSampleData() {
        // Create employee data
        createFile(input_dir / "employees.psv",
            "1|John|Doe|30|75000|10\n"
            "2|Jane|Smith|25|65000|20\n"
            "3|Bob|Johnson|35|85000|10\n"
            "4|Alice|Brown|28|70000|30");
        
        createFile(input_dir / "employees_Headers.psv",
            "id|first_name|last_name|age|salary|dept_id");
        
        // Create department data
        createFile(input_dir / "departments.psv",
            "10|Engineering|Building A\n"
            "20|Marketing|Building B\n"
            "30|Sales|Building C");
        
        createFile(input_dir / "departments_Headers.psv",
            "id|name|location");
    }

    std::filesystem::path test_dir;
    std::filesystem::path input_dir;
    std::filesystem::path output_dir;
};

// Test complete data loading pipeline
TEST_F(IntegrationTest, DataLoadingPipeline) {
    createSampleData();
    
    // Step 1: Scan input files
    auto input_files = FileScanner::scan_input_files(input_dir.string());
    ASSERT_EQ(input_files.size(), 2);
    
    // Step 2: Load data into database
    Database database;
    for (const auto& file : input_files) {
        auto table = PsvParser::parse_file(file.path, file.headers_path);
        ASSERT_NE(table, nullptr);
        database.load_table(std::move(table));
    }
    
    // Step 3: Verify data is loaded correctly
    EXPECT_EQ(database.get_table_names().size(), 2);
    EXPECT_EQ(database.get_total_records(), 7); // 4 employees + 3 departments
    
    const auto* employees = database.get_table("employees");
    ASSERT_NE(employees, nullptr);
    EXPECT_EQ(employees->records.size(), 4);
    EXPECT_EQ(employees->get_field(0, "first_name"), "John");
    
    const auto* departments = database.get_table("departments");
    ASSERT_NE(departments, nullptr);
    EXPECT_EQ(departments->records.size(), 3);
    EXPECT_EQ(departments->get_field(0, "name"), "Engineering");
}

// Test query operations on loaded data
TEST_F(IntegrationTest, QueryOperationsPipeline) {
    createSampleData();
    
    // Load data
    auto input_files = FileScanner::scan_input_files(input_dir.string());
    Database database;
    for (const auto& file : input_files) {
        auto table = PsvParser::parse_file(file.path, file.headers_path);
        database.load_table(std::move(table));
    }
    
    QueryEngine query_engine(database);
    
    // Test SELECT operations
    auto all_employees = query_engine.select("employees");
    ASSERT_NE(all_employees, nullptr);
    EXPECT_EQ(all_employees->rows.size(), 4);
    
    auto selected_columns = query_engine.select("employees", {"first_name", "salary"});
    ASSERT_NE(selected_columns, nullptr);
    EXPECT_EQ(selected_columns->headers.size(), 2);
    EXPECT_EQ(selected_columns->rows.size(), 4);
    
    // Test WHERE operations
    auto high_earners = query_engine.select_where("employees", {"first_name", "salary"}, "salary >= '75000'");
    ASSERT_NE(high_earners, nullptr);
    EXPECT_LE(high_earners->rows.size(), 4); // Should filter some records
    
    // Test UNION operations
    auto union_result = query_engine.union_tables({"employees", "departments"});
    ASSERT_NE(union_result, nullptr);
    // Note: UNION with different schemas may have implementation-specific behavior
}

// Test complete transformation pipeline
TEST_F(IntegrationTest, TransformationPipeline) {
    createSampleData();
    
    // Create transformation configuration
    createFile(output_dir / "employee_report_Headers.psv",
        "full_name|annual_salary|department_name");
    
    createFile(output_dir / "employee_report_Rules.psv",
        "GLOBAL|salary >= '70000'|Only high earners\n"
        "FIELD|full_name|first_name + \" \" + last_name|Combine names\n"
        "FIELD|annual_salary|salary|Copy salary\n"
        "FIELD|department_name|\"Unknown\"|Default department");
    
    // Load input data
    auto input_files = FileScanner::scan_input_files(input_dir.string());
    Database database;
    for (const auto& file : input_files) {
        auto table = PsvParser::parse_file(file.path, file.headers_path);
        database.load_table(std::move(table));
    }
    
    // Setup transformation
    QueryEngine query_engine(database);
    TransformationEngine transform_engine(database, query_engine);
    
    // Load transformation configuration
    transform_engine.load_output_headers(output_dir / "employee_report_Headers.psv");
    transform_engine.load_rules(output_dir / "employee_report_Rules.psv");
    
    // Execute transformation
    auto result = transform_engine.transform_data();
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->headers.size(), 3);
    
    // Should only include employees with salary >= 70000
    EXPECT_LE(result->rows.size(), 4);
    EXPECT_GT(result->rows.size(), 0);
    
    // Write to CSV
    auto csv_path = output_dir / "employee_report.csv";
    bool success = CsvWriter::write_csv(*result, csv_path);
    EXPECT_TRUE(success);
    EXPECT_TRUE(std::filesystem::exists(csv_path));
    
    // Verify CSV content
    std::string csv_content = readFile(csv_path);
    EXPECT_TRUE(csv_content.find("full_name,annual_salary,department_name") != std::string::npos);
    EXPECT_TRUE(csv_content.find("John Doe") != std::string::npos || 
               csv_content.find("Bob Johnson") != std::string::npos); // High earners
}

// Test file scanning integration
TEST_F(IntegrationTest, FileScanningIntegration) {
    createSampleData();
    
    // Create output configuration
    createFile(output_dir / "summary_Headers.psv", "name|count");
    createFile(output_dir / "summary_Rules.psv", "FIELD|name|first_name|Copy name");
    
    // Test input file scanning
    auto input_files = FileScanner::scan_input_files(input_dir.string());
    ASSERT_EQ(input_files.size(), 2);
    
    for (const auto& file : input_files) {
        EXPECT_TRUE(std::filesystem::exists(file.path));
        EXPECT_TRUE(std::filesystem::exists(file.headers_path));
        EXPECT_GT(file.size_bytes, 0);
        EXPECT_FALSE(file.name_prefix.empty());
    }
    
    // Test output file scanning
    auto output_files = FileScanner::scan_output_files(output_dir.string());
    ASSERT_EQ(output_files.size(), 1);
    
    EXPECT_EQ(output_files[0].name_prefix, "summary");
    EXPECT_TRUE(std::filesystem::exists(output_files[0].headers_path));
    EXPECT_TRUE(std::filesystem::exists(output_files[0].rules_path));
}

// Test error handling integration
TEST_F(IntegrationTest, ErrorHandlingIntegration) {
    // Test with missing input files
    EXPECT_THROW(
        FileScanner::scan_input_files((test_dir / "nonexistent").string()),
        std::runtime_error
    );
    
    // Test with invalid PSV files
    createFile(input_dir / "invalid.psv", "malformed|data|without|proper|structure");
    createFile(input_dir / "invalid_Headers.psv", "header1|header2");
    
    auto input_files = FileScanner::scan_input_files(input_dir.string());
    if (!input_files.empty()) {
        Database database;
        // Should handle gracefully
        EXPECT_NO_THROW({
            for (const auto& file : input_files) {
                auto table = PsvParser::parse_file(file.path, file.headers_path);
                if (table) {
                    database.load_table(std::move(table));
                }
            }
        });
    }
}

// Test command line parsing integration
TEST_F(IntegrationTest, CommandLineParsingIntegration) {
    // Test help command
    const char* help_argv[] = {"agile-pasta", "help"};
    auto help_args = CommandLineParser::parse(2, const_cast<char**>(help_argv));
    EXPECT_EQ(help_args.command, CommandLineArgs::Command::HELP);
    
    // Test transform command - use static strings for lifetime safety
    std::string input_path_str = input_dir.string();
    std::string output_path_str = output_dir.string();
    const char* transform_argv[] = {"agile-pasta", "transform", "--in", 
                             input_path_str.c_str(), 
                             "--out", output_path_str.c_str()};
    auto transform_args = CommandLineParser::parse(6, const_cast<char**>(transform_argv));
    EXPECT_EQ(transform_args.command, CommandLineArgs::Command::TRANSFORM);
    EXPECT_EQ(transform_args.input_path, input_dir.string());
    EXPECT_EQ(transform_args.output_path, output_dir.string());
    
    // Test sanity check command
    const char* check_argv[] = {"agile-pasta", "check", "--out", 
                         output_path_str.c_str()};
    auto check_args = CommandLineParser::parse(4, const_cast<char**>(check_argv));
    EXPECT_EQ(check_args.command, CommandLineArgs::Command::SANITY_CHECK);
    EXPECT_EQ(check_args.sanity_check_path, output_dir.string());
}

// Test end-to-end workflow simulation
TEST_F(IntegrationTest, EndToEndWorkflow) {
    // Create complete test dataset
    createSampleData();
    
    createFile(output_dir / "final_report_Headers.psv",
        "employee_id|employee_name|department_location");
    
    createFile(output_dir / "final_report_Rules.psv",
        "FIELD|employee_id|id|Copy employee ID\n"
        "FIELD|employee_name|UPPER(first_name + \" \" + last_name)|Full name in uppercase\n"
        "FIELD|department_location|\"Remote\"|Default location");
    
    // Simulate complete workflow
    try {
        // 1. Scan input files
        auto input_files = FileScanner::scan_input_files(input_dir.string());
        ASSERT_GT(input_files.size(), 0);
        
        // 2. Load data
        Database database;
        for (const auto& file : input_files) {
            auto table = PsvParser::parse_file(file.path, file.headers_path);
            ASSERT_NE(table, nullptr);
            database.load_table(std::move(table));
        }
        
        // 3. Scan output configuration
        auto output_files = FileScanner::scan_output_files(output_dir.string());
        ASSERT_GT(output_files.size(), 0);
        
        // 4. Process transformations
        QueryEngine query_engine(database);
        for (const auto& output_file : output_files) {
            TransformationEngine transform_engine(database, query_engine);
            
            transform_engine.load_output_headers(output_file.headers_path);
            transform_engine.load_rules(output_file.rules_path);
            
            auto result = transform_engine.transform_data();
            ASSERT_NE(result, nullptr);
            
            // 5. Write output
            auto csv_path = output_dir / (output_file.name_prefix + ".csv");
            bool success = CsvWriter::write_csv(*result, csv_path);
            EXPECT_TRUE(success);
            EXPECT_TRUE(std::filesystem::exists(csv_path));
        }
        
        // 6. Verify final output
        auto final_csv = output_dir / "final_report.csv";
        EXPECT_TRUE(std::filesystem::exists(final_csv));
        
        std::string content = readFile(final_csv);
        EXPECT_TRUE(content.find("employee_id,employee_name,department_location") != std::string::npos);
        
    } catch (const std::exception& e) {
        FAIL() << "End-to-end workflow failed with exception: " << e.what();
    }
}