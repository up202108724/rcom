
#include "applicationlayer.h"
#include "DataLink.h"
void setupTransferConditions(char *port, int baudrate, const char *role, unsigned int numTransmissions, unsigned int timeout, const char *filename){
    LinkLayer sp_config;
    strcpy( sp_config.serialPort ,port);
    if (strcmp("transmitter", role)==0 || strcmp("tx", role)==0) {
        sp_config.role= Transmissor;
    }
    if (strcmp("receiver", role)==0 || strcmp("rx", role)==0) {
        sp_config.role= Receptor;
    }
    sp_config.numTransmissions= numTransmissions;
    sp_config.timeout= timeout;

    int fd = llopen(sp_config, sp_config.role);
    if (fd < 0) {
        perror("Connection error\n");
        return -1;
    }
    if(sp_config.role==Transmissor){
        FILE *file = fopen(filename, "rb");
        if(file==NULL){
            perror("Error opening file\n");
            return -1;
        }
        int curr = ftell(file);
        fseek(file, 0, SEEK_END);
        long int file_size = ftell(file) - curr;
        fseek(file, curr, SEEK_SET);


    }

}
