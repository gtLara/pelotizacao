#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <conio.h>		// _getch

/* #define _CHECKERROR	1 */		
/* #include "CheckForError.h" */

// casting para parâmetros de _beginthreadx
// TODO: inicializar threads de forma mais "moderna"

typedef unsigned (WINAPI *CAST_FUNCTION)(LPVOID);
typedef unsigned *CAST_LPDWORD;

#define	ESC			0x1B

HANDLE toggle_event;
HANDLE end_event;

DWORD WINAPI toggle_experiment(LPVOID);
void show_time();

SYSTEMTIME tempo;

int main()
{
	HANDLE example_thread;
	
	DWORD thread_id;
	DWORD thread_exit_code = 0;

    /* evento com reset automatico */

    toggle_event = CreateEvent(NULL, FALSE, FALSE, NULL);  
	/* CheckForError(example_event); */

    /* evento com reset manual */

	end_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	/* CheckForError(end_event); */

	example_thread = (HANDLE) _beginthreadex(NULL, 0, (CAST_FUNCTION)toggle_experiment, 
                                            (LPVOID)0,
                                                    0,
                                            (CAST_LPDWORD)&thread_id	
                                            );

	if (example_thread) printf("thread exemplo criada com id = %0x \n", thread_id);

    char key;

	printf("\nqualquer tecla para fazer o toggle, esc para encerrar:\n");
	do {
        key = _getch();
		if (key != ESC) SetEvent(toggle_event); // sinaliza para thread bloquear ou desbloquear
		else SetEvent(end_event); 
	} while (key != ESC);

	GetExitCodeThread(example_thread, &thread_exit_code);
	CloseHandle(example_thread);
	CloseHandle(toggle_event);
	CloseHandle(end_event);

	printf("\nthread encerrando\n");	

    /* TODO: ver como fazer main esperar por termino de thread. aqui isso nao parece ter acontecido. */

	return EXIT_SUCCESS;
}  // main



DWORD WINAPI toggle_experiment(LPVOID id)
{	
	HANDLE Events[2]= {toggle_event, end_event};
	DWORD ret;
	int event_id= 0;


	do {

        show_time();

        /* espera por 1 s por objeto de toggle ou finalizador */

		ret=WaitForMultipleObjects(2, Events, FALSE, 1000);

        /* se tiver esperado o tempo limite, prosseguir com logica */

        if(ret == WAIT_TIMEOUT){ 
            continue;
        }

        /* se nao, executar comportamento demandado */

		event_id = ret - WAIT_OBJECT_0;
		if (event_id == 0){

			printf("\n thread bloqueada. para desbloquear, aperte uma outra tecla qualquer \n");
            WaitForSingleObject(toggle_event, INFINITE);
			printf("\n thread desbloqueada.\n");

		}

	} while (event_id != 1);
        
	_endthreadex((DWORD) 0);

	return(0);
} 

void show_time()
{	

	GetLocalTime(&tempo);
	printf("\n %02d:%02d:%02d \n ", tempo.wHour, tempo.wMinute, tempo.wSecond);

};
