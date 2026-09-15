#pragma once

#include <AP_HAL/AP_HAL.h>
#include <AP_HAL/utility/RingBuffer.h>

#include <stddef.h>
#include <stdint.h>

#include "AP_HAL_SITL_Namespace.h"

class HALSITL::WASMUARTDriver : public AP_HAL::UARTDriver {
public:
    bool is_initialized() override { return _initialized; }
    bool tx_pending() override;
    uint32_t txspace() override;

    size_t js_write(const uint8_t *buf, size_t len);
    size_t js_read(uint8_t *buf, size_t max_len);
    size_t js_read_available() const;

protected:
    void _begin(uint32_t baud, uint16_t rxSpace, uint16_t txSpace) override;
    size_t _write(const uint8_t *buffer, size_t size) override;
    ssize_t _read(uint8_t *buffer, uint16_t count) override;
    void _end() override;
    void _flush() override;
    uint32_t _available() override;
    bool _discard_input() override;

private:
    static constexpr size_t BUF_SIZE = 16384;

    ByteBuffer _tx_buf{BUF_SIZE};
    ByteBuffer _rx_buf{BUF_SIZE};

    bool _initialized = false;
};