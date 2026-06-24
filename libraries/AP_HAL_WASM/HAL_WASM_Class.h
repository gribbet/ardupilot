#pragma once

#include <AP_HAL/AP_HAL.h>

#if CONFIG_HAL_BOARD == HAL_BOARD_SITL && AP_HAL_WASM

#include <AP_HAL_SITL/HAL_SITL_Class.h>
#include "AP_HAL_WASM_Namespace.h"

/*
 * HAL_WASM extends HAL_SITL, reusing all simulation physics and drivers.
 * The constructor replaces serial_array[0] (MAVLink/serial0) with the
 * ring-buffer driver so JS can exchange raw MAVLink bytes with the autopilot
 * via EMSCRIPTEN_KEEPALIVE exported functions.
 *
 * run() is inherited from HAL_SITL unchanged; with PROXY_TO_PTHREAD the
 * while(true) main loop runs in a Web Worker, not blocking the browser UI.
 */
class HALWASM::HAL_WASM : public HAL_SITL
{
public:
    HAL_WASM();
};

#endif // CONFIG_HAL_BOARD == HAL_BOARD_SITL && AP_HAL_WASM
