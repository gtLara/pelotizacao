#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <conio.h>		// _getch

#define _CHECKERROR	1		
#include "CheckForError.h"

// casting para parâmetros de _beginthreadx
// TODO: inicializar threads de forma mais "moderna"

typedef unsigned (WINAPI *CAST_FUNCTION)(LPVOID);
typedef unsigned *CAST_LPDWORD;

/* define teclas para controle de bloqueio de threads */

#define	ESC			0x1B
#define	tecla_g			0x67
#define	tecla_c			0x63
#define	tecla_r			0x72
#define	tecla_p			0x70
#define	tecla_a			0x61
#define	tecla_l			0x6C

/* cria handles para eventos de toggle da parte 1 */ 

HANDLE leitura_medicao_toggle_event;
HANDLE leitura_dados_toggle_event;
HANDLE captura_mensagens_toggle_event;

/* cria handles para semaforos necessarios */

HANDLE sem_livre;
HANDLE sem_ocupado;
HANDLE sem_rw;

/* cria variaveis necessarias para sincronizacao */

int buffer_size = 10;
int p_livre = 0;
int p_ocupado = 0;
int timeout = 10;

/* cria variaveis de processo */

int buffer[200];
int medicao_counter = 1;
int data_counter = 0;

/* cria evento de termino de threads */

HANDLE end_event;

/* declara funcoes para cada thread */

DWORD WINAPI leitura_medicao(LPVOID);
DWORD WINAPI leitura_dados(LPVOID);
DWORD WINAPI captura_mensagens(LPVOID);

char* show_time();

SYSTEMTIME tempo;

int main()
{

    /* criacao de semaforos */

    sem_livre = CreateSemaphore(NULL, buffer_size, buffer_size, NULL);
    CheckForError(sem_livre);

    sem_ocupado = CreateSemaphore(NULL, 0, buffer_size, NULL);
    CheckForError(sem_ocupado);

    sem_rw = CreateSemaphore(NULL, 1, 1, NULL);
    CheckForError(sem_rw);
    /* declaracao de threads */

	HANDLE thread_leitura_medicao;
	DWORD thread_leitura_medicao_id;

	HANDLE thread_leitura_dados;
	DWORD thread_leitura_dados_id;

	HANDLE thread_captura_mensagens;
	DWORD thread_captura_mensagens_id;

    HANDLE threads[3] = {thread_leitura_medicao, thread_leitura_dados, thread_captura_mensagens};

    /* eventos com reset automatico */

    leitura_dados_toggle_event = CreateEvent(NULL, FALSE, FALSE, NULL);  
	CheckForError(leitura_dados_toggle_event);

    leitura_medicao_toggle_event = CreateEvent(NULL, FALSE, FALSE, NULL);  
	CheckForError(leitura_medicao_toggle_event);

    captura_mensagens_toggle_event = CreateEvent(NULL, FALSE, FALSE, NULL);  
	CheckForError(captura_mensagens_toggle_event);

    /* evento com reset manual */

	end_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	CheckForError(end_event);

    /* cria threads */

	thread_leitura_medicao= (HANDLE) _beginthreadex(NULL, 0, (CAST_FUNCTION)leitura_medicao, 
                                            (LPVOID)0,
                                                    0,
                                            (CAST_LPDWORD)&thread_leitura_medicao_id
                                            );

    CheckForError(thread_leitura_medicao);

	thread_leitura_dados= (HANDLE) _beginthreadex(NULL, 0, (CAST_FUNCTION)leitura_dados, 
                                            (LPVOID)0,
                                                    0,
                                            (CAST_LPDWORD)&thread_leitura_dados_id	
                                            );

    CheckForError(thread_leitura_dados);

	thread_captura_mensagens = (HANDLE) _beginthreadex(NULL, 0, (CAST_FUNCTION)captura_mensagens, 
                                            (LPVOID)0,
                                                    0,
                                            (CAST_LPDWORD)&thread_captura_mensagens_id	
                                            );

    CheckForError(thread_captura_mensagens);
    
	if (thread_leitura_medicao) printf("thread leitura de medicao criada com id = %0x \n", thread_leitura_medicao_id);
	if (thread_leitura_dados) printf("thread leitura dados criada com id = %0x \n", thread_leitura_dados_id);
	if (thread_captura_mensagens) printf("thread captura mensagens criada com id = %0x \n", thread_captura_mensagens_id);

    char key;

	printf("\ntecla a: toggle da leitura de medicao\ntecla c: toggle leitura de dados\ntecla r: toggle captura\nesc para encerrar:\n");
	do {

        key = _getch();

        switch(key){
            case tecla_g:
                SetEvent(leitura_medicao_toggle_event);
                break;
            case tecla_c:
                SetEvent(leitura_dados_toggle_event);
                break;
            case tecla_r:
                SetEvent(captura_mensagens_toggle_event);
                break;
        }
	} while (key != ESC);
    
    /* como ESC foi inserido, sinaliza para finalizar threads */

    SetEvent(end_event);

    /* aguarda termino de threads */ 

    WaitForMultipleObjects(3, threads, FALSE, INFINITE);

	CloseHandle(thread_leitura_medicao);
	CloseHandle(thread_leitura_dados);
	CloseHandle(thread_captura_mensagens);
	CloseHandle(leitura_medicao_toggle_event);
	CloseHandle(leitura_dados_toggle_event);
	CloseHandle(captura_mensagens_toggle_event);
	CloseHandle(end_event);

	printf("\nprocesso encerrando\n");	

	return EXIT_SUCCESS;
}  // main



