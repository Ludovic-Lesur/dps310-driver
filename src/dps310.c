/*
 * dps310.c
 *
 *  Created on: 30 aug. 2024
 *      Author: Ludo
 */

#include "dps310.h"

#ifndef DPS310_DRIVER_DISABLE_FLAGS_FILE
#include "dps310_driver_flags.h"
#endif
#include "dps310_hw.h"
#include "error.h"
#include "math.h"
#include "types.h"

#ifndef DPS310_DRIVER_DISABLE

/*** DPS310 local macros ***/

#define DPS310_SUB_DELAY_MS                 10
#define DPS310_TIMEOUT_MS                   2000

#define DPS310_WRITE_COMMAND_BUFFER_SIZE    2

#define DPS310_SAMPLING_FACTOR_KT           524288
#define DPS310_SAMPLING_FACTOR_KP           1572864

#define DPS310_NUMBER_OF_COEF_REGISTERS     18

/*** DPS310 local structures ***/

/*******************************************************************/
typedef enum {
    DPS310_REGISTER_PRS_B2 = 0x00,
    DPS310_REGISTER_PRS_B1 = 0x01,
    DPS310_REGISTER_PRS_B0 = 0x02,
    DPS310_REGISTER_TMP_B2 = 0x03,
    DPS310_REGISTER_TMP_B1 = 0x04,
    DPS310_REGISTER_TMP_B0 = 0x05,
    DPS310_REGISTER_PRS_CFG = 0x06,
    DPS310_REGISTER_TMP_CFG = 0x07,
    DPS310_REGISTER_MEAS_CFG = 0x08,
    DPS310_REGISTER_CFG_REG = 0x09,
    DPS310_REGISTER_INT_STS = 0x0A,
    DPS310_REGISTER_FIFO_STS = 0x0B,
    DPS310_REGISTER_RESET = 0x0C,
    DPS310_REGISTER_PRODUCT_ID = 0x0D,
    DPS310_REGISTER_COEF_C0B = 0x10,
    DPS310_REGISTER_COEF_C0A_C1B = 0x11,
    DPS310_REGISTER_COEF_C1A = 0x12,
    DPS310_REGISTER_COEF_C00C = 0x13,
    DPS310_REGISTER_COEF_C00B = 0x14,
    DPS310_REGISTER_COEF_C00A_C10C = 0x15,
    DPS310_REGISTER_COEF_C10B = 0x16,
    DPS310_REGISTER_COEF_C10A = 0x17,
    DPS310_REGISTER_COEF_C01B = 0x18,
    DPS310_REGISTER_COEF_C01A = 0x19,
    DPS310_REGISTER_COEF_C11B = 0x1A,
    DPS310_REGISTER_COEF_C11A = 0x1B,
    DPS310_REGISTER_COEF_C20B = 0x1C,
    DPS310_REGISTER_COEF_C20A = 0x1D,
    DPS310_REGISTER_COEF_C21B = 0x1E,
    DPS310_REGISTER_COEF_C21A = 0x1F,
    DPS310_REGISTER_COEF_C30B = 0x20,
    DPS310_REGISTER_COEF_C30A = 0x21,
    DPS310_REGISTER_COEF_COEF_SRCE = 0x28,
    DPS310_REGISTER_LAST
} DPS310_register_t;

/*******************************************************************/
typedef struct {
    // Measurements.
    int32_t tmp_raw;
    int32_t prs_raw;
    // Calibration coefficients.
    uint8_t coef_ready_flag;
    int32_t coef_c0;
    int32_t coef_c1;
    int32_t coef_c00;
    int32_t coef_c10;
    int32_t coef_c01;
    int32_t coef_c11;
    int32_t coef_c20;
    int32_t coef_c21;
    int32_t coef_c30;
} DPS310_context_t;

/*** DPS310 local global variables ***/

static DPS310_context_t dps310_ctx = { .coef_ready_flag = 0 };

/*** DPS310 local functions ***/

/*******************************************************************/
static DPS310_status_t _DPS310_write_register(uint8_t i2c_address, DPS310_register_t register_address, uint8_t value) {
    // Local variables.
    DPS310_status_t status = DPS310_SUCCESS;
    uint8_t register_write_command[DPS310_WRITE_COMMAND_BUFFER_SIZE] = { register_address, value };
    // I2C transfer.
    status = DPS310_HW_i2c_write(i2c_address, register_write_command, DPS310_WRITE_COMMAND_BUFFER_SIZE, 1);
    if (status != DPS310_SUCCESS) goto errors;
errors:
    return status;
}

