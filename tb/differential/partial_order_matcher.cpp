#include "trace_json_util.hpp"

#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

int to_int(const std::string& value) {
    if (value.empty() || value == "null") return -1;
    return std::atoi(value.c_str());
}

int run_partial_order(const char* ref_path, const char* dut_path) {
    std::vector<trace_json_util::Event> ref = trace_json_util::load_events(ref_path, false);
    std::vector<trace_json_util::Event> dut = trace_json_util::load_events(dut_path, false);
    std::vector<trace_json_util::Event> ref_commits;
    std::vector<trace_json_util::Event> dut_commits;
    for (std::size_t i = 0; i < ref.size(); ++i) if (ref[i].type == "commit") ref_commits.push_back(ref[i]);
    for (std::size_t i = 0; i < dut.size(); ++i) if (dut[i].type == "commit") dut_commits.push_back(dut[i]);
    if (ref_commits.size() != dut_commits.size()) {
        std::cerr << "FAIL commit_count ref=" << ref_commits.size() << " dut=" << dut_commits.size() << "\n";
        return 1;
    }
    for (std::size_t i = 0; i < ref_commits.size(); ++i) {
        std::string a = trace_json_util::signature(ref_commits[i], false);
        std::string b = trace_json_util::signature(dut_commits[i], false);
        if (a != b) {
            std::cerr << "FAIL commit_signature index=" << i << "\nref=" << a << "\ndut=" << b << "\n";
            return 1;
        }
    }
    int legal_reorders = 0;
    for (std::size_t i = 0; i < ref.size(); ++i) {
        if (ref[i].type != "branch") continue;
        for (std::size_t j = 0; j < ref_commits.size(); ++j) {
            if (ref_commits[j].pc == ref[i].pc && ref_commits[j].instruction == ref[i].instruction) break;
            if (to_int(ref_commits[j].normalized_cycle) > to_int(ref[i].normalized_cycle)) {
                legal_reorders++;
                break;
            }
        }
    }
    std::cout << "LEGAL_REORDER compared_commits=" << ref_commits.size()
              << " legal_reorders=" << legal_reorders << "\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: partial_order_matcher <ref-normalized.jsonl> <dut-normalized.jsonl>\n";
        return 2;
    }
    return run_partial_order(argv[1], argv[2]);
}
