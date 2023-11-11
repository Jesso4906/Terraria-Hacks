#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "memoryTools.h"

void SetBytes(void* dst, BYTE* bytes, unsigned int size)
{
	DWORD old;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &old); // Get permission
	memcpy(dst, bytes, size); // Overwrite after dst with the set of bytes
	VirtualProtect(dst, size, old, &old);
}

void SetByte(void* dst, BYTE byte, unsigned int n)
{
	DWORD old;
	VirtualProtect(dst, n, PAGE_EXECUTE_READWRITE, &old); // Get permission
	memset(dst, byte, n); // Overwrite the next n bytes with byte
	VirtualProtect(dst, n, old, &old);
}

void SetJmp(void* src, void* dst, unsigned int len, bool x64)
{
	DWORD old;
	VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &old); // Get permission

	memset(src, 0x90, len); // Incase len is larger than the length of the jmp instruction the left over bytes will be replaced with NOP

	int size = 5;
	if (x64) { size = 9; }

	*(BYTE*)src = 0xE9; // Set jump instructoin
	DWORD relativeAddress = ((DWORD)dst - (DWORD)src) - size; // Jmp takes a relative, size is 1 byte for jmp instruction, plus either 4 or 8 bytes for the address size
	*(DWORD*)((DWORD)src + 1) = relativeAddress; // Set the address to jump to. + 1 so 0xe9 is not overwritten

	VirtualProtect(src, len, old, &old);
}

DWORD FindArrayOfBytes(DWORD baseAddress, BYTE* bytes, int totalBytes) 
{
	HANDLE currentProcess = GetCurrentProcess();
	_MEMORY_BASIC_INFORMATION mbi;
	int currentByte = 0;

	DWORD result = 0;

	totalBytes--; // make it 0 indexed

	while (baseAddress < 0x7fffffff && VirtualQuery((DWORD*)baseAddress, &mbi, sizeof(mbi)) && result == 0)
	{
		if (mbi.State == MEM_COMMIT && mbi.Protect == PAGE_EXECUTE_READWRITE)
		{
			unsigned char* buffer = (unsigned char*)mbi.BaseAddress;

			//ReadProcessMemory(currentProcess, mbi.BaseAddress, buffer, mbi.RegionSize, 0);

			for (int i = 0; i < mbi.RegionSize; i++)
			{
				if (*(buffer+i) == bytes[currentByte])
				{
					if (currentByte == totalBytes)
					{
						result = ((DWORD)(mbi.BaseAddress) + i) - totalBytes; // returns the address at the start of the array
						break;
					}
				}
				else
				{
					currentByte = -1;
				}

				currentByte++;
			}

			//delete[] buffer;
		}

		baseAddress += mbi.RegionSize;
	}

	return result;
}

DWORD ResolvePtrChain(DWORD ptr, std::vector<unsigned int> offsets)
{
	DWORD addr = ptr;
	for (int i = 0; i < offsets.size(); ++i)
	{
		addr = *(DWORD*)addr;
		addr += offsets[i];
	}
	return addr;
}