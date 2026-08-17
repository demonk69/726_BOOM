#include "boom_interfaces.hpp"
#include "boom_state.hpp"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace boom {
void frontend_module(BoomCoreState&, PipeSignals&);
}

namespace {

const uint64_t kTargetInstructions = 2048;

struct Metrics {
    std::string scenario;
    std::string path;
    uint64_t instructions;
    uint64_t cycles;
    uint64_t frontend_stall_cycles;
    uint64_t buffer_full_cycles;
    uint64_t max_occupancy;
    uint64_t enqueue_count;
    uint64_t dequeue_count;
    uint64_t decode_stall_cycles;
    uint64_t imem_request_count;
    uint64_t imem_response_count;
    uint64_t producer_count;
    uint64_t protocol_constraint_cycles;

    Metrics(const std::string& scenario_name, const std::string& path_name)
        : scenario(scenario_name), path(path_name), instructions(0), cycles(0),
          frontend_stall_cycles(0), buffer_full_cycles(0), max_occupancy(0),
          enqueue_count(0), dequeue_count(0), decode_stall_cycles(0),
          imem_request_count(0), imem_response_count(0), producer_count(0),
          protocol_constraint_cycles(0) {}
};

struct ProofCounters {
    uint64_t stalled_responses;
    uint64_t stalled_productions;
    uint64_t stalled_enqueues_below_full;
    uint64_t stalled_occupancy_rises;
    uint64_t capacity_backpressure_below_full;
    uint64_t full_capacity_backpressure;
    bool stalled_occupancy_level[FETCH_BUFFER_DEPTH + 1];

