#ifndef MESSAGE_H_
#define MESSAGE_H_

typedef struct{
    int hour;
    int minute;
    int second;
    int millisecond;
} Timestamp;

/* Definicao de mensagens da leitura de granulometria */

typedef struct{
    int tipo;
    int nseq;
    int id_disco;
    double gr_medio; 
    double gr_max; 
    double gr_min; 
    double sigma; 
    Timestamp time;

} MessageGranulometria;

MessageGranulometria create_message(int nseq, int id_disco, double gr_medio, double gr_max,
                                    double gr_min, double sigma, Timestamp time, int tipo);

MessageGranulometria generate_message_gran(int &nseq);

void show_message(MessageGranulometria msg);

/* Definicao de mensagens da leitura de PLC */

typedef struct{
    int tipo;
    int nseq;
    int id_disco;
    double vz_entrada; 
    double vz_saida; 
    double velocidade; 
    double inclinacao; 
    double potencia; 
    Timestamp time;

} MessagePLC;

MessagePLC create_message(int nseq, int id_disco, double vz_entrada, double vz_saida,
                                    double velocidade, double inclinacao, double potencia, 
                                    Timestamp time, int tipo);

MessagePLC generate_message_plc(int &nseq);

void show_message(MessagePLC msg);

/* Definicao de meta mensagem */

typedef struct{
    MessagePLC plc;
    MessageGranulometria granulometria;
    int type;
} Message;

#endif // MESSAGE_H_
