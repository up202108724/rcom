#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include <stdio.h>


void setupTransferConditions(char *port, int baudrate, const char *role, unsigned int numTransmissions, unsigned int timeout, const char *filename);

unsigned char * getStartPacket(const unsigned int c, const char* filename, long int length, unsigned int* size);

unsigned char * getDataPacket(unsigned char sequence, unsigned char *data, int dataSize, int *size);


#endif
