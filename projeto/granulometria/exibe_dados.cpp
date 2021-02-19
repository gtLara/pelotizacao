#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<process.h>

void exibe_dados() {

    /*int event_id;*/
    DWORD ret;
    HANDLE end_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("end_event"));
    /* toggle_event = OpenEvent(EVENT_ALL_ACCESS, TURE, TEXT("exibe_dados_toggle_event"); */
    /* HANDLE events[2] = {toggle_event, end_event}; */

    do {

        Sleep(1000);

        printf("\nProcesso de exibe dados de processo\n");

        /* throwaway */

        ret = WaitForSingleObject(end_event, INFINITE);
        if (ret == WAIT_TIMEOUT) {
            printf("\n Timeout\n");
        }

        /* ret = WaitForMultipleObjects(2, events, FALSE, timeout); */

        /* if(ret == WAIT_TIMEOUT){ */
        /*     continue; */
        /* } */

        /* event_id = ret - WAIT_OBJECT_0; */

        /* if(event_id == 0){ */
        /*     prinft("\nProcesso exibicao de dados de processo bloqueando\n"); */
        /*     WaitForSingleObject(toggle_event, INFINITE); */
        /* } */

    } while (true);
};
