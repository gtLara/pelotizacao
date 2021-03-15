#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <conio.h>		// _getch
#include <locale.h>

#include "message.h"

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
                                          sizeof(Message)*buffer_2_size,   // quando a lista segurar mensagens esse campo deve ser reajustado
                                          "lista_2");


/* CheckForError(mapped_memory); */

Message * second_buffer_local = (Message*)MapViewOfFile(
                                                mapped_memory,
                                                FILE_MAP_WRITE,
                                                0,
                                                0,
                                                sizeof(Message) * buffer_2_size);

/* CheckForError(second_buffer_local); */

/* cria apontadores para posicoes livres e ocupadas */

HANDLE mapped_memory_p_ocupado = CreateFileMapping(
                                          (HANDLE)0xFFFFFFFF,
                                          NULL,
                                          PAGE_READWRITE,
                                          0,
                                          sizeof(int),
                                          "p_ocupado");

int second_p_ocupado_offset = sizeof(Message) * buffer_2_size + 100;

int second_p_ocupado = (int)MapViewOfFile(
                                            mapped_memory_p_ocupado,
                                            FILE_MAP_WRITE,
                                            0,
                                            second_p_ocupado_offset, // desclocamento para nao sobrepor a segunda lista
                                            sizeof(int));

HANDLE mapped_memory_p_livre = CreateFileMapping(
                                          (HANDLE)0xFFFFFFFF,
                                          NULL,
                                          PAGE_READWRITE,
                                          0,
                                          sizeof(int),
                                          "p_livre");

int second_p_livre_offset = sizeof(Message) * buffer_2_size + 200;

int second_p_livre = (int)MapViewOfFile(
                                            mapped_memory_p_livre,
                                            FILE_MAP_WRITE,
                                            0,
                                            second_p_livre_offset, // desclocamento para nao sobrepor a segunda lista nem p_ocupado
                                            sizeof(int));


/* cria variaveis de processo */

Message buffer[buffer_size];
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

    /* define tamanho das estruturas em bytes */

    ZeroMemory(&si_exibe_dados, sizeof(si_exibe_dados));
    si_exibe_dados.cb = sizeof(si_exibe_dados);

    ZeroMemory(&si_analise_granulometria, sizeof(si_analise_granulometria));
    si_analise_granulometria.cb = sizeof(si_analise_granulometria);	// Tamanho da estrutura em bytes

    /* Cria semáforos para sincronização de escrita e leitura da primeira lista circular em memória */

    sem_livre = CreateSemaphore(NULL, buffer_size, buffer_size, NULL);

    sem_ocupado = CreateSemaphore(NULL, 0, buffer_size, NULL);

    sem_rw = CreateSemaphore(NULL, 1, 1, NULL);

    /* Cria semáforos para sincronização de escrita e leitura da segund lista circular em memória
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
    analise_granulometria_toggle_event = CreateEvent(NULL, FALSE, FALSE, TEXT("analise_granulometria_toggle_event"));   

    /* evento de sinalizacao de criacao de mailslots deve ser manual porque sinaliza duas threads */

    exibe_dados_mailslot_event = CreateEvent(NULL, TRUE, FALSE, TEXT("exibe_dados_mailslot_event"));

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

    /* abre mailslot de limpa de tela de processo de exibicao de dados */

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
        "\n Tecla l: Limpar tela de monitor de exibição de dados de processo"
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
    int message_counter;

    do {

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

        /* cria mensagem */

        Message message;
        message.granulometria = generate_message_gran(message_counter);

        /* deposita informação e incrementa apontador para posição livre na lista circular */

        buffer[index] = message;
        p_livre++;

        show_message(message.granulometria);

        /* libera semáforo binário de acesso ao buffer */

        ReleaseSemaphore(sem_rw, 1, NULL);

        ReleaseSemaphore(sem_ocupado, 1, &previous_ocuppied_count);

        /* TODO: decidir se tempo ser arrendondado para segundos ou nao */

        int rand_timeout = rand() % 5;
        ret = WaitForMultipleObjects(2, Events, FALSE, rand_timeout*1000);

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
    int message_counter;

    do {

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

        /* criacao de mensagem */

        Message message;
        message.plc = generate_message_plc(message_counter);
        message.type = 99;

        /* deposito de mensagem em lista */

        buffer[index] = message;
        p_livre++;

        show_message(message.plc);

        ReleaseSemaphore(sem_rw, 1, NULL);

        ReleaseSemaphore(sem_ocupado, 1, &previous_ocuppied_count);
        /* espera por 1 s por objeto de toggle ou finalizador */

        int local_timeout = 500;
        ret = WaitForMultipleObjects(2, Events, FALSE, local_timeout);

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
    HANDLE second_buffer_block_objects[2] = { second_sem_livre, end_event };
    DWORD ret;
    int event_id = 0;

    int index;
    int second_index;

    /* verifica se mailslots foram criados */

    WaitForSingleObject(exibe_dados_mailslot_event, INFINITE);

    /* abre mailslot de envio de mensagem para processo de exibicao de dados */

    HANDLE message_mailslot = CreateFile(
                                 "\\\\.\\mailslot\\exibe_dados_mailslot_mensagem",
                                GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                                );
                  
    CheckForError(message_mailslot != INVALID_HANDLE_VALUE);

    do {

        /* lógica padrão da thread */

        /* acesso a primeira lista circular */ 

        ret = WaitForSingleObject(sem_ocupado, timeout);

        if(ret == WAIT_TIMEOUT){
            ret = WaitForMultipleObjects(2, buffer_block_objects, FALSE, INFINITE);
            ret = ret - WAIT_OBJECT_0;
            if (ret == 1) { break; }
        }

        /* espera mutex para acessar primeiro buffer em memoria */
        WaitForSingleObject(sem_rw, INFINITE);

        index = p_ocupado % buffer_size;

        /* consome dado de lista 1 */

        Message message;
        message = buffer[index];

        p_ocupado++;

        ReleaseSemaphore(sem_rw, 1, NULL);

        ReleaseSemaphore(sem_livre, 1, NULL);

        /* decisao : fazer tratamento de dados depois de leitura para nao aninhar semaforos */

        /* analisa dado consumido */

        if(message.type == 99){
            printf("\nEnviando mensagem para tarefa de exibe dados\n");
            WriteFile(message_mailslot, &message, sizeof(Message), NULL, NULL);
        }else{ 
            /* caso em que dados sao escritos na segunda lista. requer sincronizacao */
            /* printf("\nEnviando dado %i para segunda lista circular\n", data); */

            /* espera por posicoes livres na lista */

            ret = WaitForSingleObject(second_sem_livre, 100);

            if (ret == WAIT_TIMEOUT) {
                printf("\n************************************************************************************************************"
                    "\nCapacidade máxima da segunda lista circular em memória atingida."
                    "\nThread de captura de dados tentou depositar informação e está se bloqueando até livrar posição."
                    "\n************************************************************************************************************\n");
                ret = WaitForMultipleObjects(2, second_buffer_block_objects, FALSE, INFINITE);
                ret = ret - WAIT_OBJECT_0;
                if (ret == 1) { break; }

            }

            WaitForSingleObject(second_sem_rw, INFINITE);

            second_index = second_p_livre % buffer_2_size;

            second_buffer_local[second_index] = message;
            printf("\nThread capturadora de mensagens escreveu dado em memoria");
            second_p_livre++;

            ReleaseSemaphore(second_sem_rw, 1, NULL);
            ReleaseSemaphore(second_sem_ocupado, 1, NULL);
        };

        /* espera por 1 s por objeto de toggle ou finalizador */

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