/*******************************************************************/
static DPS310_status_t _DPS310_read_register(uint8_t i2c_address, DPS310_register_t register_address, uint8_t* value) {
    // Local variables.
    DPS310_status_t status = DPS310_SUCCESS;
    DPS310_register_t local_addr = register_address;
    // I2C transfer.
    status = DPS310_HW_i2c_write(i2c_address, &local_addr, 1, 1);
    if (status != DPS310_SUCCESS) goto errors;
    status = DPS310_HW_i2c_read(i2c_address, value, 1);
    if (status != DPS310_SUCCESS) goto errors;
errors:
    return status;
}

/*******************************************************************/
static DPS310_status_t _DPS310_wait_flag(uint8_t i2c_address, DPS310_register_t register_address, uint8_t bit_index, DPS310_status_t timeout_error) {
    // Local variables.
    DPS310_status_t status = DPS310_SUCCESS;
    uint8_t reg_value = 0;
    uint32_t loop_count_ms = 0;
    // Read register.
    status = _DPS310_read_register(i2c_address, register_address, &reg_value);
    if (status != DPS310_SUCCESS) goto errors;
    // Wait for flag to be set.
    while ((reg_value & (0b1 << bit_index)) == 0) {
        // Low power delay.
        status = DPS310_HW_delay_milliseconds(DPS310_SUB_DELAY_MS);
        if (status != DPS310_SUCCESS) goto errors;
        // Exit if timeout.
        loop_count_ms += DPS310_SUB_DELAY_MS;
        if (loop_count_ms > DPS310_TIMEOUT_MS) {
            status = timeout_error;
            goto errors;
        }
        // Read register.
        status = _DPS310_read_register(i2c_address, register_address, &reg_value);
        if (status != DPS310_SUCCESS) goto errors;
    }
errors:
    return status;
}

/*******************************************************************/
static DPS310_status_t _DPS310_read_calibration_coefficients(uint8_t i2c_address) {
    // Local variables.
    DPS310_status_t status = DPS310_SUCCESS;
    MATH_status_t math_status = MATH_SUCCESS;
    uint8_t local_addr = DPS310_REGISTER_COEF_C0B;
    uint8_t idx = 0;
    uint8_t coef_registers[DPS310_NUMBER_OF_COEF_REGISTERS];
    uint32_t c0 = 0;
    uint32_t c1 = 0;
    uint32_t c00 = 0;
    uint32_t c10 = 0;
    uint32_t c01 = 0;
    uint32_t c11 = 0;
    uint32_t c20 = 0;
    uint32_t c21 = 0;
    uint32_t c30 = 0;
    // Reset all coefficients.
    for (idx = 0; idx < DPS310_NUMBER_OF_COEF_REGISTERS; idx++)
        coef_registers[idx] = 0;
    // Wait for coefficients to be ready for reading.
    status = _DPS310_wait_flag(i2c_address, DPS310_REGISTER_MEAS_CFG, 7, DPS310_ERROR_COEFFICIENTS_TIMEOUT);
    if (status != DPS310_SUCCESS) goto errors;
    // Read all coefficients with auto-increment method.
    status = DPS310_HW_i2c_write(i2c_address, &local_addr, 1, 1);
    if (status != DPS310_SUCCESS) goto errors;
    status = DPS310_HW_i2c_read(i2c_address, coef_registers, DPS310_NUMBER_OF_COEF_REGISTERS);
    if (status != DPS310_SUCCESS) goto errors;
    // Compute coefficients.
    c0 |= (coef_registers[0] << 4) | ((coef_registers[1] & 0xF0) >> 4);
    c1 |= ((coef_registers[1] & 0x0F) << 8) | (coef_registers[2]);
    c00 |= (coef_registers[3] << 12) | (coef_registers[4] << 4) | ((coef_registers[5] & 0xF0) >> 4);
    c10 |= ((coef_registers[5] & 0x0F) << 16) | (coef_registers[6] << 8) | (coef_registers[7]);
    c01 |= (coef_registers[8] << 8) | (coef_registers[9]);
    c11 |= (coef_registers[10] << 8) | (coef_registers[11]);
    c20 |= (coef_registers[12] << 8) | (coef_registers[13]);
    c21 |= (coef_registers[14] << 8) | (coef_registers[15]);
    c30 |= (coef_registers[16] << 8) | (coef_registers[17]);
    // Convert to sign values.
    math_status = MATH_two_complement_to_integer(c0, 11, &dps310_ctx.coef_c0);
    MATH_exit_error(DPS310_ERROR_BASE_MATH);
    math_status = MATH_two_complement_to_integer(c1, 11, &dps310_ctx.coef_c1);
    MATH_exit_error(DPS310_ERROR_BASE_MATH);
    math_status = MATH_two_complement_to_integer(c00, 19, &dps310_ctx.coef_c00);
    MATH_exit_error(DPS310_ERROR_BASE_MATH);
    math_status = MATH_two_complement_to_integer(c01, 15, &dps310_ctx.coef_c01);
    MATH_exit_error(DPS310_ERROR_BASE_MATH);
    math_status = MATH_two_complement_to_integer(c10, 19, &dps310_ctx.coef_c10);
    MATH_exit_error(DPS310_ERROR_BASE_MATH);
    math_status = MATH_two_complement_to_integer(c11, 15, &dps310_ctx.coef_c11);
    MATH_exit_error(DPS310_ERROR_BASE_MATH);
    math_status = MATH_two_complement_to_integer(c20, 15, &dps310_ctx.coef_c20);
    MATH_exit_error(DPS310_ERROR_BASE_MATH);
    math_status = MATH_two_complement_to_integer(c21, 15, &dps310_ctx.coef_c21);
    MATH_exit_error(DPS310_ERROR_BASE_MATH);
    math_status = MATH_two_complement_to_integer(c30, 15, &dps310_ctx.coef_c30);
    MATH_exit_error(DPS310_ERROR_BASE_MATH);
    // Set flag.
    dps310_ctx.coef_ready_flag = 1;
errors:
    return status;
}

