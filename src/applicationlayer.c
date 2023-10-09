
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

unsigned char * getStartPacket(const unsigned int c, const char* filename, long int length, unsigned int* size){
	
	int L1 = ceil( log2(length ) / 8);
	int L2 = strlen(filename);
	*size = 1 + 1 + 1 + L1 + 2 + L2;
	unsigned char *packet = malloc(*size);
	int i = 0;
	packet[i++]= c;
    packet[i++]= 0;
    packet[i++] = L1;
    
    for (unsigned char j = 0 ; j < L1 ; j++) {
        packet[2+L1-j] = length & 0xFF;
        length >>= 8;
    }	
	
	i+=L1;
	packet[i++]=1;
    packet[i++]=L2;
    memcpy(packet+i, filename, L2);
    return packet;
	
}

unsigned char * getDataPacket(unsigned char sequence, unsigned char *data, int dataSize, int *size){
	
	*size = 1 + 1 + 2 + dataSize;
	unsigned char *packet = malloc(*size);
	
	
	
}
