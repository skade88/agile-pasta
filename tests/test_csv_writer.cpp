#include <gtest/gtest.h>
#include "csv_writer.h"
#include "query_engine.h"
#include <filesystem>
#include <fstream>
#include <sstream>

class CsvWriterTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = std::filesystem::temp_directory_path() / "csv_writer_tests";
        std::filesystem::create_directories(test_dir);
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }

    std::string readFile(const std::filesystem::path& path) {
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::unique_ptr<QueryResult> createTestResult() {
        auto result = std::make_unique<QueryResult>();
        result->headers = {"id", "name", "age", "department"};
        result->rows = {
            {"1", "John Doe", "30", "Engineering"},
            {"2", "Jane Smith", "25", "Marketing"},
            {"3", "Bob Johnson", "35", "Sales"}
        };
        return result;
    }

    std::filesystem::path test_dir;
};

// Test basic CSV writing
TEST_F(CsvWriterTest, WriteBasicCsv) {
    auto result = createTestResult();
    auto output_path = test_dir / "test_output.csv";
    
    bool success = CsvWriter::write_csv(*result, output_path);
    
    EXPECT_TRUE(success);
    EXPECT_TRUE(std::filesystem::exists(output_path));
    
    std::string content = readFile(output_path);
    
    // Check headers
    EXPECT_TRUE(content.find("id,name,age,department") != std::string::npos);
    
    // Check data rows
    EXPECT_TRUE(content.find("1,John Doe,30,Engineering") != std::string::npos);
    EXPECT_TRUE(content.find("2,Jane Smith,25,Marketing") != std::string::npos);
    EXPECT_TRUE(content.find("3,Bob Johnson,35,Sales") != std::string::npos);
}

TEST_F(CsvWriterTest, WriteEmptyResult) {
    auto result = std::make_unique<QueryResult>();
    result->headers = {"id", "name"};
    // No rows
    
    auto output_path = test_dir / "empty_output.csv";
    
    bool success = CsvWriter::write_csv(*result, output_path);
    
    EXPECT_TRUE(success);
    EXPECT_TRUE(std::filesystem::exists(output_path));
    
    std::string content = readFile(output_path);
    
    // Should have headers but no data rows
    EXPECT_TRUE(content.find("id,name") != std::string::npos);
    
    // Count lines (header + possible empty line)
    size_t line_count = std::count(content.begin(), content.end(), '\n');
    EXPECT_LE(line_count, 2); // Header line + possible trailing newline
}

TEST_F(CsvWriterTest, WriteResultWithSpecialCharacters) {
    auto result = std::make_unique<QueryResult>();
    result->headers = {"id", "name", "description"};
    result->rows = {
        {"1", "John, Jr.", "Software \"Engineer\""},
        {"2", "Jane\nSmith", "Data,Analyst"},
        {"3", "Bob's Company", "Contains 'quotes'"}
    };
    
    auto output_path = test_dir / "special_chars.csv";
    
    bool success = CsvWriter::write_csv(*result, output_path);
    
    EXPECT_TRUE(success);
    EXPECT_TRUE(std::filesystem::exists(output_path));
    
    std::string content = readFile(output_path);
    
    // Fields with commas or quotes should be properly escaped
    EXPECT_TRUE(content.find("\"John, Jr.\"") != std::string::npos);
    EXPECT_TRUE(content.find("\"Software \"\"Engineer\"\"\"") != std::string::npos ||
               content.find("Software \\\"Engineer\\\"") != std::string::npos);
    EXPECT_TRUE(content.find("\"Data,Analyst\"") != std::string::npos);
}

TEST_F(CsvWriterTest, WriteResultWithEmptyFields) {
    auto result = std::make_unique<QueryResult>();
    result->headers = {"id", "name", "optional_field"};
    result->rows = {
        {"1", "John Doe", ""},
        {"2", "", "Some value"},
        {"", "Jane Smith", "Another value"}
    };
    
    auto output_path = test_dir / "empty_fields.csv";
    
    bool success = CsvWriter::write_csv(*result, output_path);
    
    EXPECT_TRUE(success);
    EXPECT_TRUE(std::filesystem::exists(output_path));
    
    std::string content = readFile(output_path);
    
    // Should handle empty fields properly
    EXPECT_TRUE(content.find("1,John Doe,") != std::string::npos);
    EXPECT_TRUE(content.find("2,,Some value") != std::string::npos);
    EXPECT_TRUE(content.find(",Jane Smith,Another value") != std::string::npos);
}

