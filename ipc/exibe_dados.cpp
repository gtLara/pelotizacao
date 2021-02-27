#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<process.h>

#define _CHECKERROR	1
#include "CheckForError.h"

int main(){

    int event_id = -1;
    DWORD ret;
    HANDLE mailslot;
    HANDLE end_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("end_event"));
    HANDLE toggle_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("exibe_dados_toggle_event"));
    HANDLE mailslot_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("exibe_dados_mailslot_event"));
    int signal = 0;

    printf("\nCriando mailslot");

    mailslot = CreateMailslot(
                "\\\\.\\mailslot\\exibe_dados_mailslot",
                0,
                MAILSLOT_WAIT_FOREVER,
                NULL);

    CheckForError(mailslot != INVALID_HANDLE_VALUE);

    /* sinaliza para cliente que mailslot foi criado */

    SetEvent(mailslot_event);

    /* no momento esta retornando 0 */

    HANDLE events[2] = {toggle_event, end_event};

    bool status = ReadFile(mailslot, &signal, sizeof(int), NULL, NULL);

    printf("\nSinal recebido:%i\n", signal);

    do{

    Sleep(1000);


    CheckForError(status);

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
