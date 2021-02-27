#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <conio.h>		// _getch
#include <locale.h>

#define _CHECKERROR	1
#include "CheckForError.h"

// casting para parâmetros de _beginthreadx

typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

/* define teclas para controle de bloqueio e desbloqueio de threads */

#define	ESC			    0x1B
#define	tecla_g			0x67
#define	tecla_c			0x63
#define	tecla_r			0x72
#define	tecla_p			0x70
#define	tecla_a			0x61
#define	tecla_l			0x6C

/* cria handles para eventos de toggle */

HANDLE leitura_medicao_toggle_event;
HANDLE leitura_dados_toggle_event;
HANDLE captura_mensagens_toggle_event;
HANDLE exibe_dados_toggle_event;
HANDLE analise_granulometria_toggle_event;

/* cria handles para eventos de ipc */

HANDLE exibe_dados_mailslot_event;
DWORD sent_bytes;

/* cria handles para semaforos para sincronizacao da lista circular 1 */

HANDLE sem_livre;
HANDLE sem_ocupado;
HANDLE sem_rw;

/* cria variaveis necessarias para sincronizacao */

const int buffer_size = 200;
int p_livre = 0;
int p_ocupado = 0;
int timeout = 100;

/* cria variaveis de processo */

int buffer[buffer_size];
int medicao_counter = 1;
int data_counter = 0;

/* cria evento de termino de processo */

HANDLE end_event;

/* declara funcoes para cada thread */

DWORD WINAPI leitura_medicao(LPVOID);
DWORD WINAPI leitura_dados(LPVOID);
DWORD WINAPI captura_mensagens(LPVOID);

char* show_time();

