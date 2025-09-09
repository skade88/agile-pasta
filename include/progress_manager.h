#pragma once

#include "custom_progress_bar.h"
#include <memory>
#include <string>

class ProgressManager {
public:
    // Create progress bar for file loading
    static std::unique_ptr<CustomProgressBar> create_file_progress(
        const std::string& filename, size_t total_size);
    
    // Create progress bar for data processing
    static std::unique_ptr<CustomProgressBar> create_processing_progress(
        const std::string& task_name, size_t total_items);
    
    // Create block progress bar for overall progress
    static std::unique_ptr<CustomBlockProgressBar> create_overall_progress(
        const std::string& task_name);
    
    // Update progress bar
    static void update_progress(CustomProgressBar& bar, size_t current);
    static void complete_progress(CustomProgressBar& bar);
};