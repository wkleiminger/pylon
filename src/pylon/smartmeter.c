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

#include "smartmeter.h"
#include "meter.h"

#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include <sml/sml_transport.h>

#include "io.h"
#include "common.h"
#include "timer.h"

////////////////////////////////////////////////////////////////////////////////
// STATIC VARIABLES
///////////////////////////////////////////////////////////////////////////////

// Struct to hold OBIS identification strings
typedef union OBIS_s {
	unsigned char raw[6];
} OBIS;

// Struct to relate OBIS IDs with Var IDs
typedef struct OBIS_Entry_s {
	SmartMeter_VarID id;
	OBIS obis;
} OBIS_Entry;

////////////////////////////////////////////////////////////////////////////////
// STATIC VARIABLES
////////////////////////////////////////////////////////////////////////////////

// Stores context for the thread performing the measurements
static MeterHandle* m_handle;

// Socket for the TCP connection
static int m_socket;

// Address of the Smart Meter
static char m_host[32];

// Port to connect to
static const char* m_port;

// Flag to indicate if module was initialized
static int m_initialized;

// Time in milliseconds between two measurements
static int m_interval;

// Callback to notify client module about measurements
static smartmeter_cb m_callback;

// Holds the mappings for OBIS ID to Var ID
static const OBIS_Entry obisTable[] = {
	{POWER_ALL_PHASES, {"\x01\x00\x0f\x07\x00\xff"}},
	{POWER_L1, {"\x01\x00\x23\x07\x00\xff"}},
	{POWER_L2, {"\x01\x00\x37\x07\x00\xff"}},
	{POWER_L3, {"\x01\x00\x4b\x07\x00\xff"}},
	{CURRENT_NEUTRAL, {"\x01\x00\x5b\x07\x00\xff"}},
	{CURRENT_L1, {"\x01\x00\x1f\x07\x00\xff"}},
	{CURRENT_L2, {"\x01\x00\x33\x07\x00\xff"}},
	{CURRENT_L3, {"\x01\x00\x47\x07\x00\xff"}},
	{VOLTAGE_L1, {"\x01\x00\x20\x07\x00\xff"}},
	{VOLTAGE_L2, {"\x01\x00\x34\x07\x00\xff"}},
	{VOLTAGE_L3, {"\x01\x00\x48\x07\x00\xff"}},
	{PHASE_ANGLE_VOLTAGE_L2_L1, {"\x01\x00\x51\x07\x01\xff"}},
	{PHASE_ANGLE_VOLTAGE_L3_L1, {"\x01\x00\x51\x07\x02\xff"}},
	{PHASE_ANGLE_CURRENT_VOLTAGE_L1, {"\x01\x00\x51\x07\x04\xff"}},
	{PHASE_ANGLE_CURRENT_VOLTAGE_L2, {"\x01\x00\x51\x07\x0f\xff"}},
	{PHASE_ANGLE_CURRENT_VOLTAGE_L3, {"\x01\x00\x51\x07\x1a\xff"}},	
	{0}
};

////////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

// Connects to the Smart Meter
static int smartmeter_connect(void);

// Disconnects from the Smart Meter
static void smartmeter_disconnect(void);

// Detects the IP of the Smart Meter
int detectAddress(IP_Address* addr);

// Requests measurement data from the Smart Meter
static int sendRequest(void);

// Callback function invoked by the meter thread
static void performMeasurement(MeterHandle* handle);

// Lookup the specified OBIS ID in the table
static const OBIS_Entry* lookupObis(const octet_string* obis);

// Functions to process SML responses
static int handleSmlFile(sml_file* file, SmartMeter_Data* m);
static int handleProcParamResponse(const sml_get_proc_parameter_response* data, SmartMeter_Data* m);
static int handleTree(const sml_tree* tree, SmartMeter_Data* m);
static int handleParameterValue(const sml_proc_par_value* ppv, SmartMeter_Data* m);
static int handlePeriodEntry(const sml_period_entry* entry, SmartMeter_Data* m);


