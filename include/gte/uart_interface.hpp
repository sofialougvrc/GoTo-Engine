#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

namespace gte {

struct UartWrite {
    std::vector<std::uint8_t> bytes;
    std::chrono::steady_clock::time_point timestamp;
};

class UartInterface {
public:
    virtual void writeBytes(const std::vector<std::uint8_t>& bytes) = 0;
    virtual ~UartInterface() = default;
};

class FakeUart : public UartInterface {
public:
    void writeBytes(const std::vector<std::uint8_t>& bytes) override;

    const std::vector<UartWrite>& writes() const;
    void clearWrites();

private:
    std::vector<UartWrite> writes_;
};

} // namespace gte
