#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<process.h>

#define _CHECKERROR	1
#include "CheckForError.h"

/* funcao que limpa saida de console */

void clear_screen(){

  DWORD                      count;
  DWORD                      cell_count;

  COORD                      home_coords = { 0, 0 };

  HANDLE                     std_out;
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  std_out = GetStdHandle( STD_OUTPUT_HANDLE );
  if (std_out == INVALID_HANDLE_VALUE) return;

  /* recupera numero de celular em buffer atual */

  if (!GetConsoleScreenBufferInfo( std_out, &csbi )) return;
  cell_count = csbi.dwSize.X *csbi.dwSize.Y;

  /* preenche o buffer inteiro com espacos */

  if (!FillConsoleOutputCharacter(
    std_out,
    (TCHAR) ' ',
    cell_count,
    home_coords,
    &count
    )) return;

  /* if (!FillConsoleOutputAttribute( */
  /*   std_out, */
  /*   csbi.wAttributes, */
  /*   cell_count, */
  /*   home_coords, */
  /*   &count */
  /*   )) return; */

  /* move cursor */

  SetConsoleCursorPosition( std_out, home_coords );

}

int main(){

    int event_id = -1;
    DWORD ret;
    HANDLE mailslot;
    HANDLE end_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("end_event"));
    HANDLE toggle_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("exibe_dados_toggle_event"));
    HANDLE mailslot_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("exibe_dados_mailslot_event"));

    printf("\nCriando mailslot");

    mailslot = CreateMailslot(
                "\\\\.\\mailslot\\exibe_dados_mailslot",
                0,
                0, // se nao houver mensagem  prossegue com execucao
                NULL);

    CheckForError(mailslot != INVALID_HANDLE_VALUE);

    /* sinaliza para cliente que mailslot foi criado */

    SetEvent(mailslot_event);

    /* no momento esta retornando 0 */

    HANDLE events[2] = {toggle_event, end_event};


    do{

    Sleep(1000);

    int signal = 0;

    bool status = ReadFile(mailslot, &signal, sizeof(int), NULL, NULL);

    printf("\nSinal recebido:%i\n", signal);

    if(signal) clear_screen();

    ret = WaitForMultipleObjects(2, events, FALSE, 100);

    if(ret == WAIT_TIMEOUT){
        continue;
    }

    event_id = ret - WAIT_OBJECT_0;

    if(event_id == 0){
        printf("\nProcesso exibicao de dados de processo bloqueando\n");

        ret = WaitForMultipleObjects(2, events, FALSE, INFINITE);

        event_id = ret - WAIT_OBJECT_0;

        if(event_id == 0){printf("\nProcesso exibicao de dados desbloqueado\n");}

        }

    }while(event_id != 1);

    printf("Sinal de finalizacao recebido");

};

