#include <gtest/gtest.h>
#include "psv_parser.h"
#include <filesystem>
#include <fstream>
#include <sstream>

class PsvParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        test_dir = std::filesystem::temp_directory_path() / "agile_pasta_tests";
        std::filesystem::create_directories(test_dir);
    }

    void TearDown() override {
        // Cleanup test files
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
};

// Test header parsing
TEST_F(PsvParserTest, ParseValidHeaders) {
    createTestFile("test_headers.psv", "id|name|age|department");
    
    auto headers = PsvParser::parse_headers(test_dir / "test_headers.psv");
    
    ASSERT_EQ(headers.size(), 4);
    EXPECT_EQ(headers[0], "id");
    EXPECT_EQ(headers[1], "name");
    EXPECT_EQ(headers[2], "age");
    EXPECT_EQ(headers[3], "department");
}

TEST_F(PsvParserTest, ParseHeadersWithSpaces) {
    createTestFile("test_headers.psv", " id | name | age | department ");
    
    auto headers = PsvParser::parse_headers(test_dir / "test_headers.psv");
    
    ASSERT_EQ(headers.size(), 4);
    EXPECT_EQ(headers[0], "id");
    EXPECT_EQ(headers[1], "name");
    EXPECT_EQ(headers[2], "age");
    EXPECT_EQ(headers[3], "department");
}

TEST_F(PsvParserTest, ParseEmptyHeaders) {
    createTestFile("empty_headers.psv", "");
    
    EXPECT_THROW(
        PsvParser::parse_headers(test_dir / "empty_headers.psv"),
        std::runtime_error
    );
}

TEST_F(PsvParserTest, ParseSingleHeader) {
    createTestFile("single_header.psv", "id");
    
    auto headers = PsvParser::parse_headers(test_dir / "single_header.psv");
    
    ASSERT_EQ(headers.size(), 1);
    EXPECT_EQ(headers[0], "id");
}

TEST_F(PsvParserTest, ParseHeadersNonExistentFile) {
    EXPECT_THROW(
        PsvParser::parse_headers(test_dir / "nonexistent.psv"),
        std::runtime_error
    );
}

// Test data parsing
TEST_F(PsvParserTest, ParseValidData) {
    createTestFile("test_data.psv", 
        "1|John Doe|30|Engineering\n"
        "2|Jane Smith|25|Marketing\n"
        "3|Bob Johnson|35|Sales");
    
    size_t total_records = 0;
    auto records = PsvParser::parse_data(test_dir / "test_data.psv", total_records);
    
    ASSERT_EQ(records.size(), 3);
    EXPECT_EQ(total_records, 3);
    
    // Check first record
    ASSERT_EQ(records[0].fields.size(), 4);
    EXPECT_EQ(records[0].fields[0], "1");
    EXPECT_EQ(records[0].fields[1], "John Doe");
    EXPECT_EQ(records[0].fields[2], "30");
    EXPECT_EQ(records[0].fields[3], "Engineering");
    
    // Check second record
    ASSERT_EQ(records[1].fields.size(), 4);
    EXPECT_EQ(records[1].fields[0], "2");
    EXPECT_EQ(records[1].fields[1], "Jane Smith");
    EXPECT_EQ(records[1].fields[2], "25");
    EXPECT_EQ(records[1].fields[3], "Marketing");
}

TEST_F(PsvParserTest, ParseDataWithQuotes) {
    createTestFile("test_data.psv", 
        "1|\"John, Jr.\"|30|Engineering\n"
        "2|Jane \"The Boss\" Smith|25|Marketing");
    
    size_t total_records = 0;
    auto records = PsvParser::parse_data(test_dir / "test_data.psv", total_records);
    
    ASSERT_EQ(records.size(), 2);
    
    // Check handling of quotes (note: this depends on implementation)
    EXPECT_EQ(records[0].fields[1], "\"John, Jr.\"");
    EXPECT_EQ(records[1].fields[1], "Jane \"The Boss\" Smith");
}

