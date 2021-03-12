//
//	Programação Multithreaded em ambiente Windows NT© - uma visão de  Automação
// 
//	Autores: Constantino Seixas Filho/ Marcelo Szuster
//
//	Capítulo 2 - CheckForError.h
//
//	Versão: 1.1	20/01/1999
//

#pragma comment( lib, "USER32" )

#define CheckForError(ret) if (!(ret)) CheckReturn(__FILE__, __LINE__)

__inline void CheckReturn(LPSTR FileName, int Line)
{
#ifdef _CHECKERROR
	LPSTR	lpMsgBuff;
	char	OutBuffer[256];

	// Traduz retorno de GetLastError em um string !!!
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
				| FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				GetLastError(),
				LANG_NEUTRAL,
				(LPTSTR)&lpMsgBuff,  // Aloca o buffer dinamicamente
				0,
				NULL);
	sprintf(OutBuffer, "\nErro: %s Arquivo %s Linha %d\n",lpMsgBuff, FileName, Line);
#ifndef	_WINDOWS // Modo Console
	printf("%s", OutBuffer);
#else			 // Modo Windows
	MessageBox(NULL, OutBuffer, "ERRO", MB_ICONWARNING|MB_OK|MB_TASKMODAL|MB_SETFOREGROUND);
#endif
	exit(EXIT_FAILURE);
#endif

}

