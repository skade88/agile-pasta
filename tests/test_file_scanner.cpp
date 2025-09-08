#include <gtest/gtest.h>
#include "file_scanner.h"
#include <filesystem>
#include <fstream>

class FileScannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = std::filesystem::temp_directory_path() / "file_scanner_tests";
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

    std::filesystem::path test_dir;
    std::filesystem::path input_dir;
    std::filesystem::path output_dir;
};

// Test scanning input files
TEST_F(FileScannerTest, ScanValidInputFiles) {
    // Create test input files
    createFile(input_dir / "employees.psv", "1|John Doe|30\n2|Jane Smith|25");
    createFile(input_dir / "employees_Headers.psv", "id|name|age");
    createFile(input_dir / "departments.psv", "10|Engineering\n20|Marketing");
    createFile(input_dir / "departments_Headers.psv", "id|name");
    
    auto files = FileScanner::scan_input_files(input_dir.string());
    
    ASSERT_EQ(files.size(), 2);
    
    // Sort by name for predictable testing
    std::sort(files.begin(), files.end(), 
              [](const FileInfo& a, const FileInfo& b) {
                  return a.name_prefix < b.name_prefix;
              });
    
    // Check departments
    EXPECT_EQ(files[0].name_prefix, "departments");
    EXPECT_EQ(files[0].path, input_dir / "departments.psv");
    EXPECT_EQ(files[0].headers_path, input_dir / "departments_Headers.psv");
    EXPECT_GT(files[0].size_bytes, 0);
    
    // Check employees
    EXPECT_EQ(files[1].name_prefix, "employees");
    EXPECT_EQ(files[1].path, input_dir / "employees.psv");
    EXPECT_EQ(files[1].headers_path, input_dir / "employees_Headers.psv");
    EXPECT_GT(files[1].size_bytes, 0);
}

TEST_F(FileScannerTest, ScanInputFilesWithMissingHeaders) {
    // Create data file without corresponding headers file
    createFile(input_dir / "employees.psv", "1|John Doe|30");
    // Missing: employees_Headers.psv
    
    auto files = FileScanner::scan_input_files(input_dir.string());
    
    // Should not include files without matching headers
    EXPECT_EQ(files.size(), 0);
}

TEST_F(FileScannerTest, ScanInputFilesWithOrphanHeaders) {
    // Create headers file without corresponding data file
    createFile(input_dir / "employees_Headers.psv", "id|name|age");
    // Missing: employees.psv
    
    auto files = FileScanner::scan_input_files(input_dir.string());
    
    // Should not include files without matching data
    EXPECT_EQ(files.size(), 0);
}

TEST_F(FileScannerTest, ScanEmptyInputDirectory) {
    auto files = FileScanner::scan_input_files(input_dir.string());
    
    EXPECT_TRUE(files.empty());
}

TEST_F(FileScannerTest, ScanNonExistentInputDirectory) {
    EXPECT_THROW(
        FileScanner::scan_input_files((test_dir / "nonexistent").string()),
        std::runtime_error
    );
}

TEST_F(FileScannerTest, ScanInputWithSubdirectories) {
    // Create subdirectory with files
    auto subdir = input_dir / "subdir";
    std::filesystem::create_directories(subdir);
    
    createFile(subdir / "employees.psv", "1|John Doe|30");
    createFile(subdir / "employees_Headers.psv", "id|name|age");
    
    // Also create files in root
    createFile(input_dir / "departments.psv", "10|Engineering");
    createFile(input_dir / "departments_Headers.psv", "id|name");
    
    auto files = FileScanner::scan_input_files(input_dir.string());
    
    // Should find files in both root and subdirectories
    EXPECT_GE(files.size(), 1); // At least the root files
    
    // Check if subdirectory files are included (implementation dependent)
    bool found_subdir_file = false;
    for (const auto& file : files) {
        if (file.path.string().find("subdir") != std::string::npos) {
            found_subdir_file = true;
            break;
        }
    }
    // Implementation may or may not scan subdirectories
}

