#include "Fonts.h"

#include "../../Definitions/Interfaces/IMatSystemSurface.h"
#include <ranges>

void CFonts::Reload(float flDPI, bool bOutline)
{
	int iFlags = !bOutline ? FONTFLAG_NONE : FONTFLAG_NONE | FONTFLAG_DROPSHADOW;

	m_mFonts[FONT_ESP] = { "Tahoma", int(12.f * flDPI), iFlags, 0 };
	m_mFonts[FONT_INDICATORS] = { "Tahoma", int(13.f * flDPI), iFlags, 800 };

	for (auto& fFont : m_mFonts | std::views::values)
	{
		if (fFont.m_dwFont = I::MatSystemSurface->CreateFont())
			I::MatSystemSurface->SetFontGlyphSet(fFont.m_dwFont, fFont.m_szName, fFont.m_nTall, fFont.m_nWeight, 0, 0, fFont.m_nFlags);
	}
}

const Font_t& CFonts::GetFont(EFonts eFont)
{
	return m_mFonts[eFont];
}