/*******************************************************************/
static DPS310_status_t _DPS310_compute_raw_temperature(uint8_t i2c_address) {
    // Local variables.
    DPS310_status_t status = DPS310_SUCCESS;
    MATH_status_t math_status = MATH_SUCCESS;
    uint8_t read_byte = 0;
    uint32_t tmp_raw = 0;
    // Wait for sensor to be ready.
    status = _DPS310_wait_flag(i2c_address, DPS310_REGISTER_MEAS_CFG, 6, DPS310_ERROR_SENSOR_TIMEOUT);
    if (status != DPS310_SUCCESS) goto errors;
    // Trigger temperature measurement (external sensor, rate=1meas/s, no oversampling).
    status = _DPS310_write_register(i2c_address, DPS310_REGISTER_TMP_CFG, 0x80);
    if (status != DPS310_SUCCESS) goto errors;
    status = _DPS310_write_register(i2c_address, DPS310_REGISTER_MEAS_CFG, 0x02);
    if (status != DPS310_SUCCESS) goto errors;
    // Wait for temperature to be ready.
    status = _DPS310_wait_flag(i2c_address, DPS310_REGISTER_MEAS_CFG, 5, DPS310_ERROR_TEMPERATURE_TIMEOUT);
    if (status != DPS310_SUCCESS) goto errors;
    // Read temperature.
    // B2.
    status = _DPS310_read_register(i2c_address, DPS310_REGISTER_TMP_B2, &read_byte);
    if (status != DPS310_SUCCESS) goto errors;
    tmp_raw |= (read_byte << 16);
    // B1.
    status = _DPS310_read_register(i2c_address, DPS310_REGISTER_TMP_B1, &read_byte);
    if (status != DPS310_SUCCESS) goto errors;
    tmp_raw |= (read_byte << 8);
    // B0.
    status = _DPS310_read_register(i2c_address, DPS310_REGISTER_TMP_B0, &read_byte);
    if (status != DPS310_SUCCESS) goto errors;
    tmp_raw |= read_byte;
    // Compute two complement.
    math_status = MATH_two_complement_to_integer(tmp_raw, 23, &dps310_ctx.tmp_raw);
    MATH_exit_error(DPS310_ERROR_BASE_MATH);
errors:
    return status;
}

