#ifndef MESSAGE_H_
#define MESSAGE_H_

typedef struct{
    int hour;
    int minute;
    int second;
    int millisecond;
} Timestamp;

typedef struct{
    int tipo;
    int nseq;
    int id_disco;
    double gr_medio; 
    double gr_max; 
    double gr_min; 
    double sigma; 
    Timestamp time;

} Message;

Message create_message(int nseq, int id_disco, double gr_medio, double gr_max,
                       double gr_min, double sigma, Timestamp time, int tipo);

void show_message(Message msg);

#endif // MESSAGE_H_