TEST_F(CsvWriterTest, WriteToInvalidPath) {
    auto result = createTestResult();
    
    // Try to write to a directory that doesn't exist and can't be created
    auto invalid_path = std::filesystem::path("/invalid/nonexistent/path/output.csv");
    
    bool success = CsvWriter::write_csv(*result, invalid_path);
    
    EXPECT_FALSE(success);
}

TEST_F(CsvWriterTest, WriteWithProgress) {
    auto result = createTestResult();
    auto output_path = test_dir / "progress_output.csv";
    
    // write_csv_with_progress should work the same as write_csv but with progress reporting
    bool success = CsvWriter::write_csv_with_progress(*result, output_path);
    
    EXPECT_TRUE(success);
    EXPECT_TRUE(std::filesystem::exists(output_path));
    
    std::string content = readFile(output_path);
    EXPECT_TRUE(content.find("id,name,age,department") != std::string::npos);
    EXPECT_TRUE(content.find("John Doe") != std::string::npos);
}

TEST_F(CsvWriterTest, WriteLargeResult) {
    auto result = std::make_unique<QueryResult>();
    result->headers = {"id", "name", "value"};
    
    // Create a larger dataset
    for (int i = 0; i < 1000; ++i) {
        result->rows.push_back({
            std::to_string(i),
            "Name " + std::to_string(i),
            "Value " + std::to_string(i * 10)
        });
    }
    
    auto output_path = test_dir / "large_output.csv";
    
    bool success = CsvWriter::write_csv(*result, output_path);
    
    EXPECT_TRUE(success);
    EXPECT_TRUE(std::filesystem::exists(output_path));
    
    // Check file size is reasonable
    auto file_size = std::filesystem::file_size(output_path);
    EXPECT_GT(file_size, 1000); // Should be more than 1KB for 1000 rows
    
    // Quick check that content is there
    std::string content = readFile(output_path);
    EXPECT_TRUE(content.find("id,name,value") != std::string::npos);
    EXPECT_TRUE(content.find("Name 999") != std::string::npos); // Last entry
}

TEST_F(CsvWriterTest, OverwriteExistingFile) {
    auto result1 = std::make_unique<QueryResult>();
    result1->headers = {"old_header"};
    result1->rows = {{"old_value"}};
    
    auto result2 = createTestResult();
    auto output_path = test_dir / "overwrite_test.csv";
    
    // Write first file
    bool success1 = CsvWriter::write_csv(*result1, output_path);
    EXPECT_TRUE(success1);
    
    // Write second file (should overwrite)
    bool success2 = CsvWriter::write_csv(*result2, output_path);
    EXPECT_TRUE(success2);
    
    // Check that file contains new content, not old
    std::string content = readFile(output_path);
    EXPECT_TRUE(content.find("id,name,age,department") != std::string::npos);
    EXPECT_FALSE(content.find("old_header") != std::string::npos);
    EXPECT_FALSE(content.find("old_value") != std::string::npos);
}

TEST_F(CsvWriterTest, ResultWithMismatchedRowLength) {
    auto result = std::make_unique<QueryResult>();
    result->headers = {"id", "name", "age"};
    result->rows = {
        {"1", "John"},        // Missing age field
        {"2", "Jane", "25", "extra"}, // Extra field
        {"3", "Bob", "30"}    // Correct length
    };
    
    auto output_path = test_dir / "mismatched_rows.csv";
    
    bool success = CsvWriter::write_csv(*result, output_path);
    
    EXPECT_TRUE(success); // Should handle gracefully
    EXPECT_TRUE(std::filesystem::exists(output_path));
    
    std::string content = readFile(output_path);
    EXPECT_TRUE(content.find("id,name,age") != std::string::npos);
    
    // Should handle mismatched rows appropriately
    // (exact behavior depends on implementation)
}