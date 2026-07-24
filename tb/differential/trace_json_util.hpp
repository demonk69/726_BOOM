#ifndef TRACE_JSON_UTIL_HPP
#define TRACE_JSON_UTIL_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

namespace trace_json_util {

struct Event {
    std::string type;
    std::string pc;
    std::string instruction;
    std::string rd_valid;
    std::string rd;
    std::string rd_value;
    std::string exception;
    std::string exception_cause;
    std::string taken;
    std::string branch_mispredict;
    std::string normalized_cycle;
};

inline std::string value(const std::string& line, const std::string& key) {
    std::string needle = "\"" + key + "\":";
    std::size_t pos = line.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    if (pos >= line.size()) return "";
    if (line[pos] == '"') {
        std::size_t end = line.find('"', pos + 1);
        if (end == std::string::npos) return "";
        return line.substr(pos + 1, end - pos - 1);
    }
    std::size_t end = pos;
    while (end < line.size() && line[end] != ',' && line[end] != '}') end++;
    return line.substr(pos, end - pos);
}

inline Event parse_event(const std::string& line) {
    Event e;
    e.type = value(line, "event");
    e.pc = value(line, "pc");
    e.instruction = value(line, "instruction");
    e.rd_valid = value(line, "rd_valid");
    e.rd = value(line, "rd");
    e.rd_value = value(line, "rd_value");
    e.exception = value(line, "exception");
    e.exception_cause = value(line, "exception_cause");
    e.taken = value(line, "taken");
    e.branch_mispredict = value(line, "branch_mispredict");
    e.normalized_cycle = value(line, "normalized_cycle");
    return e;
}

inline std::vector<Event> load_events(const char* path, bool commits_only) {
    std::ifstream in(path);
    std::vector<Event> events;
    std::string line;
    while (std::getline(in, line)) {
        Event e = parse_event(line);
        if (e.type == "commit" || (!commits_only && e.type == "branch")) events.push_back(e);
    }
    return events;
}

inline std::string signature(const Event& e, bool include_cycle) {
    std::string sig = e.type + "|" + e.pc + "|" + e.instruction;
    if (e.type == "commit") {
        sig += "|" + e.rd_valid;
        sig += "|" + (e.rd_valid == "true" ? e.rd : "null");
        sig += "|" + (e.rd_valid == "true" ? e.rd_value : "null");
        sig += "|" + e.exception;
        sig += "|" + (e.exception == "true" ? e.exception_cause : "null");
    } else if (e.type == "branch") {
        sig += "|" + e.taken + "|" + e.branch_mispredict;
    }
    if (include_cycle) sig += "|" + e.normalized_cycle;
    return sig;
}

inline int compare_files(const char* ref_path, const char* dut_path, bool commits_only, bool include_cycle) {
    std::vector<Event> ref = load_events(ref_path, commits_only);
    std::vector<Event> dut = load_events(dut_path, commits_only);
    if (ref.size() != dut.size()) {
        std::cerr << "FAIL event_count ref=" << ref.size() << " dut=" << dut.size() << "\n";
        return 1;
    }
    for (std::size_t i = 0; i < ref.size(); ++i) {
        std::string a = signature(ref[i], include_cycle);
        std::string b = signature(dut[i], include_cycle);
        if (a != b) {
            std::cerr << "FAIL index=" << i << "\nref=" << a << "\ndut=" << b << "\n";
            return 1;
        }
    }
    std::cout << "PASS compared=" << ref.size() << "\n";
    return 0;
}

} // namespace trace_json_util

#endif
