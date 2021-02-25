#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<process.h>
#include<locale.h>

int main() {

    // Habilitando acentuacão gráfica
    setlocale(LC_ALL, "Portuguese");

    int event_id = -1;
    DWORD ret;
    HANDLE end_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("end_event"));
    HANDLE toggle_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("analise_granulometria_toggle_event"));

    printf("\nProcesso analise de granulometria disparado\n");

    HANDLE events[2] = { toggle_event, end_event };

    do {

        Sleep(1000);


        ret = WaitForMultipleObjects(2, events, FALSE, 100);

        if (ret == WAIT_TIMEOUT) {
            continue;
        }

        event_id = ret - WAIT_OBJECT_0;

        if (event_id == 0) {
            printf("\n...................................................................................."
                    "\nProcesso ANÁLISE DE GRANULOMETRIA *BLOQUEADO*. Para desbloquear, tecle <a>."
                    "\n....................................................................................\n");

            ret = WaitForMultipleObjects(2, events, FALSE, INFINITE);

            event_id = ret - WAIT_OBJECT_0;

            if (event_id == 0) { 
                printf("\n...................................................................................."
                    "\nProcesso ANÁLISE DE GRANULOMETRIA *DESBLOQUEADO*."
                    "\n....................................................................................\n"); }

        }

    } while (event_id != 1);

};
