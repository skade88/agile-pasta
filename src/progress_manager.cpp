#include "progress_manager.h"
#include <indicators/progress_bar.hpp>
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>

std::unique_ptr<indicators::ProgressBar> ProgressManager::create_file_progress(
    const std::string& filename, size_t total_size) {
    
    auto progress = std::make_unique<indicators::ProgressBar>(
        indicators::option::BarWidth{50},
        indicators::option::Start{"["},
        indicators::option::Fill{"█"},
        indicators::option::Lead{"█"},
        indicators::option::Remainder{"-"},
        indicators::option::End{"]"},
        indicators::option::PrefixText{"Loading " + filename + " "},
        indicators::option::ForegroundColor{indicators::Color::green},
        indicators::option::ShowElapsedTime{true},
        indicators::option::ShowRemainingTime{true},
        indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}}
    );
    
    progress->set_option(indicators::option::MaxProgress{total_size});
    return progress;
}

std::unique_ptr<indicators::ProgressBar> ProgressManager::create_processing_progress(
    const std::string& task_name, size_t total_items) {
    
    auto progress = std::make_unique<indicators::ProgressBar>(
        indicators::option::BarWidth{50},
        indicators::option::Start{"["},
        indicators::option::Fill{"█"},
        indicators::option::Lead{"█"},
        indicators::option::Remainder{"-"},
        indicators::option::End{"]"},
        indicators::option::PrefixText{task_name + " "},
        indicators::option::ForegroundColor{indicators::Color::blue},
        indicators::option::ShowElapsedTime{true},
        indicators::option::ShowRemainingTime{true},
        indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}}
    );
    
    progress->set_option(indicators::option::MaxProgress{total_items});
    return progress;
}

std::unique_ptr<indicators::BlockProgressBar> ProgressManager::create_overall_progress(
    const std::string& task_name) {
    
    auto progress = std::make_unique<indicators::BlockProgressBar>(
        indicators::option::BarWidth{50},
        indicators::option::ForegroundColor{indicators::Color::cyan},
        indicators::option::PrefixText{task_name + " "},
        indicators::option::ShowElapsedTime{true}
    );
    
    return progress;
}

void ProgressManager::update_progress(indicators::ProgressBar& bar, size_t current) {
#if defined(_MSC_VER) || defined(_WIN32) || defined(_WIN64)
    // On Windows, explicitly erase the line before updating to prevent 
    // progress bars from creating new lines instead of updating in place.
    // On Linux, the indicators library handles in-place updates natively,
    // so erase_line() is not needed and actually interferes with proper display.
    indicators::erase_line();
#endif
    bar.set_progress(current);
}

void ProgressManager::complete_progress(indicators::ProgressBar& bar) {
#if defined(_MSC_VER) || defined(_WIN32) || defined(_WIN64)
    // On Windows, explicitly erase the line before completing to ensure clean output.
    // On Linux, the indicators library handles completion properly without manual erasing.
    indicators::erase_line();
#endif
    bar.mark_as_completed();
}