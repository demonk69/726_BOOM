#include "ftq.hpp"

namespace boom {

template class FtqFoundation<2>;
template class FtqFoundation<4>;
template class FtqFoundation<8>;
template class FtqFoundation<16>;
template class FtqFoundation<32>;
template class FtqFoundation<64>;
template class FtqFoundation<32, true>;

}  // namespace boom
