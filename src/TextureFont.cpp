#include "TextureFont.h"
#include "gui/GuiScreen.h"
#include "libs.h"

#include FT_GLYPH_H

#define PARAGRAPH_SPACING 1.5f

#define TEXTURE_FONT_ENTER \
	glEnable(GL_BLEND); \
	glEnable(GL_TEXTURE_2D); \
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

#define TEXTURE_FONT_LEAVE \
	glDisable(GL_TEXTURE_2D); \
	glDisable(GL_BLEND);

void TextureFont::RenderGlyph(int chr, float x, float y)
{
	glfglyph_t *glyph = &m_glyphs[chr];
	glBindTexture(GL_TEXTURE_2D, glyph->tex);
	const float ox = x + float(glyph->offx);
	const float oy = y + float(m_pixSize - glyph->offy);
	glBegin(GL_QUADS);
		float allocSize[2] = { m_texSize*glyph->width, m_texSize*glyph->height };
		const float w = glyph->width;
		const float h = glyph->height;
		glTexCoord2f(0,h);
		glVertex2f(ox,oy+allocSize[1]);
		glTexCoord2f(w,h);
		glVertex2f(ox+allocSize[0],oy+allocSize[1]);
		glTexCoord2f(w,0);
		glVertex2f(ox+allocSize[0],oy);
		glTexCoord2f(0,0);
		glVertex2f(ox,oy);
	glEnd();
}

void TextureFont::MeasureString(const char *str, float &w, float &h)
{
	w = 0;
	h = GetHeight();
	float line_width = 0;
	unsigned int len = strlen(str);
	for (unsigned int i=0; i<len; i++) {
		if (str[i] == '\n') {
			if (line_width > w) w = line_width;
			line_width = 0;
			h += GetHeight()*PARAGRAPH_SPACING;
		} else {
			line_width += m_glyphs[str[i]].advx;
			if (i+1 < len) {
				FT_UInt a = FT_Get_Char_Index(m_face, str[i]);
		        FT_UInt b = FT_Get_Char_Index(m_face, str[i+1]);
				FT_Vector kern;
				FT_Get_Kerning(m_face, a, b, FT_KERNING_UNFITTED, &kern);
				line_width += float(kern.x) / 64.0;
			}
		}
	}
	if (line_width > w) w = line_width;
	h += m_descender;
}

void TextureFont::RenderString(const char *str, float x, float y)
{
	TEXTURE_FONT_ENTER;
	float px = x;
	float py = y;
	unsigned int len = strlen(str);
	for (unsigned int i=0; i<len; i++) {
		if (str[i] == '\n') {
			px = x;
			py += GetHeight()*PARAGRAPH_SPACING;
		} else {
			glfglyph_t *glyph = &m_glyphs[str[i]];
			if (glyph->tex) RenderGlyph(str[i], roundf(px), py);
			if (i+1 < len) {
				FT_UInt a = FT_Get_Char_Index(m_face, str[i]);
		        FT_UInt b = FT_Get_Char_Index(m_face, str[i+1]);
				FT_Vector kern;
				FT_Get_Kerning(m_face, a, b, FT_KERNING_UNFITTED, &kern);
				px += float(kern.x) / 64.0;
			}
			px += glyph->advx;
		}
	}
	TEXTURE_FONT_LEAVE;
}

void TextureFont::RenderMarkup(const char *str, float x, float y)
{
	TEXTURE_FONT_ENTER;
	float px = x;
	float py = y;
	int len = strlen(str);
	for (int i=0; i<len; i++) {
		if (str[i] == '#') {
			int hexcol;
			if (sscanf(str+i, "#%3x", &hexcol)==1) {
				Uint8 col[3];
				col[0] = (hexcol&0xf00)>>4;
				col[1] = (hexcol&0xf0);
				col[2] = (hexcol&0xf)<<4;
				glColor3ubv(col);
				i+=3;
				continue;
			}
		}
		if (str[i] == '\n') {
			px = x;
			py += GetHeight()*PARAGRAPH_SPACING;
		} else {
			glfglyph_t *glyph = &m_glyphs[str[i]];
			if (glyph->tex) RenderGlyph(str[i], roundf(px), py);
			// XXX kerning doesn't skip markup
			if (i+1 < len) {
				FT_UInt a = FT_Get_Char_Index(m_face, str[i]);
		        FT_UInt b = FT_Get_Char_Index(m_face, str[i+1]);
				FT_Vector kern;
				FT_Get_Kerning(m_face, a, b, FT_KERNING_UNFITTED, &kern);
				px += float(kern.x) / 64.0;
			}
			px += glyph->advx;
		}
	}
	TEXTURE_FONT_LEAVE;
}

