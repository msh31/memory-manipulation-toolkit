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

// get the base address of the game
HMODULE GetModuleBaseAddress(HANDLE processHandle, const wchar_t *moduleName)
{
    HMODULE modules[1024];
    DWORD bytesNeeded;

    if (!EnumProcessModules(processHandle, modules, sizeof(modules), &bytesNeeded)) { return NULL; };

    for (unsigned int i = 0; i < (bytesNeeded / sizeof(HMODULE)); ++i) {
        wchar_t szModName[MAX_PATH];

        if (!GetModuleFileNameExW(processHandle, modules[i], szModName, sizeof(szModName))) { return NULL; }

        std::wstring modName(szModName);

        if (modName.find(L"memory-game.exe") != std::wstring::npos) 
        {
            std::wcout << L"Found target module: " << modName << "\n\n";
            return modules[i];
        }
    }

    return NULL;
}

struct MemoryReadEntry {
    LPCVOID address;     // remote address in target
    void* buffer;        // pointer to local buffer
    SIZE_T size;         // size of data type
};

void writeToProcess(HANDLE handle, LPVOID address, int newValue, SIZE_T *bytesToRead)
{
    BOOL success = WriteProcessMemory(handle, address, &newValue, sizeof(newValue), bytesToRead);

    if (!success) { std::cerr << "Failed to write memory at address " << address; } 
}

int main() {
    DWORD pid = findProcessID(L"memory-game.exe");
    if (pid == 0) { std::cout << "Process not found!"; return 1; }
    std::cout << "Found process ID: " << pid << "\n\n";

    HandleWrapper processHandle(OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid));
    if (processHandle == NULL) { std::cout << "Failed to open process!\n\n"; return 1; }

    auto baseAddress = GetModuleBaseAddress(processHandle, L"memory-game.exe");

    int healthValue, goldValue, levelValue;
    double xpValue;

    LPVOID healthAddress = (LPVOID)((uintptr_t)baseAddress + 0xEB078);
    LPVOID goldAddress = (LPVOID)((uintptr_t)baseAddress + 0xEB07C);    // +4
    LPVOID levelAddress = (LPVOID)((uintptr_t)baseAddress + 0xEB080);   // +8  
    LPVOID xpAddress = (LPVOID)((uintptr_t)baseAddress + 0xEB088);      // +16 (double)

    SIZE_T NumberOfBytesRead;

    bool finished = false;

    while (!finished)
    {
        std::cout << "\n> ";
        std::string input;
        std::getline(std::cin, input);

        if(input == "gold")
        {
            int newValue = 999999;
            writeToProcess(processHandle, goldAddress, newValue, &NumberOfBytesRead);

            std::cout << "Set gold to 999999!";
        }
        else if (input == "heal")
        {
            int newValue = 999999;
            writeToProcess(processHandle, healthAddress, newValue, &NumberOfBytesRead);

            std::cout << "Set health to 999999!";
        }
        else if (input == "level")
        {
            int newValue = 100;
            writeToProcess(processHandle, levelAddress, newValue, &NumberOfBytesRead);

            std::cout << "Set level to 100!";
        }
        else if (input == "xp")
        {
            double newXpValue = 99999.99;
            BOOL success = WriteProcessMemory(processHandle, xpAddress, &newXpValue, sizeof(newXpValue), &NumberOfBytesRead);

            if (!success) { std::cerr << "Failed to write XP!" << std::endl; }
            
            std::cout << "Set XP to 99999.99!";
        }
        else if (input == "q")
        {
            finished = true;
        }
    }

    // std::cout << "Health: " << healthValue << "\n";
    // std::cout << "Gold: "   << goldValue << "\n";
    // std::cout << "Level: "  << levelValue << "\n";
    // std::cout << "XP: "     << xpValue << "\n";
    
    return 0;
}