#include "database.h"
#include <algorithm>

void Database::load_table(std::unique_ptr<PsvTable> table) {
    if (table && !table->name.empty()) {
        tables_[table->name] = std::move(table);
    }
}

const PsvTable* Database::get_table(const std::string& name) const {
    auto it = tables_.find(name);
    return (it != tables_.end()) ? it->second.get() : nullptr;
}

std::vector<std::string> Database::get_table_names() const {
    std::vector<std::string> names;
    names.reserve(tables_.size());
    
    for (const auto& pair : tables_) {
        names.push_back(pair.first);
    }
    
    std::sort(names.begin(), names.end());
    return names;
}

size_t Database::get_total_records() const {
    size_t total = 0;
    for (const auto& pair : tables_) {
        total += pair.second->records.size();
    }
    return total;
}

void Database::clear() {
    tables_.clear();
}