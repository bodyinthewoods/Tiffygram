#pragma once
#include "../../../SDK/SDK.h"
#include <unordered_set>

class CSpectatorList
{
public:
	void DrawImGui();
	struct Spectator_t
	{
		std::string m_sName;
		std::string m_sMode;
		int         m_iIndex;
	};

	std::vector<Spectator_t> m_vSpectators = {};
	std::unordered_map<int, float> m_mRespawnCache = {};

	void GetSpectators(CTFPlayer* pLocal, CTFPlayer* pTarget);
	void Draw(CTFPlayer* pLocal);
};

ADD_FEATURE(CSpectatorList, SpectatorList);
