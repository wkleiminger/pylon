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
  Purpose   : Provides routines for network and hardware address management
              and translations.
  
  Version   : 1.0
  Date      : 25.04.2012
  Author    : Daniel Pauli
\******************************************************************************/

#ifndef __IP_H
#define __IP_H

#include <arpa/inet.h>

////////////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////////////

// Type to hold IP adresses (currently IPv4 only)
typedef struct in_addr IP_Address;

// Type to hold MAC addresses
#define MAC_STR_SIZE    18
typedef union MAC_Address_s {
	unsigned char raw[6];
	struct {
		unsigned char vendor[3];
		unsigned char serial[3];
	} fields;
} MAC_Address;

////////////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////////////

// Constant defining an all-zero (invalid) IP address
static const IP_Address NULL_IP   = {0};


////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////////////


// Parses a network address
int ip_fromStr(IP_Address* ip, const char* str);

// Formats a network address as a string
int ip_toStr(const IP_Address* ip, char* buffer, size_t size);

// Resolves the network address from the specified hostname
int ip_fromHostname(IP_Address* ip, const char* host);

// Parses a hardware address
int mac_fromStr(MAC_Address* mac, const char* str);

// Checks if the given string is a correct encoding of a hardware address
int mac_isValid(const char* str);

// Formats a hardware address as a string
int mac_toStr(const MAC_Address* mac, char* buffer, size_t size);

// Retrieves the hardware address by the specified network address from the ARP cache
int ip_lookupArpCache(const IP_Address* ip, MAC_Address* mac);


#endif // __IP_H

