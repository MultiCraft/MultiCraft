/*
   CGUITTFont FreeType class for Irrlicht
   Copyright (c) 2009-2010 John Norman
   Copyright (c) 2016 Nathanaël Courant

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any
   purpose, including commercial applications, and to alter it and
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
      distribution.

   The original version of this class can be located at:
   http://irrlicht.suckerfreegames.com/

   John Norman
   john@suckerfreegames.com
*/

#include <irrlicht.h>
#include <cstring>
#include <iostream>
#include "CGUITTFont.h"
#include "porting.h"

namespace irr
{
namespace gui
{

// Manages the FT_Face cache.
struct SGUITTFace : public virtual irr::IReferenceCounted
{
	SGUITTFace() : face_buffer(0), face_buffer_size(0)
	{
		memset((void*)&face, 0, sizeof(FT_Face));
	}

	~SGUITTFace()
	{
		FT_Done_Face(face);
		delete[] face_buffer;
	}

	FT_Face face;
	FT_Byte* face_buffer;
	FT_Long face_buffer_size;
};

// Static variables.
FT_Library CGUITTFont::c_library;
core::map<io::path, SGUITTFace*> CGUITTFont::c_faces;
bool CGUITTFont::c_libraryLoaded = false;
scene::IMesh* CGUITTFont::shared_plane_ptr_ = 0;
scene::SMesh CGUITTFont::shared_plane_;

//

/** Checks that no dimension of the FT_BitMap object is negative.  If either is
 * negative, abort execution.
 */
inline void checkFontBitmapSize(const FT_Bitmap &bits)
{
	if ((s32)bits.rows < 0 || (s32)bits.width < 0) {
		std::cout << "Insane font glyph size. File: "
		          << __FILE__ << " Line " << __LINE__
		          << std::endl;
		abort();
	}
}

u32 getBestFixedSizeIndex(FT_Face face, u32 font_size)
{
	u32 index = 0;
	u32 last_font_size =0;

	for (u32 i = 0; i < face->num_fixed_sizes; i++) {
		u32 current_size = face->available_sizes[i].height;

		if (current_size > last_font_size) {
			last_font_size = current_size;
			index = i;
		}
	}

	return index;
}

video::IImage* SGUITTGlyph::createGlyphImage(const FT_Face& face, const FT_Bitmap& bits, video::IVideoDriver* driver) const
{
	// Make sure our casts to s32 in the loops below will not cause problems
	checkFontBitmapSize(bits);

	// Determine what our texture size should be.
	// Add 1 because textures are inclusive-exclusive.
	core::dimension2du d(bits.width + 1, bits.rows + 1);
	core::dimension2du texture_size;
	//core::dimension2du texture_size(bits.width + 1, bits.rows + 1);

	// Create and load our image now.
	video::IImage* image = 0;
	switch (bits.pixel_mode)
	{
		case FT_PIXEL_MODE_MONO:
		{
			// Create a blank image and fill it with transparent pixels.
			texture_size = d.getOptimalSize(true, true);
			image = driver->createImage(video::ECF_A1R5G5B5, texture_size);
			image->fill(video::SColor(0, 255, 255, 255));

			// Load the monochrome data in.
			const u32 image_pitch = image->getPitch() / sizeof(u16);
			u16* image_data = (u16*)image->lock();
			u8* glyph_data = bits.buffer;

			for (s32 y = 0; y < (s32)bits.rows; ++y)
			{
				u16* row = image_data;
				for (s32 x = 0; x < (s32)bits.width; ++x)
				{
					// Monochrome bitmaps store 8 pixels per byte.  The left-most pixel is the bit 0x80.
					// So, we go through the data each bit at a time.
					if ((glyph_data[y * bits.pitch + (x / 8)] & (0x80 >> (x % 8))) != 0)
						*row = 0xFFFF;
					++row;
				}
				image_data += image_pitch;
			}
			image->unlock();
			break;
		}

		case FT_PIXEL_MODE_GRAY:
		{
			// Create our blank image.
			texture_size = d.getOptimalSize(!driver->queryFeature(video::EVDF_TEXTURE_NPOT), !driver->queryFeature(video::EVDF_TEXTURE_NSQUARE), true, 0);
			image = driver->createImage(video::ECF_A8R8G8B8, texture_size);
			image->fill(video::SColor(0, 255, 255, 255));

			// Load the grayscale data in.
			const float gray_count = static_cast<float>(bits.num_grays);
			const u32 image_pitch = image->getPitch() / sizeof(u32);
			u32* image_data = (u32*)image->lock();
			u8* glyph_data = bits.buffer;
			for (s32 y = 0; y < (s32)bits.rows; ++y)
			{
				u8* row = glyph_data;
				for (s32 x = 0; x < (s32)bits.width; ++x)
				{
					image_data[y * image_pitch + x] |= static_cast<u32>(255.0f * (static_cast<float>(*row++) / gray_count)) << 24;
					//data[y * image_pitch + x] |= ((u32)(*bitsdata++) << 24);
				}
				glyph_data += bits.pitch;
			}
			image->unlock();
			break;
		}
		case FT_PIXEL_MODE_BGRA:
		{
			int font_size = parent->getFontSize();
			bool needs_scaling = (face->num_fixed_sizes > 0 && bits.rows > 0 && bits.rows > font_size);

			if (needs_scaling)
				texture_size = d;
			else
				texture_size = d.getOptimalSize(!driver->queryFeature(video::EVDF_TEXTURE_NPOT), !driver->queryFeature(video::EVDF_TEXTURE_NSQUARE), true, 0);

			image = driver->createImage(video::ECF_A8R8G8B8, texture_size);
			image->fill(video::SColor(0, 255, 255, 255));

			const u32 image_pitch = image->getPitch();
			u8* image_data = (u8*)image->lock();
			u8* glyph_data = bits.buffer;
			for (s32 y = 0; y < (s32)bits.rows; ++y)
			{
				std::memcpy((void*)(&image_data[y * image_pitch]), glyph_data, bits.width * 4);
				glyph_data += bits.pitch;
			}
			image->unlock();

			if (needs_scaling) {
				float scale = (float)font_size / bits.rows;

				core::dimension2du d_new(bits.width * scale, bits.rows * scale);

				irr::video::IImage* scaled_img = driver->createImage(video::ECF_A8R8G8B8, d_new);
				image->copyToScalingBoxFilter(scaled_img);
				image->drop();
				image = scaled_img;
			}

			break;
		}
		default:
			// TODO: error message?
			return 0;
	}
	return image;
}

void SGUITTGlyph::preload(u32 char_index, FT_Face face,
		video::IVideoDriver* driver, u32 font_size, const FT_Int32 loadFlags,
		bool bold, bool italic)
{
	if (isLoaded) return;

	float scale = 1.0f;
	float bold_offset = 0;

	// Set the size of the glyph.
	FT_Set_Pixel_Sizes(face, 0, font_size);

	if (FT_HAS_COLOR(face) && face->num_fixed_sizes > 0) {
		best_fixed_size_index = getBestFixedSizeIndex(face, font_size);
		FT_Select_Size(face, best_fixed_size_index);
	}

	// Attempt to load the glyph.
	if (FT_Load_Glyph(face, char_index, loadFlags) != FT_Err_Ok)
		// TODO: error message?
		return;

	FT_GlyphSlot glyph = face->glyph;

	if (FT_HAS_COLOR(face) && face->num_fixed_sizes > 0 && glyph->bitmap.rows > 0) {
		scale = (float)font_size / glyph->bitmap.rows;
	}

	if (!FT_HAS_COLOR(face)) {
		if (bold) {
			float embolden_amount = (float)font_size * 2.0f;
			bold_offset = embolden_amount * 2.0f;
			FT_Outline_Embolden(&(glyph->outline), embolden_amount);
		}

		if (italic) {
			FT_Matrix italic_matrix;
			float slant = 0.2;
			italic_matrix.xx = 0x10000;
			italic_matrix.xy = (FT_Fixed)(slant * 0x10000);
			italic_matrix.yx = 0;
			italic_matrix.yy = 0x10000;

			FT_Outline_Transform(&(glyph->outline), &italic_matrix);
		}
	}

	if (!FT_HAS_COLOR(face) || face->num_fixed_sizes == 0) {
		FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);
	}

