#include <AP_HAL/AP_HAL.h>

#if CONFIG_HAL_BOARD == HAL_BOARD_SITL && AP_HAL_WASM

#include "UARTDriver.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

using namespace HALWASM;

// Module-level pointer used by the exported C functions below.
static UARTDriver *_g_uart0_instance;

// ── Constructor ───────────────────────────────────────────────────────────────

UARTDriver::UARTDriver()
    : _tx_head(0), _tx_tail(0),
      _rx_head(0), _rx_tail(0),
      _initialized(false)
{
    pthread_mutex_init(&_tx_mtx, nullptr);
    pthread_mutex_init(&_rx_mtx, nullptr);
    _g_uart0_instance = this;
}

UARTDriver::~UARTDriver()
{
    pthread_mutex_destroy(&_tx_mtx);
    pthread_mutex_destroy(&_rx_mtx);
}

// ── Ring-buffer helpers (caller must hold the relevant mutex) ─────────────────

size_t UARTDriver::_rbuf_available(volatile size_t head, volatile size_t tail) const
{
    if (tail >= head) {
        return tail - head;
    }
    return BUF_SIZE - head + tail;
}

size_t UARTDriver::_rbuf_space(volatile size_t head, volatile size_t tail) const
{
    return BUF_SIZE - 1 - _rbuf_available(head, tail);
}

size_t UARTDriver::_rbuf_write(uint8_t *buf,
                               volatile size_t head,
                               volatile size_t &tail,
                               const uint8_t *data, size_t len)
{
    const size_t space = _rbuf_space(head, tail);
    if (len > space) {
        len = space;
    }
    for (size_t i = 0; i < len; i++) {
        buf[tail] = data[i];
        tail = (tail + 1) % BUF_SIZE;
    }
    return len;
}

size_t UARTDriver::_rbuf_read(uint8_t *buf,
                              volatile size_t &head,
                              volatile size_t tail,
                              uint8_t *data, size_t max_len)
{
    const size_t avail = _rbuf_available(head, tail);
    if (max_len > avail) {
        max_len = avail;
    }
    for (size_t i = 0; i < max_len; i++) {
        data[i] = buf[head];
        head = (head + 1) % BUF_SIZE;
    }
    return max_len;
}

// ── AP_HAL::UARTDriver backend interface ──────────────────────────────────────

void UARTDriver::_begin(uint32_t /*baud*/, uint16_t /*rxSpace*/, uint16_t /*txSpace*/)
{
    _initialized = true;
}

size_t UARTDriver::_write(const uint8_t *buffer, size_t size)
{
    pthread_mutex_lock(&_tx_mtx);
    const size_t n = _rbuf_write(_tx_buf, _tx_head, _tx_tail, buffer, size);
    pthread_mutex_unlock(&_tx_mtx);
    return n;
}

ssize_t UARTDriver::_read(uint8_t *buffer, uint16_t count)
{
    pthread_mutex_lock(&_rx_mtx);
    const size_t n = _rbuf_read(_rx_buf, _rx_head, _rx_tail, buffer, (size_t)count);
    pthread_mutex_unlock(&_rx_mtx);
    return (ssize_t)n;
}

void UARTDriver::_end() {}
void UARTDriver::_flush() {}

uint32_t UARTDriver::_available()
{
    pthread_mutex_lock(&_rx_mtx);
    const uint32_t n = (uint32_t)_rbuf_available(_rx_head, _rx_tail);
    pthread_mutex_unlock(&_rx_mtx);
    return n;
}

bool UARTDriver::_discard_input()
{
    pthread_mutex_lock(&_rx_mtx);
    _rx_head = _rx_tail = 0;
    pthread_mutex_unlock(&_rx_mtx);
    return true;
}

uint32_t UARTDriver::txspace()
{
    pthread_mutex_lock(&_tx_mtx);
    const uint32_t n = (uint32_t)_rbuf_space(_tx_head, _tx_tail);
    pthread_mutex_unlock(&_tx_mtx);
    return n;
}

// ── JS-facing static helpers ──────────────────────────────────────────────────

/*
 * Push MAVLink bytes from the JS host into the autopilot's RX buffer.
 * Call from JS: module.ccall('ardupilot_serial0_write', 'number',
 *                             ['number','number'], [ptr, len])
 */
size_t UARTDriver::js_write(const uint8_t *buf, size_t len)
{
    if (!_g_uart0_instance) {
        return 0;
    }
    UARTDriver *d = _g_uart0_instance;
    pthread_mutex_lock(&d->_rx_mtx);
    const size_t n = d->_rbuf_write(d->_rx_buf, d->_rx_head, d->_rx_tail, buf, len);
    pthread_mutex_unlock(&d->_rx_mtx);
    return n;
}

/*
 * Pull MAVLink bytes produced by the autopilot out to the JS host.
 * Call from JS: module.ccall('ardupilot_serial0_read', 'number',
 *                             ['number','number'], [ptr, maxLen])
 */
size_t UARTDriver::js_read(uint8_t *buf, size_t max_len)
{
    if (!_g_uart0_instance) {
        return 0;
    }
    UARTDriver *d = _g_uart0_instance;
    pthread_mutex_lock(&d->_tx_mtx);
    const size_t n = d->_rbuf_read(d->_tx_buf, d->_tx_head, d->_tx_tail, buf, max_len);
    pthread_mutex_unlock(&d->_tx_mtx);
    return n;
}

/*
 * Number of bytes currently available to js_read().
 */
size_t UARTDriver::js_read_available()
{
    if (!_g_uart0_instance) {
        return 0;
    }
    UARTDriver *d = _g_uart0_instance;
    pthread_mutex_lock(&d->_tx_mtx);
    const size_t n = d->_rbuf_available(d->_tx_head, d->_tx_tail);
    pthread_mutex_unlock(&d->_tx_mtx);
    return n;
}

// ── EMSCRIPTEN_KEEPALIVE C-linkage exports ────────────────────────────────────
//
// These are callable from the browser via module.ccall() / module.cwrap().
// The EMSCRIPTEN_KEEPALIVE attribute prevents the linker from dead-stripping
// them; they are also listed in -sEXPORTED_FUNCTIONS in the board config.

extern "C"
{

#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_KEEPALIVE
#endif
    size_t ardupilot_serial0_write(const uint8_t *buf, size_t len)
    {
        return HALWASM::UARTDriver::js_write(buf, len);
    }

#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_KEEPALIVE
#endif
    size_t ardupilot_serial0_read(uint8_t *buf, size_t max_len)
    {
        return HALWASM::UARTDriver::js_read(buf, max_len);
    }

#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_KEEPALIVE
#endif
    size_t ardupilot_serial0_read_available(void)
    {
        return HALWASM::UARTDriver::js_read_available();
    }

} // extern "C"

#endif // CONFIG_HAL_BOARD == HAL_BOARD_SITL && AP_HAL_WASM
