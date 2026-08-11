#ifndef BOOM_RVC_HPP
#define BOOM_RVC_HPP

#include <cstdint>

namespace boom {

struct RvcDecodeResult {
    bool valid;
    bool legal;
    uint32_t instruction;
    uint16_t compressed;
    uint8_t length_bytes;

    RvcDecodeResult()
        : valid(false), legal(false), instruction(0), compressed(0),
          length_bytes(0) {}

    explicit RvcDecodeResult(uint16_t parcel)
        : valid((parcel & 3u) != 3u), legal(false), instruction(0),
          compressed(parcel), length_bytes((parcel & 3u) != 3u ? 2 : 0) {}
};

RvcDecodeResult decompress_rvc(uint16_t compressed);

using RvcDecompressResult = RvcDecodeResult;
RvcDecodeResult decompress_rv64c(uint16_t compressed);

}  // namespace boom

#endif