TextureFont::TextureFont(FontManager &fm, const std::string &config_filename) : Font(fm, config_filename)
{
	std::string filename_ttf = GetConfig().String("FontFile");
	if (filename_ttf.length() == 0) {
		fprintf(stderr, "'%s' does not name a FontFile to use\n", config_filename.c_str());
		abort();
	}

	float scale[2];
	Gui::Screen::GetCoords2Pixels(scale);

	int a_width = GetConfig().Int("PixelWidth") / scale[0];
	int a_height = GetConfig().Int("PixelHeight") / scale[1];

	float advx_adjust = GetConfig().Float("AdvanceXAdjustment");

	int err;
	m_pixSize = a_height;
	if (0 != (err = FT_New_Face(GetFontManager().GetFreeTypeLibrary(), std::string(PIONEER_DATA_DIR "/fonts/" + filename_ttf).c_str(), 0, &m_face))) {
		fprintf(stderr, "Terrible error! Couldn't load '%s'; error %d.\n", filename_ttf.c_str(), err);
		abort();
	}

	FT_Set_Pixel_Sizes(m_face, a_width, a_height);
	int nbit = 0;
	int sz = a_height;
	while (sz) { sz >>= 1; nbit++; }
	sz = (64 > (1<<nbit) ? 64 : (1<<nbit));
	m_texSize = sz;

	unsigned char *pixBuf = new unsigned char[2*sz*sz];

	for (int chr=32; chr<127; chr++) {
		memset(pixBuf, 0, 2*sz*sz);

		unsigned int glyph_index = FT_Get_Char_Index(m_face, chr);
		if (FT_Load_Glyph(m_face, glyph_index, FT_LOAD_FORCE_AUTOHINT) != 0) {
			fprintf(stderr, "couldn't load glyph for '%c'\n", chr);
			continue;
		}
		FT_Render_Glyph(m_face->glyph, FT_RENDER_MODE_NORMAL);

		// face->glyph->bitmap
		// copy to square buffer GL can stomach
		const int pitch = m_face->glyph->bitmap.pitch;
		for (int row=0; row<m_face->glyph->bitmap.rows; row++) {
			for (int col=0; col<m_face->glyph->bitmap.width; col++) {
				pixBuf[2*sz*row + 2*col] = m_face->glyph->bitmap.buffer[pitch*row + col];
				pixBuf[2*sz*row + 2*col+1] = m_face->glyph->bitmap.buffer[pitch*row + col];
			}
		}

		glfglyph_t glyph;
		glEnable (GL_TEXTURE_2D);
		glGenTextures (1, &glyph.tex);
		glBindTexture (GL_TEXTURE_2D, glyph.tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, sz, sz, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, pixBuf);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glDisable (GL_TEXTURE_2D);

		glyph.width = m_face->glyph->bitmap.width / float(sz);
		glyph.height = m_face->glyph->bitmap.rows / float(sz);
		glyph.offx = m_face->glyph->bitmap_left;
		glyph.offy = m_face->glyph->bitmap_top;
		glyph.advx = float(m_face->glyph->advance.x) / 64.0 + advx_adjust;
		glyph.advy = float(m_face->glyph->advance.y) / 64.0;
		m_glyphs[chr] = glyph;
	}

	delete [] pixBuf;
	
	m_height = float(a_height);
	m_width = float(a_width);
	m_descender = -float(m_face->descender) / 64.0;
}

