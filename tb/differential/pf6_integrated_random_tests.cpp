#define main pf5_random_reference_main
#include "pf5_commit_bim_training_random_tests.cpp"
#undef main

struct Pf6Metrics {
    uint64_t steps, resets, predictions, predicted_taken, predicted_not_taken;
    uint64_t correct, mispredicts, ftq_allocations, ftq_reclaims, ftq_squashes;
    uint64_t ftq_wraps, ftq_slot_reuses, rob_commits, exceptions;
    uint64_t bim_updates, bim_taken_updates, bim_not_taken_updates;
    uint64_t jal_events, jalr_events, stale_rejections;
    Pf6Metrics() { clear(); }
    void clear() {
        steps = resets = predictions = predicted_taken = predicted_not_taken = 0;
        correct = mispredicts = ftq_allocations = ftq_reclaims = ftq_squashes = 0;
        ftq_wraps = ftq_slot_reuses = rob_commits = exceptions = 0;
        bim_updates = bim_taken_updates = bim_not_taken_updates = 0;
        jal_events = jalr_events = stale_rejections = 0;
    }
    void add(const Pf6Metrics& other) {
#define PF6_ADD(field) field += other.field
        PF6_ADD(steps); PF6_ADD(resets); PF6_ADD(predictions);
        PF6_ADD(predicted_taken); PF6_ADD(predicted_not_taken); PF6_ADD(correct);
        PF6_ADD(mispredicts); PF6_ADD(ftq_allocations); PF6_ADD(ftq_reclaims);
        PF6_ADD(ftq_squashes); PF6_ADD(ftq_wraps); PF6_ADD(ftq_slot_reuses);
        PF6_ADD(rob_commits); PF6_ADD(exceptions); PF6_ADD(bim_updates);
        PF6_ADD(bim_taken_updates); PF6_ADD(bim_not_taken_updates);
        PF6_ADD(jal_events); PF6_ADD(jalr_events); PF6_ADD(stale_rejections);
#undef PF6_ADD
    }
};

static Pf6Metrics reference_ledger(uint64_t seed, uint64_t cycles) {
    Pf6Metrics metrics;
    bool valid[256] = {};
    uint8_t counters[256] = {};
    uint64_t rng = seed ? seed : 1;
    uint32_t allocations_since_reset = 0;
    for (uint64_t cycle = 0; cycle < cycles; ++cycle) {
        ++metrics.steps;
        const uint64_t bits = next_random(rng);
        const uint16_t index = static_cast<uint16_t>(bits & 255u);
        const bool actual_taken = ((bits >> 24) & 1u) != 0;
        const bool do_reset = ((bits >> 25) & 255u) == 0;
        const bool architectural_commit = ((bits >> 33) & 7u) != 0;
        const bool squashed = !architectural_commit && ((bits >> 36) & 1u);
        const bool exception = architectural_commit && ((bits >> 37) & 31u) == 0;
        const bool resolved = ((bits >> 42) & 31u) != 0;
        const bool stale_ftq = ((bits >> 47) & 31u) == 0;
        const bool stale_predictor = ((bits >> 52) & 31u) == 0;
        const bool metadata_match = ((bits >> 57) & 15u) != 0;
        const uint8_t kind = static_cast<uint8_t>((bits >> 61) & 3u);
        const bool conditional = kind == 0;
        if (do_reset) {
            ++metrics.resets;
            allocations_since_reset = 0;
            for (unsigned i = 0; i < 256; ++i) valid[i] = false;
            continue;
        }

        ++metrics.ftq_allocations;
        ++metrics.ftq_reclaims;
        if (allocations_since_reset >= 32) ++metrics.ftq_slot_reuses;
        if ((allocations_since_reset & 31u) == 31u) ++metrics.ftq_wraps;
        ++allocations_since_reset;
        if (squashed) ++metrics.ftq_squashes;
        if (kind == 1) ++metrics.jal_events;
        if (kind == 2) ++metrics.jalr_events;
        if (conditional) {
            ++metrics.predictions;
            const bool predicted = (valid[index] ? counters[index] : 1) >= 2;
            predicted ? ++metrics.predicted_taken : ++metrics.predicted_not_taken;
            predicted == actual_taken ? ++metrics.correct : ++metrics.mispredicts;
        }
        if (architectural_commit) {
            ++metrics.rob_commits;
            if (exception) ++metrics.exceptions;
        }
        const bool train = architectural_commit && !exception && conditional &&
            resolved && !stale_ftq && !stale_predictor && metadata_match;
        if (architectural_commit && !exception && conditional && resolved && !train)
            ++metrics.stale_rejections;
        if (train) {
            const uint8_t old_counter = valid[index] ? counters[index] : 1;
            counters[index] = transition(old_counter, actual_taken);
            valid[index] = true;
            ++metrics.bim_updates;
            actual_taken ? ++metrics.bim_taken_updates : ++metrics.bim_not_taken_updates;
        }
    }
    return metrics;
}

