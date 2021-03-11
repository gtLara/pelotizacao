#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<process.h>
#include<locale.h>

#define _CHECKERROR	1
#include "CheckForError.h"

int main() {

    // Habilitando acentuacão gráfica
    setlocale(LC_ALL, "Portuguese");

    int event_id = -1;
    int buffer_2_size = 3;
    DWORD ret;

    /* abre eventos para pausa e finalizacao */

    HANDLE end_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("end_event"));
    HANDLE toggle_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("analise_granulometria_toggle_event"));

    /* abre semaforos para sincronizacao da lista 2 */

    HANDLE sem_livre = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, TEXT("sem_livre"));
    HANDLE sem_ocupado = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, TEXT("sem_ocupado"));
    HANDLE sem_rw = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, TEXT("sem_rw"));


    HANDLE mapped_memory = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "lista_2"); // abre segunda lista em memoria
    /* CheckForError(mapped_memory); */

    int* second_buffer_local = (int*)MapViewOfFile(
                                                    mapped_memory,
                                                    FILE_MAP_WRITE,
                                                    0,
                                                    0,
                                                    sizeof(int) * buffer_2_size);

    /* CheckForError(second_buffer_local); */

    printf("\nProcesso analise de granulometria disparado\n");

    HANDLE events[2] = { toggle_event, end_event };

    int dummy_counter = 0;
    int recovered_data;

    do {

        /* printf("\n Entrando em loop \n"); */

        Sleep(3000);

        WaitForSingleObject(sem_ocupado, INFINITE);
        WaitForSingleObject(sem_rw, INFINITE);

        printf("\n Carregando dado de posicao %i de buffer em memoria \n", dummy_counter);
        
        recovered_data = second_buffer_local[dummy_counter];
        dummy_counter++;

        printf("\nDado recuperado: %i\n", recovered_data);

        ReleaseSemaphore(sem_rw, 1, NULL);
        ReleaseSemaphore(sem_livre, 1, NULL);

        ret = WaitForMultipleObjects(2, events, FALSE, 100);

        if (ret == WAIT_TIMEOUT) {  // o codigo desse ponto para frente so eh executado se houver sinalizacao de bloqueio ou termino
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
