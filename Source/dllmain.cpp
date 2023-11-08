#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <iostream>
#include <TlHelp32.h>
//#include <math.h> math functions
#include "memoryTools.h"
#include "vectorStructs.h"

//https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes windows key codes

void UpdateConsole(bool, bool, bool, float, float);

DWORD playerAddress = 0;

int getPlayerHookLen = 5;
DWORD updateFuncAddress = 0;
DWORD getPlayerReturnAddress = 0;

void __declspec(naked) GetPlayerAddress()
{
	__asm
	{
		mov playerAddress, ecx // take the address from ecx

		// overwritten stuff
		push ebp
		mov ebp,esp
		push edi
		push esi

		jmp[getPlayerReturnAddress]
	}
}

bool hasSpawnedItem = false;
unsigned int itemId = 65;

int spawnItemHookLen = 5;
DWORD spawnItemReturnAddress = 0;

void __declspec(naked) SetSpawnItem()
{
	__asm
	{
		mov hasSpawnedItem, 1 // set true

		push itemId // the hook overwrites the original push

		jmp[spawnItemReturnAddress]
	}
}

DWORD WINAPI Thread(LPVOID param)
{
	AllocConsole(); //Create Console
	FILE* f;
	freopen_s(&f, "CONOUT$", "w", stdout);

	// print base text
	std::cout << "Key Binds: \n";
	std::cout << "N - Toggle No Clip: Off\n";
	std::cout << "H - Toggle Infinite Health: Off\n";
	std::cout << "J - Toggle Infinite Mana: Off\n";
	std::cout << "\nI - Spawn Current Item ID\n";
	std::cout << "L - Change Current Item ID (+1)\n";
	std::cout << "K - Change Current Item ID (-1)\n";
	std::cout << "P - Change Current Item ID (+100)\n";
	std::cout << "O - Change Current Item ID (-100)\n";
	std::cout << "ins - Unload DLL\n\n";
	std::cout << "X: 0\nY: 0\nItem ID: 65";
	std::cout << "\n\nCreated by Ass Monke#4906\n";

	DWORD moduleBase = (DWORD)GetModuleHandle(L"Terraria.exe");

	DWORD dryCollisionFuncAddress = FindArrayOfBytes(moduleBase, (BYTE*)"\x55\x8B\xEC\x57\x56\x53\x81\xEC\xE8\x00\x00\x00\x8B\xF1\x8D\xBD\x38\xFF\xFF\xFF\xB9\x2C\x00\x00\x00", 25); // this is used for NOPing later

	DWORD drawInterfaceCheckAddress = FindArrayOfBytes(moduleBase, (BYTE*)"\x55\x8B\xEC\x56\x83\xEC\x14\x33\xC0\x89\x45\xE8\x89\x45\xEC\x89\x45\xF0\x89\x45\xF4\x8B\xF1\x8B\xCE\xE8\xA2\xFF\xFF\xFF", 30) + 0x25;
	DWORD drawInventoryCheckAddress = FindArrayOfBytes(moduleBase, (BYTE*)"\x6A\x64\x6A\x64\x6A\x64\x8D\x8D\xE4\xFC\xFF\xFF\x8D\x52\x64", 15) + 0x112;
	DWORD rightClickCheckAddress = FindArrayOfBytes(moduleBase, (BYTE*)"\x75\x05\xE9\x15\x02\x00\x00\x85\xF6\x75\x0F", 11);
	DWORD tryOpenContainerCheckAddress = FindArrayOfBytes(moduleBase, (BYTE*)"\x8B\xCE\xBA\x15\x0C\x00\x00\x39\x09", 9) + 0x2F;
	DWORD openOysterPushAddress = FindArrayOfBytes(moduleBase, (BYTE*)"\x8B\xD8\x8B\x0B\xBA\x05\x00\x00\x00\x8B\x01\x8B\x40\x28\xFF\x50\x1C\x85\xC0\x75\x5A", 21) + 0x6F;

	spawnItemReturnAddress = openOysterPushAddress + spawnItemHookLen;

	updateFuncAddress = FindArrayOfBytes(moduleBase, (BYTE*)"\x55\x8B\xEC\x57\x56\x53\x81\xEC\x40\x0A\x00\x00", 12);
	getPlayerReturnAddress = updateFuncAddress + getPlayerHookLen;

	SetJmp((void*)updateFuncAddress, GetPlayerAddress, getPlayerHookLen, false);
	bool restoreHook = true;

	bool noClip = false;
	bool infHealth = false;
	bool infMana = false;
	float x=0, y=0;
	
	while (!GetAsyncKeyState(0x2D)) // exit when ins key is pressed
	{
		UpdateConsole(noClip, infHealth, infMana, x, y);

		if (playerAddress == 0) { continue; }
		else if(restoreHook) 
		{
			SetBytes((void*)updateFuncAddress, (BYTE*)"\x55\x8B\xEC\x57\x56", getPlayerHookLen); // restore the original instructions
			restoreHook = false;
		}

		if (GetAsyncKeyState(0x4E) & 1) // N
		{
			noClip = !noClip;

			if (noClip) { SetByte((void*)(dryCollisionFuncAddress + 0x8A1), 0x90, 4); } // nop the instruction writing to the Y axis, it is 4 bytes long
			else { SetBytes((void*)(dryCollisionFuncAddress + 0x8A1), (BYTE*)"\x66\x0F\xD6\x07", 4); } // restore
		}

		if (GetAsyncKeyState(0x4C) & 1) // L
		{
			itemId++;
		}
		if (itemId > 0 && GetAsyncKeyState(0x4B) & 1) // K
		{
			itemId--;
		}
		if (GetAsyncKeyState(0x50) & 1) // P
		{
			itemId += 100;
		}
		if (itemId > 100 && GetAsyncKeyState(0x4F) & 1) // O
		{
			itemId -= 100;
		}
		if (itemId > 5455) { itemId = 5455; } // max item ID

		if (!hasSpawnedItem && GetAsyncKeyState(0x49) & 1) // I
		{
			SetByte((void*)drawInterfaceCheckAddress, 0x90, 2); // je -> nop
			SetByte((void*)drawInventoryCheckAddress, 0x90, 6); // ja -> nop
			SetByte((void*)rightClickCheckAddress, 0xEB, 1); // jne -> jmp
			SetByte((void*)(rightClickCheckAddress + 0x3B), 0x90, 2); // je -> nop
			SetByte((void*)(rightClickCheckAddress + 0x44), 0x90, 6); // je -> nop
			SetByte((void*)tryOpenContainerCheckAddress, 0x90, 2); // jne -> nop

			SetByte((void*)(openOysterPushAddress - 0x5C), 0xEB, 1); // jne -> jmp

			SetJmp((void*)openOysterPushAddress, SetSpawnItem, spawnItemHookLen, false);
		}
		else if (hasSpawnedItem) // restore
		{
			SetBytes((void*)drawInterfaceCheckAddress, (BYTE*)"\x74\x5F", 2);
			SetBytes((void*)drawInventoryCheckAddress, (BYTE*)"\x0F\x87\x89\x01\x00\x00", 6);
			SetByte((void*)rightClickCheckAddress, 0x75, 1); // jmp -> jne
			SetBytes((void*)(rightClickCheckAddress + 0x3B), (BYTE*)"\x74\x1B", 2);
			SetBytes((void*)(rightClickCheckAddress + 0x44), (BYTE*)"\x0F\x84\xD2\x01\x00\x00", 6);
			SetBytes((void*)tryOpenContainerCheckAddress, (BYTE*)"\x75\x0E", 2);

			SetByte((void*)(openOysterPushAddress - 0x5C), 0x75, 1);

			SetBytes((void*)openOysterPushAddress, (BYTE*)"\x68\x3B\x11\x00\x00", 5);

			hasSpawnedItem = false;
		}

		if (GetAsyncKeyState(0x48) & 1) // H
		{
			infHealth = !infHealth;
		}
		if (infHealth) { *(float*)(playerAddress + 0x408) = 999; }

		if (GetAsyncKeyState(0x4A) & 1) // J
		{
			infMana = !infMana;
		}
		if (infMana) { *(float*)(playerAddress + 0x40C) = 999; }
		
		if (!noClip) 
		{ 
			x = *(float*)(playerAddress + 0x28);
			y = *(float*)(playerAddress + 0x2C);
			continue; 
		}
		if (GetAsyncKeyState(0x57)) // W
		{
			y--;
		}
		if (GetAsyncKeyState(0x41)) // A
		{
			x--;
		}
		if (GetAsyncKeyState(0x53)) // S
		{
			y++;
		}
		if (GetAsyncKeyState(0x44)) // D
		{
			x++;
		}

		*(float*)(playerAddress + 0x28) = x;
		*(float*)(playerAddress + 0x2C) = y;
	}
	
	fclose(f);
	FreeConsole();
	FreeLibraryAndExitThread((HMODULE)param, 0);
	return 0;
}

void UpdateConsole(bool noClip, bool infhealth, bool infMana, float x, float y)
{
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { 0, 1 });

	std::string noClipStr = noClip ? "On \n" : "Off\n";
	std::string infHealthStr = infhealth ? "On \n" : "Off\n";
	std::string infManaStr = infMana ? "On \n" : "Off\n";

	std::cout << "N - Toggle No Clip: " << noClipStr;
	std::cout << "H - Toggle Infinite Health: " << infHealthStr;
	std::cout << "J - Toggle Infinite Mana: " << infManaStr;
	
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { 0, 12 });

	std::cout << "X: " << x << "                \n";
	std::cout << "Y: " << y << "                \n";
	std::cout << "Item ID: " << std::dec << itemId << "                \n";
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD  dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		CreateThread(0, 0, Thread, hModule, 0, 0);
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}