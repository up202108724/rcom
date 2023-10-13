
#include "applicationlayer.h"
#include "DataLink.h"
#include <stdio.h>
#include <stdlib.h>

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
    packet[0] = 1;
    packet[1] = sequence;
    packet[2] = dataSize >> 8 & 0xFF;
    packet[3] = dataSize & 0xFF;
    memcpy(packet+4, data, dataSize);

    return packet;
	
	
	
}

unsigned char* getfilecontent(FILE *fd , long int filelength){

    unsigned char* content= (unsigned char*) malloc(filelength);
    fread( content,sizeof(unsigned char), filelength ,fd);
    return content;
}

unsigned char* parseControlPacket(unsigned char *packet, unsigned long int *filesize){
    unsigned char size_valuefieldsizeNbytes=packet[2]; // size of file size parameter
    unsigned char placeholder_value_field[size_valuefieldsizeNbytes];
    memcpy(placeholder_value_field, packet+3 , size_valuefieldsizeNbytes);

    for(int i =0 ; i < size_valuefieldsizeNbytes; i++  ){
        *filesize |= (placeholder_value_field[size_valuefieldsizeNbytes-i-1]<< (8*i));

    }
    unsigned char name_valuefieldsizeNbytes=packet[size_valuefieldsizeNbytes+4]; // size of file name parameter
    unsigned char* placeholder_name_value_field=(unsigned char*) malloc(name_valuefieldsizeNbytes);
    memcpy(placeholder_value_field, packet+size_valuefieldsizeNbytes+5, name_valuefieldsizeNbytes);
    
    return placeholder_name_value_field;
}
