#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<process.h>
#include<locale.h>
#include "message.h"

//#define _CHECKERROR     1
//#include "CheckForError.h"

/* funcao que limpa saida de console */

void clear_screen() {

    DWORD count;
    DWORD cell_count;

    COORD home_coords = { 0, 0 };

    HANDLE std_out;
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    std_out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (std_out == INVALID_HANDLE_VALUE) return;

    /* recupera numero de celular em buffer atual */

    if (!GetConsoleScreenBufferInfo(std_out, &csbi)) return;
    cell_count = csbi.dwSize.X * csbi.dwSize.Y;

    /* preenche o buffer inteiro com espacos */

    if (!FillConsoleOutputCharacter(
        std_out,
        (TCHAR)' ',
        cell_count,
        home_coords,
        &count
    )) return;

    SetConsoleCursorPosition(std_out, home_coords);

}


int main() {

    // Habilitando acentuac�o gr�fica
    setlocale(LC_ALL, "Portuguese");

    int event_id = -1;
    DWORD ret;
    HANDLE mailslot;
    HANDLE mailslot_mensagem;
    HANDLE end_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("end_event"));
    HANDLE toggle_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("exibe_dados_toggle_event"));
    HANDLE mailslot_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("exibe_dados_mailslot_event"));

    /* Cria uma mailslot para receber sinal de limpa de tela e outro para receber mensagem */

    printf("\nCriando mailslots");

    /* mailslot de sinal de limpa de tela */

    mailslot = CreateMailslot(
        L"\\\\.\\mailslot\\exibe_dados_mailslot",
        0,
        0, // se nao houver mensagem  prossegue com execucao
        NULL);

    //CheckForError(mailslot != INVALID_HANDLE_VALUE);

    /* mailslot de mensagem */

    mailslot_mensagem = CreateMailslot(
        L"\\\\.\\mailslot\\exibe_dados_mailslot_mensagem",
        0,
        0, // se nao houver mensagem  prossegue com execucao
        NULL);

    //CheckForError(mailslot_mensagem != INVALID_HANDLE_VALUE);

    // sinaliza para cliente que mailslots estao prontos

    SetEvent(mailslot_event);

    HANDLE events[2] = { toggle_event, end_event };

    printf("\nProcesso de exibe dados de processo disparado\n");

    Message message;
    int signal;

    do {

        /* Verifica se recebeu sinal de limpa de tela */

        signal = 0;

        bool status = ReadFile(mailslot, &signal, sizeof(int), NULL, NULL);

        if (signal) clear_screen();

        /* Verifica se recebeu mensagem */

        /* printf("\n Passando de recebimento de sinal de limpa \n"); */

        message.type = -1;

        status = ReadFile(mailslot_mensagem, &message, sizeof(Message), NULL, NULL);

        /* printf("\n Passando de recebimento de sinal de limpa \n"); */

        if (message.type != -1) show_message(message.plc);

        ret = WaitForMultipleObjects(2, events, FALSE, 100);

        if (ret == WAIT_TIMEOUT) {
            continue;
        }

        event_id = ret - WAIT_OBJECT_0;

        if (event_id == 0) {
            printf("\n...................................................................................."
                "\nProcesso EXIBI��O DE DADOS DE PROCESSO *BLOQUEADO*. Para desbloquear, tecle <p>."
                "\n....................................................................................\n");

            ret = WaitForMultipleObjects(2, events, FALSE, INFINITE);

            event_id = ret - WAIT_OBJECT_0;

            if (event_id == 0) {
                printf("\n...................................................................................."
                    "\nProcesso EXIBI��O DE DADOS DE PROCESSO *DESBLOQUEADO*."
                    "\n....................................................................................\n");
            }

        }

    } while (event_id != 1);

};