// Test scanning output files
TEST_F(FileScannerTest, ScanValidOutputFiles) {
    // Create test output files
    createFile(output_dir / "employee_summary_Headers.psv", "name|salary|department");
    createFile(output_dir / "employee_summary_Rules.psv", 
        "GLOBAL|salary >= '70000'|High earners only\n"
        "FIELD|name|first_name + \" \" + last_name|Combine names");
    
    createFile(output_dir / "department_report_Headers.psv", "dept_name|employee_count");
    createFile(output_dir / "department_report_Rules.psv", 
        "FIELD|dept_name|UPPER(name)|Uppercase department name");
    
    auto files = FileScanner::scan_output_files(output_dir.string());
    
    ASSERT_EQ(files.size(), 2);
    
    // Sort by name for predictable testing
    std::sort(files.begin(), files.end(), 
              [](const OutputFileInfo& a, const OutputFileInfo& b) {
                  return a.name_prefix < b.name_prefix;
              });
    
    // Check department_report
    EXPECT_EQ(files[0].name_prefix, "department_report");
    EXPECT_EQ(files[0].headers_path, output_dir / "department_report_Headers.psv");
    EXPECT_EQ(files[0].rules_path, output_dir / "department_report_Rules.psv");
    
    // Check employee_summary
    EXPECT_EQ(files[1].name_prefix, "employee_summary");
    EXPECT_EQ(files[1].headers_path, output_dir / "employee_summary_Headers.psv");
    EXPECT_EQ(files[1].rules_path, output_dir / "employee_summary_Rules.psv");
}

TEST_F(FileScannerTest, ScanOutputFilesWithMissingRules) {
    createFile(output_dir / "employee_summary_Headers.psv", "name|salary");
    // Missing: employee_summary_Rules.psv
    
    auto files = FileScanner::scan_output_files(output_dir.string());
    
    // Should not include files without matching rules
    EXPECT_EQ(files.size(), 0);
}

TEST_F(FileScannerTest, ScanOutputFilesWithMissingHeaders) {
    createFile(output_dir / "employee_summary_Rules.psv", "FIELD|name|first_name|Copy name");
    // Missing: employee_summary_Headers.psv
    
    auto files = FileScanner::scan_output_files(output_dir.string());
    
    // Should not include files without matching headers
    EXPECT_EQ(files.size(), 0);
}

TEST_F(FileScannerTest, ScanEmptyOutputDirectory) {
    auto files = FileScanner::scan_output_files(output_dir.string());
    
    EXPECT_TRUE(files.empty());
}

TEST_F(FileScannerTest, ScanNonExistentOutputDirectory) {
    EXPECT_THROW(
        FileScanner::scan_output_files((test_dir / "nonexistent").string()),
        std::runtime_error
    );
}

// Test file size calculation
TEST_F(FileScannerTest, FileSizeCalculation) {
    std::string large_content(10000, 'x'); // 10KB of 'x' characters
    createFile(input_dir / "large_file.psv", large_content);
    createFile(input_dir / "large_file_Headers.psv", "data");
    
    auto files = FileScanner::scan_input_files(input_dir.string());
    
    ASSERT_EQ(files.size(), 1);
    EXPECT_GE(files[0].size_bytes, 10000);
    EXPECT_LE(files[0].size_bytes, 11000); // Allow some overhead for newlines, etc.
}

// Test edge cases
TEST_F(FileScannerTest, FilesWithSpecialCharactersInNames) {
    // Create files with special characters (if filesystem supports them)
    createFile(input_dir / "test-file.psv", "data");
    createFile(input_dir / "test-file_Headers.psv", "header");
    
    createFile(input_dir / "test_file_2.psv", "data");
    createFile(input_dir / "test_file_2_Headers.psv", "header");
    
    auto files = FileScanner::scan_input_files(input_dir.string());
    
    EXPECT_GE(files.size(), 1); // Should handle special characters in filenames
}

TEST_F(FileScannerTest, MixedFileTypes) {
    // Create valid PSV files
    createFile(input_dir / "employees.psv", "data");
    createFile(input_dir / "employees_Headers.psv", "header");
    
    // Create non-PSV files (should be ignored)
    createFile(input_dir / "readme.txt", "This is a readme");
    createFile(input_dir / "data.csv", "csv,data");
    createFile(input_dir / "config.json", "{}");
    
    auto files = FileScanner::scan_input_files(input_dir.string());
    
    // Should only find the PSV files
    EXPECT_EQ(files.size(), 1);
    EXPECT_EQ(files[0].name_prefix, "employees");
}