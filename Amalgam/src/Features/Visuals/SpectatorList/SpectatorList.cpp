#include "SpectatorList.h"

#include "../../Players/PlayerUtils.h"
#include "../../Spectate/Spectate.h"

void CSpectatorList::DrawImGui()
{
}

void CSpectatorList::GetSpectators(CTFPlayer* pLocal, CTFPlayer* pTarget)
{
	m_vSpectators.clear();

	auto pResource = H::Entities.GetResource();
	if (!pResource)
		return;

	int iTarget    = pTarget->entindex();
	int iLocalTeam = pResource->m_iTeam(I::EngineClient->GetLocalPlayer());

	for (int n = 1; n <= I::EngineClient->GetMaxClients(); n++)
	{
		if (n == iTarget)
			continue;
		if (!pResource->m_bValid(n) || pResource->IsFakePlayer(n))
			continue;

		auto pPlayer = I::ClientEntityList->GetClientEntity(n)->As<CTFPlayer>();
		if (!pPlayer || !pPlayer->IsPlayer())
			continue;

		bool bLocal = n == I::EngineClient->GetLocalPlayer();
		int  nTeam  = pResource->m_iTeam(n);

		if (iLocalTeam != TEAM_SPECTATOR && nTeam == TEAM_SPECTATOR)
		{
			m_vSpectators.push_back({ F::PlayerUtils.GetPlayerName(n, pResource->GetName(n)), "[?]", n });
			continue;
		}

		if (pPlayer->IsAlive()) continue;
		if (pTarget->IsDormant() != pPlayer->IsDormant()) continue;
		if (pResource->m_iTeam(iTarget) != nTeam) continue;

		int iObserverTarget = !pPlayer->IsDormant()
			? pPlayer->m_hObserverTarget().GetEntryIndex()
			: iTarget;
		int iObserverMode = pPlayer->m_iObserverMode();
		if (bLocal && F::Spectate.HasTarget())
		{
			iObserverTarget = F::Spectate.m_hOriginalTarget.GetEntryIndex();
			iObserverMode   = F::Spectate.m_iOriginalMode;
		}
		if (iObserverTarget != iTarget) continue;
		if (bLocal && !I::EngineClient->IsPlayingDemo() && !F::Spectate.HasTarget()) continue;

		if (pPlayer->IsDormant())
		{
			m_vSpectators.push_back({ F::PlayerUtils.GetPlayerName(n, pResource->GetName(n)), "[?]", n });
			continue;
		}

		bool bTeammate = (nTeam == iLocalTeam);
		std::string sMode;

		switch (iObserverMode)
		{
		case OBS_MODE_FIRSTPERSON:
			if (!bTeammate) continue;
			sMode = "[firstperson]";
			break;
		case OBS_MODE_THIRDPERSON:
			if (!bTeammate) continue;
			sMode = "[thirdperson]";
			break;
		case OBS_MODE_FREEZECAM:
			if (bTeammate) continue;
			sMode = "[Freeze]";
			break;
		case OBS_MODE_DEATHCAM:
			if (bTeammate) continue;
			sMode = "[Death]";
			break;
		default:
			continue;
		}

		m_vSpectators.push_back({ F::PlayerUtils.GetPlayerName(n, pResource->GetName(n)), sMode, n });
	}
}

void CSpectatorList::Draw(CTFPlayer* pLocal)
{
	if (!(Vars::Menu::Indicators.Value & Vars::Menu::IndicatorsEnum::Spectators))
	{
		m_mRespawnCache.clear();
		m_vSpectators.clear();
		return;
	}

	auto pTarget = pLocal;
	switch (pLocal->m_iObserverMode())
	{
	case OBS_MODE_FIRSTPERSON:
	case OBS_MODE_THIRDPERSON:
		pTarget = pLocal->m_hObserverTarget()->As<CTFPlayer>();
	}
	if (!pTarget || !pTarget->IsPlayer())
	{
		m_vSpectators.clear();
		return;
	}

	GetSpectators(pLocal, pTarget);
	// Rendering is handled by CMenu::DrawSpecList() in Menu.cpp
}
