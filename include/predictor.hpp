#ifndef BOOM_PREDICTOR_HPP
#define BOOM_PREDICTOR_HPP

#include <cstddef>
#include <cstdint>

#include "predecode.hpp"

namespace boom {

struct PredictorRequest {
    uint64_t pc;
    uint8_t cfi_lane;
    uint8_t cfi_type;
    bool static_target_valid;
    uint64_t static_target;
    uint32_t generation;
    uint64_t request_token;

    PredictorRequest()
        : pc(0), cfi_lane(0), cfi_type(CFI_NONE),
          static_target_valid(false), static_target(0), generation(0),
          request_token(0) {}
};

struct PredictorResponse {
    bool prediction_valid;
    bool taken;
    bool target_valid;
    uint64_t target;
    uint8_t cfi_lane;
    uint8_t cfi_type;
    uint16_t metadata_token;
    uint32_t generation;
    uint64_t request_token;

    PredictorResponse()
        : prediction_valid(false), taken(false), target_valid(false), target(0),
          cfi_lane(0), cfi_type(CFI_NONE), metadata_token(0), generation(0),
          request_token(0) {}
};

struct PredictorUpdate {
    bool valid;
    bool commit_qualified;
    uint8_t cfi_type;
    uint64_t pc;
    uint16_t metadata_token;
    bool taken;
    uint32_t generation;

    PredictorUpdate()
        : valid(false), commit_qualified(false), cfi_type(CFI_NONE), pc(0),
          metadata_token(0), taken(false), generation(0) {}
};

struct PredictorStepInput {
    bool reset;
    uint32_t active_generation;
    bool req_valid;
    PredictorRequest request;
    bool resp_ready;
    PredictorUpdate update;

    PredictorStepInput()
        : reset(false), active_generation(0), req_valid(false), request(),
          resp_ready(false), update() {}
};

struct PredictorStepOutput {
    bool req_ready;
    bool resp_valid;
    PredictorResponse response;

    PredictorStepOutput() : req_ready(false), resp_valid(false), response() {}
};

template <std::size_t Entries, bool FullPayloadReset = false>
class PredictorFoundation {
public:
    PredictorFoundation();
    PredictorStepOutput step(const PredictorStepInput& input);

private:
    static_assert(Entries == 64 || Entries == 128 || Entries == 256 ||
                  Entries == 512,
                  "PredictorFoundation supports 64, 128, 256, or 512 entries");

    uint8_t counters_[Entries];
    bool valid_[Entries];
    bool response_pending_;
    PredictorResponse pending_response_;
};

extern template class PredictorFoundation<64>;
extern template class PredictorFoundation<128>;
extern template class PredictorFoundation<256>;
extern template class PredictorFoundation<512>;
extern template class PredictorFoundation<256, true>;

}  // namespace boom

#endif
