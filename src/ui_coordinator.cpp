#include "ui_coordinator.h"
#include "progress_manager.h"
#include "ansi_output.h"
#include <iostream>
#include <algorithm>

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#else
#include <unistd.h>
#endif

UICoordinator::UICoordinator(const std::vector<FileInfo>& input_files, 
                             const std::vector<OutputFileInfo>& output_files)
    : input_file_count_(input_files.size())
    , output_file_count_(output_files.size()) {
    
    // Pre-create progress bars for all input files
    file_progress_bars_.reserve(input_files.size());
    for (const auto& file : input_files) {
        FileProgressInfo info;
        info.filename = file.path.filename().string();
        info.size_bytes = file.size_bytes;
        info.progress_bar = ProgressManager::create_file_progress(info.filename, info.size_bytes);
        file_progress_bars_.push_back(std::move(info));
    }
    
    // Pre-create progress bars for all output files
    output_progress_bars_.reserve(output_files.size());
    for (const auto& output : output_files) {
        OutputProgressInfo info;
        info.output_name = output.name_prefix;
        // Start with unknown max, will be set later when we know record count
        info.progress_bar = ProgressManager::create_processing_progress(
            "Writing " + output.name_prefix + ".csv", 100);
        output_progress_bars_.push_back(std::move(info));
    }
}

void UICoordinator::initialize_ui() {
    if (ui_initialized_) return;
    
    AnsiOutput::info("\nLoading data files...");
    
    // Display initial progress bars at 0%
    for (auto& file_info : file_progress_bars_) {
        file_info.progress_bar->set_progress(0);
        file_info.progress_bar->display();
        std::cout << std::endl;
    }
    
    AnsiOutput::info("\nOutput file generation:");
    
    for (auto& output_info : output_progress_bars_) {
        output_info.progress_bar->set_progress(0);
        output_info.progress_bar->display();
        std::cout << std::endl;
    }
    
    ui_initialized_ = true;
}

void UICoordinator::update_file_progress(const std::string& filename, size_t current) {
    auto* info = find_file_progress(filename);
    if (info && !info->completed) {
        info->progress_bar->set_progress(current);
    }
}

void UICoordinator::complete_file_progress(const std::string& filename) {
    auto* info = find_file_progress(filename);
    if (info) {
        info->progress_bar->mark_as_completed();
        info->completed = true;
    }
}

void UICoordinator::set_output_max_progress(const std::string& output_name, size_t max_records) {
    auto* info = find_output_progress(output_name);
    if (info) {
        info->progress_bar->set_max_progress(max_records + 1); // +1 for header
    }
}

void UICoordinator::update_output_progress(const std::string& output_name, size_t current) {
    auto* info = find_output_progress(output_name);
    if (info && !info->completed) {
        info->progress_bar->set_progress(current);
    }
}

void UICoordinator::complete_output_progress(const std::string& output_name, size_t records_written) {
    auto* info = find_output_progress(output_name);
    if (info) {
        info->progress_bar->mark_as_completed();
        info->completed = true;
        info->records_written = records_written;
    }
}

void UICoordinator::display_summary(size_t total_records_loaded) {
    std::cout << std::endl;
    AnsiOutput::success("Loaded " + std::to_string(total_records_loaded) + 
                       " total records from " + std::to_string(input_file_count_) + " files.");
    
    // Show output summary
    for (const auto& output_info : output_progress_bars_) {
        if (output_info.completed) {
            AnsiOutput::success("Successfully wrote " + std::to_string(output_info.records_written) + 
                               " records to " + output_info.output_name + ".csv");
        }
    }
}

UICoordinator::FileProgressInfo* UICoordinator::find_file_progress(const std::string& filename) {
    auto it = std::find_if(file_progress_bars_.begin(), file_progress_bars_.end(),
                          [&filename](const FileProgressInfo& info) {
                              return info.filename == filename;
                          });
    return it != file_progress_bars_.end() ? &(*it) : nullptr;
}

UICoordinator::OutputProgressInfo* UICoordinator::find_output_progress(const std::string& output_name) {
    auto it = std::find_if(output_progress_bars_.begin(), output_progress_bars_.end(),
                          [&output_name](const OutputProgressInfo& info) {
                              return info.output_name == output_name;
                          });
    return it != output_progress_bars_.end() ? &(*it) : nullptr;
}

void UICoordinator::update_progress_at_position(size_t position, const std::string& rendered_bar) {
    // Check if we're outputting to a terminal
    bool is_terminal = false;
#if defined(_WIN32) || defined(_WIN64)
    is_terminal = _isatty(_fileno(stdout));
#else
    is_terminal = isatty(STDOUT_FILENO);
#endif
    
    if (!is_terminal) {
        return; // Don't update progress bars when output is redirected
    }
    
    // Move cursor to the specific line and update
    std::cout << "\033[" << (position + 1) << ";1H";  // Move to line (1-indexed)
    std::cout << "\033[K";  // Clear line
    std::cout << "\r" << rendered_bar << std::flush;
}