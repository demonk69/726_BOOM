#ifndef BOOM_INTERFACES_HPP
#define BOOM_INTERFACES_HPP

#include "boom_config.hpp"
#include "boom_types.hpp"
#include <cstdint>

#ifdef __VITIS_HLS__
#include <hls_stream.h>
#else
#include <queue>
namespace hls {
template <typename T>
class stream {
    std::queue<T> q_;
    static constexpr int MAX_SIZE = 1024;
public:
    void write(const T& v) { q_.push(v); }
    T read() { T v = q_.front(); q_.pop(); return v; }
    bool empty() const { return q_.empty(); }
    bool full() const { return q_.size() >= MAX_SIZE; }
};
}
#endif

struct PipeSignals {
    hls::stream<ImemRequest>  imem_req;
    hls::stream<ImemResponse> imem_resp;
    hls::stream<DmemRequest>  dmem_req;
    hls::stream<DmemResponse> dmem_resp;
    hls::stream<CommitEntry>  commit_trace;
};

#endif
