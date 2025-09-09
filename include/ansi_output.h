#pragma once

#include <string>
#include <iostream>

// Centralized ANSI/VT text output system to replace standard cout usage
// Provides consistent styling and formatting across the application
class AnsiOutput {
public:
    // Color definitions
    enum class Color {
        red,
        green,
        yellow,
        blue,
        cyan,
        white,
        reset
    };

    // Style definitions
    enum class Style {
        normal,
        bold,
        reset
    };

    // Static methods for different types of output
    static void info(const std::string& message);
    static void success(const std::string& message);
    static void error(const std::string& message);
    static void warning(const std::string& message);
    static void header(const std::string& message);
    static void separator(int length = 80, char character = '=');
    static void plain(const std::string& message);
    
    // Output with custom styling
    static void styled(const std::string& message, Color color = Color::white, Style style = Style::normal);
    
    // Utility methods
    static std::string get_color_code(Color color);
    static std::string get_style_code(Style style);
    static std::string get_reset_code();
    
    // Check if output is to a terminal (for conditional ANSI usage)
    static bool is_terminal_output();

private:
    // Helper method to output with formatting
    static void output_with_formatting(const std::string& message, Color color, Style style = Style::normal);
};