////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////

int smartmeter_init(const char* address, const char* port, int interval, 
	smartmeter_cb callback)
{
	// Check if IP of Smart Meter specified
	if (!address) {
	
		IP_Address ip = NULL_IP;
		if (!detectAddress(&ip)) {
			LOG(0, "Failed to detect network address\n");
			return 0;
		}
		
		// Copy detected address
		ip_toStr(&ip, m_host, sizeof(m_host));
		
	} else {
		// Copy the provided address
		strncpy(m_host, address, sizeof(m_host));
	}

	// Set parameters
	m_port = port;
	m_interval = interval;
	m_callback = callback;
	m_socket = INVALID_SOCKET;
	m_initialized = 1;
	return 1;
}

void performMeasurement(MeterHandle* handle)
{
	// Perform measurement
	SmartMeter_Data m = {{0}};
	if (!smartmeter_measure(&m)) {
		LOG(0, "Failed to perform measurement\n");
		return;
	}
	
	// Invoke callback
	if (m_callback) {
		m_callback(&m);
	} else {
		LOG(1, "No callback specified\n");
	}
}

int smartmeter_start(void)
{
	if (!m_initialized) {
		LOG(0, "Module not initialized\n");
		return 0;
	}
	
	m_handle = meter_start(m_interval, performMeasurement);
	return m_handle != NULL;
}

int smartmeter_stop(void)
{
	return meter_stop(m_handle);
}

int smartmeter_join(void)
{
	return meter_join(m_handle);
}

int smartmeter_connect(void)
{
	if (m_socket != INVALID_SOCKET) {
		LOG(1, "Already connected\n");
		return 1;
	}

	// Create TCP client socket
	m_socket = io_createClientSocket(m_host, m_port, m_interval);

	return m_socket != INVALID_SOCKET;
}

void smartmeter_disconnect(void)
{
	io_closeSocket(&m_socket);
}

int detectAddress(IP_Address* addr)
{
	/* NOTE: 
	
     This feature exhibits the fact that the Smart Meter frequently performs 
	   BRE multicasts.
	
	   If no packets are received, the problem may be that the socket
	   is bound to the wrong interface. In Java, this can be fixed using
	  
	   socket.setNetworkInterface(NetworkInterface.getByName("eth0"));
	  
	   but I haven't found a way yet to achieve the same in C.
	         
	   A workaround is to add a route for the multicast traffic:
	   
	   route add -net 224.0.0.0 netmask 224.0.0.0 eth0
	
	*/

	struct sockaddr_in sa = {0};
	
	// Retry upon timeout to increase robustness
	int cnt = 0;
	while (1) {
	
		// Create socket to receive BRE multicast
		int sfd = io_createBroadcastSocket(7259, 10000, "232.0.100.0");
		if (sfd == INVALID_SOCKET) {
			LOG(0, "Failed to create multicast socket. Try route add -net 224.0.0.0 netmask 224.0.0.0 eth0\n");
			return 0;
		}
	
		// Receive a packet
		size_t len = sizeof(sa);
		size_t size = recvfrom(sfd, NULL, 0, 0, (struct sockaddr*) &sa, &len);
	
		// Cleanup
		close(sfd);
	
		// Check for error
		if (size == -1) {
			if (errno == EAGAIN) {

				if (cnt == 0) {
					LOG(1, "Waiting for Smart Meter...\n");
				}
				++cnt;
				
				continue; // Try again
			}

			LOG(0, "Failed to receive from socket: %s\n", strerror(errno));
			return 0;
		}
		
		break; // Success
	}
	
	if (cnt > 0) {
		LOG(1, "Found after retrying %d times\n", cnt);
	}
	
	LOG(2, "Found at %s\n", inet_ntoa(sa.sin_addr));
		
	*addr = sa.sin_addr;
	return 1;
}

