#include "pch.h"

// Prevent reviving Demons when the stacks quantity is more than 19
_LHF_(OnCheckIfPossibleToPitLordSummon) {


    if (P_CombatManager->heroMonCount[IntAt(c->ebp + 0x8)]>=20)
    {
        c->return_address = 0x5A4222;  // Correctly set the return address
        return NO_EXEC_DEFAULT;  // Do not execute the original code
    }

    return EXEC_DEFAULT;  // Execute the original code
}

// Set a cap for AI army value to prevent game freezing
// The limit should be checked multiple times to find a sweet spot
_LHF_(OnGetAIArmyValue) {
    // Check if eax is greater than or equal to 10,000,000 or less than 0
    if (c->eax >= 10000000 || c->eax < 0) {
        // Set eax to max
        c->eax = 10000000;
    }

    return EXEC_DEFAULT;  // Execute the original code
}

// Function to install hooks
void InstallHooks() {
    // Write the hook at the specified address
    _PI->WriteLoHook(0x5A4170, OnCheckIfPossibleToPitLordSummon);
    _PI->WriteLoHook(0x44A985, OnGetAIArmyValue);
}

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    static bool pluginIsOn = false;

    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        if (!pluginIsOn) {
            pluginIsOn = true;
            Patcher* globalPatcher = GetPatcher();
            PatcherInstance* _PI = globalPatcher->CreateInstance("EraPlugin.ArchBugFixes.Archer30");
            InstallHooks();
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}