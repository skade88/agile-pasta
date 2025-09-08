#pragma once

#include <string>
#include <vector>

struct CommandLineArgs {
    enum class Command {
        HELP,
        TRANSFORM,
        SANITY_CHECK,
        INVALID
    };
    
    Command command = Command::INVALID;
    std::string input_path;
    std::string output_path;
    std::string sanity_check_path;  // For sanity check command
    bool show_help = false;
};

class CommandLineParser {
public:
    static CommandLineArgs parse(int argc, char* argv[]);
    static void print_help();
    static void print_usage();
};