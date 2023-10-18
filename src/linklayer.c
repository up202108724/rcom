#include "DataLink.h"

LinkLayerState state = START;

volatile int alarmEnabled = FALSE;
volatile int alarmCount = 0;
int fd;
unsigned attempts= 0;
unsigned char info_frame_number_transmitter=0;
unsigned char info_frame_number_receiver=1;
unsigned timeout_=0;
unsigned baudrate=0;
struct termios oldtio;
struct termios newtio;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

int establish_connection(const char* port, LinkLayer sp_config){
    fd = open(port, O_RDWR | O_NOCTTY);

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
    newtio.c_cflag = sp_config.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Not blocking the read at all
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }
    printf("New termios structure set\n");

    return 0;
}

int sendSupervisionFrame(unsigned char A, unsigned char C){
    unsigned char FRAME[5] = {FLAG, A, C, A ^ C, FLAG};
    return write(fd, FRAME, 5);
}

int llopen(LinkLayer sp_config){
    //alarmCount=0;
    (void)signal(SIGALRM, alarmHandler);
    fd= establish_connection(sp_config.serialPort,sp_config);
    if (fd<0){return -1;}
    attempts= sp_config.numTransmissions;
    timeout_= sp_config.timeout;
    unsigned char byte;
    switch (sp_config.role){

            case Transmissor: {
            while((alarmCount < attempts) && state!=STOP){
            if(alarmEnabled==FALSE){
            printf("sending supervision frame \n");
            sendSupervisionFrame(A_FSENDER, C_SET);
            alarm(timeout_);
            alarmEnabled=TRUE;
            }
            printf("looping \n");
            printf("%d",timeout_);
            while (alarmEnabled==TRUE && state!=STOP){
            
            if(read(fd ,&byte, 1)>0){
                printf("read a byte");
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
                    else if(byte==FLAG){
                        state=FLAG_RCV;
                    } 
                    else{
                        state=START;
                    }
                    break;
                case A_RCV:
                    if (byte==C_UA){ state= C_RCV;}
                    else if(byte==FLAG){
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
                    else if(byte==FLAG){
                        state=FLAG_RCV;
                    }
                    else{
                        state=START;
                    }
                break;    
                case BCC1:
                    if(byte==FLAG){
                        state=STOP;
                        printf("it came to stop");
                    }
                    else{
                        state=START;
                    }
                    break;
                case STOP:
                    break;    
                default:
                    break;
                }
                

            }
            }
             }
             break;
    } 
            case Receptor:{
                if(read(fd, &byte, 1)>0){
                
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
                    else if(byte==FLAG){
                        state=FLAG_RCV;
                    } 
                    else{
                        state=START;
                    }
                    break;
                case A_RCV:
                    if (byte==C_SET){ state= C_RCV;}
                    else if(byte==FLAG){
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
                    break;
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
            break;
            }
    default:

        return 1;
        break;

}
return 0;
}

int llwrite(unsigned char *buf, int bufSize){
    //alarmCount=0;
    int frameSize = 6 + bufSize;
    unsigned char *frame = (unsigned char *)malloc(frameSize);
    frame[0] = FLAG;
    frame[1] = A_FSENDER;
    frame[2] = C_INF(info_frame_number_transmitter); 
    frame[3] = frame[1] ^ frame[2];
    alarmEnabled=FALSE;
    memcpy(frame + 4, buf, bufSize);
    unsigned char BCC2 = buf[0];
    for (int i = 1; i < bufSize; i++)
    {
        BCC2 ^= buf[i];

    }
    unsigned char* bufwithbcc=(unsigned char*)malloc(bufSize+1);
    memcpy(bufwithbcc, buf, bufSize);
    bufwithbcc[bufSize]= BCC2;
    int j = 4;
    for (int i = 0; i < bufSize+1; i++)
    {
        if (bufwithbcc[i] == FLAG || bufwithbcc[i] == ESC)
        {
            frame = realloc(frame,frameSize + 1);
            if(frame==NULL){return -1;}
            frame[j++] = ESC;
            if(bufwithbcc[i] == FLAG){
                frame[j++]=0x5E;
            }
            if(bufwithbcc[i] == ESC){
                frame[j++]=0x5D;
                
            }
        }
        else{
       frame[j++] = bufwithbcc[i];
        }
    }

    //frame[j++] = BCC2;
    frame[j++] = FLAG;
    frameSize = j;
    
    int reject = 0;
    int accept = 0;
    while (alarmCount< attempts)
    {
        if(alarmEnabled==FALSE){
        
        alarm(timeout_);
        reject=0;
        accept=0;
        write(fd, frame, frameSize); // só há write quando começa o timeout
        alarmEnabled=TRUE;
        }
        while (reject==0 && accept==0 && alarmEnabled==TRUE)
        {
            
            unsigned char result = readControlByte();
            if(result==0) continue;
            else if(result==C_REJ(0) || result==C_REJ(1)){
                reject=1;
            }
            else if (result==C_RR(0) || result==C_RR(1)){
                accept=1;
                info_frame_number_transmitter= (info_frame_number_transmitter+1)%2;
                // não reseta o número de transmissões
            }
        }
        if(accept==1){
            break;
        }
        
        
    }

    free(frame);
    if(accept==1){
        return frameSize;
    }
    else{
        llclose();
        return -1;
    }
    
    
}
int llread(unsigned char *buf){
    
    unsigned char byte;
    char control_field;
    int data_byte_counter=0;
    
    LinkLayerState state = START;
    while (state!= STOP){
        if(read(fd, &byte,1) >0){
        
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
                    sendSupervisionFrame(A_FRECEIVER, C_DISC );
                    return 0;
                }
                else { state=START;}
                

            case C_RCV:                 
                if (byte== (A_RCV ^ control_field) ){
                    state= BCC1;
                }
                if(byte== FLAG){
                    state= FLAG_RCV;
                }
            case BCC1:
                state= READING_DATA;
            case READING_DATA:
                if (byte== ESC){ state= DATA_RECEIVED_ESC;}
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
                        sendSupervisionFrame(A_FRECEIVER, C_RR(info_frame_number_receiver));
                        info_frame_number_receiver=(info_frame_number_receiver+1)%2;
                        return data_byte_counter;
                    }
                    
                    else{
                        sendSupervisionFrame(A_FRECEIVER, C_REJ(info_frame_number_receiver));
                        data_byte_counter=0;
                        state= START;
                    }

                }
                else{
                    buf[data_byte_counter++]= byte;
                }
                break;
            case DATA_RECEIVED_ESC:
                state= READING_DATA;
                if(byte == 0X5E){
                    buf[data_byte_counter++]= FLAG;
                }
                if(byte == 0x5D){
                    buf[data_byte_counter++]= ESC;
                    
                }
                else{
                    data_byte_counter=0;
                    if(byte==FLAG){
                        state=FLAG_RCV;
                    }
                    else{
                        state=START;
                    }
                }

            break;
            case STOP:
                break;               
        }
    }
   }
return -1;
}

int llclose(){
    //alarmCount=0;
    LinkLayerState state= START;
    unsigned char byte;
    (void) signal(SIGALRM,alarmHandler);
    while(state != STOP &&  (alarmCount < attempts) ){
        if(alarmEnabled==FALSE){
            sendSupervisionFrame(A_FSENDER, C_DISC);
            alarm(timeout_);
            alarmEnabled=TRUE;
        }
        if(alarmEnabled==TRUE){
               if(read(fd ,&byte, 1)>0){
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
                case STOP:
                      if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
                    {
                            perror("tcsetattr");
                            exit(-1);
                    }

                     close(fd);
                     return 0;
                default:
                    break;
                }
                

            }
            }
            
        }
        return -1;
    }


unsigned char readControlByte(){

    unsigned char byte = 0;
    unsigned char control_byte = 0;
    LinkLayerState state = START;

    while (state != STOP && alarmEnabled == TRUE){ 
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
                if(byte == (A_FRECEIVER ^ control_byte)){
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
            case STOP:
                 return control_byte;
            default:
                break;
            }


        }
    }

    return control_byte;

}



