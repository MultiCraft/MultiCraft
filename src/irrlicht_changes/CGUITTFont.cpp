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
FT_Stroker CGUITTFont::stroker;
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

video::IImage* SGUITTGlyph::createGlyphImage(const FT_Face& face, const FT_Bitmap& bits, video::IVideoDriver* driver, video::SColor color) const
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
			image->fill(color);

			// Load the monochrome data in.
			const u32 image_pitch = image->getPitch() / sizeof(u16);
			u16* image_data = (u16*)image->getData();
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
			break;
		}

		case FT_PIXEL_MODE_GRAY:
		{
			// Create our blank image.
			texture_size = d.getOptimalSize(!driver->queryFeature(video::EVDF_TEXTURE_NPOT), !driver->queryFeature(video::EVDF_TEXTURE_NSQUARE), true, 0);
			image = driver->createImage(video::ECF_A8R8G8B8, texture_size);
			image->fill(color);

			// Load the grayscale data in.
			const float gray_count = static_cast<float>(bits.num_grays);
			const u32 image_pitch = image->getPitch() / sizeof(u32);
			u32* image_data = (u32*)image->getData();
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
			break;
		}
		case FT_PIXEL_MODE_BGRA:
		{
			float scale = parent->getColorEmojiScale();
			bool needs_scaling = (face->num_fixed_sizes > 0 && scale != 1.0f);

			if (needs_scaling)
				texture_size = d;
			else
				texture_size = d.getOptimalSize(!driver->queryFeature(video::EVDF_TEXTURE_NPOT), !driver->queryFeature(video::EVDF_TEXTURE_NSQUARE), true, 0);

			image = driver->createImage(video::ECF_A8R8G8B8, texture_size);
			image->fill(color);

			const u32 image_pitch = image->getPitch();
			u8* image_data = (u8*)image->getData();
			u8* glyph_data = bits.buffer;
			for (s32 y = 0; y < (s32)bits.rows; ++y)
			{
				std::memcpy((void*)(&image_data[y * image_pitch]), glyph_data, bits.width * 4);
				glyph_data += bits.pitch;
			}

			if (needs_scaling) {
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
		bool bold, bool italic, u16 outline, u8 outline_type, s8 character_spacing)
{
	if (isLoaded) return;

	float scale = 1.0f;
	float bold_offset = 0;

	// Set the size of the glyph.
	FT_Set_Pixel_Sizes(face, 0, font_size);

	if (FT_HAS_COLOR(face) && face->num_fixed_sizes > 0) {
		best_fixed_size_index = getBestFixedSizeIndex(face, font_size);
		FT_Select_Size(face, best_fixed_size_index);
		scale = parent->getColorEmojiScale();
	}

	// Attempt to load the glyph.
	if (FT_Load_Glyph(face, char_index, loadFlags) != FT_Err_Ok)
		// TODO: error message?
		return;

	FT_Glyph glyph = nullptr;
	FT_Bitmap bits;

	FT_GlyphSlot glyph_slot = face->glyph;
	if (!FT_HAS_COLOR(face)) {
		if (FT_Get_Glyph(glyph_slot, &glyph) != FT_Err_Ok)
			return;

		FT_OutlineGlyph glyph_outline = (FT_OutlineGlyph)glyph;

		if (bold) {
			float embolden_amount = (float)font_size * 2.0f;
			bold_offset = embolden_amount * 2.0f;
			FT_Outline_Embolden(&(glyph_outline->outline), embolden_amount);
		}

		if (italic) {
			FT_Matrix italic_matrix;
			float slant = 0.2;
			italic_matrix.xx = 0x10000;
			italic_matrix.xy = (FT_Fixed)(slant * 0x10000);
			italic_matrix.yx = 0;
			italic_matrix.yy = 0x10000;

			FT_Outline_Transform(&(glyph_outline->outline), &italic_matrix);
		}

		bold_offset += (float)character_spacing * 64.0f;
		if (outline > 0) {
			FT_Pos outline_strength = outline * 64;

			FT_Stroker stroker = parent->getStroker();

			if (outline_type == 1) {
				FT_Stroker_Set(stroker, outline * 64,
						FT_STROKER_LINECAP_ROUND,
						FT_STROKER_LINEJOIN_BEVEL,
						0);
			} else if (outline_type == 2) {
				FT_Stroker_Set(stroker, outline_strength,
						FT_STROKER_LINECAP_BUTT,
						FT_STROKER_LINEJOIN_BEVEL,
						0);
			} else if (outline_type == 3) {
				FT_Stroker_Set(stroker, outline_strength,
						FT_STROKER_LINECAP_BUTT,
						FT_STROKER_LINEJOIN_MITER_VARIABLE,
						8 << 16);
			} else if (outline_type == 4) {
				FT_Stroker_Set(stroker, outline_strength,
						FT_STROKER_LINECAP_ROUND,
						FT_STROKER_LINEJOIN_MITER_FIXED,
						2 << 16);
			} else if (outline_type == 5) {
				FT_Stroker_Set(stroker, outline_strength,
						FT_STROKER_LINECAP_SQUARE,
						FT_STROKER_LINEJOIN_ROUND,
						0);
			} else {
				FT_Stroker_Set(stroker, outline_strength,
						FT_STROKER_LINECAP_BUTT,
						FT_STROKER_LINEJOIN_MITER_FIXED,
						2 << 16);
			}

			FT_Glyph_Stroke(&glyph, stroker, 0);
		}

		FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, nullptr, 1);
		FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;
		bits = bitmap_glyph->bitmap;
		offset = core::vector2di(bitmap_glyph->left * scale, bitmap_glyph->top * scale);
	} else {
		if (face->num_fixed_sizes == 0)
			FT_Render_Glyph(glyph_slot, FT_RENDER_MODE_NORMAL);

		bits = glyph_slot->bitmap;
		offset = core::vector2di(glyph_slot->bitmap_left * scale, glyph_slot->bitmap_top * scale);
	}

	// Setup the glyph information here:
	if (FT_HAS_COLOR(face) && face->num_fixed_sizes > 0) {
		int bitmap_top = parent->getColorEmojiOffset();
		offset = core::vector2di(glyph_slot->bitmap_left * scale, bitmap_top);
	} else {
		offset = core::vector2di(glyph_slot->bitmap_left * scale, glyph_slot->bitmap_top * scale);
	}

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

	video::SColor color;
	if (outline > 0 && !FT_HAS_COLOR(face))
		color = video::SColor(0,0,0,0);
	else
		color = video::SColor(0,255,255,255);

	// We grab the glyph bitmap here so the data won't be removed when the next glyph is loaded.
	surface = createGlyphImage(face, bits, driver, color);

	if (glyph)
		FT_Done_Glyph(glyph);

	if (outline > 0 && !FT_HAS_COLOR(face)) {
		FT_Glyph glyph;
		if (FT_Get_Glyph(glyph_slot, &glyph) != FT_Err_Ok)
			return;

		FT_OutlineGlyph glyph_outline = (FT_OutlineGlyph)glyph;

		if (bold) {
			float embolden_amount = (float)font_size * 2.0f;
			FT_Outline_Embolden(&(glyph_outline->outline), embolden_amount);
		}

		if (italic) {
			FT_Matrix italic_matrix;
			float slant = 0.2;
			italic_matrix.xx = 0x10000;
			italic_matrix.xy = (FT_Fixed)(slant * 0x10000);
			italic_matrix.yx = 0;
			italic_matrix.yy = 0x10000;

			FT_Outline_Transform(&(glyph_outline->outline), &italic_matrix);
		}

		FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, nullptr, 1);
		FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;

		FT_Bitmap bits = bitmap_glyph->bitmap;
		video::IImage* image = createGlyphImage(face, bits, driver, video::SColor(0,255,255,255));

		core::dimension2du surface_size = surface->getDimension();
		core::dimension2du image_size = image->getDimension();
		s32 pos_x = std::round(((float)(surface_size.Width - image_size.Width)) / 2);
		s32 pos_y = std::round(((float)(surface_size.Height - image_size.Height)) / 2);
		core::position2d<s32> pos = core::position2d<s32>(pos_x, pos_y);
		core::rect<s32> source_rect = core::rect<s32>(core::position2d<s32>(0, 0), image->getDimension());
		video::SColor color = video::SColor(255,255,255,255);
		image->copyToWithAlpha(surface, pos, source_rect, color, nullptr, true);

		image->drop();
		FT_Done_Glyph(glyph);
	}

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
		const u16 outline, const u8 outline_type, const s8 character_spacing,
		const u32 shadow, const u32 shadow_alpha)
{
	if (!c_libraryLoaded)
	{
		if (FT_Init_FreeType(&c_library))
			return 0;
		c_libraryLoaded = true;
	}

	FT_Stroker_New(c_library, &stroker);

	CGUITTFont* font = new CGUITTFont(env);

	font->shadow_alpha = shadow_alpha;
	font->bold = bold;
	font->italic = italic;
	font->outline = outline;
	font->outline_type = outline_type;
	font->character_spacing = character_spacing;

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
		const bool transparency, const bool bold, const bool italic,
		const u16 outline, const u8 outline_type, const s8 character_spacing)
{
	if (!c_libraryLoaded)
	{
		if (FT_Init_FreeType(&c_library))
			return 0;
		c_libraryLoaded = true;
	}

	FT_Stroker_New(c_library, &stroker);

	CGUITTFont* font = new CGUITTFont(device->getGUIEnvironment());

	font->bold = bold;
	font->italic = italic;
	font->outline = outline;
	font->outline_type = outline_type;
	font->character_spacing = character_spacing;
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
		const bool bold, const bool italic, const u16 outline,
		const u8 outline_type, const s8 character_spacing)
{
	return CGUITTFont::createTTFont(env, filename, size, antialias,
			transparency, bold, italic, outline, outline_type,
			character_spacing);
}

