#include <cstdlib>
#include <iostream>

int main(int argc, char**) {
    if (argc != 3) {
        std::cerr << "usage: event_matcher <ref-normalized.jsonl> <dut-normalized.jsonl>\n";
        return 2;
    }
    std::cerr << "legacy global event total-order matching is invalid for BOOM/HLS OOO comparison; use partial_order_matcher or scripts/run_partial_order_diff.sh\n";
    return 2;
}