	FT_Bitmap bits = glyph->bitmap;

	// Setup the glyph information here:
	advance = glyph->advance;
	advance.x += bold_offset;
	advance.x *= scale;
	advance.y *= scale;
	offset = core::vector2di(glyph->bitmap_left * scale, glyph->bitmap_top * scale);

	// Try to get the last page with available slots.
	CGUITTGlyphPage* page = parent->getLastGlyphPage();

	if (page) {
		if (page->used_width + bits.width * scale > page->texture->getOriginalSize().Width) {
			page->used_width = 0;
			page->used_height += page->line_height;
			page->line_height = 0;
		}
	}

	// If we need to make a new page, do that now.
	if (!page || page->used_height + font_size > page->texture->getOriginalSize().Height)
	{
		page = parent->createGlyphPage(bits.pixel_mode);
		if (!page)
			// TODO: add error message?
			return;
	}

	glyph_page = parent->getLastGlyphPageIndex();

	core::vector2di page_position(page->used_width, page->used_height);
	source_rect.UpperLeftCorner = page_position;
	source_rect.LowerRightCorner = core::vector2di(page_position.X + bits.width * scale, page_position.Y + bits.rows * scale);

	page->dirty = true;
	page->used_width += bits.width * scale;
	page->line_height = std::max(page->line_height, (u32)(bits.rows * scale));

	// We grab the glyph bitmap here so the data won't be removed when the next glyph is loaded.
	surface = createGlyphImage(face, bits, driver);

