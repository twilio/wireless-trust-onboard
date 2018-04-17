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

#include "ConnectShield_Net.h"

extern "C" {
#include "modem.h"
}

ConnectShieldNet::ConnectShieldNet(uint8_t io) {
	modem_init(io);
}

ConnectShieldNet::~ConnectShieldNet(void) {
}

bool ConnectShieldNet::up(void) {
	return modem_bring_up();
}

bool ConnectShieldNet::down(void) {
	return false;
}

int ConnectShieldNet::openTCPSocket(const char* address, const uint16_t port) {
	return modem_open_tcp_socket(address, port);
}
		
int ConnectShieldNet::readSocket(int handle, uint8_t* data, uint16_t len) {
	return modem_read_socket(handle, data, len);
}
			
int ConnectShieldNet::writeSocket(int handle, uint8_t* data, uint16_t len) {
	return modem_write_socket(handle, data, len);
}
		
void ConnectShieldNet::closeSocket(int handle) {
	modem_close_socket(handle);
}
	
bool ConnectShieldNet::isSocketClosed(int handle) {
	return false;
}

/** C Accessors	***************************************************************/

extern "C" ConnectShieldNet* ConnectShieldNet_create(uint8_t io) {
	return new ConnectShieldNet(io);
}

extern "C" void ConnectShieldNet_destroy(ConnectShieldNet* connect_shield) {
	delete connect_shield;
}
