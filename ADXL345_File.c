/*!
  *  @file		ADXL345_File.c
  *  @author	Lucien
	*  @brief		Using an Accelerometer for Inclination Sensing (AN-1057)
	*						Notice : 10bits for +-2g range gives +-126LSB for +-1g
*/

#include "ADXL345_Header.h"
#include <string.h> // memset function comes from this library
#include "usart.h"
#include "math.h"

const double conv_rad_deg = 360.0/2.0/3.14159; // convert radian to degree

/**
 * @brief Feed an `ADXL345_sensor` struct with sensor features
 *
 * @param Pointer to sensor `ADXL345_sensor` struct
 */
void init_Sensor(ADXL345_sensor *sensor) {
	// store members set up by user, if so!
	ADXL345_read pread;
	ADXL345_write pwrite;
	ADXL345_delay pdelay;
	// store addresses
	pread = sensor->read_device;
	pwrite = sensor->write_device;
	pdelay = sensor->ms_delay;
  /* Now clear the ADXL345_sensor object */
  memset(sensor, 0, sizeof(ADXL345_sensor));
  /* Insert the sensor name in the fixed length char array */
  strncpy(sensor->sensor_name, DEV_NAME, sizeof(sensor->sensor_name) - 1);
  sensor->sensor_name[sizeof(sensor->sensor_name) - 1] = 0;
	// read stored addresses
	sensor->read_device = pread;
	sensor->write_device = pwrite;
	sensor->ms_delay = pdelay	;
	// get ID device
	sensor->read_device(ADXL345_ADDRESS, ADXL345_REG_DEVID, &sensor->dev_id,1);
	sensor->full_res = DEFAULT; // default not full resolution
  sensor->data_rate = ADXL345_DATARATE_100_HZ; // default value
}
/**************************************************************************/
/*!
  * @brief  set up sensor and make it ready to send data
  * @param  my_sensor:  my_range:
  * @return true: success false: a sensor with the correct ID was not found
*/
/**************************************************************************/
ADXL345_StatusTypeDef ADXL345_begin(ADXL345_sensor *my_sensor, range_t my_range) {
	const char MesureMode   = 0x08; // mesure may begin
	init_Sensor(my_sensor); // Clear the ADXL345_sensor object and set it up
	/* Check device ID */
  if (my_sensor->dev_id != 0xE5) {
    /* No ADXL345 detected ... return false */
    return HAL_ERROR;
  }	
// Select range +/-xG writting my_range in register DATA_FORMAT.
	my_sensor->write_device(ADXL345_ADDRESS, ADXL345_REG_DATA_FORMAT, my_range);
// Mettre le ADXL345 en mode de mesure en écrivant 0x08 dans le registre POWER_CTL.
	my_sensor->write_device(ADXL345_ADDRESS, ADXL345_REG_POWER_CTL, MesureMode);	

	return HAL_OK;	
}
/**
	* @brief set resolution
    @param sensor Pointer
					 resolution to set up	
 */
void set_resolution(ADXL345_sensor *sensor, resolution resol){
	uint8_t value; // value to write in bit D3 DATA_FORMAT register
	uint8_t data_format_value; // register value

		sensor->read_device(ADXL345_ADDRESS, ADXL345_REG_DATA_FORMAT, &data_format_value, 1);
		if (resol == FULL){
			value = 0x08; // D3 = 1 in DATA_FORMAT register
			data_format_value |= value;
		}
		else{
			value = 0xF7; // D3 = 0 in DATA_FORMAT register
			data_format_value &= value;
		}
		sensor->write_device(ADXL345_ADDRESS, ADXL345_REG_DATA_FORMAT, data_format_value);
 		sensor->full_res = resol;
}
/**
	* @brief get rsolution
    @param sensor Pointer
    
 */
resolution get_resolution(ADXL345_sensor *sensor){
	
	return sensor->full_res;
}
/**
	* @brief tilt on one axis
  * @param sensor Pointer, offset value, axis to set up
  *				 Offset value is the average value of a mesurement sample	  
 */
float inclination_calculation_1axis(ADXL345_sensor *sensor,three_axis axis){
	// here we need to take care of the resolution : TO DO (cf. 256.0)
	if (axis == X_AXIS)
		return asin(sensor->x_value/256.0)*conv_rad_deg;
	if (axis == Y_AXIS)
		return acos(sensor->y_value/256.0)*conv_rad_deg;
	if (axis == Z_AXIS)
		return acos(sensor->z_value/256.0)*conv_rad_deg;

	return 0.0;
};
/**
	* @brief set offset according to actual resolution
  * @param sensor Pointer, offset value, axis to set up
  *				 Offset value is the average value of a mesurement sample	  
 */
void setOffset(ADXL345_sensor sensor, int8_t average_value, three_axis axis){

	average_value /= -4;
	switch (axis){
	case X_AXIS :
		sensor.write_device(ADXL345_ADDRESS, ADXL345_REG_OFSX, (uint8_t)average_value);
	break;
	case Y_AXIS :
		sensor.write_device(ADXL345_ADDRESS, ADXL345_REG_OFSY, (uint8_t)average_value);
	break;
	case Z_AXIS :
		sensor.write_device(ADXL345_ADDRESS, ADXL345_REG_OFSZ, (uint8_t)average_value);
	break;
}
	}
/**************************************************************************/
/*!
    @brief Sets the g range for the accelerometer
    @param range The new `range_t` to set the accelerometer to
*/
/**************************************************************************/
void set_range(ADXL345_sensor *sensor, range_t range) {
  /* Read the data format register to preserve bits */
  uint8_t format;
  sensor->read_device(ADXL345_ADDRESS, ADXL345_REG_DATA_FORMAT, &format, 1);
  /* Update the data rate */
  format &= 0xFC;  // D0, D1 are cleared
  format |= range; // DO, D1 are set up
  /* Write the register back to the IC */
  sensor->write_device(ADXL345_ADDRESS, ADXL345_REG_DATA_FORMAT, format);
	sensor->dev_range = range;
}

