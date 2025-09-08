#include <gtest/gtest.h>
#include "database.h"
#include "psv_parser.h"
#include <memory>

class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        database.clear();
    }

    void TearDown() override {
        database.clear();
    }

    std::unique_ptr<PsvTable> createTestTable(const std::string& name, 
                                             const std::vector<std::string>& headers,
                                             const std::vector<std::vector<std::string>>& data) {
        auto table = std::make_unique<PsvTable>();
        table->name = name;
        table->headers = headers;
        
        for (const auto& row : data) {
            PsvRecord record;
            record.fields = row;
            table->records.push_back(record);
        }
        
        table->build_header_index();
        return table;
    }

    Database database;
};

// Test loading tables
TEST_F(DatabaseTest, LoadSingleTable) {
    auto table = createTestTable("employees", 
        {"id", "name", "age"}, 
        {{"1", "John", "30"}, {"2", "Jane", "25"}});
    
    database.load_table(std::move(table));
    
    EXPECT_EQ(database.get_table_names().size(), 1);
    EXPECT_EQ(database.get_table_names()[0], "employees");
    EXPECT_EQ(database.get_total_records(), 2);
}

TEST_F(DatabaseTest, LoadMultipleTables) {
    auto employees = createTestTable("employees", 
        {"id", "name", "dept_id"}, 
        {{"1", "John", "10"}, {"2", "Jane", "20"}});
    
    auto departments = createTestTable("departments", 
        {"id", "name"}, 
        {{"10", "Engineering"}, {"20", "Marketing"}, {"30", "Sales"}});
    
    database.load_table(std::move(employees));
    database.load_table(std::move(departments));
    
    auto names = database.get_table_names();
    ASSERT_EQ(names.size(), 2);
    
    // Names should be sorted
    EXPECT_EQ(names[0], "departments");
    EXPECT_EQ(names[1], "employees");
    
    EXPECT_EQ(database.get_total_records(), 5); // 2 + 3
}

TEST_F(DatabaseTest, LoadNullTable) {
    database.load_table(nullptr);
    
    EXPECT_TRUE(database.get_table_names().empty());
    EXPECT_EQ(database.get_total_records(), 0);
}

TEST_F(DatabaseTest, LoadTableWithEmptyName) {
    auto table = createTestTable("", {"id", "name"}, {{"1", "John"}});
    
    database.load_table(std::move(table));
    
    EXPECT_TRUE(database.get_table_names().empty());
    EXPECT_EQ(database.get_total_records(), 0);
}

// Test table retrieval
TEST_F(DatabaseTest, GetExistingTable) {
    auto table = createTestTable("employees", 
        {"id", "name"}, 
        {{"1", "John"}, {"2", "Jane"}});
    
    database.load_table(std::move(table));
    
    const PsvTable* retrieved = database.get_table("employees");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->name, "employees");
    EXPECT_EQ(retrieved->headers.size(), 2);
    EXPECT_EQ(retrieved->records.size(), 2);
}

TEST_F(DatabaseTest, GetNonExistentTable) {
    const PsvTable* retrieved = database.get_table("nonexistent");
    EXPECT_EQ(retrieved, nullptr);
}

TEST_F(DatabaseTest, GetTableAfterClear) {
    auto table = createTestTable("employees", {"id", "name"}, {{"1", "John"}});
    database.load_table(std::move(table));
    
    EXPECT_NE(database.get_table("employees"), nullptr);
    
    database.clear();
    
    EXPECT_EQ(database.get_table("employees"), nullptr);
    EXPECT_TRUE(database.get_table_names().empty());
    EXPECT_EQ(database.get_total_records(), 0);
}

// Test table replacement
TEST_F(DatabaseTest, ReplaceTableWithSameName) {
    // Load first table
    auto table1 = createTestTable("employees", 
        {"id", "name"}, 
        {{"1", "John"}});
    
    database.load_table(std::move(table1));
    EXPECT_EQ(database.get_total_records(), 1);
    
    // Load second table with same name
    auto table2 = createTestTable("employees", 
        {"id", "name", "age"}, 
        {{"1", "John", "30"}, {"2", "Jane", "25"}});
    
    database.load_table(std::move(table2));
    
    // Should replace the original table
    EXPECT_EQ(database.get_table_names().size(), 1);
    EXPECT_EQ(database.get_total_records(), 2);
    
    const PsvTable* retrieved = database.get_table("employees");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->headers.size(), 3); // New table has 3 headers
    EXPECT_EQ(retrieved->records.size(), 2);
}

// Test edge cases
TEST_F(DatabaseTest, EmptyDatabase) {
    EXPECT_TRUE(database.get_table_names().empty());
    EXPECT_EQ(database.get_total_records(), 0);
    EXPECT_EQ(database.get_table("anything"), nullptr);
}

TEST_F(DatabaseTest, LoadEmptyTable) {
    auto table = createTestTable("empty", {"id", "name"}, {});
    
    database.load_table(std::move(table));
    
    EXPECT_EQ(database.get_table_names().size(), 1);
    EXPECT_EQ(database.get_total_records(), 0);
    
    const PsvTable* retrieved = database.get_table("empty");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->records.size(), 0);
}

TEST_F(DatabaseTest, TableNamesAreSorted) {
    auto table_c = createTestTable("customers", {"id"}, {{"1"}});
    auto table_a = createTestTable("accounts", {"id"}, {{"1"}});
    auto table_b = createTestTable("bookings", {"id"}, {{"1"}});
    
    database.load_table(std::move(table_c));
    database.load_table(std::move(table_a));
    database.load_table(std::move(table_b));
    
    auto names = database.get_table_names();
    ASSERT_EQ(names.size(), 3);
    EXPECT_EQ(names[0], "accounts");
    EXPECT_EQ(names[1], "bookings");
    EXPECT_EQ(names[2], "customers");
}

TEST_F(DatabaseTest, TotalRecordsCalculation) {
    auto table1 = createTestTable("table1", {"id"}, {{"1"}, {"2"}, {"3"}});
    auto table2 = createTestTable("table2", {"id"}, {{"1"}, {"2"}});
    auto table3 = createTestTable("table3", {"id"}, {}); // Empty table
    
    database.load_table(std::move(table1));
    EXPECT_EQ(database.get_total_records(), 3);
    
    database.load_table(std::move(table2));
    EXPECT_EQ(database.get_total_records(), 5);
    
    database.load_table(std::move(table3));
    EXPECT_EQ(database.get_total_records(), 5); // 3 + 2 + 0
}