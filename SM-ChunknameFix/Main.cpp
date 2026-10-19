#include <windows.h>

#include <cstring>
#include <string>
#include <type_traits>

struct lua_State;

using luaL_loadBufferx_Func = std::add_pointer_t<int(lua_State*, const char*, size_t, const char*, const char*)>;
luaL_loadBufferx_Func gOriginal = nullptr;

static int HookedLoadBufferX(lua_State* L, const char* buff, size_t sz, const char* name, const char* mode) {
    if (!name || name[0] == '@' || name[0] == '=')
        return gOriginal(L, buff, sz, name, mode);

    static thread_local std::string prefixed;
    prefixed.clear();
    prefixed.assign(1, '@');
    prefixed.append(name);

    return gOriginal(L, buff, sz, prefixed.c_str(), mode);
}

inline static bool IATHook(HMODULE module, const char* targetImport, void* targetFunction, void** originalFunction) {
    BYTE* baseAddress = reinterpret_cast<BYTE*>(module);
    IMAGE_DOS_HEADER* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(baseAddress);
    IMAGE_NT_HEADERS* ntHeader = reinterpret_cast<IMAGE_NT_HEADERS*>(baseAddress + dosHeader->e_lfanew);

    const IMAGE_DATA_DIRECTORY& importDataDirectory = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!importDataDirectory.VirtualAddress)
        return false;

    IMAGE_IMPORT_DESCRIPTOR* desc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(baseAddress + importDataDirectory.VirtualAddress);
    for (; desc->Name; ++desc) {
        const DWORD lookupRva = desc->OriginalFirstThunk ? desc->OriginalFirstThunk : desc->FirstThunk;
        IMAGE_THUNK_DATA* lookup = reinterpret_cast<IMAGE_THUNK_DATA*>(baseAddress + lookupRva);
        IMAGE_THUNK_DATA* iat = reinterpret_cast<IMAGE_THUNK_DATA*>(baseAddress + desc->FirstThunk);

        for (; lookup->u1.AddressOfData; ++lookup, ++iat) {
            if (IMAGE_SNAP_BY_ORDINAL(lookup->u1.Ordinal))
                continue;

            IMAGE_IMPORT_BY_NAME* import = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(baseAddress + lookup->u1.AddressOfData);
            if (strcmp(import->Name, targetImport) != 0)
                continue;

            void** slot = reinterpret_cast<void**>(&iat->u1.Function);
            DWORD oldProtect;
            if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &oldProtect))
                return false;

            *originalFunction = reinterpret_cast<void*>(*slot);
            *slot = targetFunction;
            VirtualProtect(slot, sizeof(void*), oldProtect, &oldProtect);
            return true;
        }
    }
 
    return false;
}

static void Attach() {
	HMODULE module = GetModuleHandleA("ScrapMechanic.exe");
	if (!module)
		return;

    if (!IATHook(module, "luaL_loadbufferx", reinterpret_cast<void*>(HookedLoadBufferX), reinterpret_cast<void**>(&gOriginal)))
		MessageBoxA(nullptr, "Failed to hook luaL_loadbufferx", "SM-ChunknameFix", MB_ICONERROR);
}

static void Detach() {
    HMODULE module = GetModuleHandleA("ScrapMechanic.exe");
    if (!module || !gOriginal)
        return;

    void* current = nullptr;
    IATHook(module, "luaL_loadbufferx", reinterpret_cast<void*>(gOriginal), &current);
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        Attach();
    } else if (reason == DLL_PROCESS_DETACH && !reserved) {
        Detach();
    }

    return TRUE;
}