#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <conio.h>		// _getch

// casting para par�metros de _beginthreadx
// TODO: inicializar threads de forma mais "moderna"

typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

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

const int buffer_size = 10;
int p_livre = 0;
int p_ocupado = 0;
int timeout = 100;

/* cria variaveis de processo */

int buffer[buffer_size];
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

    sem_ocupado = CreateSemaphore(NULL, 0, buffer_size, NULL);

    sem_rw = CreateSemaphore(NULL, 1, 1, NULL);
    /* declaracao de threads */

    HANDLE thread_leitura_medicao;
    DWORD thread_leitura_medicao_id;

    HANDLE thread_leitura_dados;
    DWORD thread_leitura_dados_id;

    HANDLE thread_captura_mensagens;
    DWORD thread_captura_mensagens_id;

    /* eventos com reset automatico */

    leitura_dados_toggle_event = CreateEvent(NULL, FALSE, FALSE, NULL);

    leitura_medicao_toggle_event = CreateEvent(NULL, FALSE, FALSE, NULL);

    captura_mensagens_toggle_event = CreateEvent(NULL, FALSE, FALSE, NULL);

    /* evento com reset manual */

    end_event = CreateEvent(NULL, TRUE, FALSE, NULL);

    /* cria threads */

    thread_leitura_medicao = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)leitura_medicao,
        (LPVOID)0,
        0,
        (CAST_LPDWORD)&thread_leitura_medicao_id
    );

    thread_leitura_dados = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)leitura_dados,
        (LPVOID)0,
        0,
        (CAST_LPDWORD)&thread_leitura_dados_id
    );

    thread_captura_mensagens = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)captura_mensagens,
        (LPVOID)0,
        0,
        (CAST_LPDWORD)&thread_captura_mensagens_id
    );

    HANDLE threads[3] = { thread_leitura_medicao, thread_leitura_dados, thread_captura_mensagens };

    /* curioso: se o vetor acima for criado antes das atribuicoes das threads acima a main nao espera o termino */
    /* das threads por meio da funcao WaitForMultipleObjects */

    if (thread_leitura_medicao) printf("thread leitura de medicao criada com id = %0x \n", thread_leitura_medicao_id);
    if (thread_leitura_dados) printf("thread leitura dados criada com id = %0x \n", thread_leitura_dados_id);
    if (thread_captura_mensagens) printf("thread captura mensagens criada com id = %0x \n", thread_captura_mensagens_id);

    char key;

    printf("\ntecla g: leitura de medicao \n tecla c: leitura de dados \n tecla r: captura de mensagens \nesc para encerrar:\n");
    do {

        key = _getch();

        switch (key) {
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

    WaitForMultipleObjects(3, threads, TRUE, INFINITE);

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
    HANDLE Events[2] = { leitura_medicao_toggle_event, end_event };
    DWORD ret;
    int event_id = 0;

    do {

        /* espera ter posicoes livres */
        ret = WaitForSingleObject(sem_livre, timeout);

        if (ret == WAIT_TIMEOUT) {
            printf("\nThread leitora de medicao tentou depositar informacao mas buffer estava cheio. Se bloqueando ate livrar espaco\n");
            WaitForSingleObject(sem_livre, INFINITE);
        }

        /* espera mutex para acessar buffer */
        WaitForSingleObject(sem_rw, INFINITE);

        int index = p_livre % buffer_size;
        buffer[index] = medicao_counter;
        p_livre++;

        printf("\nThread leitora de medicao depositou informacao %i em buffer[%i]\n", medicao_counter, index);
        medicao_counter += 2;

        ReleaseSemaphore(sem_rw, 1, NULL);

        ReleaseSemaphore(sem_ocupado, 1, NULL);

        /* espera por 1 s por objeto de toggle ou finalizador */

        Sleep(1000);
        ret = WaitForMultipleObjects(2, Events, FALSE, timeout);

        /* se tiver esperado o tempo limite, prosseguir com logica */

        if (ret == WAIT_TIMEOUT) {
            continue;
        }

        /* se nao, executar comportamento demandado */

        event_id = ret - WAIT_OBJECT_0;
        if (event_id == 0) {

            printf("\n thread leitura granulometria bloqueada. para desbloquear, aperte uma outra tecla qualquer \n");
            WaitForSingleObject(leitura_medicao_toggle_event, INFINITE);
            printf("\n thread leitura granulometria desbloqueada.\n");

        }

    } while (event_id != 1);

    printf("\nprocesso leitura de medicao encerrando\n");

    _endthreadex((DWORD)0);

    return(0);
}

DWORD WINAPI leitura_dados(LPVOID id)
{
    HANDLE Events[2] = { leitura_dados_toggle_event, end_event };
    DWORD ret;
    int event_id = 0;


    do {

        /* char* time = show_time(); essa linha estava quebrando o codigo! investigar depois.*/

        /* espera ter posicoes livres */
        ret = WaitForSingleObject(sem_livre, timeout);

        if (ret == WAIT_TIMEOUT) {
            printf("\nThread leitora de dados tentou depositar informacao mas buffer estava cheio. Se bloqueando ate livrar espaco\n");
            WaitForSingleObject(sem_livre, INFINITE);
        }
        /* espera mutex para acessar buffer */
        WaitForSingleObject(sem_rw, INFINITE);

        int index = p_livre % buffer_size;
        buffer[index] = data_counter;
        p_livre++;

        printf("\nThread leitora de dados depositou informacao %i em buffer[%i]\n", data_counter, index);
        data_counter += 2;

        ReleaseSemaphore(sem_rw, 1, NULL);

        ReleaseSemaphore(sem_ocupado, 1, NULL);
        /* espera por 1 s por objeto de toggle ou finalizador */

        Sleep(1000);
        ret = WaitForMultipleObjects(2, Events, FALSE, timeout);

        /* se tiver esperado o tempo limite, prosseguir com logica */

        if (ret == WAIT_TIMEOUT) {
            continue;
        }

        /* se nao, executar comportamento demandado */

        event_id = ret - WAIT_OBJECT_0;
        if (event_id == 0) {

            printf("\n thread leitura dados bloqueada. para desbloquear, aperte uma outra tecla qualquer \n");
            WaitForSingleObject(leitura_dados_toggle_event, INFINITE);
            printf("\n thread leitura dados desbloqueada.\n");

        }

    } while (event_id != 1);

    printf("\nprocesso leitura de dados encerrando\n");

    _endthreadex((DWORD)0);

    return(0);
}

DWORD WINAPI captura_mensagens(LPVOID id)
{
    HANDLE Events[2] = { captura_mensagens_toggle_event, end_event };
    DWORD ret;
    int event_id = 0;


    do {

        /* logica padrao da thread */

        WaitForSingleObject(sem_ocupado, INFINITE);
        /* espera mutex para acessar buffer */
        WaitForSingleObject(sem_rw, INFINITE);

        int index = p_ocupado % buffer_size;
        int data = buffer[index];
        printf("\nThread capturadora de mensagens leu informacao %i em buffer[%i]\n", data, index);
        p_ocupado++;

        ReleaseSemaphore(sem_rw, 1, NULL);

        ReleaseSemaphore(sem_livre, 1, NULL);

        /* espera por 1 s por objeto de toggle ou finalizador */

        Sleep(1000);
        ret = WaitForMultipleObjects(2, Events, FALSE, timeout);

        /* se tiver esperado o tempo limite, prosseguir com logica */

        if (ret == WAIT_TIMEOUT) {
            continue;
        }

        /* se nao, bloquear */

        event_id = ret - WAIT_OBJECT_0;
        if (event_id == 0) {

            printf("\n thread captura mensagens bloqueada. para desbloquear, aperte uma outra tecla qualquer \n");
            WaitForSingleObject(captura_mensagens_toggle_event, INFINITE);
            printf("\n thread captura mensagens desbloqueada.\n");

        }

        /* enquanto Esc nao for pressionado */

    } while (event_id != 1);

    printf("\nprocesso captura de dados encerrando\n");
    _endthreadex((DWORD)0);

    return(0);
}