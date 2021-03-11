#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <conio.h>		// _getch
#include <locale.h>

#define _CHECKERROR	1
#include "CheckForError.h"

// casting para par�metros de _beginthreadx

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

/* cria handles para ipc */

HANDLE exibe_dados_mailslot_event;
DWORD sent_bytes;

/* cria handles para semaforos para sincronizacao da lista circular 1 */

HANDLE sem_livre;
HANDLE sem_ocupado;
HANDLE sem_rw;

/* cria handles para semaforos para sincronizacao da lista circular 2 */

HANDLE second_sem_livre;
HANDLE second_sem_ocupado;
HANDLE second_sem_rw;

/* cria variaveis necessarias para sincronizacao */

const int buffer_size = 200;
const int buffer_2_size = 100;
int p_livre = 0;
int p_ocupado = 0;
int timeout = 100;

/* cria variaveis mapeadas em memoria para sincronizacao da lista 2 */

/* cria segunda lista mapeada em memoria */

/* instancia a lista de forma global */

HANDLE mapped_memory = CreateFileMapping(
                                          (HANDLE)0xFFFFFFFF,
                                          NULL,
                                          PAGE_READWRITE,
                                          0,
                                          sizeof(int)*buffer_2_size,   // quando a lista segurar mensagens esse campo deve ser reajustado
                                          "lista_2");


/* CheckForError(mapped_memory); */

int * second_buffer_local = (int*)MapViewOfFile(
                                        mapped_memory,
                                        FILE_MAP_WRITE,
                                        0,
                                        0,
                                        sizeof(int) * buffer_2_size);