TEST_F(PsvParserTest, ParseDataWithEmptyFields) {
    createTestFile("test_data.psv", 
        "1|John Doe||Engineering\n"
        "2||25|Marketing\n"
        "|||");
    
    size_t total_records = 0;
    auto records = PsvParser::parse_data(test_dir / "test_data.psv", total_records);
    
    ASSERT_EQ(records.size(), 3);
    
    // Check empty fields
    EXPECT_EQ(records[0].fields[2], "");
    EXPECT_EQ(records[1].fields[1], "");
    EXPECT_EQ(records[2].fields[0], "");
    EXPECT_EQ(records[2].fields[1], "");
    EXPECT_EQ(records[2].fields[2], "");
}

TEST_F(PsvParserTest, ParseEmptyDataFile) {
    createTestFile("empty_data.psv", "");
    
    size_t total_records = 0;
    auto records = PsvParser::parse_data(test_dir / "empty_data.psv", total_records);
    
    EXPECT_TRUE(records.empty());
    EXPECT_EQ(total_records, 0);
}

// Test complete file parsing
TEST_F(PsvParserTest, ParseCompleteFile) {
    createTestFile("test_headers.psv", "id|name|age|department");
    createTestFile("test_data.psv", 
        "1|John Doe|30|Engineering\n"
        "2|Jane Smith|25|Marketing");
    
    auto table = PsvParser::parse_file(test_dir / "test_data.psv", test_dir / "test_headers.psv");
    
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(table->name, "test_data");
    EXPECT_EQ(table->source_file, test_dir / "test_data.psv");
    
    // Check headers
    ASSERT_EQ(table->headers.size(), 4);
    EXPECT_EQ(table->headers[0], "id");
    EXPECT_EQ(table->headers[1], "name");
    EXPECT_EQ(table->headers[2], "age");
    EXPECT_EQ(table->headers[3], "department");
    
    // Check records
    ASSERT_EQ(table->records.size(), 2);
    EXPECT_EQ(table->records[0].fields[0], "1");
    EXPECT_EQ(table->records[0].fields[1], "John Doe");
    
    // Check header index is built
    EXPECT_EQ(table->header_index.size(), 4);
    EXPECT_EQ(table->header_index.at("id"), 0);
    EXPECT_EQ(table->header_index.at("name"), 1);
    EXPECT_EQ(table->header_index.at("age"), 2);
    EXPECT_EQ(table->header_index.at("department"), 3);
}

// Test PsvTable methods
TEST_F(PsvParserTest, PsvTableGetField) {
    createTestFile("test_headers.psv", "id|name|age");
    createTestFile("test_data.psv", 
        "1|John Doe|30\n"
        "2|Jane Smith|25");
    
    auto table = PsvParser::parse_file(test_dir / "test_data.psv", test_dir / "test_headers.psv");
    
    // Test valid field access
    EXPECT_EQ(table->get_field(0, "id"), "1");
    EXPECT_EQ(table->get_field(0, "name"), "John Doe");
    EXPECT_EQ(table->get_field(0, "age"), "30");
    EXPECT_EQ(table->get_field(1, "name"), "Jane Smith");
    
    // Test invalid field access
    EXPECT_EQ(table->get_field(0, "nonexistent"), "");
    EXPECT_EQ(table->get_field(999, "name"), "");
}

TEST_F(PsvParserTest, PsvTableBuildHeaderIndex) {
    PsvTable table;
    table.headers = {"id", "name", "age", "department"};
    
    table.build_header_index();
    
    ASSERT_EQ(table.header_index.size(), 4);
    EXPECT_EQ(table.header_index.at("id"), 0);
    EXPECT_EQ(table.header_index.at("name"), 1);
    EXPECT_EQ(table.header_index.at("age"), 2);
    EXPECT_EQ(table.header_index.at("department"), 3);
}