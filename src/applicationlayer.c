// Application layer protocol implementation

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

#include "applicationlayer.h"
#include "DataLink.h"

#define C_DATA 1
#define C_START 2
#define C_END 3

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    if (strcmp(role, "tx") == 0) {
        int fd = open(filename, O_RDONLY);
        if (fd == -1) {
            printf("Error opening \"%s\".\n", filename);
            return;
        }

        struct stat st;
        stat(filename, &st);
        unsigned long size = (unsigned long) st.st_size;
        printf("Size: %lu\n", size);

        
        int L1 = ceil( floor(log2(size )) + 1 / 8);
	    int L2 = strlen(filename);
        unsigned long size_aux = 1 + 1 + 1 + L1 + 2 + L2;
        int i = 0;
        unsigned char* control_packet = (unsigned char*) malloc(size_aux);
        control_packet[i++] = C_START;
        control_packet[i++] = 0;
        control_packet[i++] = L1;

        

        
       for (unsigned char j = 0 ; j < L1 ; j++) {
        control_packet[2+L1-j] = size_aux & 0xFF;
        size_aux >>= 8;
    }	
    i += L1;
    control_packet[i++]=1;
    control_packet[i++]=L2;

    memcpy(control_packet + i,filename,L2);





        LinkLayer connectionParameters;
        Role role= Transmissor;
        strcpy(connectionParameters.serialPort, serialPort);
        connectionParameters.role = role;
        connectionParameters.baudRate = baudRate;
        connectionParameters.numTransmissions = nTries;
        connectionParameters.timeout = timeout;

        if (llopen(connectionParameters) == -1) {
            printf("Error setting connection.\n");
            return;
        }
       printf("Good opening");
        if (llwrite(control_packet, size_aux) == -1) {
            printf("Error transmitting information.1\n");
            return;
        }

        printf("Good llwrite\n");

        unsigned long left = size;

        unsigned char* data_packet = (unsigned char*) malloc (MAX_PAYLOAD_SIZE);
        unsigned int line_size = MAX_PAYLOAD_SIZE - 3;

        while (left > 0) {
            if (left <= line_size) line_size = left;

            data_packet[0] = C_DATA;
            data_packet[2] = (unsigned char) (0xff & (line_size));
            data_packet[1] = (unsigned char) (0xff & ((line_size) >> 8));
            
            read(fd, data_packet + 3, line_size);

            if (llwrite(data_packet, line_size + 3) != line_size + 3) {
                printf("Error transmitting information.2\n");
                return;
            }

            left -= line_size;
        }

        free(data_packet);

        control_packet[0] = C_END;
        if (llwrite(control_packet, 11) == -1) {
            printf("Error transmitting information.3\n");
            return;
        }
        free(control_packet);

        if (llclose() == -1) {
            printf("Error closing connection.\n");
            return;
        }

        close(fd);

    } else if (strcmp(role, "rx") == 0) {
        int fd;
        int fd_temp = open(filename, O_WRONLY | O_TRUNC);
        if(fd_temp == -1)
            fd = open(filename, O_APPEND | O_CREAT | O_WRONLY);
        else {
            write(fd_temp, "", 0);
            close(fd_temp);
            fd = open(filename, O_APPEND | O_WRONLY);
        }
        if (fd == -1) {
            printf("Error opening \"%s\".\n", filename);
            return;
        }
        LinkLayer connectionParameters;

        Role role = Receptor;
        strcpy(connectionParameters.serialPort, serialPort);
        connectionParameters.role = role;
        connectionParameters.baudRate = baudRate;
        connectionParameters.numTransmissions = nTries;
        connectionParameters.timeout = timeout;
        printf("one");
        if (llopen(connectionParameters) == -1) {
            printf("Error setting connection.\n");
            return;
        } else{printf("two");}
        printf("Good opening");
        unsigned char* buffer = (unsigned char*) malloc (MAX_PAYLOAD_SIZE);
        if(llread(buffer) == -1) {
            printf("Error receiving information.\n");
            return;
        }
        printf("-------------------------------------------------Received First Packet\n");

        //--------------------
        unsigned long size = 0;
        for (int i = 0; i < 8; i++) {
            size <<= 8;
            size += buffer[i + 3];
        }
        //-------------------

        while(1) {
            if(llread(buffer) == -1) {
                printf("------------------------------------------------------------Continuou\n");
                continue;
            }
            if(buffer[0] != C_DATA) break;
            unsigned int current_size = buffer[1];
            current_size <<= 8;
            current_size += buffer[2];
            
            write(fd, buffer + 3, current_size);
            printf("-----------------------------------------------------------------------------\n");
            for (int i = 0; i < current_size; i++) {
                printf("%c", buffer[i + 3]);
            }
            printf("\n-----------------------------------------------------------------------------\n");
        }

        if(buffer[0] != C_END) {
            printf("Error receiving information(C_END).\n");
            return;
        }

        unsigned long new_size = 0;
        for(int i = 0; i < 8; i++){
            new_size <<= 8;
            new_size += buffer[i + 3];
        }
        if(new_size != size) {
            printf("Error receiving information(size).\n");
            return;
        }

        if(llread(buffer) == -1) {
            printf("Error receiving information(DISC).\n");
            return;
        }

        free(buffer);

        if(llclose(0) == -1) {
            printf("Error closing connection.\n");
            return;
        }
        close(fd);
    }
}