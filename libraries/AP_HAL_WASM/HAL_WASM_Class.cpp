#include <AP_HAL/AP_HAL.h>

#if CONFIG_HAL_BOARD == HAL_BOARD_SITL && AP_HAL_WASM

#include "HAL_WASM_Class.h"
#include "UARTDriver.h"

using namespace HALWASM;

// Global ring-buffer serial0 driver (MAVLink channel).
static HALWASM::UARTDriver wasm_serial0;

HAL_WASM::HAL_WASM()
    : HAL_SITL()
{
    // Replace the TCP-backed SITL serial0 with the WASM ring-buffer driver.
    // serial_array is protected in AP_HAL::HAL so subclasses can override it.
    serial_array[0] = &wasm_serial0;
    // console mirrors serial0 so AP_HAL::panic() / printf go to the same pipe
    console = &wasm_serial0;
}

// ── HAL singleton ─────────────────────────────────────────────────────────────

static HAL_WASM hal_wasm_inst;

const AP_HAL::HAL &AP_HAL::get_HAL()
{
    return hal_wasm_inst;
}

AP_HAL::HAL &AP_HAL::get_HAL_mutable()
{
    return hal_wasm_inst;
}

#endif // CONFIG_HAL_BOARD == HAL_BOARD_SITL && AP_HAL_WASM