int main()
{
    // Habilitando acentuacão gráfica
    setlocale(LC_ALL, "Portuguese"); 

    /* declara variaveis necessarias para criacao processos secundarios */

    BOOL status;
    STARTUPINFO si_exibe_dados;
    STARTUPINFO si_analise_granulometria;
    PROCESS_INFORMATION NewProcess;

    /* define titulo para console principal */

    //SetConsoleTitle("Console Principal");

    /* define tamanho das estruturas em bytes */

    ZeroMemory(&si_exibe_dados, sizeof(si_exibe_dados));
    si_exibe_dados.cb = sizeof(si_exibe_dados);

    ZeroMemory(&si_analise_granulometria, sizeof(si_analise_granulometria));
    si_analise_granulometria.cb = sizeof(si_analise_granulometria);	// Tamanho da estrutura em bytes

    /* Cria semáforos para sincronização de escrita e leitura da primeira lista circular em memória */

    sem_livre = CreateSemaphore(NULL, buffer_size, buffer_size, NULL);

    sem_ocupado = CreateSemaphore(NULL, 0, buffer_size, NULL);

    sem_rw = CreateSemaphore(NULL, 1, 1, NULL);

    /* declara threads */

    HANDLE thread_leitura_medicao;
    DWORD thread_leitura_medicao_id;

    HANDLE thread_leitura_dados;
    DWORD thread_leitura_dados_id;

    HANDLE thread_captura_mensagens;
    DWORD thread_captura_mensagens_id;

    /* cria eventos para bloqueio e desbloqueio com reset automatico */

    leitura_dados_toggle_event = CreateEvent(NULL, FALSE, FALSE, NULL);

    leitura_medicao_toggle_event = CreateEvent(NULL, FALSE, FALSE, NULL);

    captura_mensagens_toggle_event = CreateEvent(NULL, FALSE, FALSE, NULL);

    // esse evento sao nomeados para compartilhamento interprocessos

    exibe_dados_toggle_event = CreateEvent(NULL, FALSE, FALSE, TEXT("exibe_dados_toggle_event")); 
    exibe_dados_mailslot_event = CreateEvent(NULL, FALSE, FALSE, TEXT("exibe_dados_mailslot_event"));
    analise_granulometria_toggle_event = CreateEvent(NULL, FALSE, FALSE, TEXT("analise_granulometria_toggle_event"));   

    /* cria evento com reset manual para encerramento */

    // esse evento eé nomeado para compartilhamento interprocessos
    end_event = CreateEvent(NULL, TRUE, FALSE, TEXT("end_event")); 
    
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

    /* Curiosidade: se o vetor acima for criado antes das atribuições das threads, a main não espera o término
     * das threads por meio da função WaitForMultipleObjects */

    if (thread_leitura_medicao) printf("thread leitura de medicao criada com id = %0x \n", thread_leitura_medicao_id);
    if (thread_leitura_dados) printf("thread leitura dados criada com id = %0x \n", thread_leitura_dados_id);
    if (thread_captura_mensagens) printf("thread captura mensagens criada com id = %0x \n", thread_captura_mensagens_id);

    /* Cria processo de exibição de dados */

    status = CreateProcess(
        "exibe_dados.exe",
        NULL,
        NULL,
        NULL,
        FALSE,
        CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        &si_exibe_dados,
        &NewProcess);

    if (!status) printf("Erro na criação do processo! Código = %d\n", GetLastError());

    /* Cria processo de análise de granulometria */

    status = CreateProcess(
        "analise_granulometria.exe",
        NULL,
        NULL,
        NULL,
        FALSE,
        CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        &si_analise_granulometria,
        &NewProcess);

    if (!status) printf("Erro na criação do processo! Código = %d\n", GetLastError());

    WaitForSingleObject(exibe_dados_mailslot_event, INFINITE);

    HANDLE mailslot = CreateFile(
                                 "\\\\.\\mailslot\\exibe_dados_mailslot",
                                GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                                );
                  
    CheckForError(mailslot != INVALID_HANDLE_VALUE);

    int signal = 40;
    printf("\n Enviando dados para tarefa de exibicao de processos");
    WriteFile(mailslot, &signal, sizeof(int), &sent_bytes, NULL);

    printf("\nBytes enviados para tarefa de exibicao de processos: %d\n", sent_bytes);

    char key;

    /* imprime na tecla opcoes de bloqueio */

    /* para bloquear as threads eh utilizado SetEvent, lembrando que o sinal eh consumido por ser do tipo resetautomatico */

    printf("\n___________________________________________________"
        "\n SISTEMA DE CONTROLE E SUPERVISÃO"
        "\n___________________________________________________"
        "\n Processo de pelotização e medição de granulometria"
        "\n..................................................."
        "\n Bloqueio/retomada da execução de tarefas:"
        "\n Tecla g: Tarefa de leitura de dados de medição"
        "\n Tecla c: Tarefa de leitura de dados de processo"
        "\n Tecla r: Tarefa de captura de mensagens"
        "\n Tecla p: Tarefa de exibição de dados de processo"
        "\n Tecla a: Tarefa de análise de granulometria"
        "\n..................................................."
        "\n Para encerrar o processo: ESC"
        "\n___________________________________________________\n");
        
    /* bloqueio/desbloqueido das threads caso criadas com resetautomatico */

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
        case tecla_p:
            SetEvent(exibe_dados_toggle_event);
            break;
        case tecla_a:
            SetEvent(analise_granulometria_toggle_event);
            break;
        }
    } while (key != ESC);

    /* como ESC foi inserido, sinaliza para finalizar threads */

    SetEvent(end_event);

    /* aguarda término de threads */

    WaitForMultipleObjects(3, threads, TRUE, INFINITE);

    /* destrói handles threads */

    CloseHandle(thread_leitura_medicao);
    CloseHandle(thread_leitura_dados);
    CloseHandle(thread_captura_mensagens);
    CloseHandle(leitura_medicao_toggle_event);
    CloseHandle(leitura_dados_toggle_event);
    CloseHandle(captura_mensagens_toggle_event);
    CloseHandle(end_event);

    printf("\n___________________________________________________"
        "\n              Processo encerrando."
        "\n___________________________________________________");

    return EXIT_SUCCESS;
}  // main



