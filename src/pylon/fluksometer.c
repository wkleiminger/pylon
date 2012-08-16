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
  Module    : fluksometer
  Used by   : smlogger
  Purpose   : Provides operations to read data from the internal Flukso
              sensor board.
  
  Version   : 1.0
  Date      : 28.04.2012
  Author    : Daniel Pauli
\******************************************************************************/

#include "fluksometer.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "meter.h"
#include "timer.h"
#include "common.h"


////////////////////////////////////////////////////////////////////////////////
// STATIC VARIABLES
////////////////////////////////////////////////////////////////////////////////

// Stores context for the thread performing the measurements
static MeterHandle* m_handle;

// Holds the name of the FIFO special file to read from
static const char* m_fifo;

// Handle to the FIFO
static FILE* m_fd;

// Callback to notify the client module about measurements
static fluksometer_cb m_callback;


////////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

// Callback function invoked by the meter thread
static void performMeasurement(MeterHandle* handle);


////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////

int fluksometer_init(const char* fifo, fluksometer_cb callback)
{
	m_fifo = fifo ? fifo : FLUKSOMETER_DEFAULT_FIFO;
	m_callback = callback;
	
	return 1; // Success
}

int fluksometer_measure(SmartMeter_Data* m)
{
	// Open FIFO with sensor readings
	if (!m_fd) {
		m_fd = fopen(m_fifo, "r");
		if (!m_fd) {
			LOG(1, "Failed to open FIFO '%s': %s\n", m_fifo, strerror(errno));
			return 0;
		}
	}
	
	// Read line from stream
	char line[128];
	if (!fgets(line, sizeof(line), m_fd)) {

		LOG(1, "Failed to read from FIFO: %s\n", 
			errno ? strerror(errno) : "EOF reached");
			
		// Close FIFO to retry next time
		fclose(m_fd);
		m_fd = NULL;
		
		return 0;	
	}

	// Parse line (newline included)
	int phaseid[3];
	int counter[3];
	int cnt = sscanf(line, "%lf %d %d %lf %d %d %lf %d %d %lf", 
		&m->val[TIMESTAMP], 
		&phaseid[0], // == 0
		&counter[0],
		&m->val[POWER_L1],
		&phaseid[1], // == 1
		&counter[1],
		&m->val[POWER_L2],
		&phaseid[2], // == 2
		&counter[2],
		&m->val[POWER_L3]
	);
	
	if (cnt < 4) {
		LOG(1, "Failed to parse line: %s", line);
		return 0;
	}

	// Compute total power
	m->val[POWER_ALL_PHASES] = m->val[POWER_L1] + m->val[POWER_L2] + m->val[POWER_L3];
	
	return 1; // Success
}

void performMeasurement(MeterHandle* handle)
{
	// Read measurement
	SmartMeter_Data m = {{0}};
	if (!fluksometer_measure(&m)) {
		LOG(1, "Failed to perform measurement\n");
	
		// Wait a bit before retrying
		timer_sleep(1000);
		return;
	}
	
	// Invoke callback
	if (m_callback) {
		m_callback(&m);
	} else {
		LOG(1, "No callback specified\n");
	}
}

int fluksometer_start(void)
{
	m_handle = meter_start(-1, performMeasurement);
	return m_handle != NULL;
}

int fluksometer_stop(void)
{
	return meter_stop(m_handle);
}

int fluksometer_join(void)
{
	return meter_join(m_handle);
}

