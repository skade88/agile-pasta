#include "ansi_output.h"
#include <iostream>

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#else
#include <unistd.h>
#endif

void AnsiOutput::info(const std::string& message) {
    output_with_formatting(message, Color::cyan);
}

void AnsiOutput::success(const std::string& message) {
    output_with_formatting(message, Color::green);
}

void AnsiOutput::error(const std::string& message) {
    output_with_formatting(message, Color::red);
}

void AnsiOutput::warning(const std::string& message) {
    output_with_formatting(message, Color::yellow);
}

void AnsiOutput::header(const std::string& message) {
    output_with_formatting(message, Color::white, Style::bold);
}

void AnsiOutput::separator(int length, char character) {
    if (is_terminal_output()) {
        // Use ANSI/VT line drawing characters for terminal output
        std::string line_char;
        if (character == '=') {
            line_char = "\xE2\x95\x90"; // U+2550 BOX DRAWINGS DOUBLE HORIZONTAL (UTF-8)
        } else if (character == '-') {
            line_char = "\xE2\x94\x80"; // U+2500 BOX DRAWINGS LIGHT HORIZONTAL (UTF-8)
        } else {
            line_char = "\xE2\x94\x80"; // Default to light horizontal line
        }
        
        std::string separator_line;
        for (int i = 0; i < length; i++) {
            separator_line += line_char;
        }
        output_with_formatting(separator_line, Color::white);
    } else {
        // Use ASCII characters for non-terminal output (files, pipes)
        std::string separator_line(length, character);
        output_with_formatting(separator_line, Color::white);
    }
}

void AnsiOutput::plain(const std::string& message) {
    output_with_formatting(message, Color::white);
}

void AnsiOutput::styled(const std::string& message, Color color, Style style) {
    output_with_formatting(message, color, style);
}

std::string AnsiOutput::get_color_code(Color color) {
    switch (color) {
        case Color::red:    return "\033[31m";
        case Color::green:  return "\033[32m";
        case Color::yellow: return "\033[33m";
        case Color::blue:   return "\033[34m";
        case Color::cyan:   return "\033[36m";
        case Color::white:  return "\033[37m";
        case Color::reset:  return "\033[0m";
        default:            return "\033[37m"; // Default to white
    }
}

std::string AnsiOutput::get_style_code(Style style) {
    switch (style) {
        case Style::bold:   return "\033[1m";
        case Style::reset:  return "\033[0m";
        case Style::normal:
        default:            return "";
    }
}

std::string AnsiOutput::get_reset_code() {
    return "\033[0m";
}

bool AnsiOutput::is_terminal_output() {
#if defined(_WIN32) || defined(_WIN64)
    return _isatty(_fileno(stdout));
#else
    return isatty(STDOUT_FILENO);
#endif
}

void AnsiOutput::output_with_formatting(const std::string& message, Color color, Style style) {
    if (is_terminal_output()) {
        // Apply formatting if output is to a terminal
        std::cout << get_style_code(style) << get_color_code(color) << message << get_reset_code() << std::endl;
    } else {
        // Plain output if redirected to file
        std::cout << message << std::endl;
    }
}