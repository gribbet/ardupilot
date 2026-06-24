#pragma once

#include <AP_HAL/AP_HAL.h>

#if CONFIG_HAL_BOARD == HAL_BOARD_SITL && AP_HAL_WASM

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

#include "AP_HAL_WASM_Namespace.h"

/*
 * Ring-buffer UART driver for serial0 / MAVLink.
 *
 * Two independent ring buffers:
 *   TX  (_tx_buf)  autopilot writes here → JS reads via ardupilot_serial0_read()
 *   RX  (_rx_buf)  JS writes here via ardupilot_serial0_write() → autopilot reads
 *
 * Both buffers are protected by a pthread_mutex_t so they are safe to access
 * concurrently from the browser main thread (JS exports) and the ArduPlane
 * worker thread (autopilot loop running under PROXY_TO_PTHREAD).
 */
class HALWASM::UARTDriver : public AP_HAL::UARTDriver
{
public:
    UARTDriver();
    ~UARTDriver();

    bool is_initialized() override { return _initialized; }
    bool tx_pending() override { return false; }
    uint32_t txspace() override;

    // ── JS-callable statics (thin wrappers around the global instance) ────────

    // Push MAVLink bytes from JS into the autopilot (fills the RX buffer)
    static size_t js_write(const uint8_t *buf, size_t len);

    // Pull MAVLink bytes produced by the autopilot out to JS (drains TX buffer)
    static size_t js_read(uint8_t *buf, size_t max_len);

    // Number of bytes available to js_read()
    static size_t js_read_available();

protected:
    void _begin(uint32_t baud, uint16_t rxSpace, uint16_t txSpace) override;
    size_t _write(const uint8_t *buffer, size_t size) override;
    ssize_t _read(uint8_t *buffer, uint16_t count) override;
    void _end() override;
    void _flush() override;
    uint32_t _available() override;
    bool _discard_input() override;

private:
    static constexpr size_t BUF_SIZE = 8192;

    // TX: autopilot → JS
    uint8_t _tx_buf[BUF_SIZE];
    volatile size_t _tx_head;
    volatile size_t _tx_tail;
    pthread_mutex_t _tx_mtx;

    // RX: JS → autopilot
    uint8_t _rx_buf[BUF_SIZE];
    volatile size_t _rx_head;
    volatile size_t _rx_tail;
    pthread_mutex_t _rx_mtx;

    bool _initialized;

    // ── internal ring-buffer helpers (do NOT lock – caller must hold mutex) ───
    size_t _rbuf_available(volatile size_t head, volatile size_t tail) const;
    size_t _rbuf_space(volatile size_t head, volatile size_t tail) const;
    size_t _rbuf_write(uint8_t *buf, volatile size_t head, volatile size_t &tail,
                       const uint8_t *data, size_t len);
    size_t _rbuf_read(uint8_t *buf, volatile size_t &head, volatile size_t tail,
                      uint8_t *data, size_t max_len);
};

#endif // CONFIG_HAL_BOARD == HAL_BOARD_SITL && AP_HAL_WASM
