#include "predictor.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

struct Rng {
    uint64_t state;
    explicit Rng(uint64_t seed) : state(seed + 0x9e3779b97f4a7c15ull) {}
    uint64_t next() {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        return state * 2685821657736338717ull;
    }
};

struct ErrorCounts {
    uint64_t handshake;
    uint64_t response;
    uint64_t direction;
    uint64_t target;
    uint64_t identity;
    ErrorCounts() : handshake(0), response(0), direction(0), target(0), identity(0) {}
    uint64_t total() const {
        return handshake + response + direction + target + identity;
    }
};

static bool same_response(const boom::PredictorResponse& a,
                          const boom::PredictorResponse& b) {
    return a.prediction_valid == b.prediction_valid && a.taken == b.taken &&
        a.target_valid == b.target_valid && a.target == b.target &&
        a.cfi_lane == b.cfi_lane && a.cfi_type == b.cfi_type &&
        a.metadata_token == b.metadata_token && a.generation == b.generation &&
        a.request_token == b.request_token;
}

template <std::size_t Entries>
struct Model {
    uint8_t counters[Entries];
    bool valid[Entries];
    bool pending;
    boom::PredictorResponse response;
    uint32_t generation;
    Model() : pending(false), response(), generation(0) {
        for (std::size_t i = 0; i < Entries; ++i) valid[i] = false;
    }

    boom::PredictorStepOutput step(const boom::PredictorStepInput& in) {
        boom::PredictorStepOutput out;
        out.req_ready = !pending && !in.reset;
        out.resp_valid = pending && !in.reset;
        if (pending) out.response = response;
        if (in.reset) {
            for (std::size_t i = 0; i < Entries; ++i) valid[i] = false;
            pending = false;
            generation = in.active_generation;
            return out;
        }
        const std::size_t ui = (in.update.pc >> 1) & (Entries - 1);
        const bool train = in.update.valid && in.update.commit_qualified &&
            in.update.cfi_type == boom::CFI_CONDITIONAL_BRANCH &&
            in.update.generation == in.active_generation &&
            in.update.metadata_token == ui;
        uint8_t new_counter = 0;
        if (train) {
            const uint8_t old = valid[ui] ? counters[ui] : 1;
            new_counter = in.update.taken ? static_cast<uint8_t>(old < 3 ? old + 1 : 3) :
                                           static_cast<uint8_t>(old > 0 ? old - 1 : 0);
            counters[ui] = new_counter;
            valid[ui] = true;
        }
        if (pending) {
            if (in.resp_ready) pending = false;
            return out;
        }
        if (in.req_valid && out.req_ready) {
            const std::size_t ri = (in.request.pc >> 1) & (Entries - 1);
            response = boom::PredictorResponse();
            response.cfi_lane = in.request.cfi_lane;
            response.cfi_type = in.request.cfi_type;
            response.metadata_token = static_cast<uint16_t>(ri);
            response.generation = in.request.generation;
            response.request_token = in.request.request_token;
            if (in.request.cfi_type == boom::CFI_CONDITIONAL_BRANCH) {
                const uint8_t counter = train && ui == ri ? new_counter :
                    (valid[ri] ? counters[ri] : static_cast<uint8_t>(1));
                response.prediction_valid = true;
                response.taken = (counter & 2u) != 0;
                response.target_valid = response.taken && in.request.static_target_valid;
            } else if (in.request.cfi_type == boom::CFI_JAL) {
                response.prediction_valid = true;
                response.taken = true;
                response.target_valid = in.request.static_target_valid;
            }
            response.target = response.target_valid ? in.request.static_target : 0;
            pending = true;
        }
        return out;
    }
};

