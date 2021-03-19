#include "message.h"
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>

MessageGranulometria create_message(int nseq, int id_disco, double gr_medio, double gr_max,
                                     double gr_min, double sigma, Timestamp time, int tipo=0){
    MessageGranulometria message;
    message.tipo = tipo;
    message.nseq = nseq;
    message.id_disco = id_disco;
    message.gr_medio = gr_medio;
    message.gr_max = gr_max;
    message.gr_min = gr_min;
    message.sigma = sigma;
    message.time = time;
    return message;

}

MessageGranulometria generate_message_gran(int &nseq){

    int id_disco_gran;
    int gr1;
    int gr2;
    double gr_med;
    double gr_max;
    double gr_min;
    double sigma;
    Timestamp timestamp;

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

    SYSTEMTIME time;

    GetLocalTime(&time);
    
    timestamp.hour = time.wHour;
    timestamp.minute = time.wMinute;
    timestamp.second = time.wSecond;
    timestamp.millisecond = time.wMilliseconds;

    nseq = ( nseq + 1 ) % 9999;
    MessageGranulometria messagegran = create_message(nseq, id_disco_gran, gr_med/100, gr_max/100, gr_min/100, sigma, timestamp, 0);
    
    return messagegran;

}

void show_message(MessageGranulometria msg){
    printf("%i:%i:%i NSEQ: %i   ID: %i  GMED: %.2f  GMAX: %.2f  GMIN: %.2f  SIG: %.2f\n",
                        msg.time.hour, msg.time.minute, msg.time.second,
                        msg.nseq, msg.id_disco, msg.gr_medio,
                        msg.gr_max, msg.gr_min, msg.sigma);
}

MessagePLC create_message(int nseq, int id_disco, double vz_entrada, double vz_saida,
                                     double velocidade, double inclinacao, double potencia,
                                     Timestamp time, int tipo=0){
    MessagePLC message;
    message.tipo = tipo;
    message.nseq = nseq;
    message.id_disco = id_disco;
    message.vz_entrada = vz_entrada;
    message.vz_saida = vz_saida;
    message.velocidade = velocidade;
    message.inclinacao = inclinacao;
    message.potencia = potencia;
    message.time = time;
    return message;

}

MessagePLC generate_message_plc(int &nseq){

    int id_disco_gran;
    int id_disco_plc;
    double vz_entrada;
    double vz_saida;
    double velocidade;
    double potencia;
    double inclinacao;
    SYSTEMTIME time;
    Timestamp timestamp;

    id_disco_gran = (rand() % 2)+1;
    id_disco_plc = (rand() % 6)+1;
    vz_entrada = (rand() % 10000) ;
    vz_saida = (rand() % 10000) ;
    velocidade = (rand() % 10000) ;
    inclinacao = (rand() % 450) ;
    potencia = (rand() % 5000) ;

    GetLocalTime(&time);
    
    timestamp.hour = time.wHour;
    timestamp.minute = time.wMinute;
    timestamp.second = time.wSecond;
    timestamp.millisecond = time.wMilliseconds;

    nseq = ( nseq + 1 ) % 9999;

    MessagePLC messageplc = create_message(nseq, id_disco_plc, vz_entrada/10, vz_saida/10, velocidade/10, inclinacao/10, potencia/1000, timestamp, 99);

    return messageplc;

}

void show_message(MessagePLC msg){

    printf("%i:%i:%i:%i NSEQ: %i   ID: %i  VZ E: %.1f  VZ S: %.1f  V: %.1f  ANG: %.1f  P: %.1f\n",
            msg.time.hour, msg.time.minute, msg.time.second, msg.time.millisecond,
            msg.nseq, msg.id_disco,  msg.vz_entrada, msg.vz_saida,
            msg.velocidade, msg.inclinacao, msg.potencia);
}