DWORD WINAPI leitura_medicao(LPVOID id)
{
    /* cria arrays para funçõeses WaitForMultipleObjects */

    /* para fazer o toggle desta thread */

    HANDLE Events[2] = { leitura_medicao_toggle_event, end_event };

    /* para permitir que essa thread encerre, se bloqueada ao tentar escrever no buffer */

    HANDLE buffer_block_objects[2] = { sem_livre, end_event };
    LONG previous_ocuppied_count = 0;
    DWORD ret;
    int event_id = 0;

    do {

        /* verifica se ha posicoes livres em buffer*/

        /* TODO: preencher valor de timeout. se nao ha no momento da verificacao, a thread aguarda por timeout milisegundos
         e se bloqueia por que o buffer estava cheio */
        
        /* if(previous_ocuppied_count == 9){ */

        /*     printf("\nThread leitora de medicao tentou depositar informacao mas buffer estava cheio. Se bloqueando ate livrar espaco\n"); */

        /*     ret = WaitForMultipleObjects(2, buffer_block_objects, FALSE, INFINITE); */

        /*     if (ret == 1) { break; } */

        /* } */
        
        ret = WaitForSingleObject(sem_livre, timeout);

        if (ret == WAIT_TIMEOUT) {
            /* se timeout, o buffer não estava com posições imediatamente livres e a thread se bloqueia */

            printf("\n************************************************************************************************************"
                "\nCapacidade máxima da primeira lista circular em memória atingida."
                "\nThread de leitura do sistema de medição tentou depositar informação e está se bloqueando até livrar posição."
                "\n************************************************************************************************************\n");

            ret = WaitForMultipleObjects(2, buffer_block_objects, FALSE, INFINITE);

            ret = ret - WAIT_OBJECT_0;

            if (ret == 1) { break; }
        }

        /* espera semáforo binário para acessar buffer */

        WaitForSingleObject(sem_rw, INFINITE);

        /* indexação circular */

        int index = p_livre % buffer_size;

        /* deposita informação e incrementa apontador para posição livre na lista circular */

        buffer[index] = medicao_counter;
        p_livre++;

        printf("\nThread leitora de medição de granulometria depositou informação %i em buffer[%i]\n", medicao_counter, index);

        medicao_counter += 2;

        /* libera semáforo binário de acesso ao buffer */

        ReleaseSemaphore(sem_rw, 1, NULL);

        ReleaseSemaphore(sem_ocupado, 1, &previous_ocuppied_count);

        /* TODO:adiciona valor de timout espera por .1 s por objeto de toggle ou finalizador
         como os sinais de toggle e finalizacao sao acionados pela funcao SetEvent, sempre que o sinal for
         dado esta thread vai na proxima execucao (ou no intervalo de timeout) detecta-lo e manifestar o comportamento
         desejado. a funcao na realidade mais confere do que espera, como mencionado no comentario anterior */

        Sleep(1000);
        ret = WaitForMultipleObjects(2, Events, FALSE, timeout);

        /* caso espere o tempo limite, prosseguir com lógica */

        if (ret == WAIT_TIMEOUT) {
            continue;
        }

        /* caso contrário, se bloqueia ou sai do laço para encerrar execução */

        event_id = ret - WAIT_OBJECT_0;

        if (event_id == 0) {

            printf("\n...................................................................................."
                    "\nTarefa LEITURA DO SISTEMA DE MEDIÇÃO *BLOQUEADA*. Para desbloquear, tecle <g>."
                    "\n....................................................................................\n");

            ret = WaitForMultipleObjects(2, Events, FALSE, INFINITE);

            event_id = ret - WAIT_OBJECT_0;

            if (event_id == 0) { 
                printf("\n...................................................................................."
                "\nTarefa LEITURA DO SISTEMA DE MEDIÇÃO *DESBLOQUEADA*."
                "\n....................................................................................\n"); }

        }

    } while (event_id != 1);

    printf("\nTarefa de leitura do sistema de medição encerrando.\n");

    /* se encerra */

    _endthreadex((DWORD)0);

    return(0);

}