CGUITTFont* CGUITTFont::create(IrrlichtDevice *device, const io::path& filename,
		const u32 size, const bool antialias, const bool transparency,
		const bool bold, const bool italic, const u16 outline,
		const u8 outline_type, const s8 character_spacing)
{
	return CGUITTFont::createTTFont(device, filename, size, antialias,
			transparency, bold, italic, outline, outline_type,
			character_spacing);
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
}

CGUITTFont::~CGUITTFont()
{
	// Delete the glyphs and glyph pages.
	reset_images();

	for (auto& pair : Glyphs) {
		if (pair.second)
			delete pair.second;
	}

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
				FT_Stroker_Done(stroker);

				FT_Done_FreeType(c_library);
				c_libraryLoaded = false;
			}
		}
	}

	// Drop our driver now.
	if (Driver)
		Driver->drop();
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
	shadow_offsets.push_back(shadow);

	if (tt_faces.size() == 1) {
		// Store font metrics.
		FT_Set_Pixel_Sizes(face->face, size, 0);
		font_metrics = face->face->size->metrics;
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
		shadow_offsets.pop_back();

		core::map<io::path, SGUITTFace*>::Node *node = c_faces.find(filename);
		if (node) {
			SGUITTFace *face = node->getValue();
			if (face->drop())
				c_faces.remove(filename);
		}

		return false;
	}

	FT_Face face = nullptr;

	for (size_t i = 0; i < filenames.size(); i++) {
		if (filenames[i] == filename) {
			face = tt_faces[i];
			break;
		}
	}

	if (face)
		calculateColorEmojiParams(face);

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

