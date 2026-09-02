#include "predictor.hpp"
#include "predecode.hpp"
#include "rvc.hpp"

#include <cstdint>
#include <cstdio>

static uint32_t encode_b(int32_t imm, uint32_t f3) {
    const uint32_t x = static_cast<uint32_t>(imm) & 0x1fffu;
    return (((x >> 12) & 1u) << 31) | (((x >> 5) & 0x3fu) << 25) |
           (3u << 20) | (2u << 15) | (f3 << 12) |
           (((x >> 1) & 0xfu) << 8) | (((x >> 11) & 1u) << 7) | 0x63u;
}

static uint32_t encode_j(int32_t imm) {
    const uint32_t x = static_cast<uint32_t>(imm) & 0x1fffffu;
    return (((x >> 20) & 1u) << 31) | (((x >> 1) & 0x3ffu) << 21) |
           (((x >> 11) & 1u) << 20) | (((x >> 12) & 0xffu) << 12) |
           (1u << 7) | 0x6fu;
}

static uint32_t encode_jalr(int32_t imm) {
    return ((static_cast<uint32_t>(imm) & 0xfffu) << 20) |
           (1u << 15) | (5u << 7) | 0x67u;
}

struct Checker {
    uint64_t checks;
    uint64_t failures;
    Checker() : checks(0), failures(0) {}
    void expect(bool value, const char* label) {
        ++checks;
        if (!value) {
            if (failures < 20) std::printf("FAIL,%s\n", label);
            ++failures;
        }
    }
};

static boom::PredictorResponse compose(boom::PredictorFoundation<256>& dut,
                                       uint64_t pc, uint32_t instruction,
                                       bool is_rvc, uint8_t lane,
                                       uint64_t token, Checker& c) {
    const boom::CfiPredecodeResult decoded =
        boom::predecode_cfi(pc, instruction, is_rvc);
    boom::PredictorStepInput in;
    in.active_generation = 19;
    in.req_valid = true;
    in.request.pc = pc;
    in.request.cfi_lane = lane;
    in.request.cfi_type = decoded.cfi_type;
    in.request.static_target_valid = decoded.static_target_valid;
    in.request.static_target = decoded.static_target;
    in.request.generation = 19;
    in.request.request_token = token;
    c.expect(dut.step(in).req_ready, "composition_request_accepted");
    in.req_valid = false;
    const boom::PredictorStepOutput out = dut.step(in);
    c.expect(out.resp_valid, "composition_response_cycle");
    in.resp_ready = true;
    dut.step(in);
    return out.response;
}

int main() {
    Checker c;
    boom::PredictorFoundation<256> dut;
    boom::PredictorStepInput reset;
    reset.reset = true;
    reset.active_generation = 19;
    dut.step(reset);
    const uint32_t funct3[] = {0, 1, 4, 5, 6, 7};
    uint64_t token = 0;
    for (unsigned repetition = 0; repetition < 180; ++repetition) {
        const uint64_t pc = 0x80000000ull + repetition * 0x20u;
        for (unsigned kind = 0; kind < 6; ++kind) {
            const int32_t immediate = static_cast<int32_t>((repetition & 63u) * 2u) - 64;
            const boom::PredictorResponse r = compose(
                dut, pc + kind * 2u, encode_b(immediate, funct3[kind]), false,
                static_cast<uint8_t>(kind & 1u), token++, c);
            c.expect(r.prediction_valid && !r.taken && !r.target_valid,
                     "conditional_weak_nt");
            c.expect(r.cfi_type == boom::CFI_CONDITIONAL_BRANCH &&
                     r.cfi_lane == (kind & 1u), "conditional_metadata");
        }
        const boom::PredictorResponse jal = compose(
            dut, pc, encode_j(static_cast<int32_t>((repetition + 1) * 2)), false,
            0, token++, c);
        c.expect(jal.prediction_valid && jal.taken && jal.target_valid,
                 "jal_taken_target");
        c.expect(jal.target != 0 && jal.cfi_type == boom::CFI_JAL,
                 "jal_metadata");
        const boom::PredictorResponse jalr = compose(
            dut, pc + 2, encode_jalr(static_cast<int32_t>(repetition & 0x7ffu)),
            false, 1, token++, c);
        c.expect(!jalr.prediction_valid && !jalr.taken && !jalr.target_valid &&
                 jalr.cfi_type == boom::CFI_JALR, "jalr_unavailable");
        const boom::PredictorResponse none = compose(
            dut, pc + 4, 0x00100093u, false, 0, token++, c);
        c.expect(!none.prediction_valid && !none.taken && !none.target_valid &&
                  none.cfi_type == boom::CFI_NONE, "non_cfi_unavailable");

        const uint16_t compressed[] = {
            0xc001u,  // C.BEQZ
            0xe001u,  // C.BNEZ
            0xa001u,  // C.J
            0x8082u,  // C.JR x1
            0x9082u   // C.JALR x1
        };
        const uint8_t expected_type[] = {
            boom::CFI_CONDITIONAL_BRANCH, boom::CFI_CONDITIONAL_BRANCH,
            boom::CFI_JAL, boom::CFI_JALR, boom::CFI_JALR
        };
        for (unsigned i = 0; i < 5; ++i) {
            const boom::RvcDecodeResult decoded =
                boom::decompress_rvc(compressed[i]);
            c.expect(decoded.valid && decoded.legal, "rvc_decompress");
            const boom::PredictorResponse response = compose(
                dut, pc + i * 2u, decoded.instruction, true,
                static_cast<uint8_t>(i & 1u), token++, c);
            c.expect(response.cfi_type == expected_type[i],
                     "rvc_classification_passthrough");
            if (expected_type[i] == boom::CFI_JAL) {
                const boom::CfiPredecodeResult predecoded =
                    boom::predecode_cfi(pc + i * 2u, decoded.instruction, true);
                c.expect(response.prediction_valid && response.taken &&
                         response.target_valid &&
                         response.target == predecoded.static_target,
                         "rvc_jal_static_target");
            } else if (expected_type[i] == boom::CFI_JALR) {
                c.expect(!response.prediction_valid && !response.target_valid,
                         "rvc_jalr_unavailable");
            } else {
                c.expect(response.prediction_valid && !response.taken &&
                         !response.target_valid, "rvc_branch_weak_nt");
            }
        }
    }
    std::printf("PREDICTOR_PREDECODE_COMPOSITION,checks=%llu,failures=%llu\n",
                static_cast<unsigned long long>(c.checks),
                static_cast<unsigned long long>(c.failures));
    return c.failures == 0 && c.checks >= 2000 ? 0 : 1;
}
