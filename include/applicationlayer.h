#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include <stdio.h>


void setupTransferConditions(char *port, int baudrate, const char *role, unsigned int numTransmissions, unsigned int timeout, const char *filename);

#endif