int smartmeter_measure(SmartMeter_Data* m)
{
	// Re-establish connection
	if (!smartmeter_connect()) {
		return 0;
	}

	// Send request for data
	if (!sendRequest()) {
		LOG(0, "Failed to send data request\n");
		return 0;
	}

	// Receive measurement data
	unsigned char buffer[MTU];
	size_t size = recv(m_socket, buffer, sizeof(buffer), 0);

	// Set timestamp of measurement
	m->val[TIMESTAMP] = time(NULL);

	LOG(3, "Bytes received: %d\n", size);
	
	// Check if packet could be read
	if (size == -1) {
		LOG(0, "Failed to receive response: %s\n", strerror(errno));
		smartmeter_disconnect();
		return 0;
	}
	if (size == 0) {
		LOG(0, "Failed to receive response: Peer performed orderly shutdown\n");
		smartmeter_disconnect();
		return 0;
	}

	// Parse data using libsml
	sml_file *file = sml_file_parse(buffer + 8, size - 16);
	if (!file) {
		LOG(0, "Failed to parse SML file\n");
		smartmeter_disconnect();
		return 0;
	}

	// Retrieve measurement
	int numVariables = handleSmlFile(file, m) + 1; // Add one for timestamp
	
	// Free resources
	sml_file_free(file);
	
	// Check if all variables measured
	if (numVariables < NUM_VARIABLES) {
		LOG(1, "Only %d of %d variables measured\n", numVariables, NUM_VARIABLES);
		return 0;
	}

	// The Smart Meter seems to drop the connection after every
	// measurement, so we need to do it as well
	smartmeter_disconnect();

	// Success
	return 1;
}

int handleSmlFile(sml_file* file, SmartMeter_Data* m)
{
	// Iterate over all messages
	for (int i = 0; i < file->messages_len; i++) {

		// Check message		
		if (!file->messages[i]) {
			LOG(1, "Message %d not available\n", i);
			continue;
		}
	
		// Get message body
		const sml_message_body* body = file->messages[i]->message_body;
		if (!body) {
			LOG(1, "No message body\n");
			continue;
		}

		// Dispatch over message tag
		u16 tag = body->tag ? *body->tag : 0;
		switch (tag) {
		case SML_MESSAGE_OPEN_REQUEST:
			LOG(4, "[SML_MESSAGE_OPEN_REQUEST]\n");
			break;
		case SML_MESSAGE_OPEN_RESPONSE:
			LOG(4, "[SML_MESSAGE_OPEN_RESPONSE]\n");
			break;
		case SML_MESSAGE_CLOSE_REQUEST:
			LOG(4, "[SML_MESSAGE_CLOSE_REQUEST]\n");
			break;
		case SML_MESSAGE_CLOSE_RESPONSE:
			LOG(4, "[SML_MESSAGE_CLOSE_RESPONSE]\n");
			break;
		case SML_MESSAGE_GET_PROFILE_PACK_REQUEST:
			LOG(4, "[SML_MESSAGE_GET_PROFILE_PACK_REQUEST]\n");
			break;
		case SML_MESSAGE_GET_PROFILE_PACK_RESPONSE:
			LOG(4, "[SML_MESSAGE_GET_PROFILE_PACK_RESPONSE]\n");
			break;
		case SML_MESSAGE_GET_PROFILE_LIST_REQUEST:
			LOG(4, "[SML_MESSAGE_GET_PROFILE_LIST_REQUEST]\n");
			break;
		case SML_MESSAGE_GET_PROFILE_LIST_RESPONSE:
			LOG(4, "[SML_MESSAGE_GET_PROFILE_LIST_RESPONSE]\n");
			break;
		case SML_MESSAGE_GET_PROC_PARAMETER_REQUEST:
			LOG(4, "[SML_MESSAGE_GET_PROC_PARAMETER_REQUEST]\n");
			break;
		case SML_MESSAGE_GET_PROC_PARAMETER_RESPONSE:
			LOG(4, "[SML_MESSAGE_GET_PROC_PARAMETER_RESPONSE]\n");
			return handleProcParamResponse((const sml_get_proc_parameter_response*) body->data, m);
		case SML_MESSAGE_SET_PROC_PARAMETER_REQUEST:
			LOG(4, "[SML_MESSAGE_SET_PROC_PARAMETER_REQUEST]\n");
			break;
		case SML_MESSAGE_SET_PROC_PARAMETER_RESPONSE:
			LOG(4, "[SML_MESSAGE_SET_PROC_PARAMETER_RESPONSE]\n");
			break;
		case SML_MESSAGE_GET_LIST_REQUEST:
			LOG(4, "[SML_MESSAGE_GET_LIST_REQUEST]\n");
			break;
		case SML_MESSAGE_GET_LIST_RESPONSE:
			LOG(4, "[SML_MESSAGE_GET_LIST_RESPONSE]\n");
			break;
		case SML_MESSAGE_ATTENTION_RESPONSE:
			LOG(4, "[SML_MESSAGE_ATTENTION_RESPONSE]\n");
			break;
		default:
			LOG(1, "Unknown message tag: %d\n", tag);
		}
	}	

	LOG(0, "Failed to handle SML file\n");
	return 0; // Failed
}

