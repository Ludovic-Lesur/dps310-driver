/*
 * dps310.h
 *
 *  Created on: 30 aug. 2024
 *      Author: Ludo
 */

#ifndef __DPS310_H__
#define __DPS310_H__

#ifndef DPS310_DRIVER_DISABLE_FLAGS_FILE
#include "dps310_driver_flags.h"
#endif
#include "math.h"
#include "types.h"

/*** DPS310 structures ***/

/*!******************************************************************
 * \enum DPS310_status_t
 * \brief DPS310 driver error codes.
 *******************************************************************/
typedef enum {
	// Driver errors.
	DPS310_SUCCESS = 0,
	DPS310_ERROR_NULL_PARAMETER,
	DPS310_ERROR_COEFFICIENTS_TIMEOUT,
	DPS310_ERROR_SENSOR_TIMEOUT,
	DPS310_ERROR_TEMPERATURE_TIMEOUT,
	DPS310_ERROR_PRESSURE_TIMEOUT,
	// Low level drivers errors.
	DPS310_ERROR_BASE_I2C = 0x0100,
	DPS310_ERROR_BASE_DELAY = (DPS310_ERROR_BASE_I2C + DPS310_DRIVER_I2C_ERROR_BASE_LAST),
	DPS310_ERROR_BASE_MATH = (DPS310_ERROR_BASE_DELAY + DPS310_DRIVER_DELAY_ERROR_BASE_LAST),
	// Last base value.
	DPS310_ERROR_BASE_LAST = (DPS310_ERROR_BASE_MATH + MATH_ERROR_BASE_LAST)
} DPS310_status_t;

/*** DPS310 functions ***/

/*!******************************************************************
 * \fn DPS310_status_t DPS310_init(void)
 * \brief Init DPS310 driver.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
DPS310_status_t DPS310_init(void);

/*!******************************************************************
 * \fn DPS310_status_t DPS310_de_init(void)
 * \brief Release DPS310 driver.
 * \param[in]  	none
 * \param[out] 	none
 * \retval		Function execution status.
 *******************************************************************/
DPS310_status_t DPS310_de_init(void);

/*!******************************************************************
 * \fn DPS310_status_t DPS310_get_pressure_temperature(uint8_t i2c_address, int32_t* pressure_pa, int32_t* temperature_degrees)
 * \brief Perform pressure and temperature measurements.
 * \param[in]  	i2c_address: I2C address of the sensor.
 * \param[out] 	pressure_pa: Pointer to integer that will contain the atmospheric pressure in Pa.
 * \param[out]	temperature_degrees: Pointer to integer that will contain the temperature in Celsius degrees.
 * \retval		Function execution status.
 *******************************************************************/
DPS310_status_t DPS310_get_pressure_temperature(uint8_t i2c_address, int32_t* pressure_pa, int32_t* temperature_degrees);

/*******************************************************************/
#define DPS310_exit_error(base) { ERROR_check_exit(dps310_status, DPS310_SUCCESS, base) }

/*******************************************************************/
#define DPS310_stack_error(base) { ERROR_check_stack(dps310_status, DPS310_SUCCESS, base) }

/*******************************************************************/
#define DPS310_stack_exit_error(base, code) { ERROR_check_stack_exit(dps310_status, DPS310_SUCCESS, base, code) }

#endif /* __DPS310_H__ */
