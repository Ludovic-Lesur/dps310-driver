/*
 * dps310_hw.c
 *
 *  Created on: 30 aug. 2024
 *      Author: Ludo
 */

#include "dps310_hw.h"

#ifndef DPS310_DRIVER_DISABLE_FLAGS_FILE
#include "dps310_driver_flags.h"
#endif
#include "dps310.h"
#include "types.h"

#ifndef DPS310_DRIVER_DISABLE

/*** DPS310 HW functions ***/

/*******************************************************************/
DPS310_status_t __attribute__((weak)) DPS310_HW_init(void) {
    // Local variables.
    DPS310_status_t status = DPS310_SUCCESS;
    /* To be implemented */
    return status;
}

/*******************************************************************/
DPS310_status_t __attribute__((weak)) DPS310_HW_de_init(void) {
    // Local variables.
    DPS310_status_t status = DPS310_SUCCESS;
    /* To be implemented */
    return status;
}

/*******************************************************************/
DPS310_status_t __attribute__((weak)) DPS310_HW_i2c_write(uint8_t i2c_address, uint8_t* data, uint8_t data_size_bytes, uint8_t stop_flag) {
    // Local variables.
    DPS310_status_t status = DPS310_SUCCESS;
    /* To be implemented */
    UNUSED(i2c_address);
    UNUSED(data);
    UNUSED(data_size_bytes);
    UNUSED(stop_flag);
    return status;
}

/*******************************************************************/
DPS310_status_t __attribute__((weak)) DPS310_HW_i2c_read(uint8_t i2c_address, uint8_t* data, uint8_t data_size_bytes) {
    // Local variables.
    DPS310_status_t status = DPS310_SUCCESS;
    /* To be implemented */
    UNUSED(i2c_address);
    UNUSED(data);
    UNUSED(data_size_bytes);
    return status;
}

/*******************************************************************/
DPS310_status_t __attribute__((weak)) DPS310_HW_delay_milliseconds(uint32_t delay_ms) {
    // Local variables.
    DPS310_status_t status = DPS310_SUCCESS;
    /* To be implemented */
    UNUSED(delay_ms);
    return status;
}

#endif /* DPS310_DRIVER_DISABLE */