/* CheckForError(second_buffer_local); */

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
    // Habilitando acentuac�o gr�fica
    setlocale(LC_ALL, "Portuguese"); 

    /* declara variaveis necessarias para criacao processos secundarios */

    BOOL status;
    STARTUPINFO si_exibe_dados;
    STARTUPINFO si_analise_granulometria;
    PROCESS_INFORMATION NewProcess;

    /* define tamanho das estruturas em bytes */

    ZeroMemory(&si_exibe_dados, sizeof(si_exibe_dados));
    si_exibe_dados.cb = sizeof(si_exibe_dados);

    ZeroMemory(&si_analise_granulometria, sizeof(si_analise_granulometria));
    si_analise_granulometria.cb = sizeof(si_analise_granulometria);	// Tamanho da estrutura em bytes

    /* Cria sem�foros para sincroniza��o de escrita e leitura da primeira lista circular em mem�ria */

    sem_livre = CreateSemaphore(NULL, buffer_size, buffer_size, NULL);

    sem_ocupado = CreateSemaphore(NULL, 0, buffer_size, NULL);

    sem_rw = CreateSemaphore(NULL, 1, 1, NULL);

    /* Cria sem�foros para sincroniza��o de escrita e leitura da segund lista circular em mem�ria
     * observe que todos sao nomeados para compartilhamento interprocessos */

    second_sem_livre = CreateSemaphore(NULL, buffer_2_size, buffer_2_size, TEXT("sem_livre"));

    second_sem_ocupado = CreateSemaphore(NULL, 0, buffer_2_size, TEXT("sem_ocupado"));

    second_sem_rw = CreateSemaphore(NULL, 1, 1, TEXT("sem_rw"));



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

    end_event = CreateEvent(NULL, TRUE, FALSE, TEXT("end_event")); 

    /* cria segunda lista compartilhada em memoria */

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

    /* Curiosidade: se o vetor acima for criado antes das atribui��es das threads, a main n�o espera o t�rmino
     * das threads por meio da fun��o WaitForMultipleObjects */

    if (thread_leitura_medicao) printf("thread leitura de medicao criada com id = %0x \n", thread_leitura_medicao_id);
    if (thread_leitura_dados) printf("thread leitura dados criada com id = %0x \n", thread_leitura_dados_id);
    if (thread_captura_mensagens) printf("thread captura mensagens criada com id = %0x \n", thread_captura_mensagens_id);

    /* Cria processo de exibi��o de dados */

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

    if (!status) printf("Erro na cria��o do processo! C�digo = %d\n", GetLastError());

    /* Cria processo de an�lise de granulometria */

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

    if (!status) printf("Erro na cria��o do processo! C�digo = %d\n", GetLastError());

    WaitForSingleObject(exibe_dados_mailslot_event, INFINITE);

    /* abre mailslot de processe de exibicao de dados */

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

    char key;
    int exibicao_de_processos_clear_screen_signal = 1;

    /* imprime na tecla opcoes de bloqueio */

    printf("\n___________________________________________________"
        "\n SISTEMA DE CONTROLE E SUPERVIS�O"
        "\n___________________________________________________"
        "\n Processo de pelotiza��o e medi��o de granulometria"
        "\n..................................................."
        "\n Bloqueio/retomada da execu��o de tarefas:"
        "\n Tecla g: Tarefa de leitura de dados de medi��o"
        "\n Tecla c: Tarefa de leitura de dados de processo"
        "\n Tecla r: Tarefa de captura de mensagens"
        "\n Tecla p: Tarefa de exibi��o de dados de processo"
        "\n Tecla a: Tarefa de an�lise de granulometria"
        "\n..................................................."
        "\n Tecla l: Limpar tela de monitor de exibi��o de dados de processo"
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
        case tecla_l:
            WriteFile(mailslot, &exibicao_de_processos_clear_screen_signal, sizeof(int), &sent_bytes, NULL);
            break;
        }
    } while (key != ESC);

    /* como ESC foi inserido, sinaliza para finalizar threads */

    SetEvent(end_event);

    /* aguarda t�rmino de threads */

    WaitForMultipleObjects(3, threads, TRUE, INFINITE);

    /* destr�i handles threads */

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
    /* cria arrays para fun��eses WaitForMultipleObjects */

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
            /* se timeout, o buffer n�o estava com posi��es imediatamente livres e a thread se bloqueia */

            printf("\n************************************************************************************************************"
                "\nCapacidade m�xima da primeira lista circular em mem�ria atingida."
                "\nThread de leitura do sistema de medi��o tentou depositar informa��o e est� se bloqueando at� livrar posi��o."
                "\n************************************************************************************************************\n");

            ret = WaitForMultipleObjects(2, buffer_block_objects, FALSE, INFINITE);

            ret = ret - WAIT_OBJECT_0;

            if (ret == 1) { break; }
        }

        /* espera sem�foro bin�rio para acessar buffer */

        WaitForSingleObject(sem_rw, INFINITE);

        /* indexa��o circular */

        int index = p_livre % buffer_size;

        /* deposita informa��o e incrementa apontador para posi��o livre na lista circular */

        buffer[index] = medicao_counter;
        p_livre++;

        printf("\nThread leitora de medi��o de granulometria depositou informa��o %i em buffer[%i]\n", medicao_counter, index);

        medicao_counter += 2;

        /* libera sem�foro bin�rio de acesso ao buffer */

        ReleaseSemaphore(sem_rw, 1, NULL);

        ReleaseSemaphore(sem_ocupado, 1, &previous_ocuppied_count);

        /* TODO:adiciona valor de timout espera por .1 s por objeto de toggle ou finalizador
         como os sinais de toggle e finalizacao sao acionados pela funcao SetEvent, sempre que o sinal for
         dado esta thread vai na proxima execucao (ou no intervalo de timeout) detecta-lo e manifestar o comportamento
         desejado. a funcao na realidade mais confere do que espera, como mencionado no comentario anterior */

        Sleep(1000);
        ret = WaitForMultipleObjects(2, Events, FALSE, timeout);

        /* caso espere o tempo limite, prosseguir com l�gica */

        if (ret == WAIT_TIMEOUT) {
            continue;
        }

        /* caso contr�rio, se bloqueia ou sai do la�o para encerrar execu��o */

        event_id = ret - WAIT_OBJECT_0;

        if (event_id == 0) {

            printf("\n...................................................................................."
                    "\nTarefa LEITURA DO SISTEMA DE MEDI��O *BLOQUEADA*. Para desbloquear, tecle <g>."
                    "\n....................................................................................\n");

            ret = WaitForMultipleObjects(2, Events, FALSE, INFINITE);

            event_id = ret - WAIT_OBJECT_0;

            if (event_id == 0) { 
                printf("\n...................................................................................."
                "\nTarefa LEITURA DO SISTEMA DE MEDI��O *DESBLOQUEADA*."
                "\n....................................................................................\n"); }

        }

    } while (event_id != 1);

    printf("\nTarefa de leitura do sistema de medi��o encerrando.\n");

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
                "\nCapacidade m�xima da primeira lista circular em mem�ria atingida."
                "\nThread de leitura de dados do processo tentou depositar informa��o e est� se bloqueando at� livrar posi��o."
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

        printf("\nThread leitora de dados de processo depositou informa��o %i em buffer[%i]\n", data_counter, index);
        data_counter += 2;

        ReleaseSemaphore(sem_rw, 1, NULL);

        ReleaseSemaphore(sem_ocupado, 1, &previous_ocuppied_count);
        /* espera por 1 s por objeto de toggle ou finalizador */

        Sleep(1000);
        ret = WaitForMultipleObjects(2, Events, FALSE, timeout);

        /* caso espere o tempo limite, prosseguir com l�gica */

        if (ret == WAIT_TIMEOUT) {
            continue;
        }

        /* caso contr�rio, executar comportamento demandado */

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

    int second_list_index = 0;

    do {

        /* l�gica padr�o da thread */

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


        if(data % 2 == 0){
            printf("\nThread capturadora de mensagens leu informa��o %i em buffer[%i]\n", data, index);
        }else{
            printf("\nThread capturadora de mensagens escreveu informa��o %i em buffer em memoria[%i]\n", data, second_list_index);
            second_buffer_local[second_list_index] = data;
            second_list_index++;
        };

        p_ocupado++;

        ReleaseSemaphore(sem_rw, 1, NULL);

        ReleaseSemaphore(sem_livre, 1, NULL);

        /* espera por 1 s por objeto de toggle ou finalizador */

        Sleep(1000);
        ret = WaitForMultipleObjects(2, Events, FALSE, timeout);

        /* caso espere o tempo limite, prosseguir com l�gica */

        if (ret == WAIT_TIMEOUT) {
            continue;
        }

        /* caso contr�rio, bloquear */

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

        /* enquanto Esc n�o for pressionado */

    } while (event_id != 1);

    printf("\nTarefa de captura de dados encerrando.\n");
    _endthreadex((DWORD)0);

    return(0);
}
