/*
 *  Copyright (c) 2017 Gemalto Limited. All Rights Reserved
 *  This software is the confidential and proprietary information of GEMALTO.
 *
 *  GEMALTO MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 *  THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 *  TO THE IMPLIED WARRANTIES OR MERCHANTABILITY, FITNESS FOR A
 *  PARTICULAR PURPOSE, OR NON-INFRINGEMENT. GEMALTO SHALL NOT BE
 *  LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS RESULT OF USING,
 *  MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.

 *  THIS SOFTWARE IS NOT DESIGNED OR INTENDED FOR USE OR RESALE AS ON-LINE
 *  CONTROL EQUIPMENT IN HAZARDOUS ENVIRONMENTS REQUIRING FAIL-SAFE
 *  PERFORMANCE, SUCH AS IN THE OPERATION OF NUCLEAR FACILITIES, AIRCRAFT
 *  NAVIGATION OR COMMUNICATION SYSTEMS, AIR TRAFFIC CONTROL, DIRECT LIFE
 *  SUPPORT MACHINES, OR WEAPONS SYSTEMS, IN WHICH THE FAILURE OF THE
 *  SOFTWARE COULD LEAD DIRECTLY TO DEATH, PERSONAL INJURY, OR SEVERE
 *  PHYSICAL OR ENVIRONMENTAL DAMAGE ("HIGH RISK ACTIVITIES"). GEMALTO
 *  SPECIFICALLY DISCLAIMS ANY EXPRESS OR IMPLIED WARRANTY OF FTNESS FOR
 *  HIGH RISK ACTIVITIES;
 *
 */

#ifndef __CONNECT_SHIELD_NET_H__
#define __CONNECT_SHIELD_NET_H__

#include "NetInterface.h"

#ifdef __cplusplus

class ConnectShieldNet: public NetInterface {
	public:
		// Create an instance of Connect Shield.
		ConnectShieldNet(uint8_t io);
		virtual ~ConnectShieldNet(void);
	
		// Bring net interface up
		// Returns true in case operation was successful, false otherwise.
		virtual bool up(void);
	
		// Bring net interface down
		// Returns true in case operation was successful, false otherwise.
		virtual bool down(void);

		// Open a socket to the targetted domain name or ip address and the corresponding port.
		// Returns an handle >= 0 in case operation was successful, -1 otherwise.
		virtual int openTCPSocket(const char* address, const uint16_t port);
		
		// Read from a socket identify by the handle returned by previously called createSocket function.
		// Returns number of bytes read in case operation was successful, -1 otherwise.
		virtual int readSocket(int handle, uint8_t* data, uint16_t len);
			
		// Write to a socket identify by the handle returned by previously called createSocket function.
		// Returns number of bytes written in case operation was successful, -1 otherwise.
		virtual int writeSocket(int handle, uint8_t* data, uint16_t len);
		
		// Close a socket identify by the handle returned by previously called createSocket function.
		virtual void closeSocket(int handle);
	
		// Check if a socket identify by the handle returned by previously called createSocket function,
		// is closed.
		// Returns true in case socket is closed, false otherwise.
		virtual bool isSocketClosed(int handle);

};

#else 

typedef struct ConnectShieldNet ConnectShieldNet; 

ConnectShieldNet* ConnectShieldNet_create(uint8_t io);
void ConnectShieldNet_destroy(ConnectShieldNet* connect_shield);

#endif

#endif /* __CONNECT_SHIELD_NET_H__ */
