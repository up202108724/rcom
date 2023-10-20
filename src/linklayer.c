#include "DataLink.h"

LinkLayerState state = START;

volatile int alarmEnabled = FALSE;
volatile int alarmCount = 0;
int fd=0;
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

    /*
    printf("Alarm #%d\n", alarmCount);
    fflush(stdout);
    */
}

int establish_connection(const char* port, LinkLayer sp_config){
    fd = open(port, O_RDWR | O_NOCTTY);

    if (fd< 0)
    {
        return -1;
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
    if(sp_config.role==Transmissor){
    newtio.c_cc[VMIN] = 0;// Not blocking the read at all
    }
    if(sp_config.role==Receptor){
    newtio.c_cc[VMIN] = 1;
    }   
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

int llopen(LinkLayer sp_config) {
    //alarmCount = 0;
    (void) signal(SIGALRM, alarmHandler);
    int check_connection = establish_connection(sp_config.serialPort, sp_config);
    if (check_connection < 0) {
        return -1;
    }
    attempts = sp_config.numTransmissions;
    timeout_ = sp_config.timeout;
    unsigned char byte;
    if (sp_config.role == Transmissor) {
        while ((alarmCount < attempts) && state != STOP) {
            if (alarmEnabled == FALSE) {
                if(sendSupervisionFrame(A_FSENDER, C_SET)==-1){return -1;};
                alarm(timeout_);
                alarmEnabled = TRUE;
            }
            while (alarmEnabled == TRUE && state != STOP) {
                if (read(fd, &byte, 1) > 0) {
                    switch (state) {
                        case START:
                            if (byte == FLAG) {
                                state = FLAG_RCV;
                            }
                            break;
                        case FLAG_RCV:
                            if (byte == A_FRECEIVER) {
                                state = A_RCV;
                            } else if (byte == FLAG) {
                                state = FLAG_RCV;
                            } else {
                                state = START;
                            }
                            break;
                        case A_RCV:
                            if (byte == C_UA) {
                                state = C_RCV;
                            } else if (byte == FLAG) {
                                state = FLAG_RCV;
                            } else {
                                state = START;
                            }
                            break;
                        case C_RCV:
                            if (byte == (C_UA ^ A_FRECEIVER)) {
                                state = BCC1;
                            } else if (byte == FLAG) {
                                state = FLAG_RCV;
                            } else {
                                state = START;
                            }
                            break;
                        case BCC1:
                            if (byte == FLAG) {
                                state = STOP;
                                printf("reached!!!");
                            } else {
                                state = START;
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
        if(alarmCount==attempts && state!=STOP){return -1;}
    } else if (sp_config.role == Receptor) {
       
        while (state != STOP) {
            if (read(fd, &byte, 1) > 0) {
               
                switch (state) {
                    case START:
            
                        if (byte == FLAG) {
                            state = FLAG_RCV;
                            
                        }
                        break;
                    case FLAG_RCV:
                  
                        if (byte == A_FSENDER) {
                            state = A_RCV;
                        } else if (byte == FLAG) {
                            state = FLAG_RCV;
                        } else {
                            state = START;
                        }
                        break;
                    case A_RCV:
                    
                        if (byte == C_SET) {
                            state = C_RCV;
                        } else if (byte == FLAG) {
                            state = FLAG_RCV;
                        } else {
                            state = START;
                        }
                        break;
                    case C_RCV:
                   
                        if (byte == (C_SET ^ A_FSENDER)) {
                            state = BCC1;
                        } else if (byte == FLAG) {
                            state = FLAG_RCV;
                        } else {
                            state = START;
                        }
                        break;
                    case BCC1:
                 
                        if (byte == FLAG) {
                            state = STOP;
                            printf("Validated it all");
                        } else {
                            printf("BCC1");
                            state = START;
                        }
                       
                        break;
                    default:
                        break;
                }
            }
        }
        if(sendSupervisionFrame(A_FRECEIVER, C_UA)==-1){return -1;};
    }

    return 0;
}


int llwrite(unsigned char *buf, int bufSize){
    alarmCount=0;
    printf("Reached llwrite!");
    int frameSize = 6 + bufSize;
    unsigned char *frame = (unsigned char *)malloc(frameSize);
    frame[0] = FLAG;
    frame[1] = A_FSENDER;
    frame[2] = C_INF(info_frame_number_transmitter); 
    frame[3] = frame[1] ^ frame[2];
    alarmEnabled=FALSE;
    memcpy(frame + 4, buf, bufSize);
    unsigned char BCC2 = buf[0];
    printf("buf[0]: %x\n", buf[0]);
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
            frameSize= frameSize+1;
            frame = realloc(frame,frameSize);
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
    if(j==frameSize){printf("valid size");}
    int reject = 0;
    int accept = 0;
    //printf("AlarmEnabled: %d\n", alarmEnabled);
    /*
    for(int i=0;i<frameSize/2;i++){
        printf("frame[%d]: %x\n", i, frame[i]);
    }
    */
    while (alarmCount< attempts)
    {
        if(alarmEnabled==FALSE){
        
        alarm(timeout_);
        reject=0;
        accept=0; // só há write quando começa o timeout
        alarmEnabled=TRUE;
        }
      
      
        while (reject==0 && accept==0 && alarmEnabled==TRUE)
        {
            
            if(write(fd, frame, frameSize) == -1){
                printf("Error writing.\n");
            }
            
            unsigned char result = readControlByte();
            printf("Result: %x\n", result);
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
    printf("frameSize: %d\n", frameSize);
    free(frame);
    if(accept==1){
        
        return bufSize;
    }
    else{
        llclose();
        return -1;
    }
}
int llread(unsigned char *buf){
    printf("Reached llread!");
    unsigned char byte;
    char control_field;
    int data_byte_counter=0;
    
    LinkLayerState state = START;
    while (state!= STOP){
        if(read(fd, &byte,1) >0){
        //printf("Byte: %x\n",byte);
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
                    sendSupervisionFrame(A_FRECEIVER, C_DISC);
                    return 0;
                }
                else { state=START;}
                
                break;
            case C_RCV:                 
                if (byte== (A_FSENDER ^ control_field) ){
                    state= BCC1;
                }
                if(byte== FLAG){
                    state= FLAG_RCV;
                }
                break;
            case BCC1:
                state= READING_DATA;
                break;
            case READING_DATA:
                if (byte== ESC){ state= DATA_RECEIVED_ESC;}
                if (byte== FLAG){
                    
                    unsigned char bcc2 = buf[data_byte_counter-1];
                    data_byte_counter--;
                    buf[data_byte_counter]='\0';
                    unsigned acumulator = buf[0];
                    for (int j=1;j < data_byte_counter ;j++){
                        acumulator^=buf[j];
                    }

                    if(bcc2==acumulator){
                        printf("Sending RR\n");
                        state=STOP;
                        sendSupervisionFrame(A_FSENDER, C_RR(info_frame_number_receiver));
                        info_frame_number_receiver=(info_frame_number_receiver+1)%2;
                        printf("data_byte_counter: %d\n", data_byte_counter);
                        return data_byte_counter;
                    }
                    
                    else{
                        printf("Sending REJ\n");
                        sendSupervisionFrame(A_FSENDER, C_REJ(info_frame_number_receiver));
                        return -1;
                    }

                }
                else{
                    printf("Incrementing counter");
                    buf[data_byte_counter++]= byte;
                }
                break;
            case DATA_RECEIVED_ESC:
                state= READING_DATA;
                if(byte == 0X5E){
                    printf("Incrementing counter");
                    buf[data_byte_counter++]= FLAG;
                }
                if(byte == 0x5D){
                    printf("Incrementing counter");
                    buf[data_byte_counter++]= ESC;
                    
                }
                else{
                    printf("Cleaning counter");
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
    alarmCount=0;
    LinkLayerState state= START;
    unsigned char byte;
    (void) signal(SIGALRM,alarmHandler);
    printf("Reached llclose!");
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
                    break;
                case BCC1:
                    if(byte==FLAG){
                        state=STOP;
                    }
                    else{
                        state=START;
                    }
                    break;
                case STOP:
                      sendSupervisionFrame(A_FRECEIVER,C_UA);
                      if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
                    {
                            perror("tcsetattr");
                            return -1;
                    }

                     close(fd);
                     return 0;
                default:
                    break;
                }
                

            }
            }
            
        }
        return 0;
    }


unsigned char readControlByte(){

    unsigned char byte = 0;
    unsigned char control_byte = 0;
    LinkLayerState state_ = START;

    while (state_ != STOP && alarmEnabled == TRUE){ 
       
        if(read(fd, &byte, 1) > 0){
            printf("byte: %x\n", byte);

            switch (state_)
            {
            case START:
                if(byte == FLAG){
                    state_ = FLAG_RCV;
                }
                break;
            
            case FLAG_RCV:
                if(byte == A_FSENDER){
                    state_ = A_RCV;
                }
                else if(byte != FLAG){
                    state_ = START;
                }
                break;

            case A_RCV:
                if(byte == C_RR(0) || byte == C_RR(1) || byte == C_REJ(0) || byte == C_REJ(1) || byte == C_DISC){
                    state_ = C_RCV;
                    control_byte = byte;

                }
                else if (byte != FLAG) state_ = START;
                else{
                    state_ = FLAG_RCV;
                }
                break;
            case C_RCV:
                if(byte == (A_FSENDER ^ control_byte)){
                    state_ = BCC1;
                }
                else if(byte != FLAG){
                    state_ = START;
                }
                else{
                    state_ = FLAG_RCV;
                }
                break;
            case BCC1:
                if(byte == FLAG){
                    state_ = STOP;
                }
                else{
                    state_ = START;
                }
                break;
            default:
                break;
            }


        }
    }

    return control_byte;

}



