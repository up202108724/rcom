
#include "DataLink.h"

LinkLayerState state = START;

int alarmEnabled = FALSE;
int alarmCount = 0;
volatile int STOP = FALSE;
unsigned char info_frame_number_transmitter=0;
unsigned char info_frame_number_receiver=0;
int sendSupervisionFrame(int fd, unsigned char A, unsigned char C){
    unsigned char FRAME[5] = {FLAG, A, C, A ^ C, FLAG};
    return write(fd, FRAME, 5);
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



