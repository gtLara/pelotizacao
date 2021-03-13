#include "message.h"
#include <stdio.h>

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

void show_message(MessageGranulometria msg){
    printf("\n%i;%i;%i;%.2f;%.2f;%.2f;%.2f;%i:%i:%i\n", 
                        msg.tipo, msg.nseq, msg.id_disco, msg.gr_medio,
                        msg.gr_max, msg.gr_min, msg.sigma, msg.time.hour,
                        msg.time.minute, msg.time.second);
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

void show_message(MessagePLC msg){
    printf("\n%i;%i;%i;%.1f;%.1f;%.1f;%.1f;%.1f;%i:%i:%i.%i\n", 
                        msg.tipo, msg.nseq, msg.id_disco, msg.vz_entrada,
                        msg.vz_saida, msg.velocidade, msg.inclinacao,
                        msg.potencia,msg.time.hour, msg.time.minute,
                        msg.time.second, msg.time.millisecond);
}