int handleProcParamResponse(const sml_get_proc_parameter_response* data, SmartMeter_Data* m)
{
	return handleTree(data->parameter_tree, m);
}

int handleTree(const sml_tree* tree, SmartMeter_Data* m)
{
	if (tree) {
		int ret = handleParameterValue(tree->parameter_value, m);
		for (int i = 0; i < tree->child_list_len; i++) {
			ret += handleTree(tree->child_list[i], m);
		}
		return ret;
	} else {
		LOG(1, "Empty parameter tree\n");
		return 0;
	}	
}

int handleParameterValue(const sml_proc_par_value* ppv, SmartMeter_Data* m)
{
	if (ppv) {
		if (!ppv->tag) {
			LOG(1, "No parameter tag\n");
			return 0;
		}
		switch (*(ppv->tag)) {
			case SML_PROC_PAR_VALUE_TAG_VALUE:
				LOG(4, "[SML_PROC_PAR_VALUE_TAG_VALUE]\n");
				break;
			case SML_PROC_PAR_VALUE_TAG_PERIOD_ENTRY:
				LOG(4, "[SML_PROC_PAR_VALUE_TAG_PERIOD_ENTRY]\n");
				return handlePeriodEntry(ppv->data.period_entry, m);
			case SML_PROC_PAR_VALUE_TAG_TUPEL_ENTRY:
				LOG(4, "[SML_PROC_PAR_VALUE_TAG_TUPEL_ENTRY]\n");
				break;
			case SML_PROC_PAR_VALUE_TAG_TIME:
				LOG(4, "[SML_PROC_PAR_VALUE_TAG_TIME]\n");
				break;
			default:
				LOG(1, "Unknown tag: %d\n", *(ppv->tag));
		}
		
		LOG(0, "Failed to handle parameter value\n");
		return 0;	
	} else {
		LOG(3, "Empty parameter value\n");	
		return 0;
	}
}

int handlePeriodEntry(const sml_period_entry* entry, SmartMeter_Data* m)
{
	// Check if value available
	if (entry->value) {
	
		double value = sml_value_to_double(entry->value);
		
		// Scale value if required
		if (entry->scaler) {
			value *= pow(10, *entry->scaler);
		}
		
		// Get value unit
		const OBIS_Entry* obis = lookupObis(entry->obj_name);
		if (!obis) return 0;
		
		// Success
		m->val[obis->id] = value;
		return 1;
	
	} else {
	
		// No value available
		return 0;
	}
}