void CGUITTFont::calculateColorEmojiParams(FT_Face face)
{
	if (!FT_HAS_COLOR(face) || face->num_fixed_sizes == 0)
		return;

	u32 height = std::round((float)(getMaxFontHeight()) * 0.9f);
	float scale = 1.0f;
	u32 bitmap_top = height;

	if (FT_HAS_COLOR(face) && face->num_fixed_sizes > 0) {
		u32 best_index = getBestFixedSizeIndex(face, height);
		FT_Select_Size(face, best_index);
		scale = (float)height / face->available_sizes[best_index].height;

		FT_UInt glyph_index = FT_Get_Char_Index(face, 0x1F600); // smile
		if (glyph_index == 0)
			glyph_index = FT_Get_Char_Index(face, 'A');

		FT_Int32 flags = load_flags | FT_LOAD_COLOR;

		if (FT_Load_Glyph(face, glyph_index, flags) == FT_Err_Ok) {
			if (face->glyph->bitmap.rows > 0) {
				scale = (float)height / face->glyph->bitmap.rows;
			}
		}
	}

	color_emoji_scale = scale;

	FT_UInt glyph_index = FT_Get_Char_Index(tt_faces[0], 'A');
	FT_Int32 flags = load_flags | FT_LOAD_DEFAULT;

	if (FT_Load_Glyph(tt_faces[0], glyph_index, flags) == FT_Err_Ok) {
		if (tt_faces[0]->glyph->bitmap_top > 0)
			bitmap_top = tt_faces[0]->glyph->bitmap_top +
					(height - tt_faces[0]->glyph->bitmap.rows) / 2;
	}

	color_emoji_offset = bitmap_top;
}