	// Set our glyph as loaded.
	if (surface) {
		isColor = FT_HAS_COLOR(face);
		isLoaded = true;
	}
}

void SGUITTGlyph::unload()
{
	if (surface)
	{
		surface->drop();
		surface = 0;
	}
	isLoaded = false;
}

//////////////////////

CGUITTFont* CGUITTFont::createTTFont(IGUIEnvironment *env,
		const io::path& filename, const u32 size, const bool antialias,
		const bool transparency, const bool bold, const bool italic,
		const u32 shadow, const u32 shadow_alpha)
{
	if (!c_libraryLoaded)
	{
		if (FT_Init_FreeType(&c_library))
			return 0;
		c_libraryLoaded = true;
	}

	CGUITTFont* font = new CGUITTFont(env);

	font->shadow_alpha = shadow_alpha;
	font->bold = bold;
	font->italic = italic;

	bool ret = font->load(filename, size, antialias, transparency, shadow);
	if (!ret)
	{
		font->drop();
		return 0;
	}

	return font;
}

CGUITTFont* CGUITTFont::createTTFont(IrrlichtDevice *device,
		const io::path& filename, const u32 size, const bool antialias,
		const bool transparency, const bool bold, const bool italic)
{
	if (!c_libraryLoaded)
	{
		if (FT_Init_FreeType(&c_library))
			return 0;
		c_libraryLoaded = true;
	}

	CGUITTFont* font = new CGUITTFont(device->getGUIEnvironment());

	font->bold = bold;
	font->italic = italic;
	font->Device = device;

	bool ret = font->load(filename, size, antialias, transparency, false);
	if (!ret)
	{
		font->drop();
		return 0;
	}

	return font;
}

CGUITTFont* CGUITTFont::create(IGUIEnvironment *env, const io::path& filename,
		const u32 size, const bool antialias, const bool transparency,
		const bool bold, const bool italic)
{
	return CGUITTFont::createTTFont(env, filename, size, antialias,
			transparency, bold, italic);
}

CGUITTFont* CGUITTFont::create(IrrlichtDevice *device, const io::path& filename,
		const u32 size, const bool antialias, const bool transparency,
		const bool bold, const bool italic)
{
	return CGUITTFont::createTTFont(device, filename, size, antialias,
			transparency, bold, italic);
}

//////////////////////

//! Constructor.
CGUITTFont::CGUITTFont(IGUIEnvironment *env)
: use_monochrome(false), use_transparency(true), use_hinting(true), use_auto_hinting(true),
batch_load_size(1), Device(0), Environment(env), Driver(0), GlobalKerningWidth(0), GlobalKerningHeight(0)
{
	#ifdef _DEBUG
	setDebugName("CGUITTFont");
	#endif

	if (Environment)
	{
		// don't grab environment, to avoid circular references
		Driver = Environment->getVideoDriver();
	}

	if (Driver)
		Driver->grab();

	setInvisibleCharacters(L" ");

	// Glyphs aren't reference counted, so don't try to delete them when we free the array.
	Glyphs.set_free_when_destroyed(false);
}

bool CGUITTFont::load(const io::path& filename, const u32 size, const bool antialias, const bool transparency, const u32 shadow)
{
	// Some sanity checks.
	if (Environment == 0 || Driver == 0) return false;
	if (size == 0) return false;
	if (filename.size() == 0) return false;

	io::IFileSystem* filesystem = Environment->getFileSystem();
	irr::ILogger* logger = (Device != 0 ? Device->getLogger() : 0);
	this->size = size;
	this->filenames.push_back(filename);

	// Update the font loading flags when the font is first loaded.
	this->use_monochrome = !antialias;
	this->use_transparency = transparency;
	update_load_flags();

	// Log.
	if (logger)
		logger->log(L"CGUITTFont", core::stringw(core::stringw(L"Creating new font: ") + core::ustring(filename).toWCHAR_s() + L" " + core::stringc(size) + L"pt " + (antialias ? L"+antialias " : L"-antialias ") + (transparency ? L"+transparency" : L"-transparency")).c_str(), irr::ELL_INFORMATION);

	// Grab the face.
	SGUITTFace* face = 0;
	core::map<io::path, SGUITTFace*>::Node* node = c_faces.find(filename);
	if (node == 0)
	{
		face = new SGUITTFace();
		c_faces.set(filename, face);

		bool use_memory_face = true;

		if (filesystem)
		{
			// Read in the file data.
			io::IReadFile* file = filesystem->createAndOpenFile(filename);
			if (file == 0)
			{
				if (logger) logger->log(L"CGUITTFont", L"Failed to open the file.", irr::ELL_INFORMATION);

				c_faces.remove(filename);
				delete face;
				face = 0;
				return false;
			}

			// For devices with low RAM only fonts smaller than 5MB are loaded into memory. Otherwise just use FT_New_Face.
			int systemMemory = porting::getTotalSystemMemory();
			int maxFontSize = 5 * 1024 * 1024;
			size_t fileSize = file->getSize();
			if (systemMemory < 3072 && fileSize > maxFontSize) {
				use_memory_face = false;
			} else {
				face->face_buffer = new FT_Byte[fileSize];
				file->read(face->face_buffer, fileSize);
				face->face_buffer_size = fileSize;
				file->drop();

				// Create the face.
				if (FT_New_Memory_Face(c_library, face->face_buffer, face->face_buffer_size, 0, &face->face))
				{
					if (logger) logger->log(L"CGUITTFont", L"FT_New_Memory_Face failed.", irr::ELL_INFORMATION);

					c_faces.remove(filename);
					delete face;
					face = 0;
					return false;
				}
			}
		}

		if (!filesystem || !use_memory_face)
		{
			core::ustring converter(filename);
			if (FT_New_Face(c_library, reinterpret_cast<const char*>(converter.toUTF8_s().c_str()), 0, &face->face))
			{
				if (logger) logger->log(L"CGUITTFont", L"FT_New_Face failed.", irr::ELL_INFORMATION);

				c_faces.remove(filename);
				delete face;
				face = 0;
				return false;
			}
		}
	}
	else
	{
		// Using another instance of this face.
		face = node->getValue();
		face->grab();
	}

	// Store our face.
	tt_faces.push_back(face->face);
	tt_offsets.push_back(Glyphs.size());
	shadow_offsets.push_back(shadow);

	// Allocate our glyphs.
	for (FT_Long i = 0; i < face->face->num_glyphs; i++)
	{
		SGUITTGlyph* glyph = new SGUITTGlyph();
		glyph->isLoaded = false;
		glyph->isColor = false;
		glyph->glyph_page = 0;
		glyph->source_rect = core::recti();
		glyph->offset = core::vector2di();
		glyph->shadow_offset = 0;
		glyph->advance = FT_Vector();
		glyph->surface = 0;
		glyph->parent = this;

		Glyphs.push_back(glyph);
	}

	if (tt_faces.size() == 1) {
		// Store font metrics.
		FT_Set_Pixel_Sizes(face->face, size, 0);
		font_metrics = face->face->size->metrics;

		// Cache the first 127 ascii characters.
		u32 old_size = batch_load_size;
		batch_load_size = 127;
		getGlyphIndexByChar((uchar32_t)0);
		batch_load_size = old_size;
	}

	return true;
}

bool CGUITTFont::loadAdditionalFont(const io::path& filename, bool is_emoji_font, const u32 shadow)
{
	bool success = load(filename, size, !use_monochrome, use_transparency, shadow);

	if (!success || !is_emoji_font)
		return success;

	success = testEmojiFont(filename);

	if (!success) {
		filenames.pop_back();
		tt_faces.pop_back();
		tt_offsets.pop_back();
		shadow_offsets.pop_back();

		core::map<io::path, SGUITTFace*>::Node *node = c_faces.find(filename);
		if (node) {
			SGUITTFace *face = node->getValue();
			if (face->drop())
				c_faces.remove(filename);
		}

		return false;
	}

	return true;
}

bool CGUITTFont::testEmojiFont(const io::path& filename)
{
	FT_Face face = nullptr;

	for (size_t i = 0; i < filenames.size(); i++) {
		if (filenames[i] == filename) {
			face = tt_faces[i];
			break;
		}
	}

	if (!face)
		return false;

	uchar32_t smile = 0x1F600;
	u32 char_index = FT_Get_Char_Index(face, smile);

	if (char_index == 0)
		return false;

	FT_Int32 flags = load_flags;
	if (FT_HAS_COLOR(face))
		flags |= FT_LOAD_COLOR;
	else
		flags |= FT_LOAD_RENDER;

	FT_Set_Pixel_Sizes(face, 0, size);

	if (FT_HAS_COLOR(face) && face->num_fixed_sizes > 0) {
		u32 best_fixed_size_index = getBestFixedSizeIndex(face, size);
		FT_Select_Size(face, best_fixed_size_index);
	}

	if (FT_Load_Glyph(face, char_index, flags) != FT_Err_Ok)
		return false;

	FT_GlyphSlot glyph = face->glyph;

	if (FT_HAS_COLOR(face) && face->num_fixed_sizes == 0) {
		FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);
	}

	FT_Bitmap bits = glyph->bitmap;

	if (bits.rows < 1 || bits.width < 1)
		return false;

	if (FT_HAS_COLOR(face) && bits.pixel_mode != FT_PIXEL_MODE_BGRA)
		return false;