    ProofCounters() : stalled_responses(0), stalled_productions(0),
        stalled_enqueues_below_full(0), stalled_occupancy_rises(0),
        capacity_backpressure_below_full(0), full_capacity_backpressure(0),
        stalled_occupancy_level() {}
};

bool decode_ready(const std::string& scenario, uint64_t cycle) {
    if (scenario == "A_ALWAYS_READY") return true;
    if (scenario == "B_STALL_1_OF_4") return cycle % 4 != 3;
    if (scenario == "C_STALL_50_PERCENT") return cycle % 2 == 0;
    if (scenario == "D_BURST_STALL_4") return cycle % 20 >= 4;
    return cycle % 32 >= 8;
}

uint32_t instruction_at(uint64_t address) {
    // Legal, deterministic ADDI traffic with changing payload bits.
    const uint32_t imm = static_cast<uint32_t>((address >> 2) & 0x7ffu);
    const uint32_t rd = static_cast<uint32_t>(((address >> 4) % 31u) + 1u);
    return (imm << 20) | (rd << 7) | 0x13u;
}

ImemResponse response_for(const ImemRequest& request) {
    ImemResponse response;
    response.address = request.address;
    response.fetch_id = request.fetch_id;
    response.epoch = request.epoch;
    response.instruction = instruction_at(request.address);
    return response;
}

Metrics run_unbuffered(const std::string& scenario) {
    Metrics metrics(scenario, "gate5_2_unbuffered_model");
    bool request_in_flight = false;
    bool producer_valid = false;

    while (metrics.dequeue_count < kTargetInstructions) {
        const bool ready = decode_ready(scenario, metrics.cycles);
        if (!ready) ++metrics.decode_stall_cycles;

        if (request_in_flight) {
            request_in_flight = false;
            producer_valid = true;
            ++metrics.imem_response_count;
            ++metrics.producer_count;
        }
        if (producer_valid && ready) {
            producer_valid = false;
            ++metrics.dequeue_count;
        }
        if (producer_valid) ++metrics.frontend_stall_cycles;

        // Gate 5.2 reference: one producer register, no fetch queue. A held
        // instruction blocks the next request until Decode accepts it.
        if (!producer_valid && metrics.dequeue_count < kTargetInstructions) {
            request_in_flight = true;
            ++metrics.imem_request_count;
        }
        ++metrics.cycles;
        assert(metrics.cycles < kTargetInstructions * 16);
    }
    metrics.instructions = metrics.dequeue_count;
    return metrics;
}

Metrics run_canonical(const std::string& scenario, ProofCounters& proof) {
    Metrics metrics(scenario, "b2_canonical_frontend_depth8");
    BoomCoreState state;
    PipeSignals pipe;
    bool response_pending = false;
    ImemResponse pending_response;

    while (metrics.dequeue_count < kTargetInstructions) {
        const bool ready = decode_ready(scenario, metrics.cycles);
        const uint8_t count_before = state.frontend.fetch_buffer.count;
        const bool producer_before = state.frontend.producer_valid;
        const bool delivered_response = response_pending;
        if (!ready) ++metrics.decode_stall_cycles;

        state.rename.dispatch_packets[0].valid = !ready;
        state.decode.dec_valids[0] = false;
        if (delivered_response) {
            pipe.imem_resp.write(pending_response);
            response_pending = false;
            ++metrics.imem_response_count;
            ++metrics.producer_count;
        }

        boom::frontend_module(state, pipe);

        const bool dequeue = count_before != 0 && ready;
        const uint8_t count_after = state.frontend.fetch_buffer.count;
        const bool enqueue = count_after + (dequeue ? 1u : 0u) == count_before + 1u;
        if (enqueue) ++metrics.enqueue_count;
        if (dequeue) ++metrics.dequeue_count;
        if (state.frontend.stalled) ++metrics.frontend_stall_cycles;
        if (count_after == FETCH_BUFFER_DEPTH) ++metrics.buffer_full_cycles;
        if (count_after > metrics.max_occupancy) metrics.max_occupancy = count_after;

        if (!ready && delivered_response) ++proof.stalled_responses;
        if (!ready && delivered_response && state.frontend.producer_valid)
            ++proof.stalled_productions;
        if (!ready && enqueue && count_before < FETCH_BUFFER_DEPTH)
            ++proof.stalled_enqueues_below_full;
        if (!ready && count_after > count_before) ++proof.stalled_occupancy_rises;
        if (!ready) proof.stalled_occupancy_level[count_after] = true;
        if (state.frontend.stalled && count_after < FETCH_BUFFER_DEPTH)
            ++proof.capacity_backpressure_below_full;
        if (state.frontend.stalled && count_after == FETCH_BUFFER_DEPTH)
            ++proof.full_capacity_backpressure;

        if (!pipe.imem_req.empty()) {
            const ImemRequest request = pipe.imem_req.read();
            assert(!response_pending);
            pending_response = response_for(request);
            response_pending = true;
            ++metrics.imem_request_count;
        } else if (!ready && count_after < FETCH_BUFFER_DEPTH &&
                   !delivered_response && !enqueue && !producer_before) {
            // Canonical Frontend permits one outstanding request. This is a
            // protocol bubble, not fetch-buffer capacity backpressure.
            ++metrics.protocol_constraint_cycles;
        }

        ++metrics.cycles;
        assert(count_after <= FETCH_BUFFER_DEPTH);
        assert(!state.frontend.stalled || count_after == FETCH_BUFFER_DEPTH);
        assert(metrics.cycles < kTargetInstructions * 16);
    }
    metrics.instructions = metrics.dequeue_count;
    return metrics;
}

void emit_csv(std::ofstream& out, const Metrics& metrics) {
    const double ipc = static_cast<double>(metrics.instructions) / metrics.cycles;
    out << metrics.scenario << ',' << metrics.path << ',' << metrics.instructions << ','
        << metrics.cycles << ',' << metrics.frontend_stall_cycles << ','
        << metrics.buffer_full_cycles << ',' << metrics.max_occupancy << ','
        << metrics.enqueue_count << ',' << metrics.dequeue_count << ','
        << metrics.decode_stall_cycles << ',' << metrics.imem_request_count << ','
        << metrics.imem_response_count << ',' << metrics.producer_count << ','
        << metrics.protocol_constraint_cycles << ',' << std::fixed << std::setprecision(6)
        << ipc << '\n';
}

void write_report(const std::string& path, const std::vector<Metrics>& rows,
                  const ProofCounters& proof) {
    std::ofstream out(path.c_str());
    assert(out.good());
    out << "# Gate 5.3 B2 Phase B Throughput Analysis\n\n"
        << "## Models\n\n"
        << "`b2_canonical_frontend_depth8` calls the product `boom::frontend_module` "
           "once per reported cycle. It drives only matched `ImemResponse` traffic from "
           "requests emitted by that frontend; responses arrive on the next call and carry "
           "the request address, fetch ID, and epoch. Words are deterministic legal 32-bit "
           "ADDI instructions. The harness never writes `producer_valid` or calls "
           "`fetch_buffer_step` directly. Decode readiness is represented at the canonical "
           "frontend boundary by its existing dispatch/decode valid inputs.\n\n"
        << "`gate5_2_unbuffered_model` is the explicit no-queue comparison: one IMEM request "
           "may be outstanding and its deterministic next-call response occupies one producer "
           "register. A Decode stall holds that register and prevents another request until "
           "acceptance. It has no occupancy or enqueue events. This is a reference model, not "
           "a second implementation of the B2 buffer.\n\n"
        << "Scenarios are A always ready, B one Decode stall every four cycles, C alternating "
           "ready/stalled (50%), D four stalled then sixteen ready cycles, and E eight stalled "
           "then twenty-four ready cycles. Each row ends after 2048 Decode acceptances.\n\n"
        << "## Results\n\n"
        << "| Scenario | Path | Instructions | Cycles | FE stalls | Full cycles | Max occ. | Enq | Deq | Decode stalls | Requests | Responses | Protocol constraints | IPC |\n"
        << "|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n";
    for (std::vector<Metrics>::const_iterator it = rows.begin(); it != rows.end(); ++it) {
        out << '|' << it->scenario << '|' << it->path << '|' << it->instructions << '|'
            << it->cycles << '|' << it->frontend_stall_cycles << '|'
            << it->buffer_full_cycles << '|' << it->max_occupancy << '|'
            << it->enqueue_count << '|' << it->dequeue_count << '|'
            << it->decode_stall_cycles << '|' << it->imem_request_count << '|'
            << it->imem_response_count << '|' << it->protocol_constraint_cycles << '|'
            << std::fixed << std::setprecision(6)
            << static_cast<double>(it->instructions) / it->cycles << "|\n";
    }
    out << "\n## Executable Integrity Proof\n\n"
        << "The audit asserts every cycle that occupancy is at most eight and that canonical "
           "`FrontendState::stalled` capacity backpressure is impossible below full. Across "
           "the stalled portions of B-E it observed " << proof.stalled_responses
        << " matched responses, " << proof.stalled_productions << " resulting held/produced "
           "instructions, " << proof.stalled_enqueues_below_full << " enqueues below full, and "
        << proof.stalled_occupancy_rises << " occupancy increases. It observed "
        << proof.full_capacity_backpressure << " full-capacity backpressure cycles and zero "
           "below-full capacity backpressure cycles. Occupancies 1 through 8 were each reached "
           "while Decode was stalled. The separately reported protocol-constraint "
           "column counts no-progress bubbles attributable to the canonical one-outstanding-request "
           "boundary rather than queue capacity.\n\n"
        << "## Interpretation\n\n"
        << "The depth-eight queue does not improve long-run Decode-limited IPC: all paths must "
           "wait for the same readiness schedule. Its demonstrated benefit is decoupling: requests, "
           "responses, production, and enqueues continue during stalls while occupancy rises, "
           "and recurring stalls in B-E eventually reach capacity because their average Decode "
           "acceptance is below the frontend production rate. Request and enqueue counts can exceed "
           "2048 because the canonical "
           "frontend remains ahead when the measurement stops; those are retained in the CSV rather "
           "than being relabeled as completed instructions. These are native C++ call-level results, "
           "not HLS wrapper clock or timing claims.\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    const char* scenarios[] = {"A_ALWAYS_READY", "B_STALL_1_OF_4",
        "C_STALL_50_PERCENT", "D_BURST_STALL_4", "E_BURST_STALL_8"};
    std::vector<Metrics> rows;
    ProofCounters proof;
    for (unsigned i = 0; i < 5; ++i) {
        rows.push_back(run_unbuffered(scenarios[i]));
        rows.push_back(run_canonical(scenarios[i], proof));
    }