std::vector<ShapedRun> CGUITTFont::shapeText(const core::ustring& text) const
{
	std::vector<ShapedRun> runs;

	if (text.size() == 0)
		return runs;

	std::vector<uint32_t> utf32_text;
	core::ustring::const_iterator iter(text);

	while (!iter.atEnd()) {
		utf32_text.push_back(*iter);
		++iter;
	}

	std::vector<TextRun> font_runs = splitIntoFontRuns(utf32_text);

	for (const auto& run : font_runs) {
		ShapedRun shaped = shapeRun(run, utf32_text, run.start);
		runs.push_back(shaped);
	}

	return runs;
}

std::vector<TextRun> CGUITTFont::splitIntoFontRuns(
		const std::vector<uint32_t>& text) const
{
	std::vector<TextRun> runs;

	u32 run_start = 0;
	size_t current_face = SIZE_MAX;

	for (u32 i = 0; i < text.size(); i++) {
		s32 face_for_char = getFaceIndexByChar(text[i]);

		if (face_for_char == -1)
			face_for_char = 0;

		if ((size_t)face_for_char != current_face) {
			if (current_face != SIZE_MAX) {
				runs.push_back({current_face, run_start, i - run_start});
			}

			run_start = i;
			current_face = face_for_char;
		}
	}

	if (current_face != SIZE_MAX) {
		runs.push_back({current_face, run_start,
					   (u32)text.size() - run_start});
	}

	return runs;
}

ShapedRun CGUITTFont::shapeRun(const TextRun& run,
		const std::vector<uint32_t>& text, u32 cluster_offset) const
{
	ShapedRun result;
	result.face_index = run.face_index;
	result.start_char = run.start;
	result.end_char = run.start + run.length;

	FT_Face face = tt_faces[run.face_index];

	FT_Set_Pixel_Sizes(face, 0, size);

	float emoji_scale = 1.0f;

	if (FT_HAS_COLOR(face) && face->num_fixed_sizes > 0) {
		u32 best_index = getBestFixedSizeIndex(face, size);
		FT_Select_Size(face, best_index);
		emoji_scale = const_cast<CGUITTFont*>(this)->getColorEmojiScale();
	}

	hb_font_t* hb_font = hb_ft_font_create(face, nullptr);
	hb_buffer_t* buf = hb_buffer_create();

	hb_buffer_add_utf32(buf, text.data() + run.start, run.length, 0, run.length);

	hb_buffer_guess_segment_properties(buf);
	hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
	//~ hb_buffer_set_script(buf, HB_SCRIPT_COMMON);
	//~ hb_buffer_set_language(buf, hb_language_get_default());

	float bold_offset = 0;
	if (bold) {
		float embolden_amount = (float)size * 2.0f;
		bold_offset = (embolden_amount * 2.0f) / 64.0f;
	}

	s32 spacing_offset = character_spacing + bold_offset;

	hb_shape(hb_font, buf, nullptr, 0);

	unsigned int glyph_count;
	hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
	hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

	for (unsigned int i = 0; i < glyph_count; i++) {
		s32 x_advance = glyph_pos[i].x_advance >> 6;
		s32 y_advance = glyph_pos[i].y_advance >> 6;

		if (FT_HAS_COLOR(face) && face->num_fixed_sizes > 0) {
			x_advance *= emoji_scale;
			y_advance *= emoji_scale;
		}

		ShapedGlyph glyph;
		glyph.glyph_index = glyph_info[i].codepoint;
		glyph.cluster = glyph_info[i].cluster + cluster_offset;
		glyph.x_offset = glyph_pos[i].x_offset >> 6;
		glyph.y_offset = glyph_pos[i].y_offset >> 6;
		glyph.x_advance = x_advance + spacing_offset;
		glyph.y_advance = y_advance;
		glyph.face_index = run.face_index;

		result.glyphs.push_back(glyph);
	}

	hb_buffer_destroy(buf);
	hb_font_destroy(hb_font);

	return result;
}

