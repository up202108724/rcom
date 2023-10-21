#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include <stdio.h>
#include <stdlib.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);
                      
#define DATA_PACKET 2
#define END_PACKET 3
#define C_DATA 1
#define C_START 2
#define C_END 3

#endif
