#include "OffscreenArrows.h"

#include "../Groups/Groups.h"

// Rotate a 2D point by (flCos, flSin) and offset by vPos
static inline Vec2 Rot(Vec2 vPos, float flCos, float flSin, float x, float y)
{
	return { vPos.x + x * flCos - y * flSin, vPos.y + y * flCos + x * flSin };
}

void COffscreenArrows::DrawArrowTo(const Vec3& vFromPos, const Vec3& vToPos, Color_t tColor, int iOffset, float flMaxDistance)
{
	tColor.a *= Math::RemapVal(vFromPos.DistTo(vToPos), flMaxDistance, flMaxDistance * 0.9f, 0.f, 1.f);
	if (!tColor.a)
		return;

	Vec2 vCenter = { H::Draw.m_nScreenW / 2.f, H::Draw.m_nScreenH / 2.f };
	Vec3 vScreen;
	if (SDK::W2S(vToPos, vScreen, true))
	{
		float flMin = std::min(vCenter.x, vCenter.y), flMax = std::max(vCenter.x, vCenter.y);
		float flDist = sqrt(powf(vScreen.x - vCenter.x, 2) + powf(vScreen.y - vCenter.y, 2));
		tColor.a *= std::clamp((flDist - flMin) / (flMin != flMax ? flMax - flMin : 1), 0.f, 1.f);
		if (!tColor.a)
			return;
	}

	Vec3 vAngle = Math::VectorAngles({ vCenter.x - vScreen.x, vCenter.y - vScreen.y, 0 });
	const float flDeg = Math::Deg2Rad(vAngle.y);
	const float flCos = cos(flDeg);
	const float flSin = sin(flDeg);

	float flOffset = -iOffset;
	float flScale  = H::Draw.Scale(25);

	Vec2 vPos = { flOffset * flCos, flOffset * flSin };
	if (fabs(vPos.x) > vCenter.x - flScale || fabs(vPos.y) > vCenter.y - flScale)
	{
		Vec2 a = { -(vCenter.x - flScale) / vPos.x, -(vCenter.y - flScale) / vPos.y };
		Vec2 b = {  (vCenter.x - flScale) / vPos.x,  (vCenter.y - flScale) / vPos.y };
		Vec2 c = { std::min(a.x, b.x), std::min(a.y, b.y) };
		vPos  *= fabsf(std::max(c.x, c.y));
	}
	vPos += vCenter;

	// Chevron shape (reference image: wide flat arrowhead with notched back)
	// Local space (pointing left = toward target):
	//   tip   : (-s,    0   )
	//   right : ( s*0.3, s*0.55)
	//   notch : (-s*0.1, 0  )   <- inward notch on back edge
	//   left  : ( s*0.3,-s*0.55)
	// (mirrored for both sides to form a 5-point chevron)
	float s = flScale;
	std::vector<Vec2> pts = {
		{ -s,       0       },   // tip
		{  s * 0.3f, s * 0.55f },  // back-right
		{ -s * 0.1f, 0       },  // notch center
		{  s * 0.3f,-s * 0.55f },  // back-left
	};

	std::vector<Vertex_t> vFill, vOutline;
	for (auto& p : pts)
	{
		Vec2 r = Rot(vPos, flCos, flSin, p.x, p.y);
		vFill.push_back(   { { r.x, r.y } });
		vOutline.push_back({ { r.x, r.y } });
	}

	H::Draw.FillPolygon(vFill, tColor);

	// Outline: slightly darker, full opacity driven by arrow alpha
	Color_t tOutline = { byte(tColor.r * 0.35f), byte(tColor.g * 0.35f), byte(tColor.b * 0.35f), tColor.a };
	H::Draw.LinePolygon(vOutline, tOutline);
}

void COffscreenArrows::DrawWorldArrow(const Vec3& vFromPos, const Vec3& vToPos, Color_t tColor, float flMaxDistance)
{
	tColor.a *= Math::RemapVal(vFromPos.DistTo(vToPos), flMaxDistance, flMaxDistance * 0.9f, 0.f, 1.f);
	if (!tColor.a)
		return;

	// Direction from local to target, flattened to ground plane
	Vec3 vDelta   = vToPos - vFromPos;
	vDelta.z      = 0.f;
	float flLen   = vDelta.Length();
	if (flLen < 1.f)
		return;
	Vec3 vForward = vDelta / flLen;
	Vec3 vRight   = { -vForward.y, vForward.x, 0.f };

	// Place the arrow on the ground just outside the player's feet
	float flGroundRadius = 40.f;
	Vec3  vBase = vFromPos;
	vBase.z     = vToPos.z; // snap to target ground height (approx)

	Vec3 vOrigin = vBase + vForward * flGroundRadius;

	float s = 20.f; // world-unit scale

	// Same chevron: tip, back-right, notch, back-left
	Vec3 vTip       = vOrigin + vForward *  s;
	Vec3 vBackRight = vOrigin - vForward * (s * 0.3f) + vRight * (s * 0.55f);
	Vec3 vNotch     = vOrigin - vForward * (s * 0.1f);
	Vec3 vBackLeft  = vOrigin - vForward * (s * 0.3f) - vRight * (s * 0.55f);

	// Fill via two triangles: tip-backRight-notch, tip-notch-backLeft
	// Use RenderLine for outline edges (no filled world polygon API available)
	Color_t tOutline = { byte(tColor.r * 0.35f), byte(tColor.g * 0.35f), byte(tColor.b * 0.35f), tColor.a };
	H::Draw.RenderLine(vTip,       vBackRight, tOutline, false);
	H::Draw.RenderLine(vBackRight, vNotch,     tOutline, false);
	H::Draw.RenderLine(vNotch,     vBackLeft,  tOutline, false);
	H::Draw.RenderLine(vBackLeft,  vTip,       tOutline, false);
}

void COffscreenArrows::Store()
{
	m_mCache.clear();
	if (!F::Groups.GroupsActive())
		return;

	for (auto& [pEntity, pGroup] : F::Groups.GetGroup(false))
	{
		if (!pGroup->m_bOffscreenArrows
			|| pEntity->entindex() == I::EngineClient->GetLocalPlayer())
			continue;

		ArrowCache_t& tCache = m_mCache[pEntity];
		tCache.m_tColor = F::Groups.GetColor(pEntity, pGroup);
		tCache.m_iOffset = pGroup->m_iOffscreenArrowsOffset;
		tCache.m_flMaxDistance = pGroup->m_flOffscreenArrowsMaxDistance;
	}
}

void COffscreenArrows::Draw(CTFPlayer* pLocal)
{
	if (m_mCache.empty())
		return;

	bool bThirdPerson = I::Input->CAM_IsThirdPerson();
	Vec3 vLocalPos    = pLocal->GetEyePosition();

	for (auto& [pEntity, tCache] : m_mCache)
	{
		Vec3 vTargetPos = pEntity->GetCenter();
		DrawArrowTo(vLocalPos, vTargetPos, tCache.m_tColor, tCache.m_iOffset, tCache.m_flMaxDistance);

		if (bThirdPerson)
			DrawWorldArrow(vLocalPos, vTargetPos, tCache.m_tColor, tCache.m_flMaxDistance);
	}
}
