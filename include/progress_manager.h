#pragma once

#include <indicators/progress_bar.hpp>
#include <indicators/block_progress_bar.hpp>
#include <memory>
#include <string>

class ProgressManager {
public:
    // Create progress bar for file loading
    static std::unique_ptr<indicators::ProgressBar> create_file_progress(
        const std::string& filename, size_t total_size);
    
    // Create progress bar for data processing
    static std::unique_ptr<indicators::ProgressBar> create_processing_progress(
        const std::string& task_name, size_t total_items);
    
    // Create block progress bar for overall progress
    static std::unique_ptr<indicators::BlockProgressBar> create_overall_progress(
        const std::string& task_name);
    
    // Update progress bar
    static void update_progress(indicators::ProgressBar& bar, size_t current);
    static void complete_progress(indicators::ProgressBar& bar);
};