	if (!FT_HAS_COLOR(face) && bits.pixel_mode != FT_PIXEL_MODE_MONO &&
			bits.pixel_mode != FT_PIXEL_MODE_GRAY)
		return false;

	return true;
}

CGUITTFont::~CGUITTFont()
{
	// Delete the glyphs and glyph pages.
	reset_images();

	for (u32 i = 0; i < Glyphs.size(); i++) {
		delete Glyphs[i];
	}

	CGUITTAssistDelete::Delete(Glyphs);
	//Glyphs.clear();

	for (auto filename : filenames) {
		// We aren't using this face anymore.
		core::map<io::path, SGUITTFace*>::Node* n = c_faces.find(filename);
		if (n)
		{
			SGUITTFace* f = n->getValue();

			// Drop our face.  If this was the last face, the destructor will clean up.
			if (f->drop())
				c_faces.remove(filename);

			// If there are no more faces referenced by FreeType, clean up.
			if (c_faces.size() == 0)
			{
				FT_Done_FreeType(c_library);
				c_libraryLoaded = false;
			}
		}
	}

	// Drop our driver now.
	if (Driver)
		Driver->drop();
}

void CGUITTFont::reset_images()
{
	// Delete the glyphs.
	for (u32 i = 0; i != Glyphs.size(); ++i)
		Glyphs[i]->unload();

	// Unload the glyph pages from video memory.
	for (u32 i = 0; i != Glyph_Pages.size(); ++i)
		delete Glyph_Pages[i];
	Glyph_Pages.clear();

	// Always update the internal FreeType loading flags after resetting.
	update_load_flags();
}

void CGUITTFont::update_glyph_pages() const
{
	for (u32 i = 0; i != Glyph_Pages.size(); ++i)
	{
		if (Glyph_Pages[i]->dirty)
			Glyph_Pages[i]->updateTexture();
	}
}

CGUITTGlyphPage* CGUITTFont::getLastGlyphPage() const
{
	if (Glyph_Pages.empty())
		return 0;
	else
		return Glyph_Pages[getLastGlyphPageIndex()];
}

CGUITTGlyphPage* CGUITTFont::createGlyphPage(const u8& pixel_mode)
{
	CGUITTGlyphPage* page = 0;

	// Name of our page.
	io::path name("TTFontGlyphPage_");
	name += tt_faces[0]->family_name;
	name += ".";
	name += tt_faces[0]->style_name;
	name += ".";
	name += size;
	name += "_";
	name += Glyph_Pages.size(); // The newly created page will be at the end of the collection.

	// Create the new page.
	page = new CGUITTGlyphPage(Driver, name);

	// Determine our maximum texture size.
	// If we keep getting 0, set it to 1024x1024, as that number is pretty safe.
	core::dimension2du max_texture_size = max_page_texture_size;
	if (max_texture_size.Width == 0 || max_texture_size.Height == 0)
		max_texture_size = Driver->getMaxTextureSize();
	if (max_texture_size.Width == 0 || max_texture_size.Height == 0)
		max_texture_size = core::dimension2du(1024, 1024);

	// We want to try to put at least 144 glyphs on a single texture.
	core::dimension2du page_texture_size;
	if (size <= 21) page_texture_size = core::dimension2du(256, 256);
	else if (size <= 42) page_texture_size = core::dimension2du(512, 512);
	else if (size <= 84) page_texture_size = core::dimension2du(1024, 1024);
	else if (size <= 168) page_texture_size = core::dimension2du(2048, 2048);
	else page_texture_size = core::dimension2du(4096, 4096);

	if (page_texture_size.Width > max_texture_size.Width || page_texture_size.Height > max_texture_size.Height)
		page_texture_size = max_texture_size;

	if (!page->createPageTexture(pixel_mode, page_texture_size)) {
		// TODO: add error message?
		delete page;
		return 0;
	}

	if (page)
	{
		Glyph_Pages.push_back(page);
	}
	return page;
}

void CGUITTFont::setTransparency(const bool flag)
{
	use_transparency = flag;
	reset_images();
}

void CGUITTFont::setMonochrome(const bool flag)
{
	use_monochrome = flag;
	reset_images();
}

void CGUITTFont::setFontHinting(const bool enable, const bool enable_auto_hinting)
{
	use_hinting = enable;
	use_auto_hinting = enable_auto_hinting;
	reset_images();
}

void CGUITTFont::draw(const core::stringw& text, const core::rect<s32>& position, video::SColor color, bool hcenter, bool vcenter, const core::rect<s32>* clip)
{
	draw(EnrichedString(std::wstring(text.c_str()), color), position, hcenter, vcenter, clip);
}

