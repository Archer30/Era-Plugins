#include "pch.h"

Patcher* globalPatcher;
PatcherInstance* _PI;

// Prevent reviving Demons when the stacks quantity is already 20
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

// Fix unreasonable retaliation events
// Prevent possible to retaliate when the attacking stack is killed, prevent War Machines/stunned units to retaliate
// External Blind/Stone/Paralyze effects don't stop the attacked unit to retaliate - not any more
_LHF_(OnRetaliate) {
    // Check if the attacker is still alive
    if (auto atkStack = reinterpret_cast<H3CombatMonster*>(c->edi))
    {
        if (atkStack->numberAlive <= 0)
        {
            c->return_address = 0x441C01;  // Jump to the end of attack function if the attacker is killed
            return NO_EXEC_DEFAULT;  // Do not execute the original code   
        }
    }

    // Get the defender's stack structure and check its creature type, spell status
    if (auto defStack = reinterpret_cast<H3CombatMonster*>(c->esi))
    {
        if ((defStack->info.siegeWeapon) != 0
            || defStack->activeSpellDuration[eSpell::BLIND]
            || defStack->activeSpellDuration[eSpell::STONE]
            || defStack->activeSpellDuration[eSpell::PARALYZE]
            )
        {
            c->return_address = 0x441B85; // Jump to the end of retaliation process if the defender cannot retaliate
            return NO_EXEC_DEFAULT;  // Do not execute the original code
        }
    }

    return EXEC_DEFAULT;  // Execute the original code
}


// Prevent combo art pieces becoming the artifacts for Seer Huts
_LHF_(RMG_AtQuestArtifactListCounter)
{
    const int artId = c->eax;
    if (artId >= 0 && P_ArtifactSetup[artId].IsPartOfCombo())
    {
        // jump to next art in the loop
        c->return_address = 0x54B9D1;
        return  NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}
_LHF_(RMG_AtQuestArtifactListSelectRandom)
{
    const int artId = c->eax;
    if (artId >= 0 && P_ArtifactSetup[artId].IsPartOfCombo())
    {
        // jump to next art in the loop
        c->return_address = 0x54BA32;
        return NO_EXEC_DEFAULT;

    }

    return EXEC_DEFAULT;
}


// Fix pressing F to cast not working for Faerie Dragon when the hero cannot cast
_LHF_(OnSpellChosen)
{
    if (IntAt(c->ebp + 0xc) == 1) // 0 for hero spell, 1 for monsters'
    {
        c->return_address = 0x59EF9F;  // Correctly set the return address
        return NO_EXEC_DEFAULT;  // Do not execute the original code
    }

    return EXEC_DEFAULT;  // Execute the original code
}


// Restrain Armour damage reduction at 96% before level 868
double __stdcall OnGetArmourPower(HiHook* h, H3Hero* hero)
{
    double result = THISCALL_1(float, h->GetDefaultFunc(), hero);

    if (result <= 0.04 && hero->level < 868)
    {
        result = 0.04;
    }

    return result;
}


// Fix incorrect damage hint display when a stack is affected by Forgetfulness
_LHF_(OnGetAttackDamageHint)
{
    // Check if it is shooting
    if (IntAt(c->ebp + 0xc) > 0)
    {
        // Cast esi to H3CombatCreature and check if it is valid
        if (auto stack = reinterpret_cast<H3CombatCreature*>(c->esi))
        {
            // Verify the duration for Forgetfulness and if the quantity of stack is greater than 1
            if (stack->activeSpellDuration[eSpell::FORGETFULNESS] > 0 && c->eax > 1)
            {
                c->eax /= 2; // Halve the value at eax
            }
        }
    }

    return EXEC_DEFAULT; // Execute the original code by default
}


// Function to install hooks
void InstallCustomHooksAndWriteBytes() {
    // Write the hook at the specified address
    // Prevent Pit Lords to summon 
    _PI->WriteLoHook(0x5A4170, OnCheckIfPossibleToPitLordSummon);

    // Fix AI value overflow leading to game freezing
    _PI->WriteHiHook(0x44A950, SPLICE_, EXTENDED_, THISCALL_, Army_Get_AI_Value);
    _PI->WriteHiHook(0x4E5BD0, SPLICE_, EXTENDED_, THISCALL_, Hero_GetSumOfPrimarySkills);
    // _PI->WriteHiHook(0x528520, FASTCALL_, AICalculateMapPosWeight);
    
    // Prevent combo art pieces becoming the artifacts for Seer Huts
    _PI->WriteLoHook(0x54B9C0, RMG_AtQuestArtifactListCounter);
    _PI->WriteLoHook(0x54BA1B, RMG_AtQuestArtifactListSelectRandom);

    // Fix pressing F to cast not working for Faerie Dragon when the hero cannot cast
    _PI->WriteLoHook(0x59EF91, OnSpellChosen);

    // Restrain Armour damage reduction at 96% before level 868
    _PI->WriteHiHook(0x04E4580, THISCALL_, OnGetArmourPower);

    // Fix incorrect damage hint display when a stack is affected by Forgetfulness
    _PI->WriteLoHook(0x492F78, OnGetAttackDamageHint);

    // Fix unreasonable retaliation events
    _PI->WriteLoHook(0x441B25, OnRetaliate);

    // Improve the interactoin of creature flags and stack exp
    // Originally, setting 0 as the value of a flag ability result in canceling that ability when the rank doesn't meet the requirement
    // This code ignores it
    _PI->WriteByte(0x71BF08, 0x30);

    // Fix the incorrect message shown when attempting to capture a mine that has guards but no owner
    // The message wrongly states the guards are Troglodytes, even though they are not
    _PI->WriteByte(0x4A36D4, 0xEB);

    // Remove the message when successfully recruiting level 1 creatures from their dwellings - thanks to SadnessPower
    _PI->WriteHexPatch(0x04A1ACC, "8B45 F0 C700 00000000 E9 BB000000 9090");
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
            InstallCustomHooksAndWriteBytes();
            // Era::ConnectEra();
            // Era::RegisterHandler(OnAfterWog, "OnAfterWog"); // Not used for now
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}
