#include <Windows.h>
#include <DbgHelp.h>
#include <string>
#include <sstream>

LONG64 player_ptr = 0;

void show_error(LPSTR message)
{
  MessageBoxA(NULL, message, "Error", MB_ICONERROR | MB_OK);
}

LONG64 search_memory(LONG64 address, LONG64 size, char *pattern)
{
  LONG64 memory_ptr = address;
  int pattern_index = 0;

  while (memory_ptr < address + size)
  {
    char memory_byte = *((char *)memory_ptr);
    char pattern_byte = *(pattern + pattern_index);

    if (memory_byte == pattern_byte)
    {
      ++pattern_index;

      if (pattern_index == sizeof(pattern))
      {
        return memory_ptr - sizeof(pattern) + 1;
      }
    }
    else
    {
      pattern_index = 0;
    }

    ++memory_ptr;
  }

  return -1;
}

void write_memory(LONG64 address, char *buf, LONG64 size)
{
  for (LONG64 i = 0; i < size; ++i)
  {
    *((char *)(address + i)) = buf[i];
  }
}

DWORD WINAPI main_thread(LPVOID param)
{
  HMODULE hMod = GetModuleHandle(NULL);

  if (hMod == NULL)
  {
    show_error("Could not get module handle");

    return 0;
  }

  LONG64 exeSection = 0;
  LONG64 exeSectionSize = 0;

  PIMAGE_NT_HEADERS64 NtHeader = ImageNtHeader(hMod);
  WORD NumSections = NtHeader->FileHeader.NumberOfSections;
  PIMAGE_SECTION_HEADER Section = IMAGE_FIRST_SECTION(NtHeader);

  for (WORD i = 0; i < NumSections; ++i)
  {
    if ((Section->Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0)
    {
      ++Section;

      continue;
    }

    exeSection = (LONG64)hMod + Section->VirtualAddress;
    exeSectionSize = Section->SizeOfRawData;

    break;
  }

  if (exeSection == 0 || exeSectionSize == 0)
  {
    show_error("Could not find executable section");

    return 0;
  }

  char *pattern = (char *)"\x48\x89\x5C\x24\x20\x55\x48\x8B\xEC\x48\x81\xEC\x80\x00\x00\x00\x48\x8B\x05\x19\x0C\xBF\x01\x48\x33\xC4\x48\x89\x45\xF0";
  LONG64 address = search_memory(exeSection, exeSectionSize, pattern);

  if (address == -1)
  {
    show_error("Could not find memory address");

    return 0;
  }

  // std::stringstream stream;
  // stream.setf(std::ios::hex, std::ios::basefield);
  // stream << address;
  // std::string hex_string = stream.str();

  // MessageBoxA(NULL, hex_string.c_str(), "ADDRESS", MB_ICONINFORMATION | MB_OK);

  LONG64 player_ptr_address = (LONG64)(&player_ptr);
  char code[41] = { 0 };

  // MOV RAX, &player_ptr
  code[0] = '\x48';
  code[1] = '\xB8';
  code[2] = (char)((player_ptr_address >>  0) & 0xFF);
  code[3] = (char)((player_ptr_address >>  8) & 0xFF);
  code[4] = (char)((player_ptr_address >> 16) & 0xFF);
  code[5] = (char)((player_ptr_address >> 24) & 0xFF);
  code[6] = (char)((player_ptr_address >> 32) & 0xFF);
  code[7] = (char)((player_ptr_address >> 40) & 0xFF);
  code[8] = (char)((player_ptr_address >> 48) & 0xFF);
  code[9] = (char)((player_ptr_address >> 56) & 0xFF);

  // MOV QWORD PTR DS:[RAX], RCX
  code[10] = '\x48';
  code[11] = '\x89';
  code[12] = '\x08';

  // MOV QWORD PTR SS:[RSP+20], RBX
  code[13] = '\x48';
  code[14] = '\x89';
  code[15] = '\x5C';
  code[16] = '\x24';
  code[17] = '\x20';

  // PUSH RBP
  code[18] = '\x55';

  // MOV RBP, RSP
  code[19] = '\x48';
  code[20] = '\x8B';
  code[21] = '\xEC';

  // SUB RSP, 80
  code[22] = '\x48';
  code[23] = '\x81';
  code[24] = '\xEC';
  code[25] = '\x80';
  code[26] = '\x00';
  code[27] = '\x00';
  code[28] = '\x00';

  LONG64 address_return = address + 0x10;

  // MOV RAX, &return
  code[29] = '\x48';
  code[30] = '\xB8';
  code[31] = (char)((address_return >>  0) & 0xFF);
  code[32] = (char)((address_return >>  8) & 0xFF);
  code[33] = (char)((address_return >> 16) & 0xFF);
  code[34] = (char)((address_return >> 24) & 0xFF);
  code[35] = (char)((address_return >> 32) & 0xFF);
  code[36] = (char)((address_return >> 40) & 0xFF);
  code[37] = (char)((address_return >> 48) & 0xFF);
  code[38] = (char)((address_return >> 56) & 0xFF);

  // JMP RAX
  code[39] = '\xFF';
  code[40] = '\xE0';

  LONG64 code_address = (LONG64)VirtualAlloc(0, sizeof(code), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

  if (code_address == NULL)
  {
    show_error("Could not allocate memory for code");

    return 0;
  }

  DWORD oldProtection;

  VirtualProtect((LPVOID)code_address, sizeof(code), PAGE_EXECUTE_READWRITE, &oldProtection);

  // std::stringstream stream;
  // stream.setf(std::ios::hex, std::ios::basefield);
  // stream << code_address;
  // std::string hex_string = stream.str();

  // MessageBoxA(NULL, hex_string.c_str(), "CODE ADDRESS", MB_ICONINFORMATION | MB_OK);

  write_memory(code_address, code, sizeof(code));

  // std::stringstream stream;
  // stream.setf(std::ios::hex, std::ios::basefield);
  // stream << &player_ptr;
  // std::string hex_string = stream.str();

  // MessageBoxA(NULL, hex_string.c_str(), "PLAYER_PTR ADDRESS", MB_ICONINFORMATION | MB_OK);

  char patch[16] = { 0 };

  // MOV RAX, &code_address
  patch[0] = '\x48';
  patch[1] = '\xB8';
  patch[2] = (char)((code_address >>  0) & 0xFF);
  patch[3] = (char)((code_address >>  8) & 0xFF);
  patch[4] = (char)((code_address >> 16) & 0xFF);
  patch[5] = (char)((code_address >> 24) & 0xFF);
  patch[6] = (char)((code_address >> 32) & 0xFF);
  patch[7] = (char)((code_address >> 40) & 0xFF);
  patch[8] = (char)((code_address >> 48) & 0xFF);
  patch[9] = (char)((code_address >> 56) & 0xFF);

  // JMP RAX
  patch[10] = '\xFF';
  patch[11] = '\xE0';

  // NOPs
  patch[12] = '\x90';
  patch[13] = '\x90';
  patch[14] = '\x90';
  patch[15] = '\x90';

  VirtualProtect((LPVOID)exeSection, exeSectionSize, PAGE_EXECUTE_READWRITE, &oldProtection);

  write_memory(address, patch, sizeof(patch));

  VirtualProtect((LPVOID)exeSection, exeSectionSize, oldProtection, &oldProtection);

  while (true) {
    Sleep(1000);

    if (player_ptr == 0)
    {
      continue;
    }

    std::stringstream stream;
    stream.setf(std::ios::hex, std::ios::basefield);
    stream << player_ptr;
    std::string hex_string = stream.str();

    MessageBoxA(NULL, hex_string.c_str(), "PLAYER_PTR", MB_ICONINFORMATION | MB_OK);

    break;
  }

  while (true)
  {
    *((char *)(player_ptr + 0xAC)) = '\x00';

    Sleep(100);
  }

  return 0;
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{
  if (dwReason == DLL_PROCESS_ATTACH)
  {
    CreateThread(0, 0, main_thread, hModule, 0, 0);
  }

  return TRUE;
}
