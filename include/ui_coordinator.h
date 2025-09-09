#pragma once

#include "file_scanner.h"
#include "custom_progress_bar.h"
#include <memory>
#include <vector>
#include <string>
#include <mutex>

// Coordinated UI manager that pre-creates all progress bars
// and displays the complete UI layout from the start
class UICoordinator {
public:
    // Constructor that takes all file information upfront
    UICoordinator(const std::vector<FileInfo>& input_files, 
                  const std::vector<OutputFileInfo>& output_files);
    
    // Initialize and display the complete UI layout
    void initialize_ui();
    
    // Update progress for a specific input file
    void update_file_progress(const std::string& filename, size_t current);
    
    // Complete progress for a specific input file
    void complete_file_progress(const std::string& filename);
    
    // Update progress for a specific output file
    void update_output_progress(const std::string& output_name, size_t current);
    
    // Complete progress for a specific output file  
    void complete_output_progress(const std::string& output_name, size_t records_written);
    
    // Set the maximum progress for an output file (when we know total records)
    void set_output_max_progress(const std::string& output_name, size_t max_records);
    
    // Display final summary
    void display_summary(size_t total_records_loaded);

private:
    struct FileProgressInfo {
        std::string filename;
        size_t size_bytes;
        std::unique_ptr<CustomProgressBar> progress_bar;
        bool completed = false;
    };
    
    struct OutputProgressInfo {
        std::string output_name;
        std::unique_ptr<CustomProgressBar> progress_bar;
        bool completed = false;
        size_t records_written = 0;
    };
    
    std::vector<FileProgressInfo> file_progress_bars_;
    std::vector<OutputProgressInfo> output_progress_bars_;
    
    // Find progress bar by filename/output name
    FileProgressInfo* find_file_progress(const std::string& filename);
    OutputProgressInfo* find_output_progress(const std::string& output_name);
    
    // Thread-safe UI display using existing mutex
    void refresh_display();
    
    // Calculate cursor positions for each progress bar
    void setup_display_positions();
    
    // Move cursor to specific progress bar position and update it
    void update_progress_at_position(size_t position, const std::string& rendered_bar);
    
    size_t input_file_count_;
    size_t output_file_count_;
    bool ui_initialized_ = false;
    
    // Display position tracking
    size_t total_display_lines_ = 0;
    size_t files_section_start_ = 0;
    size_t outputs_section_start_ = 0;
};