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

#include "PCSCAccess.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define PCSC_DEBUG

PCSCAccess::PCSCAccess(void) {
}

PCSCAccess::~PCSCAccess(void) {
}

bool PCSCAccess::open(void) {
	LONG rv;
	DWORD dwReaders;
	LPSTR mszReaders = NULL;
	char *ptr, **readers = NULL;
	int nbReaders;
	int idxReader;
	unsigned int i;
	DWORD dwActiveProtocol, dwReaderLen, dwState, dwProt, dwAtrLen;
	BYTE pbAtr[MAX_ATR_SIZE] = "";
	char pbReader[MAX_READERNAME] = "";
	
	rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
	if(rv != SCARD_S_SUCCESS) {
		printf("ERROR: SCardEstablishContext returned %lX\n", rv);
		return false;
	}
	
	// Retrieve the available readers list.
	dwReaders = SCARD_AUTOALLOCATE;
	rv = SCardListReaders(hContext, NULL, (LPSTR)&mszReaders, &dwReaders);
	if(rv != SCARD_S_SUCCESS) {
		printf("ERROR: SCardListReaders returned %lX\n", rv);
		return false;
	}
	
	// Extract readers from the null separated string and get the total number of readers.
	nbReaders = 0;
	ptr = mszReaders;
	while(*ptr != '\0') {
		ptr += strlen(ptr)+1;
		nbReaders++;
	}

	if(nbReaders == 0) {
		printf("ERROR: No reader found\n");
		return false;
	}

	// Allocate the readers table.
	readers = (char**) calloc(nbReaders, sizeof(char *));
	if(NULL == readers) {
		printf("ERROR: Not enough memory to allocate readers[]\n");
		return false;
	}

	// Fill the readers table
	nbReaders = 0;
	ptr = mszReaders;
	while(*ptr != '\0') {
		printf("Found reader %d: %s\n", nbReaders, ptr);
		readers[nbReaders] = ptr;
		ptr += strlen(ptr) + 1;
		nbReaders++;
	}
	
	// By default try to connect to first available reader
	idxReader = 0;
	dwActiveProtocol = -1;
	rv = SCardConnect(hContext, readers[idxReader], SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0, &hCard, &dwActiveProtocol);
	if(rv != SCARD_S_SUCCESS) {
		printf("ERROR: SCardConnect returned %lX\n", rv);
		return false;
	}
	printf(" Protocol: %ld\n", dwActiveProtocol);

	SCardFreeMemory(hContext, mszReaders);
	free(readers);
	
	// Get card status
	dwAtrLen = sizeof(pbAtr);
	dwReaderLen = sizeof(pbReader);
	rv = SCardStatus(hCard, /*NULL*/ pbReader, &dwReaderLen, &dwState, &dwProt, pbAtr, &dwAtrLen);
	printf(" Reader: %s (length %ld bytes)\n", pbReader, dwReaderLen);
	printf(" State: 0x%lX\n", dwState);
	printf(" Prot: %ld\n", dwProt);
	printf(" ATR (length %ld bytes):", dwAtrLen);
	for(i=0; i<dwAtrLen; i++) {
		printf(" %02X", pbAtr[i]);
	}
	printf("\n");
	if(rv != SCARD_S_SUCCESS) {
		printf("ERROR: SCardStatus returned %lX\n", rv);
		return false;
	}

	rv = SCardBeginTransaction(hCard);
	if(rv != SCARD_S_SUCCESS) {
		printf("ERROR: SCardBeginTransaction returned %lX\n", rv);
		return false;
	}
	
	return true;
}

void PCSCAccess::close(void) {
	LONG rv;
	
	rv = SCardEndTransaction(hCard, SCARD_LEAVE_CARD);
	if(rv != SCARD_S_SUCCESS) {
		printf("ERROR: SCardEndTransaction returned %lX\n", rv);
	}
	
	rv = SCardDisconnect(hCard, SCARD_LEAVE_CARD);
	if(rv != SCARD_S_SUCCESS) {
		printf("ERROR: SCardDisconnect returned %lX\n", rv);
	}

	rv = SCardReleaseContext(hContext);
	if(rv != SCARD_S_SUCCESS) {
		printf("ERROR: SCardReleaseContext returned %lX\n", rv);
	}
}

bool PCSCAccess::transmitApdu(uint8_t* apdu, uint16_t apduLen, uint8_t* response, uint16_t* responseLen) {
	uint16_t i;
	LONG rv;
	DWORD dwSendLength, dwRecvLength;
	
	#ifdef PCSC_DEBUG
	printf("SND: ");
	for(i=0; i<apduLen; i++) {
		printf("%02X", apdu[i]);
	}
	printf("\n");
	#endif

	dwSendLength = apduLen;
	dwRecvLength = 256 + 2;
	rv = SCardTransmit(hCard, SCARD_PCI_T0, apdu, dwSendLength, NULL, response, &dwRecvLength);
	*responseLen = dwRecvLength;
	
	#ifdef PCSC_DEBUG
	printf("RCV: ");
	for(i=0; i<*responseLen; i++) {
		printf("%02X", response[i]);
	}
	printf("\n");
	#endif
		
	if(rv != SCARD_S_SUCCESS) {
		printf("ERROR: SCardTransmit returned %lX\n", rv);
		return false;
	}
	
	if((*responseLen == 2) && (response[0] == 0x6C)) {
		apdu[4] = response[1];
		return transmitApdu(apdu, apduLen, response, responseLen);
	}
	
	if((*responseLen == 2) && (response[0] == 0x61)) {
		apdu[0] = 0x00 | (apdu[0] & 0x03);
		apdu[1] = 0xC0;
		apdu[2] = 0x00;
		apdu[3] = 0x00;
		apdu[4] = response[1];
		apduLen = 5;
		return transmitApdu(apdu, apduLen, response, responseLen);
	}
	
	return true;
}
