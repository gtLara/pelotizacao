#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#include "message.h"

#define ESC 0x1b

SYSTEMTIME time;

int main(){

/* TODO: ver como acertar aleatoridade nas casas decimais */
    

    int nseq = -1;

    do{

    Sleep(500);
    int id_disco = (rand() % 2)+1;
    float gr_med = (rand() % 10000) ;
    float gr_max = (rand() % 10000) ;
    float gr_min = (rand() % 10000) ;
    float sigma = (rand() % 10000) ;
    Timestamp timestamp;

    GetLocalTime(&time);
    
    timestamp.hour = time.wHour;
    timestamp.minute = time.wMinute;
    timestamp.second = time.wSecond;

    nseq = ( nseq + 1 ) % 9999;
    Message message = create_message(nseq, id_disco, gr_med, gr_max, gr_min, sigma, timestamp, 0);
    show_message(message);

    }while(true);
}
