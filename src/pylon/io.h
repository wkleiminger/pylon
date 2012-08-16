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
  Module    : io
  Used by   : smartmeter
  Purpose   : Provides routines for socket communication
  
  Version   : 1.0
  Date      : 05.04.2012
  Author    : Daniel Pauli
\******************************************************************************/

#ifndef __IO_H
#define __IO_H

#include <netinet/in.h>
#include <limits.h>

#include "ip.h"

////////////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////////////

// Descriptor used for sockets that are invalid/closed
#define INVALID_SOCKET -1

// Buffer size to hold datagrams
#define MTU 1500


////////////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////////////

// Type to hold host names
typedef char Hostname[HOST_NAME_MAX];

// Callback used to notify clients about sockets that are ready for read
typedef void(*SocketReady_cb)(int sfd); 


////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

// Initializes the io module
void io_init(void);

// Deinitializes the io module
void io_deinit(void);

// Creates a new UDP broadcast socket listening at the specified port
// using the specified receive timeout in milliseconds and the optional
// multicast group membership
int io_createBroadcastSocket(int port, int timeout, const char* group);

// Creates a new raw socket to receive packets on all interfaces
int io_createRawSocket(void);

// Creates a new TCP client socket connected to the specified service/port
// using the specified receive timeout in milliseconds
int io_createClientSocket(const char* host, const char* service, int timeout);

// Closes a socket
void io_closeSocket(int* sfd);

int io_getSocketPort(int sfd);

int io_getHostname(struct in_addr addr, Hostname host);

// Registers a socket for synchronous I/O multiplexing
// The specified callback is executed by io_process() when data is available
int io_multiplexRead(int sfd, const char* name, SocketReady_cb callback);

// Receives data at the registered sockets
int io_process(void);

// Calls io_process in a loop
void io_processLoop(void);

// Checks if the specified address corresponds to the local host
int io_isLocalAddress(struct in_addr addr);

int io_getNetworkInterface(const char* name,
	IP_Address* addr, IP_Address* netmask);

#endif // __IO_H
