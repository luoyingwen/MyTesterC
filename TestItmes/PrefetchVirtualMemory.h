#pragma once
typedef struct _WIN32_MEMORY_RANGE_ENTRY {
  PVOID  VirtualAddress;
  SIZE_T NumberOfBytes;
} WIN32_MEMORY_RANGE_ENTRY, *PWIN32_MEMORY_RANGE_ENTRY;

typedef BOOL (WINAPI* PFunc_PrefetchVirtualMemory)(
  _In_  HANDLE hProcess,
  _In_  ULONG_PTR NumberOfEntries,
  _In_  PWIN32_MEMORY_RANGE_ENTRY VirtualAddresses,
  _In_  ULONG Flags
);