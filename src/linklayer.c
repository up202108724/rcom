
#include "DataLink.h"

LinkLayerState state = START;

int alarmEnabled = FALSE;
int alarmCount = 0;
volatile int STOP = FALSE;

int llwrite(int fd, const unsigned char *buf, int bufSize){

    
    
}
int llread(int fd, char *buf){

    char byte;
    LinkLayerState state = START;
    while (state!= STOP){
        if(read(fd, &byte,1) !=0){
        
        switch state:
            case START:
                if(byte== FLAG){
                    state= FLAG_RCV;
                }
                break; 
            case FLAG_RCV:
                if(byte== A_ER){
                    state = A_RCV;
                }else if { state= START;}
            break;

            case A_RCV:
                if(byte== FLAG){
                    state= FLAG_RCV;
                }
                else if(byte== 0x00 || byte == 0x40) {
                    state= C_RCV;
                }
                else { state=START;}
                

            case C_RCV:               
            
            case BCC1:

            case BCC2:               
        }
    }
    return -1;
}

int llclose(int fd){

}



