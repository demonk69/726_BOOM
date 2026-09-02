#include "predictor.hpp"

namespace boom {

template <std::size_t Entries, bool FullPayloadReset>
PredictorFoundation<Entries, FullPayloadReset>::PredictorFoundation()
    : response_pending_(false), pending_response_() {
    for (std::size_t i = 0; i < Entries; ++i) valid_[i] = false;
}

template <std::size_t Entries, bool FullPayloadReset>
PredictorStepOutput PredictorFoundation<Entries, FullPayloadReset>::step(
    const PredictorStepInput& input) {
#if defined(BOOM_PREDICTOR_STORAGE_LUTRAM)
#pragma HLS bind_storage variable=counters_ type=RAM_2P impl=LUTRAM
#elif defined(BOOM_PREDICTOR_STORAGE_BRAM)
#pragma HLS bind_storage variable=counters_ type=RAM_2P impl=BRAM
#endif
    PredictorStepOutput output;
    output.req_ready = !response_pending_ && !input.reset;
    output.resp_valid = response_pending_ && !input.reset;
    if (response_pending_) output.response = pending_response_;

    if (input.reset) {
        for (std::size_t i = 0; i < Entries; ++i) {
            valid_[i] = false;
            if (FullPayloadReset) counters_[i] = 1;
        }
        response_pending_ = false;
        return output;
    }

    const std::size_t update_index = static_cast<std::size_t>(
        (input.update.pc >> 1) & static_cast<uint64_t>(Entries - 1));
    const bool train = input.update.valid && input.update.commit_qualified &&
        input.update.cfi_type == CFI_CONDITIONAL_BRANCH &&
        input.update.generation == input.active_generation &&
        input.update.metadata_token == update_index;
    uint8_t updated_counter = 0;
    if (train) {
        const uint8_t old_counter = valid_[update_index] ?
            counters_[update_index] : static_cast<uint8_t>(1);
        updated_counter = input.update.taken ?
            static_cast<uint8_t>(old_counter == 3 ? 3 : old_counter + 1) :
            static_cast<uint8_t>(old_counter == 0 ? 0 : old_counter - 1);
        counters_[update_index] = updated_counter;
        valid_[update_index] = true;
    }

    if (response_pending_) {
        if (input.resp_ready) response_pending_ = false;
        return output;
    }

    if (input.req_valid && output.req_ready) {
        const PredictorRequest& request = input.request;
        const std::size_t request_index = static_cast<std::size_t>(
            (request.pc >> 1) & static_cast<uint64_t>(Entries - 1));
        PredictorResponse response;
        response.cfi_lane = request.cfi_lane;
        response.cfi_type = request.cfi_type;
        response.metadata_token = static_cast<uint16_t>(request_index);
        response.generation = request.generation;
        response.request_token = request.request_token;

        if (request.cfi_type == CFI_CONDITIONAL_BRANCH) {
            const uint8_t counter = train && update_index == request_index ?
                updated_counter : (valid_[request_index] ?
                    counters_[request_index] : static_cast<uint8_t>(1));
            response.prediction_valid = true;
            response.taken = (counter & 2u) != 0;
            response.target_valid = response.taken && request.static_target_valid;
        } else if (request.cfi_type == CFI_JAL) {
            response.prediction_valid = true;
            response.taken = true;
            response.target_valid = request.static_target_valid;
        }
        response.target = response.target_valid ? request.static_target : 0;
        pending_response_ = response;
        response_pending_ = true;
    }
    return output;
}

template class PredictorFoundation<64>;
template class PredictorFoundation<128>;
template class PredictorFoundation<256>;
template class PredictorFoundation<512>;
template class PredictorFoundation<256, true>;

}  // namespace boom