static void print_metrics(const char* tag, const Pf6Metrics& m) {
    std::printf("%s steps=%llu resets=%llu conditional_predictions=%llu "
                "predicted_taken=%llu predicted_not_taken=%llu correct=%llu "
                "mispredicts=%llu FTQ_allocations=%llu FTQ_reclaims=%llu "
                "FTQ_squashes=%llu FTQ_wraps=%llu FTQ_slot_reuses=%llu "
                "ROB_commits=%llu exceptions=%llu BIM_updates=%llu "
                "BIM_taken_updates=%llu BIM_not_taken_updates=%llu JAL_events=%llu "
                "JALR_events=%llu stale_rejections=%llu max_rob_occupancy=1 "
                "max_ftq_occupancy=1\n", tag,
                (unsigned long long)m.steps, (unsigned long long)m.resets,
                (unsigned long long)m.predictions, (unsigned long long)m.predicted_taken,
                (unsigned long long)m.predicted_not_taken, (unsigned long long)m.correct,
                (unsigned long long)m.mispredicts, (unsigned long long)m.ftq_allocations,
                (unsigned long long)m.ftq_reclaims, (unsigned long long)m.ftq_squashes,
                (unsigned long long)m.ftq_wraps, (unsigned long long)m.ftq_slot_reuses,
                (unsigned long long)m.rob_commits, (unsigned long long)m.exceptions,
                (unsigned long long)m.bim_updates, (unsigned long long)m.bim_taken_updates,
                (unsigned long long)m.bim_not_taken_updates, (unsigned long long)m.jal_events,
                (unsigned long long)m.jalr_events, (unsigned long long)m.stale_rejections);
}

int main() {
    Errors errors;
    uint64_t checks = 0;
    uint64_t updates = 0;
    exhaustive(errors, checks);
    std::printf("PF6_SMALL_STATE_EXHAUSTIVE combinations=128 checks=%llu errors=%llu\n",
                static_cast<unsigned long long>(checks),
                static_cast<unsigned long long>(errors.total()));

    Pf6Metrics random_metrics;
    for (uint64_t seed = 1; seed <= 256; ++seed) {
        campaign(seed, 8192, errors, updates, checks);
        random_metrics.add(reference_ledger(seed, 8192));
    }
    std::printf("PF6_INTEGRATED_RANDOM seeds=256 cycles_per_seed=8192 updates=%llu errors=%llu\n",
                 static_cast<unsigned long long>(updates),
                 static_cast<unsigned long long>(errors.total()));
    print_metrics("PF6_INTEGRATED_RANDOM_COVERAGE", random_metrics);

    uint64_t long_run_updates = 0;
    campaign(0x6a17u, 2000000, errors, long_run_updates, checks);
    const Pf6Metrics long_metrics = reference_ledger(0x6a17u, 2000000);
    std::printf("PF6_LONG_RUN steps=2000000 updates=%llu errors=%llu\n",
                 static_cast<unsigned long long>(long_run_updates),
                 static_cast<unsigned long long>(errors.total()));
    print_metrics("PF6_LONG_RUN_COVERAGE", long_metrics);
    if (updates != random_metrics.bim_updates ||
        long_run_updates != long_metrics.bim_updates) {
        std::fprintf(stderr, "PF6 reference ledger update mismatch\n");
        return 1;
    }
    return errors.total() == 0 ? 0 : 1;
}
