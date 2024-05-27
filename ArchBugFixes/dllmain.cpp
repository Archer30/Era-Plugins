#include "pch.h"

Patcher* globalPatcher;
PatcherInstance* _PI;

// Prevent reviving Demons when the stacks quantity is more than 19
_LHF_(OnCheckIfPossibleToPitLordSummon) {

    if (P_CombatManager->heroMonCount[IntAt(c->ebp + 0x8)]>=20)
    {
        c->return_address = 0x5A4222;  // Correctly set the return address
        return NO_EXEC_DEFAULT;  // Do not execute the original code
    }

    return EXEC_DEFAULT;  // Execute the original code
}

// Prevent AI freezing
// Set a cap for AI army total AI values
// The limit here (20000000) should be checked multiple times for a sweet spot
int __stdcall Army_Get_AI_Value(HiHook* h, H3Army* army)
{
    ULONGLONG result = 0;

    if (army)
    {

        int i = 6;
        do
        {
            if (army->type[i] != eCreature::UNDEFINED)
                result += static_cast<__int64>(army->count[i]) * P_CreatureInformation[army->type[i]].aiValue;

        } while (i--);

        if (result > 20000000)
            result = 20000000; // Don't use MAXINT32 >> 3 or even MAXINT32 >> 4 here as they both get the game freezing for some reason

    }

    return static_cast<int>(result);
}

// Set a cap for the sum of Primary Skills
// Here we assume that 0x4E5BD0 is used only for AI behaviour and not anything else
int __stdcall Hero_GetSumOfPrimarySkills(HiHook* h, H3Hero* hero)
{
    int result = THISCALL_1(int, h->GetDefaultFunc(), hero);
    if (!P_ActivePlayer->is_human)
        result = Clamp(0, result, 380);

    return result;
}

// daemon's WIP code
int __stdcall AICalculateMapPosWeight(HiHook* h, const H3Hero* hero, const DWORD a2, const DWORD a3)
{


    LONGLONG result = FASTCALL_3(int, h->GetDefaultFunc(), hero, a2, a3);

    constexpr int MAX_INT = MAXINT32;
    constexpr int MIN_INT = -500000000;


    if (result > MAX_INT)
        result = MAX_INT;
    else if (result < MIN_INT)
        result = MIN_INT;

    return static_cast<int>(result);
}


// Function to install hooks
void InstallHooks() {
    // Write the hook at the specified address
    // Prevent Pit Lords to summon 
    _PI->WriteLoHook(0x5A4170, OnCheckIfPossibleToPitLordSummon);

    // Fix AI value overflow leading to game freezing
    _PI->WriteHiHook(0x44A950, SPLICE_, EXTENDED_, THISCALL_, Army_Get_AI_Value);
    _PI->WriteHiHook(0x4E5BD0, SPLICE_, EXTENDED_, THISCALL_, Hero_GetSumOfPrimarySkills);
    // _PI->WriteHiHook(0x528520, FASTCALL_, AICalculateMapPosWeight);
}

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    static bool pluginIsOn = false;

    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        if (!pluginIsOn) {
            pluginIsOn = true;
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance("EraPlugin.ArchBugFixes.Archer30");
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