void CGUITTFont::draw(const EnrichedString &text, const core::rect<s32>& position, bool hcenter, bool vcenter, const core::rect<s32>* clip)
{
	const std::vector<video::SColor> &colors = text.getColors();

	if (!Driver)
		return;

	// Clear the glyph pages of their render information.
	for (u32 i = 0; i < Glyph_Pages.size(); ++i)
	{
		Glyph_Pages[i]->render_positions.clear();
		Glyph_Pages[i]->render_source_rects.clear();
		Glyph_Pages[i]->render_colors.clear();
		Glyph_Pages[i]->shadow_positions.clear();
		Glyph_Pages[i]->shadow_source_rects.clear();
	}

	// Set up some variables.
	core::dimension2d<s32> textDimension;
	core::position2d<s32> offset = position.UpperLeftCorner;

	// Determine offset positions.
	if (hcenter || vcenter)
	{
		textDimension = getDimension(text.c_str());

		if (hcenter)
			offset.X = ((position.getWidth() - textDimension.Width) >> 1) + offset.X;

		if (vcenter)
			offset.Y = ((position.getHeight() - textDimension.Height) >> 1) + offset.Y;
	}

	// Convert to a unicode string.
	core::ustring utext = text.getString();

	// Set up our render map.
	core::map<u32, CGUITTGlyphPage*> Render_Map;

	// Start parsing characters.
	u32 n;
	uchar32_t previousChar = 0;
	core::ustring::const_iterator iter(utext);
	u32 charPos = 0; // Position in Unicode characters (not UTF-16)
	while (!iter.atEnd())
	{
		uchar32_t currentChar = *iter;
		n = getGlyphIndexByChar(currentChar);
		bool visible = (Invisible.findFirst(currentChar) == -1);
		bool lineBreak=false;
		if (currentChar == L'\r') // Mac or Windows breaks
		{
			lineBreak = true;
			if (*(iter + 1) == (uchar32_t)'\n') { 	// Windows line breaks.
				currentChar = *(++iter);
				++charPos;
			}
		}
		else if (currentChar == (uchar32_t)'\n') // Unix breaks
		{
			lineBreak = true;
		}

		if (lineBreak)
		{
			previousChar = 0;
			offset.Y += font_metrics.height / 64;
			offset.X = position.UpperLeftCorner.X;

			if (hcenter)
				offset.X += (position.getWidth() - textDimension.Width) >> 1;
			++iter;
			++charPos;
			continue;
		}

		if (n > 0 && visible)
		{
			// Calculate the glyph offset.
			s32 offx = Glyphs[n-1]->offset.X;
			s32 offy = (font_metrics.ascender / 64) - Glyphs[n-1]->offset.Y;

			// Apply kerning.
			core::vector2di k = getKerning(currentChar, previousChar);
			offset.X += k.X;
			offset.Y += k.Y;

			// Determine rendering information.
			SGUITTGlyph* glyph = Glyphs[n-1];
			CGUITTGlyphPage* const page = Glyph_Pages[glyph->glyph_page];
			page->render_positions.push_back(core::position2di(offset.X + offx, offset.Y + offy));
			page->render_source_rects.push_back(glyph->source_rect);

			if (glyph->shadow_offset) {
				page->shadow_positions.push_back(core::position2di(
						offset.X + offx + glyph->shadow_offset,
						offset.Y + offy + glyph->shadow_offset));
				page->shadow_source_rects.push_back(glyph->source_rect);
			}

			// If wchar_t is 32-bit then use charPos instead
			u32 iterPos = sizeof(wchar_t) == 4 ? charPos : iter.getPos();

			if (!glyph->isColor && iterPos < colors.size())
				page->render_colors.push_back(colors[iterPos]);
			else
				page->render_colors.push_back(video::SColor(255,255,255,255));
			Render_Map.set(glyph->glyph_page, page);
		}
		offset.X += getWidthFromCharacter(currentChar);

		previousChar = currentChar;
		++iter;
		++charPos;
	}

	// Draw now.
	update_glyph_pages();
	core::map<u32, CGUITTGlyphPage*>::Iterator j = Render_Map.getIterator();
	while (!j.atEnd())
	{
		core::map<u32, CGUITTGlyphPage*>::Node* n = j.getNode();
		j++;
		if (n == 0) continue;

		CGUITTGlyphPage* page = n->getValue();

		if (!page->shadow_positions.empty()) {
			Driver->draw2DImageBatch(page->texture, page->shadow_positions, page->shadow_source_rects, clip, video::SColor(shadow_alpha,0,0,0), true);
		}
		// render runs of matching color in batch
		size_t ibegin;
		video::SColor colprev;
		for (size_t i = 0; i < page->render_positions.size(); ++i) {
			ibegin = i;
			colprev = page->render_colors[i];
			do
				++i;
			while (i < page->render_positions.size() && page->render_colors[i] == colprev);
			core::array<core::vector2di> tmp_positions;
			core::array<core::recti> tmp_source_rects;
			tmp_positions.set_pointer(&page->render_positions[ibegin], i - ibegin, false, false); // no copy
			tmp_source_rects.set_pointer(&page->render_source_rects[ibegin], i - ibegin, false, false);
			--i;
			if (!use_transparency)
				colprev.color |= 0xff000000;
			Driver->draw2DImageBatch(page->texture, tmp_positions, tmp_source_rects, clip, colprev, true);
		}
	}
}

core::dimension2d<u32> CGUITTFont::getCharDimension(const wchar_t ch) const
{
	return core::dimension2d<u32>(getWidthFromCharacter(ch), getHeightFromCharacter(ch));
}

core::dimension2d<u32> CGUITTFont::getDimension(const wchar_t* text) const
{
	return getDimension(core::ustring(text));
}

core::dimension2d<u32> CGUITTFont::getDimension(const core::ustring& text) const
{
	// Get the maximum font height.  Unfortunately, we have to do this hack as
	// Irrlicht will draw things wrong.  In FreeType, the font size is the
	// maximum size for a single glyph, but that glyph may hang "under" the
	// draw line, increasing the total font height to beyond the set size.
	// Irrlicht does not understand this concept when drawing fonts.  Also, I
	// add +1 to give it a 1 pixel blank border.  This makes things like
	// tooltips look nicer.
	s32 test1 = getHeightFromCharacter((uchar32_t)'g') + 1;
	s32 test2 = getHeightFromCharacter((uchar32_t)'j') + 1;
	s32 test3 = getHeightFromCharacter((uchar32_t)'_') + 1;
	s32 max_font_height = core::max_(test1, core::max_(test2, test3));

	core::dimension2d<u32> text_dimension(0, max_font_height);
	core::dimension2d<u32> line(0, max_font_height);

	uchar32_t previousChar = 0;
	core::ustring::const_iterator iter = text.begin();
	for (; !iter.atEnd(); ++iter)
	{
		uchar32_t p = *iter;
		bool lineBreak = false;
		if (p == '\r')	// Mac or Windows line breaks.
		{
			lineBreak = true;
			if (*(iter + 1) == '\n')
			{
				++iter;
				p = *iter;
			}
		}
		else if (p == '\n')	// Unix line breaks.
		{
			lineBreak = true;
		}

		// Kerning.
		core::vector2di k = getKerning(p, previousChar);
		line.Width += k.X;
		previousChar = p;

		// Check for linebreak.
		if (lineBreak)
		{
			previousChar = 0;
			text_dimension.Height += line.Height;
			if (text_dimension.Width < line.Width)
				text_dimension.Width = line.Width;
			line.Width = 0;
			line.Height = max_font_height;
			continue;
		}
		line.Width += getWidthFromCharacter(p);
	}
	if (text_dimension.Width < line.Width)
		text_dimension.Width = line.Width;

	return text_dimension;
}

core::dimension2d<u32> CGUITTFont::getTotalDimension(const wchar_t* text) const
{
	return getTotalDimension(core::ustring(text));
}