void CGUITTFont::loadGlyphsForShapedText(const std::vector<ShapedRun>& runs)
{
	for (const auto& run : runs) {
		FT_Face face = tt_faces[run.face_index];

		for (const auto& shaped_glyph : run.glyphs) {
			u32 glyph_idx = shaped_glyph.glyph_index;

			if (glyph_idx < 1)
				continue;

			u64 key = makeGlyphKey(shaped_glyph.face_index, shaped_glyph.glyph_index);
			SGUITTGlyph* glyph = Glyphs[key];

			if (!glyph) {
				glyph = new SGUITTGlyph();
				glyph->isLoaded = false;
				glyph->isColor = false;
				glyph->glyph_page = 0;
				glyph->source_rect = core::recti();
				glyph->offset = core::vector2di();
				glyph->shadow_offset = 0;
				glyph->surface = 0;
				glyph->parent = this;
				Glyphs[key] = glyph;
			}

			if (!glyph->isLoaded) {
				FT_Int32 flags = load_flags;
				if (FT_HAS_COLOR(face))
					flags |= FT_LOAD_COLOR;

				glyph->preload(glyph_idx, face, Driver, size, flags,
						bold, italic, outline, outline_type, character_spacing);
				glyph->shadow_offset = shadow_offsets[run.face_index];
				Glyph_Pages[glyph->glyph_page]->pushGlyphToBePaged(glyph);
			}
		}
	}
}

u64 CGUITTFont::makeGlyphKey(u32 face_index, u32 glyph_index)
{
	return ((u64)face_index << 32) | glyph_index;
}

s32 CGUITTFont::getPrevClusterPos(const core::stringw& text, s32 pos)
{
	if (pos <= 0)
		return 0;
	
	std::vector<ShapedRun> shaped_runs = shapeText(text);
	
	s32 prev_cluster = 0;
	
	for (const auto& run : shaped_runs) {
		for (const auto& glyph : run.glyphs) {
			if ((s32)glyph.cluster >= pos) {
				return prev_cluster;
			}
			prev_cluster = glyph.cluster;
		}
	}
	
	return prev_cluster;
}

s32 CGUITTFont::getNextClusterPos(const core::stringw& text, s32 pos)
{
	std::vector<ShapedRun> shaped_runs = shapeText(text);
	bool found_current = false;
	
	for (const auto& run : shaped_runs) {
		for (const auto& glyph : run.glyphs) {
			if (found_current)
				return glyph.cluster;
			
			if ((s32)glyph.cluster >= pos)
				found_current = true;
		}
	}
	
	return text.size();
}

