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

#include "io.h"

#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h> 
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ifaddrs.h>

#include "common.h"

////////////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////////////

// Maximum number of entries in the socket table
#define SOCKETTABLE_SIZE 16


////////////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////////////

// Struct to hold information about a socket
typedef struct SocketEntry_s {
	// Socket descriptor
	int sfd;
	const char* name;
	// Callback invoked upon socket is ready for read
	SocketReady_cb callback;
} SocketEntry;

// Table to hold information about open sockets
SocketEntry socketTable[SOCKETTABLE_SIZE];

// Descriptor set used for select()
static fd_set readfds;

// Maximum descriptor required by select()
static int max_sfd;

////////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

// Creates a new entry in the socket table
SocketEntry* createSocketEntry(int sfd);

// Looks up an entry in the socket table
SocketEntry* lookupSocketEntry(int sfd);

// Removes an entry from the socket table
void removeSocketEntry(int sfd);


////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////

void io_init(void)
{
	FD_ZERO(&readfds);
	max_sfd = 0;

	// Initialize socket table
	for (int i = 0; i < SOCKETTABLE_SIZE; i++) {
		socketTable[i].sfd = INVALID_SOCKET;
	}
}

void io_deinit(void)
{
	FD_ZERO(&readfds);
	max_sfd = 0;
	
	// Close all remaining sockets
	for (int i = 0; i < SOCKETTABLE_SIZE; i++) {
		io_closeSocket(&socketTable[i].sfd);
	}
}

int io_createRawSocket(void)
{
	int sfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
	if (sfd == INVALID_SOCKET) {
		LOG(0, "Failed to initialize socket: %s\n", strerror(errno));
		return INVALID_SOCKET;
	}
	
	return sfd;
}

int io_createBroadcastSocket(int port, int timeout, const char* group)
{
	// Create new UDP socket
	int sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sfd == INVALID_SOCKET) {
		LOG(0, "Failed to initialize socket: %s\n", strerror(errno));
		return INVALID_SOCKET;
	}
	
	// Initialize timeout structure
	struct timeval to = {0};
  to.tv_sec  = timeout / 1000;           // Seconds
  to.tv_usec = (timeout % 1000) * 1000;  // Microseconds
	
	// Set socket options
	int on = 1;
	if (setsockopt(sfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) == -1) {
		LOG(0, "Failed to set SO_BROADCAST: %s\n", strerror(errno));
		close(sfd);
		return INVALID_SOCKET;
	}
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
		LOG(0, "Failed to set SO_REUSEADDR: %s\n", strerror(errno));
		close(sfd);
		return INVALID_SOCKET;
	}
	if (timeout >= 0) {
		if (setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to)) == -1) {
			LOG(0, "Failed to set timeout: %s\n", strerror(errno));
			close(sfd);
			return INVALID_SOCKET;
		}
	}	

	// Join multicast group if specified
	if (group) {

		struct ip_mreqn mreq = {{0}};
		mreq.imr_multiaddr.s_addr = inet_addr(group);
		mreq.imr_address.s_addr   = htonl(INADDR_ANY);
		mreq.imr_ifindex          = 0;

		if (setsockopt(sfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1) {
			LOG(0, "Failed to join multicast group '%s': %s\n", group, strerror(errno));
			return INVALID_SOCKET;
		}
	}
	
	// Initialize socket address
	struct sockaddr_in sa = {0};	
  sa.sin_family       = AF_INET;
  sa.sin_port         = htons(port);
  sa.sin_addr.s_addr  = htonl(INADDR_ANY);
  
  // Bind socket to address
  if (bind(sfd, (struct sockaddr*) &sa, sizeof(sa)) == -1) {
  	LOG(0, "Failed to bind socket to %s:%d: %s\n", 
  		inet_ntoa(sa.sin_addr), ntohs(sa.sin_port), strerror(errno));
  	close(sfd);
  	return INVALID_SOCKET;
  }

	return sfd;
}

