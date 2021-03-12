#include "message.h"
#include <stdio.h>

Message create_message(int nseq, int id_disco, float gr_medio, float gr_max,
                       float gr_min, float sigma, Timestamp time, int tipo=0){

    Message message;
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

void show_message(Message msg){
    printf("\n%i;%i;%i;%.2f;%.2f;%.2f;%.2f;%i:%i:%i\n", 
                        msg.tipo, msg.nseq, msg.id_disco, msg.gr_medio,
                        msg.gr_max, msg.gr_min, msg.sigma, msg.time.hour,
                        msg.time.minute, msg.time.second);
}
