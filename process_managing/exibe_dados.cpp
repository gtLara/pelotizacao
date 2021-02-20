#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<process.h>

int main(){

    int event_id = -1;
    DWORD ret;
    HANDLE end_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("end_event"));
    HANDLE toggle_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("exibe_dados_toggle_event"));

    /* no momento esta retornando 0 */

    HANDLE events[2] = {toggle_event, end_event};

    do{

    Sleep(1000);

    printf("\nProcesso de exibe dados de processo\n");

    ret = WaitForMultipleObjects(2, events, FALSE, 100);

    if(ret == WAIT_TIMEOUT){
        continue;
    }

    event_id = ret - WAIT_OBJECT_0;

    if(event_id == 0){
        printf("\nProcesso exibicao de dados de processo bloqueando\n");

        ret = WaitForMultipleObjects(2, events, FALSE, INFINITE);

        event_id = ret - WAIT_OBJECT_0;

        if(event_id == 0){printf("\nProcesso exibicao de dados desbloqueado\n");}

        }

    }while(event_id != 1);

    printf("Sinal de finalizacao recebido");

};
