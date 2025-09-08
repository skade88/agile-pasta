#include <gtest/gtest.h>
#include "command_line_parser.h"

class CommandLineParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup for each test
    }

    void TearDown() override {
        // Cleanup after each test
    }
};

// Test help command parsing
TEST_F(CommandLineParserTest, ParseHelpCommand) {
    char* argv[] = {"agile-pasta", "help"};
    int argc = 2;
    
    auto args = CommandLineParser::parse(argc, argv);
    
    EXPECT_EQ(args.command, CommandLineArgs::Command::HELP);
    EXPECT_FALSE(args.show_help); // show_help is only set when no args provided
}

TEST_F(CommandLineParserTest, ParseHelpFlag) {
    char* argv[] = {"agile-pasta", "--help"};
    int argc = 2;
    
    auto args = CommandLineParser::parse(argc, argv);
    
    EXPECT_EQ(args.command, CommandLineArgs::Command::HELP);
}

TEST_F(CommandLineParserTest, ParseHelpShortFlag) {
    char* argv[] = {"agile-pasta", "-h"};
    int argc = 2;
    
    auto args = CommandLineParser::parse(argc, argv);
    
    EXPECT_EQ(args.command, CommandLineArgs::Command::HELP);
}

// Test transform command parsing
TEST_F(CommandLineParserTest, ParseTransformCommand) {
    char* argv[] = {"agile-pasta", "transform", "--in", "/input/path", "--out", "/output/path"};
    int argc = 6;
    
    auto args = CommandLineParser::parse(argc, argv);
    
    EXPECT_EQ(args.command, CommandLineArgs::Command::TRANSFORM);
    EXPECT_EQ(args.input_path, "/input/path");
    EXPECT_EQ(args.output_path, "/output/path");
}

TEST_F(CommandLineParserTest, ParseTransformCommandMissingOut) {
    char* argv[] = {"agile-pasta", "transform", "--in", "/input/path"};
    int argc = 4;
    
    auto args = CommandLineParser::parse(argc, argv);
    
    EXPECT_EQ(args.command, CommandLineArgs::Command::TRANSFORM);
    EXPECT_EQ(args.input_path, "/input/path");
    EXPECT_TRUE(args.output_path.empty());
}

TEST_F(CommandLineParserTest, ParseTransformCommandMissingIn) {
    char* argv[] = {"agile-pasta", "transform", "--out", "/output/path"};
    int argc = 4;
    
    auto args = CommandLineParser::parse(argc, argv);
    
    EXPECT_EQ(args.command, CommandLineArgs::Command::TRANSFORM);
    EXPECT_TRUE(args.input_path.empty());
    EXPECT_EQ(args.output_path, "/output/path");
}

// Test sanity check command parsing
TEST_F(CommandLineParserTest, ParseSanityCheckCommand) {
    char* argv[] = {"agile-pasta", "check", "--out", "/output/path"};
    int argc = 4;
    
    auto args = CommandLineParser::parse(argc, argv);
    
    EXPECT_EQ(args.command, CommandLineArgs::Command::SANITY_CHECK);
    EXPECT_EQ(args.sanity_check_path, "/output/path");
}

TEST_F(CommandLineParserTest, ParseSanityCheckAlternative) {
    char* argv[] = {"agile-pasta", "sanity-check", "--out", "/output/path"};
    int argc = 4;
    
    auto args = CommandLineParser::parse(argc, argv);
    
    EXPECT_EQ(args.command, CommandLineArgs::Command::SANITY_CHECK);
    EXPECT_EQ(args.sanity_check_path, "/output/path");
}

// Test error cases
TEST_F(CommandLineParserTest, NoArgumentsShowsHelp) {
    char* argv[] = {"agile-pasta"};
    int argc = 1;
    
    auto args = CommandLineParser::parse(argc, argv);
    
    EXPECT_EQ(args.command, CommandLineArgs::Command::HELP);
    EXPECT_TRUE(args.show_help);
}

TEST_F(CommandLineParserTest, InvalidCommand) {
    char* argv[] = {"agile-pasta", "invalid-command"};
    int argc = 2;
    
    auto args = CommandLineParser::parse(argc, argv);
    
    EXPECT_EQ(args.command, CommandLineArgs::Command::INVALID);
}

TEST_F(CommandLineParserTest, TransformWithInvalidParameter) {
    char* argv[] = {"agile-pasta", "transform", "--in", "/input/path", "--invalid", "value"};
    int argc = 6;
    
    auto args = CommandLineParser::parse(argc, argv);
    
    EXPECT_EQ(args.command, CommandLineArgs::Command::INVALID);
}

TEST_F(CommandLineParserTest, TransformMissingParameterValue) {
    char* argv[] = {"agile-pasta", "transform", "--in"};
    int argc = 3;
    
    auto args = CommandLineParser::parse(argc, argv);
    
    EXPECT_EQ(args.command, CommandLineArgs::Command::INVALID);
}