core::dimension2d<u32> CGUITTFont::getTotalDimension(const core::ustring& text) const
{
	core::dimension2d<u32> text_dimension = getDimension(text);

	s32 test1 = getHeightFromCharacter((uchar32_t)'g') + 1;
	s32 test2 = getHeightFromCharacter((uchar32_t)'j') + 1;
	s32 test3 = getHeightFromCharacter((uchar32_t)'_') + 1;
	s32 max_font_height = core::max_(test1, core::max_(test2, test3));

	if (italic) {
		float slant = 0.2f;
		s32 italic_extra_width = static_cast<s32>(max_font_height * slant);
		text_dimension.Width += italic_extra_width;
	}

	if (bold) {
		s32 bold_extra_width = size * 0.1f;
		text_dimension.Width += bold_extra_width;
	}

	return text_dimension;
}

inline u32 CGUITTFont::getWidthFromCharacter(wchar_t c) const
{
	return getWidthFromCharacter((uchar32_t)c);
}

inline u32 CGUITTFont::getWidthFromCharacter(uchar32_t c) const
{
	// Set the size of the face.
	// This is because we cache faces and the face may have been set to a different size.
	//FT_Set_Pixel_Sizes(tt_face, 0, size);

	u32 n = getGlyphIndexByChar(c);
	if (n > 0)
	{
		int w = Glyphs[n-1]->advance.x / 64;
		return w;
	}
	if (c >= 0xFE00 && c <= 0xFE0F) // variation selectors
		return 0;
	else if (c >= 0x2000)
		return (font_metrics.ascender / 64);
	else return (font_metrics.ascender / 64) / 2;
}

inline u32 CGUITTFont::getHeightFromCharacter(wchar_t c) const
{
	return getHeightFromCharacter((uchar32_t)c);
}

inline u32 CGUITTFont::getHeightFromCharacter(uchar32_t c) const
{
	// Set the size of the face.
	// This is because we cache faces and the face may have been set to a different size.
	//FT_Set_Pixel_Sizes(tt_face, 0, size);

	u32 n = getGlyphIndexByChar(c);
	if (n > 0)
	{
		// Grab the true height of the character, taking into account underhanging glyphs.
		s32 height = (font_metrics.ascender / 64) - Glyphs[n-1]->offset.Y + Glyphs[n-1]->source_rect.getHeight();
		return height;
	}
	if (c >= 0xFE00 && c <= 0xFE0F) // variation selectors
		return 0;
	else if (c >= 0x2000)
		return (font_metrics.ascender / 64);
	else return (font_metrics.ascender / 64) / 2;
}

u32 CGUITTFont::getGlyphIndexByChar(wchar_t c) const
{
	return getGlyphIndexByChar((uchar32_t)c);
}

u32 CGUITTFont::getGlyphIndexByChar(uchar32_t c) const
{
	// If our glyph is already loaded, don't bother doing any batch loading code.
	for (size_t i = 0; i < tt_faces.size(); i++) {
		u32 glyph = FT_Get_Char_Index(tt_faces[i], c);
		int tt_offset = tt_offsets[i];

		if (glyph != 0 && Glyphs[tt_offset + glyph - 1]->isLoaded) {
			return glyph + tt_offset;
		}
	}

	size_t current_face = 0;

begin:

	// Get the glyph.
	FT_Face tt_face = tt_faces[0];
	u32 glyph = 0;
	int tt_offset = 0;
	u32 shadow_offset = shadow_offsets[0];
	for (size_t i = current_face; i < tt_faces.size(); i++) {
		glyph = FT_Get_Char_Index(tt_faces[i], c);

		if (glyph != 0) {
			tt_face = tt_faces[i];
			tt_offset = tt_offsets[i];
			shadow_offset = shadow_offsets[i];
			current_face = i;
			break;
		}
	}

	// Check for a valid glyph.  If it is invalid, attempt to use the replacement character.
	if (glyph == 0)
		glyph = FT_Get_Char_Index(tt_face, core::unicode::UTF_REPLACEMENT_CHARACTER);

	// Determine our batch loading positions.
	u32 half_size = (batch_load_size / 2);
	u32 start_pos = 0;
	if (c > half_size) start_pos = c - half_size;
	u32 end_pos = start_pos + batch_load_size;

	// Load all our characters.
	do
	{
		// Get the character we are going to load.
		u32 char_index = FT_Get_Char_Index(tt_face, start_pos);

		// If the glyph hasn't been loaded yet, do it now.
		if (char_index)
		{
			SGUITTGlyph* glyph = Glyphs[tt_offset + char_index - 1];
			if (!glyph->isLoaded)
			{
				FT_Int32 flags = load_flags;
				if (FT_HAS_COLOR(tt_face))
					flags |= FT_LOAD_COLOR;
				else
					flags |= FT_LOAD_DEFAULT;

				glyph->preload(char_index, tt_face, Driver, size, flags, bold,
						italic);

				if (!glyph->isLoaded && current_face < tt_faces.size()) {
					current_face++;
					goto begin;
				}

				glyph->shadow_offset = shadow_offset;
				Glyph_Pages[glyph->glyph_page]->pushGlyphToBePaged(glyph);
			}
		}
	}
	while (++start_pos < end_pos);

	// Return our original character.
	return glyph + tt_offset;
}

s32 CGUITTFont::getFaceIndexByChar(uchar32_t c) const
{
	for (size_t i = 0; i < tt_faces.size(); i++) {
		u32 glyph = FT_Get_Char_Index(tt_faces[i], c);

		if (glyph != 0)
			return i;
	}

	return -1;
}

s32 CGUITTFont::getCharacterFromPos(const wchar_t* text, s32 pixel_x) const
{
	return getCharacterFromPos(core::ustring(text), pixel_x);
}

s32 CGUITTFont::getCharacterFromPos(const core::ustring& text, s32 pixel_x) const
{
	s32 x = 0;
	//s32 idx = 0;

	u32 character = 0;
	uchar32_t previousChar = 0;
	core::ustring::const_iterator iter = text.begin();
	while (!iter.atEnd())
	{
		uchar32_t c = *iter;
		x += getWidthFromCharacter(c);

		// Kerning.
		core::vector2di k = getKerning(c, previousChar);
		x += k.X;

		if (x >= pixel_x)
			return character;

		previousChar = c;
		++iter;
		++character;
	}

	return -1;
}

void CGUITTFont::setKerningWidth(s32 kerning)
{
	GlobalKerningWidth = kerning;
}

