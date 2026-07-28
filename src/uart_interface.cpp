#include "gte/uart_interface.hpp"

namespace gte {

void FakeUart::writeBytes(const std::vector<std::uint8_t>& bytes) {
    writes_.push_back({
        bytes,
        std::chrono::steady_clock::now(),
    });
}

const std::vector<UartWrite>& FakeUart::writes() const {
    return writes_;
}

void FakeUart::clearWrites() {
    writes_.clear();
}

} // namespace gte
