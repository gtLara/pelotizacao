#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#include "message.h"

#define ESC 0x1b

SYSTEMTIME time;

int main(){

    int nseq = -1;
    int id_disco_gran;
    int id_disco_plc;
    int gr1;
    int gr2;
    double gr_med;
    double gr_max;
    double gr_min;
    double sigma;
    double vz_entrada;
    double vz_saida;
    double velocidade;
    double potencia;
    double inclinacao;
    Timestamp timestamp;

    do{

    Sleep(500);

    id_disco_gran = (rand() % 2)+1;
    gr1 = (rand() % 10000);
    gr2 = (rand() % 10000);
    if (gr1 > gr2) {
        gr_max = gr1; gr_min = gr2;
    }
    else {
        gr_max = gr2; gr_min = gr1;
    }
    gr_med = (gr_min + gr_max)/2 ;
    sigma = (rand() % 10000) ;

    id_disco_plc = (rand() % 6)+1;
    vz_entrada = (rand() % 10000) ;
    vz_saida = (rand() % 10000) ;
    velocidade = (rand() % 10000) ;
    inclinacao = (rand() % 450) ;
    potencia = (rand() % 2000) ;


    GetLocalTime(&time);
    
    timestamp.hour = time.wHour;
    timestamp.minute = time.wMinute;
    timestamp.second = time.wSecond;
    timestamp.millisecond = time.wMilliseconds;

    nseq = ( nseq + 1 ) % 10;
    MessageGranulometria messagegran = create_message(nseq, id_disco_gran, gr_med/100, gr_max/100, gr_min/100, sigma, timestamp, 0);
    show_message(messagegran);

    MessagePLC messageplc = create_message(nseq, id_disco_plc, vz_entrada/10, vz_saida/10, velocidade/10, inclinacao/10, potencia/1000, timestamp, 99);
    show_message(messageplc);

    }while(true);
}