/**************************************************************************/
/*!
    @brief  Gets the g range registered in the accelerometer structure
						Assume a setrange has already been executed
    @return The current `range_t` value
*/
/**************************************************************************/
range_t get_range(ADXL345_sensor *ps) {

	return ps->dev_range;
}

/**************************************************************************/
/*!
    @brief  Sets the data rate for the ADXL345 (controls power consumption)
    @param dataRate The `dataRate_t` to set
*/
/**************************************************************************/
void set_data_rate(dataRate_t dataRate) {
  /* Note: The LOW_POWER bits are currently ignored and we always keep
     the device in 'normal' mode */
//  writeRegister(ADXL345_REG_BW_RATE, dataRate);
}

/**************************************************************************/
/*!
    @brief  Gets the data rate for the ADXL345 (controls power consumption)
    @return The current data rate
*/
/**************************************************************************/
dataRate_t get_data_rate(void) {
//  return (dataRate_t)(readRegister(ADXL345_REG_BW_RATE) & 0x0F);
	return ADXL345_DATARATE_3200_HZ; // A RETIRER	
}
/**************************************************************************/
/*!
    @brief  Gets the data rate for the ADXL345 (controls power consumption)
    @return The current data rate
*/
/**************************************************************************/
void init_self_test(ADXL345_sensor *ps, selftest value){
  uint8_t reg_value;

  ps->read_device(ADXL345_ADDRESS, ADXL345_REG_DATA_FORMAT, &reg_value, 1);
  if (value == ON){
    reg_value |= 0x80; // D7 = 1
    ps->write_device(ADXL345_ADDRESS, ADXL345_REG_DATA_FORMAT, reg_value);
  }
  else
    reg_value &= 0x7F; // D7 = 0
    ps->write_device(ADXL345_ADDRESS, ADXL345_REG_DATA_FORMAT, reg_value);
}
/**
 * @brief Get X, Y, Z
 * @param Pointer of type ADXL345_sensor to the device
 */
void  accel_lectureXYZ (ADXL345_sensor *sensor){
	uint8_t data[6];
  
	sensor->read_device(ADXL345_ADDRESS, ADXL345_REG_DATAX0, data,6);
	// Résolution is 10 bits, i.e. 2 bytes. LSB first
  // X, Y, Z are cast in 4 bytes with sign
	if (data[1] & 0x02) // check if <0 number
		sensor->x_value = (((data[1]) << 8) | data[2]);
  else
		sensor->x_value = (((int)data[1]) << 8) | data[0];
	if (data[3] & 0x02)
		sensor->y_value = (((data[3]) << 8) | data[2]);		
  else
		sensor->y_value = (((int)data[3]) << 8) | data[2];
	if (data[5] & 0x02)
		sensor->z_value = (((data[5]) << 8) | data[4]);		
  else
		sensor->z_value = (((int)data[5]) << 8) | data[4];
}
/**
 * @brief Calculation in a sphere of X angle with with the horizon as a reference
 *				Determine the angle individually for each axis of the accelerometer from a reference
 *				position. The effective incremental sensitivity is constant and the angles can be accurately
 *				measured for all points around the unit sphere.
 * @param sensor
 */
void  accel_pitch (ADXL345_sensor *sensor){
	float denominateurX;
	denominateurX = pow(sensor->y_value/126.0,2) + pow(sensor->z_value/126.0,2);
	sensor->teta_x = atan(sensor->x_value/126.0/sqrt(denominateurX))*conv_rad_deg;
}
/**
 * @brief Calculation in a sphere of X angle with the horizon as a reference
 *				Determine the angle individually for each axis of the accelerometer from a reference
 *				position. The effective incremental sensitivity is constant and the angles can be accurately
 *				measured for all points around the unit sphere.
 * @param sensor
 */
void  accel_roll (ADXL345_sensor *sensor){
	float denominateurY;
	denominateurY = pow(sensor->x_value/126.0,2) + pow(sensor->z_value/126.0,2);
	sensor->psi_y = atan(sensor->y_value/126.0/sqrt(denominateurY))*conv_rad_deg;
}
/**
 * @brief Calculation in a sphere of Z angle with the horizon as a reference
 *				Determine the angle individually for each axis of the accelerometer from a reference
 *				position. The effective incremental sensitivity is constant and the angles can be accurately
 *				measured for all points around the unit sphere.
 * @param sensor
 */
void  accel_yaw (ADXL345_sensor *sensor){
	float numerateurZ;
	numerateurZ = pow(sensor->x_value/126.0,2) + pow(sensor->y_value/126.0,2);
	sensor->phi_z = atan((sqrt(numerateurZ)/(sensor->z_value/126.0)))*conv_rad_deg;
}
/**
 * @brief Write the register
 * @param sensor, value to write in register
 */
void  start_mesurement (ADXL345_sensor *sensor){
  /* Read the data from register */
  uint8_t reg_value;
  sensor->read_device(ADXL345_ADDRESS, ADXL345_REG_POWER_CTL, &reg_value, 1);
  /* Update the Measure Bit D3 */
  reg_value &= 0xF7;  // D3 is cleared
  reg_value |= 0x08;  // D3 is set
  /* Write the register back to the IC */
  sensor->write_device(ADXL345_ADDRESS, ADXL345_REG_POWER_CTL, reg_value);
}
