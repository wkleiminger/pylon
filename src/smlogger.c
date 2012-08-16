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
  Module    : smlogger
  Used by   : 
  Purpose   : Program to log data from the Smart Meter or Flukso sensor board
  
  Version   : 1.0
  Date      : 11.04.2012
  Author    : Daniel Pauli
\******************************************************************************/

#include <stdio.h>
#include <pthread.h>

#include <curl/curl.h>

#include "pylon/io.h"
#include "pylon/smartmeter.h"
#include "pylon/fluksometer.h"
#include "pylon/strbuilder.h"
#include "pylon/uploader.h"
#include "pylon/args.h"
#include "pylon/common.h"

////////////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////////////

// Timeout in milliseconds for POST requests
#define SEND_TIMEOUT 10000

////////////////////////////////////////////////////////////////////////////////
// STATIC VARIABLES
////////////////////////////////////////////////////////////////////////////////

// Supported program arguments
static Argument args[] = {
	{"count",    "-c", "-1",   ARG_INT    | OPTIONAL, "Number of measurements, -1 for infinite"},
	{"interval", "-i", "1000", ARG_INT    | OPTIONAL, "Interval between two measurements in milliseconds"},
	{"onboard",  "-o", NULL,   ARG_FLAG   | OPTIONAL, "Use Flukso onboard sensors instead of Smart Meter"},
	{"address",  "-a", NULL,   ARG_STRING | OPTIONAL, "Hostname/IP of the Smart Meter or path of the sensor FIFO"},
	{"port",     "-p", "7259", ARG_STRING | OPTIONAL, "Port of the Smart Meter"},
	{"url",      "-u", NULL,   ARG_STRING | OPTIONAL, "URL of the energy server to receive the measurements"},
	{"token",    "-t", NULL,   ARG_STRING | OPTIONAL, "Token to identify the measurements"},
	{"upload_threads", "-n", "1",     ARG_INT    | OPTIONAL, "Number of threads used to upload measurements"},
	{"buffer_size",    "-b", "36000", ARG_INT    | OPTIONAL, "Size of the upload queue to buffer measurements"},
	{"smart",    "-s", NULL,   ARG_FLAG   | OPTIONAL, "Output values only when differing from defaults"},
	{"help",     "-h", NULL,   ARG_FLAG   | OPTIONAL, "Display program usage and help"},
	{"verbose",  "-v", "1",    ARG_INT    | OPTIONAL, "Verbose level"},
	{"quiet",    "-q", NULL,   ARG_FLAG   | OPTIONAL, "Do not output measurements on stdout"},
	{0} // End of list
};

// String builder to encode measurements in JSON
static StringBuilder* m_sb;

// The URL of the energy server
static const char* m_url;

// The token to send with the measurements
static const char* m_token;

// Counter for meter readings
static int m_numMeasurements;

// Variables to hold program argumens
static int m_smart;
static int m_count;
static int m_interval;
static int m_quiet;
static int m_onboard;


////////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

// Callback function invoked by the smartmeter/fluksometer module
static void processMeasurement(const SmartMeter_Data* m);


////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
	// Input program arguments
	if (!args_parse(args, argc, argv) || args_value(args, "help")) {
		args_printUsage(args, argv[0]);
		args_printInfo(args);
		return 0;
	}
	
	// Set log level
	log_level = atoi(args_value(args, "verbose"));
	
	// Initialize static variables
	m_count      = atoi(args_value(args, "count"));
	m_interval   = atoi(args_value(args, "interval"));
	m_smart      = args_value(args, "smart") != NULL;
	m_quiet      = args_value(args, "quiet") != NULL;
	m_token      = args_value(args, "token");
	m_url        = args_value(args, "url");
	m_onboard    = args_value(args, "onboard") != NULL;
	
	// Initialize I/O subsystem
	io_init();

	// Initialize according module
	if (!m_onboard) {
	
		// Initialize smartmeter module
		int init = smartmeter_init(
			args_value(args, "address"), 
			args_value(args, "port"),
			m_interval,
			processMeasurement);
		if (!init) {
			printf("Failed to initialize Smart Meter\n");
			return 1;
		}

		// Use Smart Meter address if no token specified
		if (!m_token) {
			m_token = smartmeter_address();
		}
		
	} else {
	
		// Initialize onboard sensor module
		int init = fluksometer_init(args_value(args, "address"), processMeasurement);
		if (!init) {
			printf("Failed to initialize onboard sensors\n");
			return 1;
		}
	}

	// Initialize POST engine
	if (m_url) {
	
		// Initialize string builder
		m_sb = strbuilder_create();
		if (!m_sb) {
			printf("Failed to create string builder\n");
			return 1;
		}

		// Initialize uploader module
		if (!uploader_init(m_url, m_token,
			atoi(args_value(args, "buffer_size")),
			atoi(args_value(args, "upload_threads"))))
		{
			printf("Failed to initialize uploader module\n");
			return 1;
		}
	
	}

	// Print headers
	if (!m_quiet && !m_smart) {
		printf("#"); // comment for gnuplot
		for (SmartMeter_VarID id = 0; id < NUM_VARIABLES; id++) {
			printf("%s%c", smartmeter_getVarName(id), id < NUM_VARIABLES-1 ? '\t' : '\n');
		}
	}

	// Perform measurements
	if (m_count != 0) {
		if (!m_onboard) {
			smartmeter_start();
			smartmeter_join();
		} else {
			fluksometer_start();
			fluksometer_join();
		}
	}
	
	// Shutdown POST engine
	if (m_url) {
		uploader_cleanup();
		strbuilder_free(m_sb);
	}	
	
	// Shutdown I/O subsystem
	io_deinit();
	
	return 0;
}

