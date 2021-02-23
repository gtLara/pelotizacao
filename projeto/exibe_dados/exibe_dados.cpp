#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<process.h>

int main() {

    int event_id = -1;
    DWORD ret;
    HANDLE end_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("end_event"));
    HANDLE toggle_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("exibe_dados_toggle_event"));

    HANDLE events[2] = { toggle_event, end_event };

    printf("\nProcesso de exibe dados de processo disparado\n");

    do {

        Sleep(1000);


        ret = WaitForMultipleObjects(2, events, FALSE, 100);

        if (ret == WAIT_TIMEOUT) {
            continue;
        }

        event_id = ret - WAIT_OBJECT_0;

        if (event_id == 0) {
            printf("\nProcesso exibicao de dados de processo bloqueando\n");

            ret = WaitForMultipleObjects(2, events, FALSE, INFINITE);

            event_id = ret - WAIT_OBJECT_0;

            if (event_id == 0) { printf("\nProcesso exibicao de dados de processo desbloqueado\n"); }

        }

    } while (event_id != 1);

};
