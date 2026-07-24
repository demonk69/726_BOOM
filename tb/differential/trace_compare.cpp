#include "trace_json_util.hpp"

#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: trace_compare <ref-normalized.jsonl> <dut-normalized.jsonl>\n";
        return 2;
    }
    return trace_json_util::compare_files(argv[1], argv[2], true, false);
}
