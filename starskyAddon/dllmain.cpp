#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <string>
#include <Windows.h>
#include <Psapi.h>
#include <fstream>

#include "detours/detours.h"
#include "StarskyAddresses.h"

using std::cout;
using std::endl;
using std::string;

typedef void (*ActionFuncType)(int a, int type, float time);
ActionFuncType originalActionFunc;

typedef HANDLE (WINAPI *CreateFileAFuncType)(LPCSTR ,DWORD ,DWORD ,LPSECURITY_ATTRIBUTES ,DWORD ,DWORD ,HANDLE);
CreateFileAFuncType originalCreateFileA;

typedef BOOL (WINAPI *ReadFileFuncType)(HANDLE ,LPVOID ,DWORD ,LPDWORD ,LPOVERLAPPED);
ReadFileFuncType originalReadFile;

void CreateConsole()
{
	AllocConsole();

	DWORD dwMode;
	GetConsoleMode(GetConsoleWindow(), &dwMode);
	dwMode |= ENABLE_LINE_INPUT;
	dwMode |= ENABLE_INSERT_MODE | ENABLE_EXTENDED_FLAGS;
	//SetConsoleMode(GetConsoleWindow(), dwMode);
	
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	freopen("CONIN$", "r", stdin);
}

void ActivatePointsCheat()
{
	short* instr = (short*)GAME_ASM_POINTS_UPDATE;//This address maybe is incorrect. It bugs collision with dynamic objects

	BYTE newInstr[] = { 0x90,0x90 };

	WriteProcessMemory(GetCurrentProcess(), (void*)instr, &newInstr, 2, 0);

	float** points;
	points = (float**)GAME_POINTS_DPTR;

	**points = 1000.0;
	
	cout << "ActivatePointCheat()" << endl;
}

float actionFactor = 4.5;
int actionType = GameActionTypes::Turbo;

DWORD WINAPI keysLoop(void * data)
{
	while (true)
	{
		Sleep(100);
		if (GetForegroundWindow() != GetConsoleWindow())
		{
			if (GetAsyncKeyState('B') & 0x8000)
			{
				ActivatePointsCheat();
			}

			if (GetAsyncKeyState('T') & 0x8000)
			{
				int* strangeNumber = (int*)0x692450;
				originalActionFunc(*strangeNumber,actionType,actionFactor);
			}
		}
	}
}

DWORD WINAPI Input(void* data)
{
	while (true)
	{
		std::string input;
		std::getline(std::cin, input);

		std::string cmd (input.substr(0, input.find_first_of(" ")));
		std::string params(input.substr(input.find_first_of(" ")+1));

		if (cmd == "setactiontime")
		{
			actionFactor = std::stof(params);
			cout << "Set action time to "<<actionFactor << endl;
		}
		if (cmd == "setactiontype")
		{
			actionType = std::stoi(params);
			cout << "Set action type to " << actionType << endl;
		}
	}
}

/*
	Actyally this function is not for turbo only. It is multi-purpose.
	It can activate several actions, like turbo, siren..
*/
void overriddenActionFunc(int a, int type, float time)
{
	printf("Action(%d %d %f)\n", a, type, time);
	originalActionFunc(a, type, time);
}

HANDLE WINAPI overriddenCreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	cout << "Opening file: " << lpFileName << endl;
	return originalCreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

BOOL WINAPI overriddenReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)//TODO cannot read file name
{
	char stringa[40];
	memset(stringa, 0, 40);

	GetFileInformationByHandleEx(hFile, FileNameInfo, (LPVOID)stringa, 40);

	cout << "Read "<<(int)nNumberOfBytesToRead<<" bytes from: " << string(stringa) << endl;

	return originalReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
	{
		CreateConsole();

		printf("ADDON STARTED\n");

		originalCreateFileA = (CreateFileAFuncType)DetourFunction((PBYTE)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "CreateFileA"), (PBYTE)overriddenCreateFileA);
		originalReadFile = (ReadFileFuncType)DetourFunction((PBYTE)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "ReadFile"), (PBYTE)overriddenReadFile);
		
		HANDLE input = CreateThread(NULL, 0, Input, NULL, 0, NULL);

		HANDLE thread = CreateThread(NULL, 0, keysLoop, NULL, 0, NULL);

		originalActionFunc=(ActionFuncType)DetourFunction((PBYTE)GAME_ACTION_FUNC, (PBYTE)overriddenActionFunc);
		break;
	}
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
	{

	}
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
