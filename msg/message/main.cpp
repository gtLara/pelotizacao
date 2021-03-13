#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#include "message.h"

#define ESC 0x1b

SYSTEMTIME time;

int main(){

    int nseq = -1;
    int id_disco;
    double gr_med;
    double gr_max;
    double gr_min;
    double sigma;
    Timestamp timestamp;

    do{

    Sleep(500);
    id_disco = (rand() % 2)+1;
    gr_med = (rand() % 10000) ;
    gr_max = (rand() % 10000) ;
    gr_min = (rand() % 10000) ;
    sigma = (rand() % 10000) ;

    GetLocalTime(&time);
    
    timestamp.hour = time.wHour;
    timestamp.minute = time.wMinute;
    timestamp.second = time.wSecond;

    nseq = ( nseq + 1 ) % 10;
    Message message = create_message(nseq, id_disco, gr_med/100, gr_max/100, gr_min/100, sigma, timestamp, 0);
    show_message(message);

    }while(true);
}
