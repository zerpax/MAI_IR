#include <unordered_map>
#include <algorithm>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

std::vector<std::pair<std::string, int>>
compute_zipf(const std::vector<std::string>& tokens) {
    std::unordered_map<std::string, int> freq;

    for (const auto& t : tokens) {
        ++freq[t];
    }

    std::vector<std::pair<std::string, int>> result;
    result.reserve(freq.size());

    for (const auto& kv : freq) {
        result.emplace_back(kv.first, kv.second);
    }

    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

    return result;
}

PYBIND11_MODULE(zipf, m) {
    m.def("zipf", &compute_zipf,
          "Compute Zipf frequency distribution");
}