void CGUITTFont::setKerningHeight(s32 kerning)
{
	GlobalKerningHeight = kerning;
}

s32 CGUITTFont::getKerningWidth(const wchar_t* thisLetter, const wchar_t* previousLetter) const
{
	if (tt_faces[0] == 0)
		return GlobalKerningWidth;
	if (thisLetter == 0 || previousLetter == 0)
		return 0;

	return getKerningWidth((uchar32_t)*thisLetter, (uchar32_t)*previousLetter);
}

s32 CGUITTFont::getKerningWidth(const uchar32_t thisLetter, const uchar32_t previousLetter) const
{
	// Return only the kerning width.
	return getKerning(thisLetter, previousLetter).X;
}

s32 CGUITTFont::getKerningHeight() const
{
	// FreeType 2 currently doesn't return any height kerning information.
	return GlobalKerningHeight;
}

core::vector2di CGUITTFont::getKerning(const wchar_t thisLetter, const wchar_t previousLetter) const
{
	return getKerning((uchar32_t)thisLetter, (uchar32_t)previousLetter);
}

core::vector2di CGUITTFont::getKerning(const uchar32_t thisLetter, const uchar32_t previousLetter) const
{
	if (tt_faces[0] == 0 || thisLetter == 0 || previousLetter == 0)
		return core::vector2di();

	// Set the size of the face.
	// This is because we cache faces and the face may have been set to a different size.
	for (auto tt_face : tt_faces) {
		FT_Set_Pixel_Sizes(tt_face, 0, size);
	}

	core::vector2di ret(GlobalKerningWidth, GlobalKerningHeight);

	// Use global kerning if chars come from two different faces
	s32 first_face_index = getFaceIndexByChar(previousLetter);
	s32 second_face_index = getFaceIndexByChar(thisLetter);
	if (first_face_index != -1 && second_face_index != -1 &&
			first_face_index != second_face_index) {
		return ret;
	}

	u32 face_index = 0;
	if (first_face_index != -1)
		face_index = first_face_index;
	else if (second_face_index != -1)
		face_index = second_face_index;

	// If we don't have kerning, no point in continuing.
	if (!FT_HAS_KERNING(tt_faces[face_index]))
		return ret;

	// Get the kerning information.
	FT_Vector v;
	FT_Get_Kerning(tt_faces[face_index], getGlyphIndexByChar(previousLetter), getGlyphIndexByChar(thisLetter), FT_KERNING_DEFAULT, &v);

	// If we have a scalable font, the return value will be in font points.
	if (FT_IS_SCALABLE(tt_faces[face_index]))
	{
		// Font points, so divide by 64.
		ret.X += (v.x / 64);
		ret.Y += (v.y / 64);
	}
	else
	{
		// Pixel units.
		ret.X += v.x;
		ret.Y += v.y;
	}
	return ret;
}

void CGUITTFont::setInvisibleCharacters(const wchar_t *s)
{
	core::ustring us(s);
	Invisible = us;
}

void CGUITTFont::setInvisibleCharacters(const core::ustring& s)
{
	Invisible = s;
}

video::IImage* CGUITTFont::createTextureFromChar(const uchar32_t& ch)
{
	u32 n = getGlyphIndexByChar(ch);
	const SGUITTGlyph* glyph = Glyphs[n-1];
	CGUITTGlyphPage* page = Glyph_Pages[glyph->glyph_page];

	if (page->dirty)
		page->updateTexture();

	video::ITexture* tex = page->texture;

	// Acquire a read-only lock of the corresponding page texture.
	#if IRRLICHT_VERSION_MAJOR==1 && IRRLICHT_VERSION_MINOR>=8
	void* ptr = tex->lock(video::ETLM_READ_ONLY);
	#else
	void* ptr = tex->lock(true);
	#endif

	video::ECOLOR_FORMAT format = tex->getColorFormat();
	core::dimension2du tex_size = tex->getOriginalSize();
	video::IImage* pageholder = Driver->createImageFromData(format, tex_size, ptr, true, false);

	// Copy the image data out of the page texture.
	core::dimension2du glyph_size(glyph->source_rect.getSize());
	video::IImage* image = Driver->createImage(format, glyph_size);
	pageholder->copyTo(image, core::position2di(0, 0), glyph->source_rect);

	tex->unlock();
	return image;
}

video::ITexture* CGUITTFont::getPageTextureByIndex(const u32& page_index) const
{
	if (page_index < Glyph_Pages.size())
		return Glyph_Pages[page_index]->texture;
	else
		return 0;
}

void CGUITTFont::createSharedPlane()
{
	/*
		2___3
		|  /|
		| / |	<-- plane mesh is like this, point 2 is (0,0), point 0 is (0, -1)
		|/  |	<-- the texture coords of point 2 is (0,0, point 0 is (0, 1)
		0---1
	*/

	using namespace core;
	using namespace video;
	using namespace scene;
	S3DVertex vertices[4];
	u16 indices[6] = {0,2,3,3,1,0};
	vertices[0] = S3DVertex(vector3df(0,-1,0), vector3df(0,0,-1), SColor(255,255,255,255), vector2df(0,1));
	vertices[1] = S3DVertex(vector3df(1,-1,0), vector3df(0,0,-1), SColor(255,255,255,255), vector2df(1,1));
	vertices[2] = S3DVertex(vector3df(0, 0,0), vector3df(0,0,-1), SColor(255,255,255,255), vector2df(0,0));
	vertices[3] = S3DVertex(vector3df(1, 0,0), vector3df(0,0,-1), SColor(255,255,255,255), vector2df(1,0));

	SMeshBuffer* buf = new SMeshBuffer();
	buf->append(vertices, 4, indices, 6);

	shared_plane_.addMeshBuffer( buf );

	shared_plane_ptr_ = &shared_plane_;
	buf->drop(); //the addMeshBuffer method will grab it, so we can drop this ptr.
}

core::dimension2d<u32> CGUITTFont::getDimensionUntilEndOfLine(const wchar_t* p) const
{
	core::stringw s;
	for (const wchar_t* temp = p; temp && *temp != '\0' && *temp != L'\r' && *temp != L'\n'; ++temp )
		s.append(*temp);

	return getDimension(s.c_str());
}