/*******************************************************************/
static DPS310_status_t _DPS310_compute_raw_pressure(uint8_t i2c_address) {
    // Local variables.
    DPS310_status_t status = DPS310_SUCCESS;
    MATH_status_t math_status = MATH_SUCCESS;
    uint8_t read_byte = 0;
    uint32_t prs_raw = 0;
    // Wait for sensor to be ready.
    status = _DPS310_wait_flag(i2c_address, DPS310_REGISTER_MEAS_CFG, 6, DPS310_ERROR_SENSOR_TIMEOUT);
    if (status != DPS310_SUCCESS) goto errors;
    // Trigger pressure measurement (rate=1meas/s, no oversampling).
    status = _DPS310_write_register(i2c_address, DPS310_REGISTER_PRS_CFG, 0x01);
    if (status != DPS310_SUCCESS) goto errors;
    status = _DPS310_write_register(i2c_address, DPS310_REGISTER_MEAS_CFG, 0x01);
    if (status != DPS310_SUCCESS) goto errors;
    // Wait for pressure to be ready.
    status = _DPS310_wait_flag(i2c_address, DPS310_REGISTER_MEAS_CFG, 4, DPS310_ERROR_PRESSURE_TIMEOUT);
    if (status != DPS310_SUCCESS) goto errors;
    // Read pressure.
    // B2.
    status = _DPS310_read_register(i2c_address, DPS310_REGISTER_PRS_B2, &read_byte);
    if (status != DPS310_SUCCESS) goto errors;
    prs_raw |= (read_byte << 16);
    // B1.
    status = _DPS310_read_register(i2c_address, DPS310_REGISTER_PRS_B1, &read_byte);
    if (status != DPS310_SUCCESS) goto errors;
    prs_raw |= (read_byte << 8);
    // B0.
    status = _DPS310_read_register(i2c_address, DPS310_REGISTER_PRS_B0, &read_byte);
    if (status != DPS310_SUCCESS) goto errors;
    prs_raw |= read_byte;
    // Compute two complement.
    math_status = MATH_two_complement_to_integer(prs_raw, 23, &dps310_ctx.prs_raw);
    MATH_exit_error(DPS310_ERROR_BASE_MATH);
errors:
    return status;
}

/*** DPS310 functions ***/

/*******************************************************************/
DPS310_status_t DPS310_init(void) {
    // Local variables.
    DPS310_status_t status = DPS310_SUCCESS;
    // Init hardware interface.
    status = DPS310_HW_init();
    if (status != DPS310_SUCCESS) goto errors;
errors:
    return status;
}

/*******************************************************************/
DPS310_status_t DPS310_de_init(void) {
    // Local variables.
    DPS310_status_t status = DPS310_SUCCESS;
    // Release hardware interface.
    status = DPS310_HW_de_init();
    if (status != DPS310_SUCCESS) goto errors;
errors:
    return status;
}

/*******************************************************************/
DPS310_status_t DPS310_get_pressure_temperature(uint8_t i2c_address, int32_t* pressure_pa, int32_t* temperature_degrees) {
    // Local variables.
    DPS310_status_t status = DPS310_SUCCESS;
    int64_t temp_s64 = 0;
    int64_t last_term = 0;
    // Check parameters.
    if ((pressure_pa == NULL) || (temperature_degrees == NULL)) {
        status = DPS310_ERROR_NULL_PARAMETER;
        goto errors;
    }
    // Reset result.
    dps310_ctx.tmp_raw = 0;
    dps310_ctx.prs_raw = 0;
    // Compute raw results.
    status = _DPS310_compute_raw_temperature(i2c_address);
    if (status != DPS310_SUCCESS) goto errors;
    status = _DPS310_compute_raw_pressure(i2c_address);
    if (status != DPS310_SUCCESS) goto errors;
    // Read calibration coefficients if needed.
    if (dps310_ctx.coef_ready_flag == 0) {
        status = _DPS310_read_calibration_coefficients(i2c_address);
        if (status != DPS310_SUCCESS) goto errors;
    }
    // Compute pressure in Pa.
    temp_s64 = dps310_ctx.coef_c20 + (dps310_ctx.prs_raw * dps310_ctx.coef_c30) / DPS310_SAMPLING_FACTOR_KP;
    temp_s64 = dps310_ctx.coef_c10 + (dps310_ctx.prs_raw * temp_s64) / DPS310_SAMPLING_FACTOR_KP;
    temp_s64 = dps310_ctx.coef_c00 + (dps310_ctx.prs_raw * temp_s64) / DPS310_SAMPLING_FACTOR_KP;
    temp_s64 += (dps310_ctx.tmp_raw * dps310_ctx.coef_c01) / DPS310_SAMPLING_FACTOR_KT;
    last_term = dps310_ctx.coef_c11 + (dps310_ctx.prs_raw * dps310_ctx.coef_c21) / DPS310_SAMPLING_FACTOR_KP;
    last_term = (dps310_ctx.prs_raw * last_term) / DPS310_SAMPLING_FACTOR_KP;
    last_term = (dps310_ctx.tmp_raw * last_term) / DPS310_SAMPLING_FACTOR_KT;
    temp_s64 += last_term;
    (*pressure_pa) = (int32_t) temp_s64;
    // Compute temperature in degrees.
    temp_s64 = (dps310_ctx.coef_c0 >> 1) + (dps310_ctx.coef_c1 * dps310_ctx.tmp_raw) / DPS310_SAMPLING_FACTOR_KT;
    (*temperature_degrees) = (int32_t) temp_s64;
errors:
    return status;
}

#endif /* DPS310_DRIVER_DISABLE */
