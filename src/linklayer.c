
#include "DataLink.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
LinkLayerState state = START;

int alarmEnabled = FALSE;
int alarmCount = 0;
volatile int STOP = FALSE;
unsigned char info_frame_number_transmitter=0;
unsigned char info_frame_number_receiver=0;

struct termios oldtio;
struct termios newtio;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

int establish_connection(const char *port){
    int fd = open(port, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(port);
        exit(-1);
    }
    
    struct termios oldtio;
    struct termios newtio;



    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        return -1;
    }
    memset(&newtio, 0, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }
    return 0;
}

int sendSupervisionFrame(int fd, unsigned char A, unsigned char C){
    unsigned char FRAME[5] = {FLAG, A, C, A ^ C, FLAG};
    return write(fd, FRAME, 5);
}

int llopen(LinkLayer sp_config, Role role){
    int fd= establish_connection(sp_config.serialPort);
    if (fd<0){return -1;}

    unsigned char *byte;
    switch (role){

            case Transmissor:
            (void)signal(SIGALRM, alarmHandler);
            while(alarmCount > sp_config.numTransmissions){
            sendSupervisionFrame(fd, A_FSENDER, C_SET);
            alarm(sp_config.timeout);

            if(alarmEnabled==TRUE){
            if(read(fd ,byte, 1)>0){
                switch (state)
                {
                case START:
                    if(byte==FLAG){
                        state=FLAG_RCV;
                    }
                    break;
                case FLAG_RCV:
                    if(byte==A_FRECEIVER){
                        state=A_RCV;
                    }
                    if(byte==FLAG){
                        state=FLAG_RCV;
                    } 
                    else{
                        state=START;
                    }
                    break;
                case A_RCV:
                    if (byte==C_UA){ state= C_RCV;}
                    if(byte==FLAG){
                        state=FLAG_RCV;
                    }
                    else{
                        state=START;
                    }
                break;
                case C_RCV:
                    if (byte==(C_UA^A_FRECEIVER)){
                        state= BCC1;
                    }
                    if(byte==FLAG){
                        state=FLAG_RCV;
                    }
                    else{
                        state=START;
                    }
                case BCC1:
                    if(byte==FLAG){
                        state=STOP;
                    }
                    else{
                        state=START;
                    }
                    break;
                default:
                    break;
                }
                

            }
            }
            case Receptor:
                break;
        }
    
}
}

int llwrite(int fd, unsigned char *buf, int bufSize){

    
    
}
int llread(int fd, unsigned char *buf){

    char byte;
    char control_field;
    int data_byte_counter=0;
    
    LinkLayerState state = START;
    while (state!= STOP){
        if(read(fd, &byte,1) !=0){
        
        switch (state){
            case START:
                if(byte== FLAG){
                    state= FLAG_RCV;
                }
                break; 
            case FLAG_RCV:
                if(byte==A_FSENDER){
                    state = A_RCV;
                }else if(byte== FLAG){
                    state= FLAG_RCV;
                }
                else  { state= START;}
            break;

            case A_RCV:
                if(byte== FLAG){
                    state= FLAG_RCV;
                }
                //trama de informação
                else if(byte== 0x00 || byte == 0x40) {
                    state= C_RCV;
                    control_field= byte;
                }
                //trama de supervisão
                
                else if (byte== 0x0B){
                    sendSupervisionFrame(fd, A_FRECEIVER, C_DISC );
                    return 0;
                }
                else { state=START;}
                

            case C_RCV:                 
                if (byte== A_RCV ^ control_field ){
                    state= BCC1;
                }
                if(byte== FLAG){
                    state= FLAG_RCV;
                }
            case BCC1:
                state= READING_DATA;

            case READING_DATA:
                if (byte== 0x07){ state= DATA_RECEIVED;}
                if (byte== FLAG){
                    
                    unsigned char bcc2 = buf[data_byte_counter-1];
                    data_byte_counter--;
                    buf[data_byte_counter]='\0';
                    unsigned acumulator = buf[0];
                    for (int j=0;j < data_byte_counter ;j++){
                        acumulator^=buf[j];
                    }

                    if(bcc2==acumulator){
                        state=STOP;
                        sendSupervisionFrame(fd, A_FRECEIVER, C_RR(info_frame_number_receiver));
                        return data_byte_counter;
                    }

                }

                break;
            case DATA_RECEIVED:
                data_byte_counter++;

            break;               
        }
    }
    return -1;
}
}

int llclose(int fd){

}



