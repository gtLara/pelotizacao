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
    float gr_medio; 
    float gr_max; 
    float gr_min; 
    float sigma; 
    Timestamp time;

} Message;

Message create_message(int nseq, int id_disco, float gr_medio, float gr_max,
                       float gr_min, float sigma, Timestamp time, int tipo);

void show_message(Message msg);

#endif // MESSAGE_H_
