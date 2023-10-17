#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include <stdio.h>
#include <stdlib.h>

void applicationLayer(const char *port, int baudrate, const char *role, unsigned int numTransmissions, unsigned int timeout, const char *filename);

unsigned char * getStartPacket(const unsigned int c, const char* filename, long int length, unsigned int* size);

unsigned char * getDataPacket(unsigned char sequence, unsigned char *data, int dataSize, int *size);

unsigned char* getFileContent(FILE *fd , long int filelength);

unsigned char* parseControlPacket(unsigned char *packet, unsigned long int *filesize);

void parseDataPacket(const unsigned char* packet, const unsigned int packetSize, unsigned char *buffer);

#define DATA_PACKET 2
#define END_PACKET 3

#endif
