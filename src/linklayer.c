
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
unsigned attempts= 0;
volatile int STOP = FALSE;
unsigned char info_frame_number_transmitter=0;
unsigned char info_frame_number_receiver=0;
unsigned timeout=0;
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

int llopen(LinkLayer sp_config){
    alarmCount=0;
    int fd= establish_connection(sp_config.serialPort);
    if (fd<0){return -1;}
    attempts= sp_config.numTransmissions;
    timeout= sp_config.timeout;
    unsigned char *byte;
    switch (sp_config.role){

            case Transmissor:
            (void)signal(SIGALRM, alarmHandler);
            while(alarmCount < sp_config.numTransmissions){
            if(alarmEnabled=FALSE){
            sendSupervisionFrame(fd, A_FSENDER, C_SET);
            alarm(timeout);
            alarmEnabled=TRUE;
            }
            while (alarmEnabled==TRUE && state!=STOP){
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
             }
            case Receptor:
                if(read(fd, byte, 1)>0){
                
                switch (state)
                {
                case START:
                    if(byte==FLAG){
                        state=FLAG_RCV;
                    }
                    break;
                case FLAG_RCV:
                    if(byte==A_FSENDER){
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
                    if (byte==C_SET){ state= C_RCV;}
                    if(byte==FLAG){
                        state=FLAG_RCV;
                    }
                    else{
                        state=START;
                    }
                break;
                case C_RCV:
                    if (byte==(C_SET^A_FSENDER)){
                        state= BCC1;
                    }
                    else if(byte==FLAG){
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
                };
                
                
        }
    default:
        
        return -1;
        break;

}
return fd;
}

int llwrite(int fd, unsigned char *buf, int bufSize){
    alarmCount=0;
    int frameSize = 6 + bufSize;
    unsigned char *frame = (unsigned char *)malloc(frameSize);
    frame[0] = FLAG;
    frame[1] = A_FSENDER;
    frame[2] = info_frame_number_transmitter; 
    frame[3] = frame[1] ^ frame[2];
    memcpy(frame + 4, buf, bufSize);
    unsigned char BCC2 = buf[0];
    for (int i = 1; i < bufSize; i++)
    {
        BCC2 ^= buf[i];
    }
    int j = 4;
    for (int i = 0; i < bufSize; i++)
    {
        if (buf[i] == FLAG || buf[i] == ESC)
        {
            frame = realloc(frame,frameSize + 1);
            frame[j++] = ESC;

        }
       frame[j++] = buf[i];
    }

    frame[j++] = BCC2;
    frame[j++] = FLAG;
    frameSize = j;
    int current_transmission = 0;
    int reject = 0;
    int accept = 0;
    while (current_transmission<attempts)
    {
        if(alarmEnabled==FALSE){
        
        alarm(timeout);
        reject=0;
        accept=0;
        alarmEnabled=TRUE;
        }
        while (reject==0 && accept==0 && alarmEnabled==TRUE)
        {
            write(fd, frame, frameSize);
            unsigned char result = readControlByte(fd);
            if(result==0) continue;
            else if(result==C_REJ(0) || result==C_REJ(1)){
                reject=1;
            }
            else if (result==C_RR(0) || result==C_RR(1)){
                accept=1;
                info_frame_number_transmitter= (info_frame_number_transmitter+1)%2;

            }
        }
        if(accept==1){
            break;
        }
        current_transmission++;
        
    }

    free(frame);
    if(accept==1){
        return frameSize;
    }
    else{
        llclose(fd);
        return -1;
    }
    
    
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
    return -1;}
}

int llclose(int fd){
    alarmCount=0;
    LinkLayerState state= START;
    unsigned char byte;
    (void) signal(SIGALRM,alarmHandler);
    while(state != STOP &&  (alarmCount < attempts) ){
        if(alarmEnabled=FALSE){
            sendSupervisionFrame(fd, A_FSENDER, C_DISC);
            alarm(timeout);
            alarmEnabled=TRUE;
        }
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
            
        }
    }


unsigned char readControlByte(int fd){

    unsigned char byte = 0;
    unsigned char control_byte = 0;
    LinkLayerState state = START;

    while (state != STOP && alarmEnabled == TRUE){ // A SER REVISTA A LÓGICA DO ALARM ENABLED AQUI
        
        if(read(fd, &byte, 1) > 0){

            switch (state)
            {
            case START:
                if(byte == FLAG){
                    state = FLAG_RCV;
                }
                break;
            
            case FLAG_RCV:
                if(byte == A_FRECEIVER){
                    state = A_RCV;
                }
                else if(byte != FLAG){
                    state = START;
                }
                break;

            case A_RCV:
                if(byte == C_RR(0) || byte == C_RR(1) || byte == C_REJ(0) || byte == C_REJ(1) || byte == C_DISC){
                    state = C_RCV;
                    control_byte = byte;

                }
                else if (byte != FLAG) state = START;
                else{
                    state = FLAG_RCV;
                }
                break;
            case C_RCV:
                if(byte == A_FRECEIVER ^ control_byte){
                    state = BCC1;
                }
                else if(byte != FLAG){
                    state = START;
                }
                else{
                    state = FLAG_RCV;
                }
                break;
            case BCC1:
                if(byte == FLAG){
                    state = STOP;
                }
                else{
                    state = START;
                }
                break;
            default:
                break;
            }


        }
    }

    return control_byte;

}



