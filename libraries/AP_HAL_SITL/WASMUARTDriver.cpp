#include <AP_HAL/AP_HAL.h>

#if HAL_SITL_WASM_ENABLED

#include <emscripten/emscripten.h>
#include <stdlib.h>

#include "WASMUARTDriver.h"

using namespace HALSITL;

static WASMUARTDriver &serial0()
{
    return *static_cast<WASMUARTDriver *>(AP_HAL::get_HAL_mutable().serial(0));
}

extern "C" {

EMSCRIPTEN_KEEPALIVE size_t ardupilot_serial0_write(const uint8_t *buf, size_t len)
{
    return serial0().js_write(buf, len);
}

EMSCRIPTEN_KEEPALIVE size_t ardupilot_serial0_read(uint8_t *buf, size_t max_len)
{
    return serial0().js_read(buf, max_len);
}

EMSCRIPTEN_KEEPALIVE size_t ardupilot_serial0_read_available(void)
{
    return serial0().js_read_available();
}

EMSCRIPTEN_KEEPALIVE void *ardupilot_malloc(size_t size)
{
    return malloc(size);
}

EMSCRIPTEN_KEEPALIVE void ardupilot_free(void *ptr)
{
    free(ptr);
}

} // extern "C"

void WASMUARTDriver::_begin(uint32_t /*baud*/, uint16_t /*rxSpace*/, uint16_t /*txSpace*/)
{
    _initialized = true;
}

size_t WASMUARTDriver::_write(const uint8_t *buffer, size_t size)
{
    return _tx_buf.write(buffer, size);
}

ssize_t WASMUARTDriver::_read(uint8_t *buffer, uint16_t count)
{
    return _rx_buf.read(buffer, count);
}

void WASMUARTDriver::_end() {}

void WASMUARTDriver::_flush()
{
    // TX is drained asynchronously by the host.
}

bool WASMUARTDriver::tx_pending()
{
    return !_tx_buf.is_empty();
}

uint32_t WASMUARTDriver::_available()
{
    return _rx_buf.available();
}

bool WASMUARTDriver::_discard_input()
{
    _rx_buf.advance(_rx_buf.available());
    return true;
}

uint32_t WASMUARTDriver::txspace()
{
    return _tx_buf.space();
}

size_t WASMUARTDriver::js_write(const uint8_t *buf, size_t len)
{
    return _rx_buf.write(buf, len);
}

size_t WASMUARTDriver::js_read(uint8_t *buf, size_t max_len)
{
    return _tx_buf.read(buf, max_len);
}

size_t WASMUARTDriver::js_read_available() const
{
    return _tx_buf.available();
}

#endif // HAL_SITL_WASM_ENABLED