void processMeasurement(const SmartMeter_Data* m)
{
	if (!m_quiet) {
	
		// Output measurement according to selected mode
		if (m_smart) {
		
			// Print only values that differ from default
			for (SmartMeter_VarID id = 0; id < NUM_VARIABLES; id++) {
				if (m->val[id] != 0 && m->val[id] != -1) {
					printf("%s: %f; ", smartmeter_getVarName(id), m->val[id]);
				}
			}
			printf("\n");
		} else {
			// Print all values
			for (SmartMeter_VarID id = 0; id < NUM_VARIABLES; id++) {
				printf("%f%c", m->val[id], id < NUM_VARIABLES-1 ? '\t' : '\n');
			}
		}
	}
	
	// Send measurement to energy server if URL specified
	if (m_url) {

		// Serialize measurement	
		strbuilder_reset(m_sb);
		strbuilder_printf(m_sb, "{\"measurement\":{");

		strbuilder_printf(m_sb, "\"powerAllPhases\": %.4f,	", m->val[POWER_ALL_PHASES]);
		strbuilder_printf(m_sb, "\"powerL1\": %.4f,", m->val[POWER_L1]);
		strbuilder_printf(m_sb, "\"powerL2\": %.4f,", m->val[POWER_L2]);
		strbuilder_printf(m_sb, "\"powerL3\": %.4f,", m->val[POWER_L3]);
		strbuilder_printf(m_sb, "\"currentNeutral\": %.4f,", m->val[CURRENT_NEUTRAL]);
		strbuilder_printf(m_sb, "\"currentL1\": %.4f,", m->val[CURRENT_L1]);
		strbuilder_printf(m_sb, "\"currentL2\": %.4f,", m->val[CURRENT_L2]);
		strbuilder_printf(m_sb, "\"currentL3\": %.4f,", m->val[CURRENT_L3]);
		strbuilder_printf(m_sb, "\"voltageL1\": %.4f,	", m->val[VOLTAGE_L1]);
		strbuilder_printf(m_sb, "\"voltageL2\": %.4f,", m->val[VOLTAGE_L2]);
		strbuilder_printf(m_sb, "\"voltageL3\": %.4f,", m->val[VOLTAGE_L3]);
		strbuilder_printf(m_sb, "\"phaseAngleVoltageL2L1\": %.4f,", m->val[PHASE_ANGLE_VOLTAGE_L2_L1]);
		strbuilder_printf(m_sb, "\"phaseAngleVoltageL3L1\": %.4f,", m->val[PHASE_ANGLE_VOLTAGE_L3_L1]);
		strbuilder_printf(m_sb, "\"phaseAngleCurrentVoltageL1\": %.4f,", m->val[PHASE_ANGLE_CURRENT_VOLTAGE_L1]);
		strbuilder_printf(m_sb, "\"phaseAngleCurrentVoltageL2\": %.4f,", m->val[PHASE_ANGLE_CURRENT_VOLTAGE_L2]);
		strbuilder_printf(m_sb, "\"phaseAngleCurrentVoltageL3\": %.4f,	", m->val[PHASE_ANGLE_CURRENT_VOLTAGE_L3]);
		strbuilder_printf(m_sb, "\"createdOn\": %llu,	", (uint64_t)m->val[TIMESTAMP]);
		strbuilder_printf(m_sb, "\"smartMeterId\": 1,");
		strbuilder_printf(m_sb, "\"smartMeterToken\": \"%s\"	", m_token);
	
		strbuilder_printf(m_sb, "}}");	
		
		// Put measurement in upload queue
		char* str = strbuilder_copy(m_sb);
		if (!uploader_send(str)) {
			LOG(1, "Unable to upload data\n");
			free(str);
		}
	}

	// Success	
	++m_numMeasurements;
	
	// Log info about buffer size every 60 measurements
	if (m_numMeasurements % 60 == 0) {
		LOG(2, "numMeasurements: %d, buffered: %d\n", m_numMeasurements, uploader_queueSize());
	}

	// Check if done	
	if (m_count > 0 && m_numMeasurements >= m_count) {
		if (!m_onboard) {
			smartmeter_stop();
		} else {
			fluksometer_stop();
		}
	}
}