core::array<scene::ISceneNode*> CGUITTFont::addTextSceneNode(const wchar_t* text, scene::ISceneManager* smgr, scene::ISceneNode* parent, const video::SColor& color, bool center)
{
	using namespace core;
	using namespace video;
	using namespace scene;

	array<scene::ISceneNode*> container;

	if (!Driver || !smgr) return container;
	if (!parent)
		parent = smgr->addEmptySceneNode(smgr->getRootSceneNode(), -1);
	// if you don't specify parent, then we add a empty node attached to the root node
	// this is generally undesirable.

	if (!shared_plane_ptr_) //this points to a static mesh that contains the plane
		createSharedPlane(); //if it's not initialized, we create one.

	dimension2d<s32> text_size(getDimension(text)); //convert from unsigned to signed.
	vector3df start_point(0, 0, 0), offset;

	/** NOTICE:
		Because we are considering adding texts into 3D world, all Y axis vectors are inverted.
	**/

	// There's currently no "vertical center" concept when you apply text scene node to the 3D world.
	if (center)
	{
		offset.X = start_point.X = -text_size.Width / 2.f;
		offset.Y = start_point.Y = +text_size.Height/ 2.f;
		offset.X += (text_size.Width - getDimensionUntilEndOfLine(text).Width) >> 1;
	}

	// the default font material
	SMaterial mat;
	mat.setFlag(video::EMF_LIGHTING, true);
	mat.setFlag(video::EMF_ZWRITE_ENABLE, false);
	mat.setFlag(video::EMF_NORMALIZE_NORMALS, true);
	mat.ColorMaterial = video::ECM_NONE;
	mat.MaterialType = use_transparency ? video::EMT_TRANSPARENT_ALPHA_CHANNEL : video::EMT_SOLID;
	mat.MaterialTypeParam = 0.01f;
	mat.DiffuseColor = color;

	wchar_t current_char = 0, previous_char = 0;
	u32 n = 0;

	array<u32> glyph_indices;

	while (*text)
	{
		current_char = *text;
		bool line_break=false;
		if (current_char == L'\r') // Mac or Windows breaks
		{
			line_break = true;
			if (*(text + 1) == L'\n') // Windows line breaks.
				current_char = *(++text);
		}
		else if (current_char == L'\n') // Unix breaks
		{
			line_break = true;
		}

		if (line_break)
		{
			previous_char = 0;
			offset.Y -= tt_faces[0]->size->metrics.ascender / 64;
			offset.X = start_point.X;
			if (center)
				offset.X += (text_size.Width - getDimensionUntilEndOfLine(text+1).Width) >> 1;
			++text;
		}
		else
		{
			n = getGlyphIndexByChar(current_char);
			if (n > 0)
			{
				glyph_indices.push_back( n );

				// Store glyph size and offset informations.
				SGUITTGlyph* glyph = Glyphs[n-1];
				u32 texw = glyph->source_rect.getWidth();
				u32 texh = glyph->source_rect.getHeight();
				s32 offx = glyph->offset.X;
				s32 offy = (font_metrics.ascender / 64) - glyph->offset.Y;

				// Apply kerning.
				vector2di k = getKerning(current_char, previous_char);
				offset.X += k.X;
				offset.Y += k.Y;

				vector3df current_pos(offset.X + offx, offset.Y - offy, 0);
				dimension2d<u32> letter_size = dimension2d<u32>(texw, texh);

				// Now we copy planes corresponding to the letter size.
				IMeshManipulator* mani = smgr->getMeshManipulator();
				IMesh* meshcopy = mani->createMeshCopy(shared_plane_ptr_);
				#if IRRLICHT_VERSION_MAJOR==1 && IRRLICHT_VERSION_MINOR>=8
				mani->scale(meshcopy, vector3df((f32)letter_size.Width, (f32)letter_size.Height, 1));
				#else
				mani->scaleMesh(meshcopy, vector3df((f32)letter_size.Width, (f32)letter_size.Height, 1));
				#endif

				ISceneNode* current_node = smgr->addMeshSceneNode(meshcopy, parent, -1, current_pos);
				meshcopy->drop();

				current_node->getMaterial(0) = mat;
				current_node->setAutomaticCulling(EAC_OFF);
				current_node->setIsDebugObject(true);  //so the picking won't have any effect on individual letter
				//current_node->setDebugDataVisible(EDS_BBOX); //de-comment this when debugging

				container.push_back(current_node);
			}
			offset.X += getWidthFromCharacter(current_char);
			previous_char = current_char;
			++text;
		}
	}

	update_glyph_pages();
	//only after we update the textures can we use the glyph page textures.

	for (u32 i = 0; i < glyph_indices.size(); ++i)
	{
		u32 n = glyph_indices[i];
		SGUITTGlyph* glyph = Glyphs[n-1];
		ITexture* current_tex = Glyph_Pages[glyph->glyph_page]->texture;
		f32 page_texture_size = (f32)current_tex->getSize().Width;
		//Now we calculate the UV position according to the texture size and the source rect.
		//
		//  2___3
		//  |  /|
		//  | / |	<-- plane mesh is like this, point 2 is (0,0), point 0 is (0, -1)
		//  |/  |	<-- the texture coords of point 2 is (0,0, point 0 is (0, 1)
		//  0---1
		//
		f32 u1 = glyph->source_rect.UpperLeftCorner.X / page_texture_size;
		f32 u2 = u1 + (glyph->source_rect.getWidth() / page_texture_size);
		f32 v1 = glyph->source_rect.UpperLeftCorner.Y / page_texture_size;
		f32 v2 = v1 + (glyph->source_rect.getHeight() / page_texture_size);

		//we can be quite sure that this is IMeshSceneNode, because we just added them in the above loop.
		IMeshSceneNode* node = static_cast<IMeshSceneNode*>(container[i]);

		S3DVertex* pv = static_cast<S3DVertex*>(node->getMesh()->getMeshBuffer(0)->getVertices());
		//pv[0].TCoords.Y = pv[1].TCoords.Y = (letter_size.Height - 1) / static_cast<f32>(letter_size.Height);
		//pv[1].TCoords.X = pv[3].TCoords.X = (letter_size.Width - 1)  / static_cast<f32>(letter_size.Width);
		pv[0].TCoords = vector2df(u1, v2);
		pv[1].TCoords = vector2df(u2, v2);
		pv[2].TCoords = vector2df(u1, v1);
		pv[3].TCoords = vector2df(u2, v1);

		container[i]->getMaterial(0).setTexture(0, current_tex);
	}

	return container;
}

} // end namespace gui
} // end namespace irr
