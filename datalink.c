#include "Datalink.h"


LinkLayerState state = START

int llwrite(int fd, const unsigned char *buf, int bufSize){


    buf[0]= FLAG;
    buf[1]= A_ER;
    buf[2]= 0x00;
    buf[3]= 0x0;
    buf[4]= (buf[2] ^ buf[3]);
    while(state!= STOP){
    
        switch state:
        
            case:	
    
    
    }    
    
}


