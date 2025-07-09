#include <windows.h>
#include <iostream>
#include <psapi.h>
#include <string>
#include <vector>

//RAII wrapper, ensures the handle automtically gets destroyed
class HandleWrapper {
private:
    HANDLE handle; 
    
public:
    HandleWrapper(HANDLE h) : handle(h) {} 
    
    ~HandleWrapper() {  
      if (handle != NULL && handle != INVALID_HANDLE_VALUE) {
          CloseHandle(handle);
      }
    }
    
    operator HANDLE() const { return handle; }
    
    HandleWrapper(const HandleWrapper&) = delete;
    HandleWrapper& operator=(const HandleWrapper&) = delete;
};

DWORD findProcessID(const wchar_t* targetProcessName)
{
    DWORD processIDs[1024];
    DWORD bytesReturned;
    wchar_t currentProcessName[MAX_PATH]; // a wide char is basically a bigger version, instead of being an 8bit character, its 16 or 32 bit

    if (!EnumProcesses(processIDs, sizeof(processIDs), &bytesReturned)) { return 0; }

    int numProcesses = bytesReturned / sizeof(DWORD);

    for (int i = 0; i < numProcesses; ++i)
    {
        HandleWrapper processHandle(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processIDs[i]));

        if (processHandle == NULL) { continue; }

        if (!GetModuleBaseNameW(processHandle, NULL, currentProcessName, sizeof(currentProcessName) / sizeof(wchar_t))) { continue; }

        //case-insensitive comparison of wide character strings
        if (_wcsicmp(currentProcessName, targetProcessName) == 0) { return processIDs[i]; }
    }

    return 0;
}

int main() {
    DWORD pid = findProcessID(L"memory-game.exe");

    if (pid == 0) { std::cout << "Process not found!"; return 1; }

    std::cout << "Found process ID: " << pid;
    

    // TODO: OpenProcess with PROCESS_VM_READ 
    // TODO: ReadProcessMemory from address 0x7FF6DE4D8D78
    // TODO: Print the Health value
    
    return 0;
}