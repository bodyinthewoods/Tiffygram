#include "../SDK/SDK.h"

// -------------------------------------------------------------------------
// Compensator - client-side skin changer
//   - Unusual effect on local player hats
//   - Killstreak sheen + effect on active weapon
//   - Holiday effects (Halloween spells, Christmas bells, etc.)
// -------------------------------------------------------------------------

// Sheen particle name lookup (maps sheen index -> particle prefix used by game)
static const char* SheenToParticle(int iSheen, bool bBlue)
{
	switch (iSheen)
	{
	case 1: return bBlue ? "team_shine_blue"      : "team_shine_red";
	case 2: return "killstreak_daffodil";
	case 3: return "killstreak_manndarin";
	case 4: return "killstreak_green";
	case 5: return "killstreak_agonizing_emerald";
	case 6: return "killstreak_villainous_violet";
	case 7: return "killstreak_hot_rod";
	default: return nullptr;
	}
}

// -------------------------------------------------------------------------
// Hook: CAttributeManager_AttribHookInt
// Intercepts attribute reads for particle_effect (unusual), killstreak_sheen,
// killstreak_effect, and halloween_footstep_type (already handled elsewhere).
// -------------------------------------------------------------------------
MAKE_SIGNATURE(CAttributeManager_AttribHookInt_Compensator, "client.dll",
	"4C 8B DC 49 89 5B ? 49 89 6B ? 49 89 73 ? 57 41 54 41 55 41 56 41 57 48 83 EC ? 48 8B 3D ? ? ? ? 4C 8D 35",
	0x0);

// Signatures to identify which callsite we're being called from
MAKE_SIGNATURE(CTFWearable_UpdateParticleSystems_AttribHook_Call, "client.dll",
	"E8 ? ? ? ? 85 C0 74 ? 48 8D 55 ? 4C 8D 05 ? ? ? ? 48 8B CD",
	0x0);
MAKE_SIGNATURE(CTFWeaponBase_UpdateKillstreakEffects_AttribHook_Call, "client.dll",
	"E8 ? ? ? ? 8B D8 85 C0 74 ? 48 8D 55 ? 4C 8D 05 ? ? ? ? 48 8B CE",
	0x0);

MAKE_HOOK(CAttributeManager_AttribHookInt_Compensator,
	S::CAttributeManager_AttribHookInt_Compensator(),
	int,
	int value, const char* name, void* econent, void* buffer, bool isGlobalConstString)
{
	DEBUG_RETURN(CAttributeManager_AttribHookInt_Compensator, value, name, econent, buffer, isGlobalConstString);

	auto pLocal = H::Entities.GetLocal();
	if (!pLocal)
		return CALL_ORIGINAL(value, name, econent, buffer, isGlobalConstString);

	const uint32_t uHash = FNV1A::Hash32(name);

	// -- Unusual particle effect on wearables -------------------------
	if (uHash == FNV1A::Hash32Const("particle effect")
		&& Vars::Misc::Compensator::UnusualEffect.Value > 0)
	{
		// Only apply to wearables owned by local player
		for (auto pWearable : pLocal->m_hMyWearables())
		{
			if (pWearable && econent == static_cast<void*>(pWearable))
				return Vars::Misc::Compensator::UnusualEffect.Value;
		}
	}

	// -- Killstreak sheen on active weapon ----------------------------
	if (uHash == FNV1A::Hash32Const("killstreak_sheen_id")
		&& Vars::Misc::Compensator::KillstreakSheen.Value != Vars::Misc::Compensator::KillstreakSheenEnum::Off)
	{
		auto pWeapon = pLocal->m_hActiveWeapon();
		if (pWeapon && econent == static_cast<void*>(pWeapon))
			return Vars::Misc::Compensator::KillstreakSheen.Value;
	}

	// -- Killstreak tier 3 particle effect on active weapon ----------
	if (uHash == FNV1A::Hash32Const("killstreak_idleeffect")
		&& Vars::Misc::Compensator::KillstreakEffect.Value != Vars::Misc::Compensator::KillstreakEffectEnum::Off)
	{
		auto pWeapon = pLocal->m_hActiveWeapon();
		if (pWeapon && econent == static_cast<void*>(pWeapon))
			return Vars::Misc::Compensator::KillstreakEffect.Value;
	}

	return CALL_ORIGINAL(value, name, econent, buffer, isGlobalConstString);
}

// -------------------------------------------------------------------------
// Hook: TF_IsHolidayActive  (extends the existing one)
// Forces the selected holiday to be considered active client-side.
// -------------------------------------------------------------------------
MAKE_SIGNATURE(TF_IsHolidayActive_Compensator, "client.dll",
	"48 83 EC ? 48 8B 05 ? ? ? ? 44 8B C9",
	0x0);

MAKE_HOOK(TF_IsHolidayActive_Compensator,
	S::TF_IsHolidayActive_Compensator(),
	bool,
	int eHoliday)
{
	DEBUG_RETURN(TF_IsHolidayActive_Compensator, eHoliday);

	if (Vars::Misc::Compensator::HolidayEffect.Value != Vars::Misc::Compensator::HolidayEffectEnum::Off)
	{
		// Map our enum to game EHoliday values
		// kHoliday_Halloween=2, kHoliday_Christmas=3, kHoliday_TFBirthday=1,
		// kHoliday_FullMoon=9,  kHoliday_Valentines=6
		static const int s_aMap[] = { 0, 2, 3, 1, 9, 6 };
		int iWanted = s_aMap[Vars::Misc::Compensator::HolidayEffect.Value];
		if (eHoliday == iWanted
			|| (iWanted == 2 && (eHoliday == 9 || eHoliday == 10)) // HalloweenOrFullMoon
			)
			return true;
	}

	return CALL_ORIGINAL(eHoliday);
}