DWORD WINAPI leitura_dados(LPVOID id)
{
    HANDLE Events[2] = { leitura_dados_toggle_event, end_event };
    HANDLE buffer_block_objects[2] = { sem_livre, end_event };
    LONG previous_ocuppied_count = 0;
    DWORD ret;

    int event_id = 0;

    do {

        /* char* time = show_time(); essa linha estava quebrando o codigo! investigar depois.*/

        /* espera ter posicoes livres */

        /* if(previous_ocuppied_count == 9){ */
        /*     printf("\nThread leitora de dados tentou depositar informacao mas buffer estava cheio. Se bloqueando ate livrar espaco\n"); */

        /*     ret = WaitForMultipleObjects(2, buffer_block_objects, FALSE, INFINITE); */

        /*     if (ret == 1) { break; } */

        /* } */

        ret = WaitForSingleObject(sem_livre, timeout);

        if (ret == WAIT_TIMEOUT) {
            printf("\n************************************************************************************************************"
                "\nCapacidade máxima da primeira lista circular em memória atingida."
                "\nThread de leitura de dados do processo tentou depositar informação e está se bloqueando até livrar posição."
                "\n************************************************************************************************************\n");
            ret = WaitForMultipleObjects(2, buffer_block_objects, FALSE, INFINITE);
            ret = ret - WAIT_OBJECT_0;
            if (ret == 1) { break; }

        }
        
        /* espera mutex para acessar buffer */
        WaitForSingleObject(sem_rw, INFINITE);

        int index = p_livre % buffer_size;
        buffer[index] = data_counter;
        p_livre++;

        printf("\nThread leitora de dados de processo depositou informação %i em buffer[%i]\n", data_counter, index);
        data_counter += 2;

        ReleaseSemaphore(sem_rw, 1, NULL);

        ReleaseSemaphore(sem_ocupado, 1, &previous_ocuppied_count);
        /* espera por 1 s por objeto de toggle ou finalizador */

        Sleep(1000);
        ret = WaitForMultipleObjects(2, Events, FALSE, timeout);

        /* caso espere o tempo limite, prosseguir com lógica */

        if (ret == WAIT_TIMEOUT) {
            continue;
        }

        /* caso contrário, executar comportamento demandado */

        event_id = ret - WAIT_OBJECT_0;
        if (event_id == 0) {

            printf("\n...................................................................................."
                    "\nTarefa LEITURA DE DADOS DO PROCESSO *BLOQUEADA*. Para desbloquear, tecle <c>."
                    "\n....................................................................................\n");

            ret = WaitForMultipleObjects(2, Events, FALSE, INFINITE);

            event_id = ret - WAIT_OBJECT_0;

            if (event_id == 0) { 
                printf("\n...................................................................................."
                "\nTarefa LEITURA DE DADOS DO PROCESSO *DESBLOQUEADA*."
                "\n....................................................................................\n"); }
        }

    } while (event_id != 1);

    printf("\nTarefa de leitura de dados de processo encerrando.\n");

    _endthreadex((DWORD)0);

    return(0);
}

DWORD WINAPI captura_mensagens(LPVOID id)
{
    HANDLE Events[2] = { captura_mensagens_toggle_event, end_event };
    HANDLE buffer_block_objects[2] = { sem_ocupado, end_event };
    DWORD ret;
    int event_id = 0;


    do {

        /* lógica padrão da thread */

        ret = WaitForSingleObject(sem_ocupado, timeout);

        if(ret == WAIT_TIMEOUT){
            ret = WaitForMultipleObjects(2, buffer_block_objects, FALSE, INFINITE);
            ret = ret - WAIT_OBJECT_0;
            if (ret == 1) { break; }
        }

        /* espera mutex para acessar buffer */
        WaitForSingleObject(sem_rw, INFINITE);

        int index = p_ocupado % buffer_size;
        int data = buffer[index];
        printf("\nThread capturadora de mensagens leu informação %i em buffer[%i]\n", data, index);
        p_ocupado++;

        ReleaseSemaphore(sem_rw, 1, NULL);

        ReleaseSemaphore(sem_livre, 1, NULL);

        /* espera por 1 s por objeto de toggle ou finalizador */

        Sleep(1000);
        ret = WaitForMultipleObjects(2, Events, FALSE, timeout);

        /* caso espere o tempo limite, prosseguir com lógica */

        if (ret == WAIT_TIMEOUT) {
            continue;
        }

        /* caso contrário, bloquear */

        event_id = ret - WAIT_OBJECT_0;
        if (event_id == 0) {

            printf("\n...................................................................................."
                "\nTarefa CAPTURA DE MENSAGENS *BLOQUEADA*. Para desbloquear, tecle <r>."
                "\n....................................................................................\n");

            ret = WaitForMultipleObjects(2, Events, FALSE, INFINITE);

            event_id = ret - WAIT_OBJECT_0;

            if (event_id == 0) { 
                printf("\n...................................................................................."
                    "\nTarefa CAPTURA DE MENSAGENS *DESBLOQUEADA*."
                    "\n....................................................................................\n"); }

        }

        /* enquanto Esc não for pressionado */

    } while (event_id != 1);

    printf("\nTarefa de captura de dados encerrando.\n");
    _endthreadex((DWORD)0);

    return(0);
}
