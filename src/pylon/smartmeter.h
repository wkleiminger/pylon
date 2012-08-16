/*******************************************************************************
* Copyright (c) 2012, Institute for Pervasive Computing, ETH Zurich.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* 3. Neither the name of the Institute nor the names of its contributors
* may be used to endorse or promote products derived from this software
* without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*
* This file is part of the Pylon Smart Metering framework.
*******************************************************************************/

/******************************************************************************\
  Project   : Pylon
  Module    : smartmeter
  Used by   : smlogger
  Purpose   : Provides operations to read data from Smart Meters via SML.
              Currently tested with the Landis+Gyr E750 only.
  
  Version   : 1.0
  Date      : 05.04.2012
  Author    : Daniel Pauli
\******************************************************************************/

#ifndef __SMARTMETER_H
#define __SMARTMETER_H

////////////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////////////

// Enum to identify measurement values
typedef enum {
	INVALID_VARIABLE = -1,
	TIMESTAMP = 0,
	POWER_ALL_PHASES,
	POWER_L1,
	POWER_L2,
	POWER_L3,
	CURRENT_NEUTRAL,
	CURRENT_L1,
	CURRENT_L2,
	CURRENT_L3,
	VOLTAGE_L1,
	VOLTAGE_L2,
	VOLTAGE_L3,
	PHASE_ANGLE_VOLTAGE_L2_L1,
	PHASE_ANGLE_VOLTAGE_L3_L1,
	PHASE_ANGLE_CURRENT_VOLTAGE_L1,
	PHASE_ANGLE_CURRENT_VOLTAGE_L2,
	PHASE_ANGLE_CURRENT_VOLTAGE_L3,
	NUM_VARIABLES
} SmartMeter_VarID;

// Structure to hold measurement data
typedef struct SmartMeter_Data_s {
	double val[NUM_VARIABLES];
} SmartMeter_Data;

// Callback used to notify about incoming data
typedef void(*smartmeter_cb)(const SmartMeter_Data* m);


////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

// Initializes the smartmeter module
//   address  : The IP/Hostname of the Smart Meter to connect or NULL
//              to detect it automatically
//   port     : The port number/service name of the Smart Meter to connect
//   interval : The time between two measurements in milliseconds
//   callback : Function to be called upon data received
int smartmeter_init(const char* address, const char* port, int interval, 
	smartmeter_cb callback);

// Starts the smartmeter thread in order to perform
// measurements at the specified time interval
int smartmeter_start(void);

// Stops the smartmeter thread
int smartmeter_stop(void);

// Waits for the smartmeter thread to terminate
int smartmeter_join(void);

// Receives a measurement from the Smart Meter
int smartmeter_measure(SmartMeter_Data* m);

// Returns the network address of the Smart Meter as a string
const char* smartmeter_address(void);

// Returns the name of the specified variable ID as a string
const char* smartmeter_getVarName(SmartMeter_VarID id);

#endif // __SMARTMETER_H

