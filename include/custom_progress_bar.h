#pragma once

#include <string>
#include <memory>
#include <chrono>
#include <mutex>

// Custom progress bar implementation using ANSI/VT escape sequences
// to replace the external indicators library

class CustomProgressBar {
public:
    // Color options
    enum class Color {
        green,
        blue, 
        cyan,
        white
    };

    // Configuration for progress bar appearance
    struct Config {
        size_t bar_width = 50;
        std::string start = "[";
        std::string fill = "█";
        std::string lead = "█";
        std::string remainder = "-";
        std::string end = "]";
        std::string prefix_text = "";
        Color foreground_color = Color::white;
        bool show_elapsed_time = false;
        bool show_remaining_time = false;
        bool bold = false;
    };

    explicit CustomProgressBar(const Config& config);
    virtual ~CustomProgressBar() = default;

    // Set the maximum progress value
    void set_max_progress(size_t max_progress);
    
    // Update the current progress
    virtual void set_progress(size_t current);
    
    // Mark the progress bar as completed
    virtual void mark_as_completed();

protected:
    Config config_;
    size_t max_progress_ = 100;
    size_t current_progress_ = 0;
    bool completed_ = false;
    std::chrono::steady_clock::time_point start_time_;
    
    // ANSI escape sequence helpers
    std::string get_color_code(Color color) const;
    std::string get_reset_code() const;
    std::string get_bold_code() const;
    std::string format_time(std::chrono::seconds seconds) const;
    
    // Render the progress bar
    virtual std::string render() const;
    
    // Display the progress bar (with platform-specific handling)
    void display() const;
    
    // Clear the current line
    void clear_line() const;

private:
    // Static mutex for thread-safe progress bar display
    static std::mutex display_mutex_;
};

// Block-style progress bar (simpler, no detailed progress info)
class CustomBlockProgressBar : public CustomProgressBar {
public:
    explicit CustomBlockProgressBar(const Config& config);
    
    void set_progress(size_t current) override;
    void mark_as_completed() override;

protected:
    std::string render() const override;
};