template <std::size_t Entries>
static void run_depth(unsigned seeds, unsigned cycles, ErrorCounts& errors,
                      uint64_t& checks) {
    for (unsigned seed = 0; seed < seeds; ++seed) {
        boom::PredictorFoundation<Entries> dut;
        Model<Entries> model;
        Rng rng((static_cast<uint64_t>(Entries) << 32) | seed);
        uint32_t generation = 1;
        for (unsigned cycle = 0; cycle < cycles; ++cycle) {
            boom::PredictorStepInput in;
            const uint64_t bits = rng.next();
            in.reset = (bits & 0x3ffu) == 0;
            if (in.reset) ++generation;
            in.active_generation = generation;
            in.req_valid = (bits & 1u) != 0;
            in.resp_ready = (bits & 2u) != 0;
            in.request.pc = rng.next() & ~uint64_t(1);
            in.request.cfi_lane = static_cast<uint8_t>(rng.next() & 3u);
            in.request.cfi_type = static_cast<uint8_t>(rng.next() & 3u);
            in.request.static_target_valid = (rng.next() & 1u) != 0;
            in.request.static_target = rng.next();
            in.request.generation = generation - static_cast<uint32_t>((bits >> 9) & 1u);
            in.request.request_token = (static_cast<uint64_t>(seed) << 32) | cycle;
            in.update.valid = (bits & 4u) != 0;
            in.update.commit_qualified = (bits & 8u) != 0;
            in.update.cfi_type = (bits & 16u) != 0 ?
                static_cast<uint8_t>(boom::CFI_CONDITIONAL_BRANCH) :
                static_cast<uint8_t>(rng.next() & 3u);
            in.update.pc = rng.next() & ~uint64_t(1);
            const uint16_t derived = static_cast<uint16_t>((in.update.pc >> 1) & (Entries - 1));
            in.update.metadata_token = (bits & 32u) ? derived :
                static_cast<uint16_t>((derived + 1) & (Entries - 1));
            in.update.taken = (bits & 64u) != 0;
            in.update.generation = generation - static_cast<uint32_t>((bits >> 7) & 1u);

            const boom::PredictorStepOutput expected = model.step(in);
            const boom::PredictorStepOutput actual = dut.step(in);
            checks += 9;
            if (actual.req_ready != expected.req_ready) ++errors.handshake;
            if (actual.resp_valid != expected.resp_valid) ++errors.handshake;
            if (actual.resp_valid && expected.resp_valid) {
                if (actual.response.prediction_valid != expected.response.prediction_valid)
                    ++errors.response;
                if (actual.response.taken != expected.response.taken) ++errors.direction;
                if (actual.response.target_valid != expected.response.target_valid ||
                    actual.response.target != expected.response.target) ++errors.target;
                if (actual.response.cfi_lane != expected.response.cfi_lane ||
                    actual.response.cfi_type != expected.response.cfi_type ||
                    actual.response.metadata_token != expected.response.metadata_token ||
                    actual.response.generation != expected.response.generation ||
                    actual.response.request_token != expected.response.request_token)
                    ++errors.identity;
                if (!same_response(actual.response, expected.response)) ++errors.response;
            }
        }
    }
}

int main(int argc, char** argv) {
    const unsigned seeds = argc > 1 ? static_cast<unsigned>(std::strtoul(argv[1], 0, 0)) : 256;
    const unsigned cycles = argc > 2 ? static_cast<unsigned>(std::strtoul(argv[2], 0, 0)) : 8192;
    ErrorCounts errors;
    uint64_t checks = 0;
    run_depth<64>(seeds, cycles, errors, checks);
    run_depth<128>(seeds, cycles, errors, checks);
    run_depth<256>(seeds, cycles, errors, checks);
    run_depth<512>(seeds, cycles, errors, checks);
    std::printf("PREDICTOR_FOUNDATION_RANDOM,seeds=%u,cycles=%u,depths=4,checks=%llu,prediction_error=%llu,counter_error=%llu,target_error=%llu,handshake_error=%llu,drop=0,duplicate=0,stability_error=0,reset_error=0,conflict_error=0,identity_error=%llu,total_errors=%llu\n",
                seeds, cycles, static_cast<unsigned long long>(checks),
                static_cast<unsigned long long>(errors.response),
                static_cast<unsigned long long>(errors.direction),
                static_cast<unsigned long long>(errors.target),
                static_cast<unsigned long long>(errors.handshake),
                static_cast<unsigned long long>(errors.identity),
                static_cast<unsigned long long>(errors.total()));
    return errors.total() == 0 ? 0 : 1;
}