DWORD WINAPI leitura_medicao(LPVOID id)
{	
	HANDLE Events[2]= {leitura_medicao_toggle_event, end_event};
	DWORD ret;
	int event_id= 0;

	do {

        /* espera ter posicoes livres */
        WaitForSingleObject(sem_livre, INFINITE); 
        /* espera mutex para acessar buffer */
        WaitForSingleObject(sem_rw, INFINITE);
        /* circular */ 
        int index = p_livre % buffer_size;
        buffer[index] = medicao_counter;
        printf("\nThread leitora de medicao depositou informacao %i em buffer[%i]\n", medicao_counter, index);
        p_livre++;
        medicao_counter += 2;

        ReleaseSemaphore(sem_rw, 1, NULL);

        ReleaseSemaphore(sem_ocupado, 1, NULL);

        Sleep(1000);

        /* espera por 1 s por objeto de toggle ou finalizador */

		ret = WaitForMultipleObjects(2, Events, FALSE, timeout);

        /* se tiver esperado o tempo limite, prosseguir com logica */

        if(ret == WAIT_TIMEOUT){ 
            continue;
        }

        /* se nao, executar comportamento demandado */

		event_id = ret - WAIT_OBJECT_0;
		if (event_id == 0){

			printf("\n thread leitura granulometria bloqueada. para desbloquear, aperte uma outra tecla a\n");
            WaitForSingleObject(leitura_medicao_toggle_event, INFINITE);
			printf("\n thread leitura granulometria desbloqueada.\n");

		}

	} while (event_id != 1);
        
	_endthreadex((DWORD) 0);

	return(0);
} 

DWORD WINAPI leitura_dados(LPVOID id)
{	
    HANDLE Events[2]= {leitura_dados_toggle_event, end_event};
	DWORD ret;
	int event_id= 0;


	do {

        char* time = show_time();

        /* espera ter posicoes livres */
        WaitForSingleObject(sem_livre, INFINITE); 
        /* espera mutex para acessar buffer */
        WaitForSingleObject(sem_rw, INFINITE);
        /* implementacao de natureza circular de buffer */ 

        int index = p_ocupado % buffer_size;
        int data = buffer[index];
        buffer[index] = data_counter;

        printf("\nThread leitora de dados depositou informacao %i em buffer[%i]\n", data_counter, index);
        p_livre++;
        data_counter += 2;

        ReleaseSemaphore(sem_rw, 1, NULL);

        ReleaseSemaphore(sem_ocupado, 1, NULL);

        Sleep(1000);

        /* espera por intervalo de tempo objeto de toggle ou finalizador */

		ret=WaitForMultipleObjects(2, Events, FALSE, timeout);

        /* se tiver esperado o tempo limite, prosseguir com logica */

        if(ret == WAIT_TIMEOUT){ 
            continue;
        }

        /* se nao, executar comportamento demandado */

		event_id = ret - WAIT_OBJECT_0;
		if (event_id == 0){

			printf("\n thread leitura dados bloqueada. para desbloquear, aperte uma outra tecla c\n");
            WaitForSingleObject(leitura_dados_toggle_event, INFINITE);
			printf("\n thread leitura dados desbloqueada.\n");

		}

	} while (event_id != 1);
        
	_endthreadex((DWORD) 0);

	return(0);
} 

DWORD WINAPI captura_mensagens(LPVOID id)
{	
	HANDLE Events[2]= {captura_mensagens_toggle_event, end_event};
	DWORD ret;
	int event_id= 0;


	do {

        /* logica padrao da thread */

        WaitForSingleObject(sem_ocupado, INFINITE); 
        /* espera mutex para acessar buffer */
        WaitForSingleObject(sem_rw, INFINITE);
        /* lista circular */
        int index = p_ocupado % buffer_size;
        int data = buffer[index];
        printf("\nThread capturadora de mensagens leu informacao %i em buffer[%i]\n", data, index);
        p_ocupado++;

        ReleaseSemaphore(sem_rw, 1, NULL);

        ReleaseSemaphore(sem_livre, 1, NULL);

        /* espera por 1 s por objeto de toggle ou finalizador */
        Sleep(1000);

		ret=WaitForMultipleObjects(2, Events, FALSE, timeout);

        /* se tiver esperado o tempo limite, prosseguir com logica */

        if(ret == WAIT_TIMEOUT){ 
            continue;
        }

        /* se nao, bloquear */

		event_id = ret - WAIT_OBJECT_0;
		if (event_id == 0){

			printf("\n thread captura mensagens bloqueada. para desbloquear, aperte uma outra tecla r \n");
            WaitForSingleObject(captura_mensagens_toggle_event, INFINITE);
			printf("\n thread captura mensagens desbloqueada.\n");

		}

    /* enquanto Esc nao for pressionado */

	} while (event_id != 1); 
        
	_endthreadex((DWORD) 0);

	return(0);
} 

char* show_time()
{	
    char* time_string = (char*)malloc(sizeof(char) * 8);
	GetLocalTime(&tempo);
	sprintf(time_string, "\n %02d:%02d:%02d \n ", tempo.wHour, tempo.wMinute, tempo.wSecond);
    return time_string;
};