int io_createClientSocket(const char* host, const char* service, int timeout)
{
	if (!host) {
		LOG(0, "Host not specified\n");
		return 0;
	}
	
	if (!service) {
		LOG(0, "Service/Port not specified\n");
		return 0;
	}
	
	// Initialize timeout structure
	struct timeval to = {0};
  to.tv_sec  = timeout / 1000;           // Seconds
  to.tv_usec = (timeout % 1000) * 1000;  // Microseconds

	// Specify address information details
	struct addrinfo hints = {0};
	hints.ai_family   = AF_UNSPEC;     // Allow IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;   // TCP Socket
	hints.ai_flags    = 0;
	hints.ai_protocol = 0;             // Any protocol
	
	// Get address information according to host/service	
	struct addrinfo* ai = NULL;
	int error = getaddrinfo(host, service, &hints, &ai);
	if (error) {
		LOG(0, "getaddrinfo failed: %s\n", 
			error != EAI_SYSTEM ? gai_strerror(error) : strerror(errno));
		return INVALID_SOCKET;
	}
	
	// Examine results
	int sfd = INVALID_SOCKET;
	for (struct addrinfo* it = ai; it; it = it->ai_next) {

		// Try to create socket with the current address details
		sfd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
		if (sfd == INVALID_SOCKET) {
			if (it->ai_next) {
				continue; // Try the next entry
			} else {
				LOG(0, "Failed to create socket: %s\n", strerror(errno));
				break;
			}
		}
		
		// Set receive timeout
		if (timeout >= 0) {
			if (setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to)) == -1) {
				LOG(0, "Failed to set timeout: %s\n", strerror(errno));
				break;
			}
		}
		
		// Try to connect with the current address details
		if (connect(sfd, it->ai_addr, it->ai_addrlen) == -1) {
			close(sfd);
			sfd = INVALID_SOCKET;
			if (it->ai_next) {
				continue; // Try the next entry
			} else {
				LOG(0, "Failed to connect to %s:%s: %s\n",
					host, service, strerror(errno));
				break;
			}
		}
		
		break; // Success
	}
	
	// Cleanup
	freeaddrinfo(ai);
	
	return sfd;
}

void io_closeSocket(int* sfd)
{
	if (sfd && *sfd != INVALID_SOCKET) {
		close(*sfd);
		removeSocketEntry(*sfd);
		*sfd = INVALID_SOCKET;
	}
}

int io_getSocketPort(int sfd)
{
	struct sockaddr sa;
	socklen_t len = sizeof(sa);
	if (getsockname(sfd, &sa, &len)== -1) {
		LOG(0, "getsockname failed: %s\n", strerror(errno));
		return -1;
	}
	
	switch (sa.sa_family) {
	case AF_INET:
		return ((struct sockaddr_in*) &sa)->sin_port;
	case AF_INET6:
		return ((struct sockaddr_in6*) &sa)->sin6_port;
	default:
		return -1;
	}
}

int io_getHostname(struct in_addr addr, Hostname host)
{
	struct sockaddr_in sa = {0};
  sa.sin_family       = AF_INET;
  sa.sin_port         = 0; // not used
  sa.sin_addr         = addr;
  int error = getnameinfo((struct sockaddr*)&sa, sizeof(sa), 
  	host, sizeof(Hostname), NULL, 0, NI_NOFQDN);
  if (error) {
  	LOG(0, "getnameinfo failed: %s\n", 
  		error != EAI_SYSTEM ? gai_strerror(error) : strerror(errno));
  	return 0;
  }
	return 1; // Success
}

int io_multiplexRead(int sfd, const char* name, SocketReady_cb callback)
{
	if (!callback) {
		LOG(0, "Callback not specified for %d\n", sfd);
		return 0;
	}

	// Create new socket entry
	SocketEntry* entry = createSocketEntry(sfd);
	if (!entry) {
		LOG(0, "Failed to create socket entry\n");
		return 0;
	}
	
	// Store data
	entry->name = name;
	entry->callback = callback;
	
	// Add descriptor to set
	FD_SET(sfd, &readfds);
	if (max_sfd < sfd) {
		max_sfd = sfd;
	}
	
	return 1; // Success
}

