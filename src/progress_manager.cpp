#include "progress_manager.h"

std::unique_ptr<CustomProgressBar> ProgressManager::create_file_progress(
    const std::string& filename, size_t total_size) {
    
    CustomProgressBar::Config config;
    config.bar_width = 50;
    config.start = "[";
    config.fill = "█";
    config.lead = "█";
    config.remainder = "-";
    config.end = "]";
    config.prefix_text = "Loading " + filename + " ";
    config.foreground_color = CustomProgressBar::Color::green;
    config.show_elapsed_time = true;
    config.show_remaining_time = true;
    config.bold = true;
    
    auto progress = std::make_unique<CustomProgressBar>(config);
    progress->set_max_progress(total_size);
    return progress;
}

std::unique_ptr<CustomProgressBar> ProgressManager::create_processing_progress(
    const std::string& task_name, size_t total_items) {
    
    CustomProgressBar::Config config;
    config.bar_width = 50;
    config.start = "[";
    config.fill = "█";
    config.lead = "█";
    config.remainder = "-";
    config.end = "]";
    config.prefix_text = task_name + " ";
    config.foreground_color = CustomProgressBar::Color::blue;
    config.show_elapsed_time = true;
    config.show_remaining_time = true;
    config.bold = true;
    
    auto progress = std::make_unique<CustomProgressBar>(config);
    progress->set_max_progress(total_items);
    return progress;
}

std::unique_ptr<CustomBlockProgressBar> ProgressManager::create_overall_progress(
    const std::string& task_name) {
    
    CustomProgressBar::Config config;
    config.bar_width = 50;
    config.prefix_text = task_name + " ";
    config.foreground_color = CustomProgressBar::Color::cyan;
    config.show_elapsed_time = true;
    config.show_remaining_time = false; // Block style typically doesn't show remaining time
    config.bold = false;
    
    return std::make_unique<CustomBlockProgressBar>(config);
}

void ProgressManager::update_progress(CustomProgressBar& bar, size_t current) {
    // No platform-specific line erasing needed - the custom progress bar handles this internally
    bar.set_progress(current);
}

void ProgressManager::complete_progress(CustomProgressBar& bar) {
    // No platform-specific line erasing needed - the custom progress bar handles this internally
    bar.mark_as_completed();
}