void CGUITTFont::reset_images()
{
	// Delete the glyphs.
	for (auto& pair : Glyphs) {
		if (pair.second) {
			pair.second->unload();
		}
	}

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

	// Shape the text with HarfBuzz
	std::vector<ShapedRun> shaped_runs = shapeText(utext.c_str());
	loadGlyphsForShapedText(shaped_runs);

	// Set up our render map.
	core::map<u32, CGUITTGlyphPage*> Render_Map;

	u32 current_char_index = 0;

	for (const auto& run : shaped_runs)
	{
		for (const auto& shaped_glyph : run.glyphs)
		{
			uchar32_t currentChar = 0;
			if (shaped_glyph.cluster < utext.size()) {
				currentChar = utext[shaped_glyph.cluster];
			}

			bool visible = (Invisible.findFirst(currentChar) == -1);
			bool lineBreak = false;

			if (currentChar == L'\r') // Mac or Windows breaks
			{
				lineBreak = true;
				if (shaped_glyph.cluster + 1 < utext.size() &&
					utext[shaped_glyph.cluster + 1] == (uchar32_t)'\n') {
					current_char_index++;
				}
			}
			else if (currentChar == (uchar32_t)'\n') // Unix breaks
			{
				lineBreak = true;
			}

			if (lineBreak)
			{
				offset.Y += font_metrics.height / 64;
				offset.X = position.UpperLeftCorner.X;

				if (hcenter)
					offset.X += (position.getWidth() - textDimension.Width) >> 1;

				current_char_index++;
				continue;
			}

			if (shaped_glyph.glyph_index > 0 && visible)
			{
				u64 key = makeGlyphKey(shaped_glyph.face_index, shaped_glyph.glyph_index);
				SGUITTGlyph* glyph = Glyphs[key];

				// Calculate the glyph offset.
				s32 offx = Glyphs[key]->offset.X + shaped_glyph.x_offset;
				s32 offy = (font_metrics.ascender / 64) - Glyphs[key]->offset.Y + shaped_glyph.y_offset;

				// Determine rendering information.
				CGUITTGlyphPage* const page = Glyph_Pages[glyph->glyph_page];
				page->render_positions.push_back(core::position2di(offset.X + offx, offset.Y + offy));
				page->render_source_rects.push_back(glyph->source_rect);

				if (glyph->shadow_offset) {
					page->shadow_positions.push_back(core::position2di(
							offset.X + offx + glyph->shadow_offset,
							offset.Y + offy + glyph->shadow_offset));
					page->shadow_source_rects.push_back(glyph->source_rect);
				}

				u32 color_index = shaped_glyph.cluster;
				if (!glyph->isColor && color_index < colors.size())
					page->render_colors.push_back(colors[color_index]);
				else
					page->render_colors.push_back(video::SColor(255,255,255,255));

				Render_Map.set(glyph->glyph_page, page);
			}

			offset.X += shaped_glyph.x_advance;
			offset.Y += shaped_glyph.y_advance;
		}
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
	return getDimension(core::ustring(ch));
}

core::dimension2d<u32> CGUITTFont::getDimension(const wchar_t* text) const
{
	return getDimension(core::ustring(text));
}

u32 CGUITTFont::getMaxFontHeight() const
{
	// Probably better just
	//     return font_metrics.height / 64;
	// but for now keep it for compatibility

	const uchar32_t test_chars[] = {'g', 'j', '_'};
	u32 max_height = font_metrics.height / 64;

	for (uchar32_t c : test_chars) {
		std::vector<ShapedRun> shaped_runs = shapeText(c);
		const_cast<CGUITTFont*>(this)->loadGlyphsForShapedText(shaped_runs);

		if (!shaped_runs.empty() && !shaped_runs[0].glyphs.empty()) {
			const ShapedGlyph& shaped_glyph = shaped_runs[0].glyphs[0];
			u64 key = const_cast<CGUITTFont*>(this)->makeGlyphKey(
					shaped_glyph.face_index, shaped_glyph.glyph_index);
			SGUITTGlyph* glyph = const_cast<CGUITTFont*>(this)->Glyphs[key];

			if (glyph && glyph->isLoaded) {
				s32 height = (font_metrics.ascender / 64) - glyph->offset.Y +
						glyph->source_rect.getHeight();
				max_height = core::max_(max_height, (u32)height);
			}
		}
	}

	return max_height + 1;
}

core::dimension2d<u32> CGUITTFont::getDimension(const core::ustring& text) const
{
	std::vector<ShapedRun> shaped_runs = shapeText(text);

	const_cast<CGUITTFont*>(this)->loadGlyphsForShapedText(shaped_runs);

	s32 max_font_height = getMaxFontHeight();

	core::dimension2d<u32> text_dimension(0, max_font_height);
	core::dimension2d<u32> line(0, max_font_height);
	core::ustring::const_iterator iter(text);
	u32 char_index = 0;

	for (const auto& run : shaped_runs) {
		for (const auto& glyph : run.glyphs) {
			bool lineBreak = false;
			if (glyph.cluster < text.size()) {
				while (!iter.atEnd() && char_index < glyph.cluster) {
					++iter;
					++char_index;
				}

				if (!iter.atEnd()) {
					uchar32_t c = *iter;
					if (c == L'\n' || c == L'\r') {
						lineBreak = true;
					}
				}
			}

			// Check for linebreak.
			if (lineBreak)
			{
				text_dimension.Height += line.Height;
				if (text_dimension.Width < line.Width)
					text_dimension.Width = line.Width;
				line.Width = 0;
				line.Height = max_font_height;
				continue;
			}
			line.Width += glyph.x_advance;
		}
	}
	if (text_dimension.Width < line.Width)
		text_dimension.Width = line.Width;

	// Each character has spacing added to it so that the next character will
	// be properly spaced, so we need to subtract character_spacing from the
	// calculated width to get the actual width.
	if (text_dimension.Width > 0)
		text_dimension.Width -= character_spacing;

	return text_dimension;
}

core::dimension2d<u32> CGUITTFont::getTotalDimension(const wchar_t* text) const
{
	return getTotalDimension(core::ustring(text));
}

core::dimension2d<u32> CGUITTFont::getTotalDimension(const core::ustring& text) const
{
	core::dimension2d<u32> text_dimension = getDimension(text);
	u32 max_font_height = getMaxFontHeight();

	if (italic) {
		float slant = 0.2f;
		u32 italic_extra_width = static_cast<u32>(max_font_height * slant);
		text_dimension.Width += italic_extra_width;
	}

	if (bold) {
		u32 bold_extra_width = size * 0.1f;
		text_dimension.Width += bold_extra_width;
	}

	return text_dimension;
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
	if (text.size() == 0)
		return -1;

	std::vector<ShapedRun> shaped_runs = shapeText(text);
	s32 x = 0;

	for (const auto& run : shaped_runs) {
		for (const auto& glyph : run.glyphs) {
			x += glyph.x_advance;

			if (x >= pixel_x)
				return glyph.cluster;
		}
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
	return 0;
}

s32 CGUITTFont::getKerningWidth(const uchar32_t thisLetter, const uchar32_t previousLetter) const
{
	return 0;
}

s32 CGUITTFont::getKerningHeight() const
{
	return 0;
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
	core::ustring temp_text;
	temp_text.append(ch);

	std::vector<ShapedRun> shaped = shapeText(temp_text);

	if (shaped.empty() || shaped[0].glyphs.empty()) {
		return nullptr;
	}

	loadGlyphsForShapedText(shaped);

	const ShapedGlyph& shaped_glyph = shaped[0].glyphs[0];
	u64 key = makeGlyphKey(shaped_glyph.face_index, shaped_glyph.glyph_index);

	SGUITTGlyph* glyph = Glyphs[key];

	if (!glyph || !glyph->isLoaded) {
		return nullptr; // Glyph not loaded
	}

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

	core::ustring utext(text);
	std::vector<ShapedRun> shaped_runs = shapeText(utext);
	loadGlyphsForShapedText(shaped_runs);

	dimension2d<s32> text_size(getDimension(text));
	vector3df start_point(0, 0, 0), offset;

	/** NOTICE:
		Because we are considering adding texts into 3D world, all Y axis vectors are inverted.
	**/

	// There's currently no "vertical center" concept when you apply text scene node to the 3D world.
	if (center)
	{
		offset.X = start_point.X = -text_size.Width / 2.f;
		offset.Y = start_point.Y = +text_size.Height / 2.f;
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

	array<u64> glyph_keys;

	// Process shaped runs
	for (const auto& run : shaped_runs)
	{
		for (const auto& shaped_glyph : run.glyphs)
		{
			// Check for line breaks
			uchar32_t currentChar = 0;
			if (shaped_glyph.cluster < utext.size()) {
				currentChar = utext[shaped_glyph.cluster];
			}

			bool lineBreak = false;
			if (currentChar == L'\r')
			{
				lineBreak = true;
			}
			else if (currentChar == (uchar32_t)'\n')
			{
				lineBreak = true;
			}

			if (lineBreak)
			{
				offset.Y -= font_metrics.ascender / 64;
				offset.X = start_point.X;
				if (center)
					offset.X += (text_size.Width - getDimensionUntilEndOfLine(text + shaped_glyph.cluster + 1).Width) >> 1;
				continue;
			}

			if (shaped_glyph.glyph_index > 0)
			{
				u64 key = makeGlyphKey(shaped_glyph.face_index, shaped_glyph.glyph_index);
				SGUITTGlyph* glyph = Glyphs[key];

				if (!glyph || !glyph->isLoaded) {
					offset.X += shaped_glyph.x_advance;
					continue;
				}

				glyph_keys.push_back(key);

				// Store glyph size and offset informations.
				u32 texw = glyph->source_rect.getWidth();
				u32 texh = glyph->source_rect.getHeight();
				s32 offx = glyph->offset.X + shaped_glyph.x_offset;
				s32 offy = (font_metrics.ascender / 64) - glyph->offset.Y + shaped_glyph.y_offset;

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

			offset.X += shaped_glyph.x_advance;
			offset.Y += shaped_glyph.y_advance;
		}
	}

	update_glyph_pages();
	//only after we update the textures can we use the glyph page textures.

	for (u32 i = 0; i < glyph_keys.size(); ++i)
	{
		u64 key = glyph_keys[i];
		SGUITTGlyph* glyph = Glyphs[key];

		if (!glyph)
			continue;

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