int io_process(void)
{
	// Copy working set (will be modified by select() )
	fd_set ready_readfds = readfds;

	// Perform synchronous I/O multiplexing
	int numReady = select(max_sfd+1, &ready_readfds, NULL, NULL, NULL);
	if (numReady == -1) {
  	LOG(0, "Failed to select socket: %s\n", strerror(errno));
  	return 0;
	}
	
	// Examine results
	for (int sfd = 0; sfd <= max_sfd; ++sfd) {
	
		// Check if socket is ready
		if (FD_ISSET(sfd, &ready_readfds)) {
		
			// Lookup the socket entry
			const SocketEntry* entry = lookupSocketEntry(sfd);
			if (!entry) {
				LOG(1, "Socket entry missing for %d\n", sfd);
				continue;
			}

			LOG(4, "%s: Data received\n", entry->name);
			
			// Invoke callback
			entry->callback(sfd);
		}
	}
	
	return 1; // Success
}

void io_processLoop(void)
{
	while (io_process()) continue;
}

SocketEntry* lookupSocketEntry(int sfd)
{
	for (int i = 0; i < SOCKETTABLE_SIZE; i++) {
		if (socketTable[i].sfd == sfd) {
			return &socketTable[i];
		}
	}
	return NULL;
}

SocketEntry* createSocketEntry(int sfd)
{
	SocketEntry* entry = lookupSocketEntry(INVALID_SOCKET);
	if (entry) {
		entry->sfd = sfd;
		return entry;
	} else {
		return NULL;
	}
}

void removeSocketEntry(int sfd)
{
	SocketEntry* entry = lookupSocketEntry(INVALID_SOCKET);
	if (entry) {
		entry->sfd = INVALID_SOCKET;
	}
}

int io_isLocalAddress(struct in_addr addr)
{
	// Get interfaces list
	struct ifaddrs* ifa = NULL;
	if (getifaddrs(&ifa) == -1) {
    LOG(0, "Failed to get network interfaces: %s\n", strerror(errno));
		return 0;
	}
	
	// Iterate over interfaces
	int result = 0;
	for (struct ifaddrs* it = ifa; it; it = it->ifa_next) {
		if (!it->ifa_addr) continue;
		
		// Look for IPv4 addresses
		if (it->ifa_addr->sa_family == AF_INET) {
			// Match?
			if (addr.s_addr == ((struct sockaddr_in*)it->ifa_addr)->sin_addr.s_addr) {
				result = 1;
				break;
			}
		}
	}
	
	// Cleanup
	freeifaddrs(ifa);
	
	return result;
}

int io_getNetworkInterface(const char* name,
	IP_Address* addr, IP_Address* netmask)
{
	// Get interfaces list
	struct ifaddrs* ifa = NULL;
	if (getifaddrs(&ifa) == -1) {
    LOG(0, "Failed to get network interfaces: %s\n", strerror(errno));
		return 0;
	}
	
	// Iterate over interfaces
	int result = 0;
	for (struct ifaddrs* it = ifa; it; it = it->ifa_next) {
		if (!it->ifa_addr) continue;
		
		// Look for IPv4 addresses
		if ((it->ifa_addr->sa_family == AF_INET) && (it->ifa_netmask->sa_family == AF_INET)) {
			// Match?
			if (!name || !strcmp(it->ifa_name, name)) {
				*addr    = ((struct sockaddr_in*)it->ifa_addr)->sin_addr;
				*netmask = ((struct sockaddr_in*)it->ifa_netmask)->sin_addr;
				result = 1;
				break;
			}
		}
	}
	
	// Cleanup
	freeifaddrs(ifa);
	
	return result;
}


