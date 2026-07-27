#pragma once
#include <map>
#include <string>

class MarkovTrainer {
private:
    std::map<int, std::map<int, int>> counts;

public:
    void train(const std::string& results_json_path);
    const std::map<int, std::map<int, int>>& get_counts() const;
    int total_observations() const;
};
