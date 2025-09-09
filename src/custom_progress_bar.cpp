#include "custom_progress_bar.h"
#include "ansi_output.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

CustomProgressBar::CustomProgressBar(const Config& config) 
    : config_(config), start_time_(std::chrono::steady_clock::now()) {
}

void CustomProgressBar::set_max_progress(size_t max_progress) {
    max_progress_ = std::max(size_t(1), max_progress); // Avoid division by zero
}

void CustomProgressBar::set_progress(size_t current) {
    current_progress_ = std::min(current, max_progress_);
    if (!completed_) {
        display();
    }
}

void CustomProgressBar::mark_as_completed() {
    if (!completed_) {
        current_progress_ = max_progress_;
        completed_ = true;
        display();
        if (AnsiOutput::is_terminal_output()) {
            std::cout << std::endl; // Move to next line when completed
        }
    }
}

std::string CustomProgressBar::get_color_code(Color color) const {
    switch (color) {
        case Color::green:  return "\033[32m";
        case Color::blue:   return "\033[34m";
        case Color::cyan:   return "\033[36m";
        case Color::white:  return "\033[37m";
        default:            return "";
    }
}

std::string CustomProgressBar::get_reset_code() const {
    return "\033[0m";
}

std::string CustomProgressBar::get_bold_code() const {
    return "\033[1m";
}

std::string CustomProgressBar::format_time(std::chrono::seconds seconds) const {
    auto total_seconds = seconds.count();
    auto hours = total_seconds / 3600;
    auto minutes = (total_seconds % 3600) / 60;
    auto secs = total_seconds % 60;
    
    std::ostringstream oss;
    if (hours > 0) {
        oss << hours << "h " << minutes << "m " << secs << "s";
    } else if (minutes > 0) {
        oss << minutes << "m " << secs << "s";
    } else {
        oss << secs << "s";
    }
    return oss.str();
}

std::string CustomProgressBar::render() const {
    std::ostringstream oss;
    
    // Add styling
    if (config_.bold) {
        oss << get_bold_code();
    }
    oss << get_color_code(config_.foreground_color);
    
    // Add prefix text
    oss << config_.prefix_text;
    
    // Calculate progress ratio
    double ratio = max_progress_ > 0 ? static_cast<double>(current_progress_) / max_progress_ : 0.0;
    size_t filled_width = static_cast<size_t>(ratio * config_.bar_width);
    
    // Build progress bar
    oss << config_.start;
    
    // Fill completed portion
    for (size_t i = 0; i < filled_width; ++i) {
        if (i == filled_width - 1 && filled_width < config_.bar_width) {
            oss << config_.lead;
        } else {
            oss << config_.fill;
        }
    }
    
    // Fill remaining portion
    for (size_t i = filled_width; i < config_.bar_width; ++i) {
        oss << config_.remainder;
    }
    
    oss << config_.end;
    
    // Add percentage
    oss << " " << std::fixed << std::setprecision(1) << (ratio * 100.0) << "%";
    
    // Add progress numbers
    oss << " (" << current_progress_ << "/" << max_progress_ << ")";
    
    // Add timing information
    if (config_.show_elapsed_time || config_.show_remaining_time) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
        
        if (config_.show_elapsed_time) {
            oss << " [" << format_time(elapsed) << "]";
        }
        
        if (config_.show_remaining_time && current_progress_ > 0 && !completed_) {
            auto total_estimated = elapsed * max_progress_ / current_progress_;
            auto remaining = total_estimated - elapsed;
            if (remaining.count() > 0) {
                oss << " ETA: " << format_time(remaining);
            }
        }
    }
    
    // Reset styling
    oss << get_reset_code();
    
    return oss.str();
}

void CustomProgressBar::display() const {
    // Check if we're outputting to a terminal (not being redirected)
    bool is_terminal = false;
#if defined(_WIN32) || defined(_WIN64)
    is_terminal = _isatty(_fileno(stdout));
#else
    is_terminal = isatty(STDOUT_FILENO);
#endif
    
    if (!is_terminal) {
        // If output is redirected, don't show progress bars
        return;
    }
    
    clear_line();
    std::cout << "\r" << render() << std::flush;
}

void CustomProgressBar::clear_line() const {
#if defined(_WIN32) || defined(_WIN64)
    // On Windows, we need to clear the line explicitly
    std::cout << "\r";
    // Print spaces to clear the line, then return to start
    for (int i = 0; i < 120; ++i) std::cout << " ";
    std::cout << "\r";
#else
    // On Unix-like systems, use ANSI escape sequence to clear line
    std::cout << "\033[K";
#endif
}

// CustomBlockProgressBar implementation
CustomBlockProgressBar::CustomBlockProgressBar(const Config& config) 
    : CustomProgressBar(config) {
    // Block progress bars are typically simpler
    config_.show_remaining_time = false; // Usually don't show detailed timing for block style
}

void CustomBlockProgressBar::set_progress(size_t current) {
    // Block progress bars might update less frequently or show different info
    CustomProgressBar::set_progress(current);
}

void CustomBlockProgressBar::mark_as_completed() {
    CustomProgressBar::mark_as_completed();
}

std::string CustomBlockProgressBar::render() const {
    std::ostringstream oss;
    
    // Add styling
    if (config_.bold) {
        oss << get_bold_code();
    }
    oss << get_color_code(config_.foreground_color);
    
    // Add prefix text
    oss << config_.prefix_text;
    
    // For block style, use solid blocks to show progress
    double ratio = max_progress_ > 0 ? static_cast<double>(current_progress_) / max_progress_ : 0.0;
    size_t filled_blocks = static_cast<size_t>(ratio * config_.bar_width);
    
    // Use Unicode block characters for a more solid look
    for (size_t i = 0; i < config_.bar_width; ++i) {
        if (i < filled_blocks) {
            oss << "█"; // Full block
        } else {
            oss << "░"; // Light shade
        }
    }
    
    // Add percentage
    oss << " " << std::fixed << std::setprecision(1) << (ratio * 100.0) << "%";
    
    // Add elapsed time if enabled
    if (config_.show_elapsed_time) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
        oss << " [" << format_time(elapsed) << "]";
    }
    
    // Reset styling
    oss << get_reset_code();
    
    return oss.str();
}