#include "rvc.hpp"

void synth_rvc_top(uint16_t compressed, bool& valid, bool& legal,
                   uint32_t& decompressed, uint8_t& length) {
    const boom::RvcDecodeResult result = boom::decompress_rvc(compressed);
    valid = result.valid;
    legal = result.legal;
    decompressed = result.instruction;
    length = result.length_bytes;
}
