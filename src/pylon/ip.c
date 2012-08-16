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
  Module    : ip
  Used by   : io
  Purpose   : Provides routines for network and MAC address management
              and translations.
  
  Version   : 1.0
  Date      : 25.04.2012
  Author    : Daniel Pauli
\******************************************************************************/

#include "ip.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>

#include "common.h"

////////////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////////////

// Format of a 48-bit hardware address
#define MAC_FORMAT "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx"

////////////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////

int ip_fromStr(IP_Address* ip, const char* str)
{
	int ret = inet_pton(AF_INET, str, ip);
	switch (ret) {
	case 1: 
		return 1; // Success
	case 0:
		LOG(1, "Invalid network address: %s\n", str);
		return 0;
	case -1:
		LOG(0, "Invalid address family\n");
		return 0;
	default:
		LOG(0, "inet_pton returned %d\n", ret);
		return 0;
	}
}

int ip_toStr(const IP_Address* ip, char* buffer, size_t size)
{
	if (!inet_ntop(AF_INET, ip, buffer, size)) {
		LOG(0, "inet_ntop: %s\n", strerror(errno));
		return 0;
	}
	
	return 1; // Success
}

int mac_fromStr(MAC_Address* mac, const char* str)
{
	int cnt = sscanf(str, MAC_FORMAT, 
		&mac->raw[0],&mac->raw[1],&mac->raw[2],&mac->raw[3],&mac->raw[4],&mac->raw[5]);
	return cnt == sizeof(mac->raw);
}

int mac_toStr(const MAC_Address* mac, char* buffer, size_t size)
{
	snprintf(buffer, size, MAC_FORMAT, 
		mac->raw[0],mac->raw[1],mac->raw[2],mac->raw[3],mac->raw[4],mac->raw[5]);
	return 1; // Success
}

int mac_isValid(const char* str)
{
	MAC_Address mac;
	return mac_fromStr(&mac, str);
}

int matches(const char* fmt, const char* str)
{
	while (*fmt) {
		if (*fmt != '#' && *str != *fmt) return 0;
		++fmt;
		++str;
	}
	return !*str;
}

int ip_lookupArpCache(const IP_Address* ip, MAC_Address* mac)
{
	// Open ARP cache
	FILE* arp = fopen("/proc/net/arp", "r");
	if (!arp) {
		LOG(0, "Failed to access ARP cache: %s\n", strerror(errno));
		return 0;
	}

	// Read from stream
	int found = 0;
	char line[128];
	while (!found && fgets(line, sizeof(line), arp)) {
		
		// Variable to hold IP/MAC address of current entry
		IP_Address currentIp   = {0};
		const char* mactoken   = NULL;
		
		// Iterate over tokens
		char* ctx = NULL;
		const char* token = strtok_r(line, " \t", &ctx);
		while (token) {
			
			// Try to match IP address
			if (!currentIp.s_addr) {
				inet_pton(AF_INET, token, &currentIp);
			}
			
			// Try to match MAC address
			if (!mactoken && mac_isValid(token)) {
				mactoken = token;
			}
			
			// Get next token
			token = strtok_r(NULL, " \t", &ctx);
		}

		// Check if correct entry found		
		found = currentIp.s_addr == ip->s_addr;
		if (found) {
			if (mactoken) {
				mac_fromStr(mac, mactoken);
			} else {
				LOG(1, "MAC address missing\n");
			}
		}
	}

	// Close ARP cache	
	fclose(arp);
	
	return found;
}

int ip_fromHostname(IP_Address* ip, const char* host)
{
	// Specify address information details
	struct addrinfo hints = {0};
	hints.ai_family   = AF_INET;     // Currently IPv4 support only
	
	// Get address information according to host
	struct addrinfo* ai = NULL;
	int error = getaddrinfo(host, NULL, &hints, &ai);
	if (error) {
		LOG(0, "getaddrinfo failed: %s\n", 
			error != EAI_SYSTEM ? gai_strerror(error) : strerror(errno));
		return 0;
	}
	
	// Examine results
	int success = 0;
	for (struct addrinfo* it = ai; it; it = it->ai_next) {

		// Check network family 
		if (it->ai_family == AF_INET) {
			*ip = ((struct sockaddr_in*)it->ai_addr)->sin_addr;
			success = 1;
			break;
		}
	}
	
	// Cleanup
	freeaddrinfo(ai);
	
	return success;
}

