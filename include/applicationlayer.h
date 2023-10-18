#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include <stdio.h>
#include <stdlib.h>
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);

typedef enum{
    LlTx,
    LlRx,
}LinkLayerRole;

#define DATA_PACKET 2
#define END_PACKET 3

#endif
