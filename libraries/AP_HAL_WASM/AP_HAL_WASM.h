#pragma once

#include <AP_HAL/AP_HAL.h>

#if CONFIG_HAL_BOARD == HAL_BOARD_SITL && AP_HAL_WASM

#include "HAL_WASM_Class.h"

#endif // CONFIG_HAL_BOARD == HAL_BOARD_SITL && AP_HAL_WASM
