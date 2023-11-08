#pragma once
#include <vector>

void SetBytes(void*, BYTE*, unsigned int);
void SetByte(void*, BYTE, unsigned int);
void SetJmp(void*, void*, unsigned int, bool);

DWORD FindArrayOfBytes(DWORD, BYTE*, int);

DWORD ResolvePtrChain(DWORD, std::vector<unsigned int>);