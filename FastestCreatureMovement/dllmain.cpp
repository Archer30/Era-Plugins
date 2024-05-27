// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

// Global variables
Patcher* globalPatcher;
PatcherInstance* _PI;

// Hook function
_LHF_(OnGetFastestSpeed) {
    c->ecx = Clamp(3, c->ecx, 11);
    return EXEC_DEFAULT;
}

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    static bool pluginIsOn = false;

    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        if (!pluginIsOn) {
            pluginIsOn = true;
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance("EraPlugin.FastestCreatureMovement.Archer30");
            _PI->WriteByte(0x4E4E2B, 0x3);
            _PI->WriteByte(0x4E4EB3, 0x7E);
            _PI->WriteLoHook(0x4E4ED1, OnGetFastestSpeed);
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}