int sendRequest(void)
{
	// NOTE: Parts of this code are probably vendor-specific

	// Create SML file
	sml_file* sml = sml_file_init();
	sml_message* msg = NULL;

	// Open request	
	msg = sml_message_init();
	msg->group_id = sml_u8_init(1);
	msg->abort_on_error = sml_u8_init(0);
	sml_open_request* openReq = sml_open_request_init();
	openReq->client_id   = sml_octet_string_init_from_hex("010203040506");
	openReq->req_file_id = sml_octet_string_init_from_hex("51");	
	openReq->server_id   = sml_octet_string_init_from_hex("FFFFFFFFFFFF");
	msg->message_body = sml_message_body_init(SML_MESSAGE_OPEN_REQUEST, openReq);
	sml_file_add_message(sml, msg);
	
	// Process parameter request
	msg = sml_message_init();
	msg->group_id = sml_u8_init(2);
	msg->abort_on_error = sml_u8_init(0);
	sml_get_proc_parameter_request* procParamReq = sml_get_proc_parameter_request_init();
	procParamReq->server_id = sml_octet_string_init_from_hex("FFFFFFFFFFFF");
	procParamReq->parameter_tree_path = sml_tree_path_init();
	sml_tree_path_add_path_entry(procParamReq->parameter_tree_path, 
		sml_octet_string_init_from_hex("8181C78501FF"));
	msg->message_body = sml_message_body_init(SML_MESSAGE_GET_PROC_PARAMETER_REQUEST, procParamReq);
	sml_file_add_message(sml, msg);

	// Close request	
	msg = sml_message_init();
	msg->group_id = sml_u8_init(3);
	msg->abort_on_error = sml_u8_init(0);
	sml_close_request* closeReq = sml_close_request_init();
	msg->message_body = sml_message_body_init(SML_MESSAGE_CLOSE_REQUEST, closeReq);
	sml_file_add_message(sml, msg);

	// Send SML file
	size_t written = sml_transport_write(m_socket, sml);	
	
	// Cleanup
	sml_file_free(sml);
	
	return written > 0;
}

const char* smartmeter_address(void)
{
	return m_host;
}

const OBIS_Entry* lookupObis(const octet_string* obis)
{
	if (!obis || obis->len > sizeof(OBIS)) {
		return NULL;
	}
	for (const OBIS_Entry* e = obisTable; e->obis.raw; e++) {
		if (memcmp(obis->str, e->obis.raw, obis->len) == 0) {
			return e;
		}
	}
	return NULL;
}

const char* smartmeter_getVarName(SmartMeter_VarID id)
{
	switch (id) {
		case TIMESTAMP:
			return "timestamp";
		case POWER_ALL_PHASES:
			return "power";
		case POWER_L1:
			return "power-l1";
		case POWER_L2:
			return "power-l2";
		case POWER_L3:
			return "power-l3";
		case CURRENT_NEUTRAL:
			return "current-neutral";
		case CURRENT_L1:
			return "current-l1";
		case CURRENT_L2:
			return "current-l2";
		case CURRENT_L3:
			return "current-l3";
		case VOLTAGE_L1:
			return "voltage-l1";
		case VOLTAGE_L2:
			return "voltage-l2";
		case VOLTAGE_L3:
			return "voltage-l3";
		case PHASE_ANGLE_VOLTAGE_L2_L1:
			return "phase-angle-voltage-l2-l1";
		case PHASE_ANGLE_VOLTAGE_L3_L1:
			return "phase-angle-voltage-l3-l1";
		case PHASE_ANGLE_CURRENT_VOLTAGE_L1:
			return "phase-angle-current-voltage-l1";
		case PHASE_ANGLE_CURRENT_VOLTAGE_L2:
			return "phase-angle-current-voltage-l2";
		case PHASE_ANGLE_CURRENT_VOLTAGE_L3:
			return "phase-angle-current-voltage-l3";
		default:
			return "(unknown)";
	}
	
	return 0;
}