    assert(proof.stalled_responses > 0);
    assert(proof.stalled_productions > 0);
    assert(proof.stalled_enqueues_below_full > 0);
    assert(proof.stalled_occupancy_rises > 0);
    assert(proof.capacity_backpressure_below_full == 0);
    assert(proof.full_capacity_backpressure > 0);
    for (unsigned occupancy = 1; occupancy <= FETCH_BUFFER_DEPTH; ++occupancy)
        assert(proof.stalled_occupancy_level[occupancy]);
    assert(rows[9].max_occupancy == FETCH_BUFFER_DEPTH);

    std::ofstream csv(argv[1]);
    assert(csv.good());
    csv << "scenario,path,instructions,cycles,frontend_stall_cycles,buffer_full_cycles,"
           "max_occupancy,enqueue_count,dequeue_count,decode_stall_cycles,imem_request_count,"
           "imem_response_count,producer_count,protocol_constraint_cycles,ipc\n";
    for (std::vector<Metrics>::const_iterator it = rows.begin(); it != rows.end(); ++it)
        emit_csv(csv, *it);
    csv.close();
    write_report(argv[2], rows, proof);

    std::cout << "ASSERT_PASS canonical_frontend_only_imem_driven\n"
              << "ASSERT_PASS stalled_receive_produce_enqueue responses="
              << proof.stalled_responses << " productions=" << proof.stalled_productions
              << " enqueues_below_full=" << proof.stalled_enqueues_below_full << '\n'
              << "ASSERT_PASS occupancy_rises_during_decode_stalls rises="
              << proof.stalled_occupancy_rises << " max=" << unsigned(rows[9].max_occupancy)
              << " levels=1..8\n"
              << "ASSERT_PASS capacity_backpressure_full_only full_cycles="
              << proof.full_capacity_backpressure << " below_full=0\n"
              << "GATE5_3_B2_PHASE_B_TEST_INTEGRITY_PASS\n";
    return 0;
}
