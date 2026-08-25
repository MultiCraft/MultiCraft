/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3.0 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <IrrlichtDevice.h>
#include <irrlicht.h>
#include "filesys.h"
#include "fontengine.h"
#include "client.h"
#include "clouds.h"
#include "daynightratio.h"
#include "util/numeric.h"
#include "guiscalingfilter.h"
#include "localplayer.h"
#include "client/hud.h"
#include "client/sky.h"
#include "gui/guiEngine.h"
#include "camera.h"
#include "minimap.h"
#include "clientmap.h"
#include "renderingengine.h"
#include "render/core.h"
#include "render/factory.h"
#include "inputhandler.h"
#include "gettext.h"
#include "../gui/guiSkin.h"
#include "gui/guiButton.h"
#include "gui/mainmenumanager.h"

#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__ANDROID__) && \
		!defined(SERVER) && !defined(__HAIKU__)
#define XORG_USED
#endif
#ifdef XORG_USED
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#endif

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
#include <SDL3/SDL.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <winuser.h>
#endif

#if defined(__ANDROID__) || defined(__APPLE__)
#include "defaultsettings.h"
#endif

RenderingEngine *RenderingEngine::s_singleton = nullptr;


class LoadScreenRoot : public gui::IGUIElement
{
public:
	LoadScreenRoot(gui::IGUIEnvironment *env, gui::IGUIElement *parent,
			const core::rect<s32> &rect) :
		gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, -1, rect)
	{
	}

	bool OnEvent(const SEvent &event) override
	{
		if (event.EventType == EET_GUI_EVENT &&
				event.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED) {
			RenderingEngine::cancel_load_screen();
			return true;
		}
		return gui::IGUIElement::OnEvent(event);
	}
};


static gui::GUISkin *createSkin(gui::IGUIEnvironment *environment,
		gui::EGUI_SKIN_TYPE type, video::IVideoDriver *driver)
{
	gui::GUISkin *skin = new gui::GUISkin(type, driver);

	gui::IGUIFont *builtinfont = environment->getBuiltInFont();
	gui::IGUIFontBitmap *bitfont = nullptr;
	if (builtinfont && builtinfont->getType() == gui::EGFT_BITMAP)
		bitfont = (gui::IGUIFontBitmap*)builtinfont;

	gui::IGUISpriteBank *bank = 0;
	skin->setFont(builtinfont);

	if (bitfont)
		bank = bitfont->getSpriteBank();

	skin->setSpriteBank(bank);

	return skin;
}


RenderingEngine::RenderingEngine(IEventReceiver *receiver)
{
	sanity_check(!s_singleton);

	const u16 screen_min_w = 640;
	const u16 screen_min_h = 480;

	// Resolution selection
	bool fullscreen = g_settings->getBool("fullscreen");
#if defined(__MACH__) && defined(__APPLE__) && !defined(__IOS__)
	u16 screen_w = g_settings->getU16("screen_w");
	u16 screen_h = g_settings->getU16("screen_h");
#elif defined(__ANDROID__) || defined(__IOS__)
	fullscreen = true;
	u16 screen_w = 0;
	u16 screen_h = 0;
#else
	u16 screen_w = std::max(g_settings->getU16("screen_w"), screen_min_w);
	u16 screen_h = std::max(g_settings->getU16("screen_h"), screen_min_h);
#endif

	// bpp, fsaa, vsync
	bool vsync = g_settings->getBool("vsync");
	u16 bits = g_settings->getU16("fullscreen_bpp");
	u16 fsaa = g_settings->getU16("fsaa");

	// stereo buffer required for pageflip stereo
	bool stereo_buffer = g_settings->get("3d_mode") == "pageflip";

	// Determine driver
#if defined(_IRR_COMPILE_WITH_ANGLE_)
	video::E_DRIVER_TYPE driverType = video::EDT_METAL;
#elif defined(__ANDROID__) || defined(__IOS__)
	video::E_DRIVER_TYPE driverType = video::EDT_OGLES2;
#else
	video::E_DRIVER_TYPE driverType = video::EDT_OPENGL;
#endif
	const std::string &driverstring = g_settings->get("video_driver");
	std::vector<video::E_DRIVER_TYPE> drivers =
			RenderingEngine::getSupportedVideoDrivers();
	u32 i;
	for (i = 0; i != drivers.size(); i++) {
		if (!strcasecmp(driverstring.c_str(),
				RenderingEngine::getVideoDriverName(drivers[i]))) {
			driverType = drivers[i];
			break;
		}
	}
	if (i == drivers.size()) {
		errorstream << "Invalid video_driver specified; "
			       "defaulting to opengl"
			    << std::endl;
	}
#if defined(__ANDROID__) || defined(__IOS__)
	// Shaders are required on OpenGL ES2, and on the ANGLE-backed context too
	g_settings->setBool("enable_shaders", driverType == video::EDT_OGLES2 ||
			driverType == video::EDT_METAL);
#endif

	SIrrlichtCreationParameters params = SIrrlichtCreationParameters();
	params.DriverType = driverType;
	params.WindowSize = core::dimension2d<u32>(screen_w, screen_h);
	params.Bits = bits;
	params.AntiAlias = fsaa;
	params.Fullscreen = fullscreen;
	params.Stencilbuffer = false;
	params.Stereobuffer = stereo_buffer;
	params.Vsync = vsync;
	params.EventReceiver = receiver;
	params.HighPrecisionFPU = g_settings->getBool("high_precision_fpu");
	params.ZBufferBits = 24;
#if ENABLE_GLES
	// there is no standardized path for these on desktop
	std::string rel_path = std::string("client") + DIR_DELIM
			+ "shaders" + DIR_DELIM + "Irrlicht";
	params.OGLES2ShaderPath = (porting::path_share + DIR_DELIM + rel_path + DIR_DELIM).c_str();
#endif

	m_device = createDeviceEx(params);
#if defined(__ANDROID__) || defined(__IOS__)
	FATAL_ERROR_IF(!m_device, ("Device create failed. Driver Type: \"" +
			std::string(RenderingEngine::getVideoDriverName(driverType)) +
			"\". SDL: " + SDL_GetError()).c_str());
#endif
	driver = m_device->getVideoDriver();

	// Textures live on the GPU, a copy in main memory only doubles their cost
	driver->setTextureCreationFlag(video::ETCF_ALLOW_MEMORY_COPY, false);

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
	const video::SExposedVideoData exposedData = driver->getExposedVideoData();
	SDL_Window *window = exposedData.OpenGLSDL.Window;
	SDL_SetWindowMinimumSize(window, screen_min_w, screen_min_h);
#endif

	s_singleton = this;

	auto skin = createSkin(m_device->getGUIEnvironment(),
			gui::EGST_WINDOWS_METALLIC, driver);
	m_device->getGUIEnvironment()->setSkin(skin);
	skin->drop();

#if defined(__ANDROID__) || defined(__APPLE__)
	// Apply settings according to screen size
	// We can get real screen size only after device initialization finished
	set_default_settings();
#endif

	m_last_time = porting::getTimeMs();
}

RenderingEngine::~RenderingEngine()
{
	core.reset();
	m_device->closeDevice();
	m_device->drop();
	s_singleton = nullptr;
}

v2u32 RenderingEngine::getWindowSize() const
{
	if (core)
		return core->getVirtualSize();
	return m_device->getVideoDriver()->getScreenSize();
}

void RenderingEngine::setResizable(bool resize)
{
	m_device->setResizable(resize);
}

bool RenderingEngine::print_video_modes()
{
	IrrlichtDevice *nulldevice;

	bool vsync = g_settings->getBool("vsync");
	u16 fsaa = g_settings->getU16("fsaa");
	MyEventReceiver *receiver = new MyEventReceiver();

	SIrrlichtCreationParameters params = SIrrlichtCreationParameters();
	params.DriverType = video::EDT_NULL;
	params.WindowSize = core::dimension2d<u32>(640, 480);
	params.Bits = 24;
	params.AntiAlias = fsaa;
	params.Fullscreen = false;
	params.Stencilbuffer = false;
	params.Vsync = vsync;
	params.EventReceiver = receiver;
	params.HighPrecisionFPU = g_settings->getBool("high_precision_fpu");

	nulldevice = createDeviceEx(params);

	if (!nulldevice) {
		delete receiver;
		return false;
	}

	std::cout << _("Available video modes (WxHxD):") << std::endl;

	video::IVideoModeList *videomode_list = nulldevice->getVideoModeList();

	if (videomode_list != NULL) {
		s32 videomode_count = videomode_list->getVideoModeCount();
		core::dimension2d<u32> videomode_res;
		s32 videomode_depth;
		for (s32 i = 0; i < videomode_count; ++i) {
			videomode_res = videomode_list->getVideoModeResolution(i);
			videomode_depth = videomode_list->getVideoModeDepth(i);
			std::cout << videomode_res.Width << "x" << videomode_res.Height
				  << "x" << videomode_depth << std::endl;
		}

		std::cout << _("Active video mode (WxHxD):") << std::endl;
		videomode_res = videomode_list->getDesktopResolution();
		videomode_depth = videomode_list->getDesktopDepth();
		std::cout << videomode_res.Width << "x" << videomode_res.Height << "x"
			  << videomode_depth << std::endl;
	}

	nulldevice->drop();
	delete receiver;

	return videomode_list != NULL;
}

bool RenderingEngine::setupTopLevelWindow(const std::string &name)
{
	// FIXME: It would make more sense for there to be a switch of some
	// sort here that would call the correct toplevel setup methods for
	// the environment Minetest is running in.

	/* Setting Xorg properties for the top level window */
	setupTopLevelXorgWindow(name);

	/* Setting general properties for the top level window */
	verbosestream << "Client: Configuring general top level"
		<< " window properties"
		<< std::endl;
	bool result = setWindowIcon();

	return result;
}

void RenderingEngine::setupTopLevelXorgWindow(const std::string &name)
{
#ifdef XORG_USED
	const video::SExposedVideoData exposedData = driver->getExposedVideoData();

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
	SDL_Window *window = exposedData.OpenGLSDL.Window;

	if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") != 0)
		return;

	Display *x11_dpl = (Display *)SDL_GetPointerProperty(
			SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
	Window x11_win = (Window)SDL_GetNumberProperty(
			SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
#else
	Display *x11_dpl = reinterpret_cast<Display *>(exposedData.OpenGLLinux.X11Display);
	Window x11_win = reinterpret_cast<Window>(exposedData.OpenGLLinux.X11Window);
#endif

	if (x11_dpl == NULL) {
		warningstream << "Client: Could not find X11 Display in ExposedVideoData"
			<< std::endl;
		return;
	}

	verbosestream << "Client: Configuring X11-specific top level"
		<< " window properties"
		<< std::endl;

	// Set application name and class hints. For now name and class are the same.
	XClassHint *classhint = XAllocClassHint();
	classhint->res_name = const_cast<char *>(name.c_str());
	classhint->res_class = const_cast<char *>(name.c_str());

	XSetClassHint(x11_dpl, x11_win, classhint);
	XFree(classhint);

	// FIXME: In the future WMNormalHints should be set ... e.g see the
	// gtk/gdk code (gdk/x11/gdksurface-x11.c) for the setup_top_level
	// method. But for now (as it would require some significant changes)
	// leave the code as is.

	// The following is borrowed from the above gdk source for setting top
	// level windows. The source indicates and the Xlib docs suggest that
	// this will set the WM_CLIENT_MACHINE and WM_LOCAL_NAME. This will not
	// set the WM_CLIENT_MACHINE to a Fully Qualified Domain Name (FQDN) which is
	// required by the Extended Window Manager Hints (EWMH) spec when setting
	// the _NET_WM_PID (see further down) but running Minetest in an env
	// where the window manager is on another machine from Minetest (therefore
	// making the PID useless) is not expected to be a problem. Further
	// more, using gtk/gdk as the model it would seem that not using a FQDN is
	// not an issue for modern Xorg window managers.

	verbosestream << "Client: Setting Xorg window manager Properties"
		<< std::endl;

	XSetWMProperties (x11_dpl, x11_win, NULL, NULL, NULL, 0, NULL, NULL, NULL);

	// Set the _NET_WM_PID window property according to the EWMH spec. _NET_WM_PID
	// (in conjunction with WM_CLIENT_MACHINE) can be used by window managers to
	// force a shutdown of an application if it doesn't respond to the destroy
	// window message.

	verbosestream << "Client: Setting Xorg _NET_WM_PID extened window manager property"
		<< std::endl;

	Atom NET_WM_PID = XInternAtom(x11_dpl, "_NET_WM_PID", false);

	pid_t pid = getpid();

	XChangeProperty(x11_dpl, x11_win, NET_WM_PID,
			XA_CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&pid),1);

	// Set the WM_CLIENT_LEADER window property here. Minetest has only one
	// window and that window will always be the leader.

	verbosestream << "Client: Setting Xorg WM_CLIENT_LEADER property"
		<< std::endl;

	Atom WM_CLIENT_LEADER = XInternAtom(x11_dpl, "WM_CLIENT_LEADER", false);

	XChangeProperty (x11_dpl, x11_win, WM_CLIENT_LEADER,
		XA_WINDOW, 32, PropModeReplace,
		reinterpret_cast<unsigned char *>(&x11_win), 1);
#endif
}

#ifdef _WIN32
static bool getWindowHandle(irr::video::IVideoDriver *driver, HWND &hWnd)
{
	const video::SExposedVideoData exposedData = driver->getExposedVideoData();

	switch (driver->getDriverType()) {
	case video::EDT_OPENGL:
		hWnd = reinterpret_cast<HWND>(exposedData.OpenGLWin32.HWnd);
		break;
	default:
		return false;
	}

	return true;
}
#endif

bool RenderingEngine::setWindowIcon()
{
#if defined(XORG_USED)
#if RUN_IN_PLACE
	return setXorgWindowIconFromPath(
			porting::path_share + "/misc/" PROJECT_NAME "-xorg-icon-128.png");
#else
	// We have semi-support for reading in-place data if we are
	// compiled with RUN_IN_PLACE. Don't break with this and
	// also try the path_share location.
	return setXorgWindowIconFromPath(
			       ICON_DIR "/hicolor/128x128/apps/" PROJECT_NAME ".png") ||
	       setXorgWindowIconFromPath(porting::path_share + "/misc/" PROJECT_NAME
							       "-xorg-icon-128.png");
#endif
#elif defined(_WIN32)
	HWND hWnd; // Window handle
	if (!getWindowHandle(driver, hWnd))
		return false;

	// Load the ICON from resource file
	const HICON hicon = LoadIcon(GetModuleHandle(NULL),
			MAKEINTRESOURCE(130) // The ID of the ICON defined in
					     // winresource.rc
	);

	if (hicon) {
		SendMessage(hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hicon));
		SendMessage(hWnd, WM_SETICON, ICON_SMALL,
				reinterpret_cast<LPARAM>(hicon));
		return true;
	}
	return false;
#else
	return false;
#endif
}

bool RenderingEngine::setXorgWindowIconFromPath(const std::string &icon_file)
{
#ifdef XORG_USED

	video::IImageLoader *image_loader = NULL;
	u32 cnt = driver->getImageLoaderCount();
	for (u32 i = 0; i < cnt; i++) {
		if (driver->getImageLoader(i)->isALoadableFileExtension(
				    icon_file.c_str())) {
			image_loader = driver->getImageLoader(i);
			break;
		}
	}

	if (!image_loader) {
		warningstream << "Could not find image loader for file '" << icon_file
			      << "'" << std::endl;
		return false;
	}

	io::IReadFile *icon_f =
			m_device->getFileSystem()->createAndOpenFile(icon_file.c_str());

	if (!icon_f) {
		warningstream << "Could not load icon file '" << icon_file << "'"
			      << std::endl;
		return false;
	}

	video::IImage *img = image_loader->loadImage(icon_f);

	if (!img) {
		warningstream << "Could not load icon file '" << icon_file << "'"
			      << std::endl;
		icon_f->drop();
		return false;
	}

	u32 height = img->getDimension().Height;
	u32 width = img->getDimension().Width;

	size_t icon_buffer_len = 2 + height * width;
	long *icon_buffer = new long[icon_buffer_len];

	icon_buffer[0] = width;
	icon_buffer[1] = height;

	for (u32 x = 0; x < width; x++) {
		for (u32 y = 0; y < height; y++) {
			video::SColor col = img->getPixel(x, y);
			long pixel_val = 0;
			pixel_val |= (u8)col.getAlpha() << 24;
			pixel_val |= (u8)col.getRed() << 16;
			pixel_val |= (u8)col.getGreen() << 8;
			pixel_val |= (u8)col.getBlue();
			icon_buffer[2 + x + y * width] = pixel_val;
		}
	}

	img->drop();
	icon_f->drop();

	const video::SExposedVideoData exposedData = driver->getExposedVideoData();

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
	SDL_Window *window = exposedData.OpenGLSDL.Window;

	if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") != 0) {
		delete[] icon_buffer;
		return false;
	}

	Display *x11_dpl = (Display *)SDL_GetPointerProperty(
			SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
	Window x11_win = (Window)SDL_GetNumberProperty(
			SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
#else
	Display *x11_dpl = (Display *)exposedData.OpenGLLinux.X11Display;
	Window x11_win = (Window)exposedData.OpenGLLinux.X11Window;
#endif

	if (x11_dpl == NULL) {
		warningstream << "Could not find x11 display for setting its icon."
			      << std::endl;
		delete[] icon_buffer;
		return false;
	}

	Atom net_wm_icon = XInternAtom(x11_dpl, "_NET_WM_ICON", False);
	Atom cardinal = XInternAtom(x11_dpl, "CARDINAL", False);
	XChangeProperty(x11_dpl, x11_win, net_wm_icon, cardinal, 32, PropModeReplace,
			(const unsigned char *)icon_buffer, icon_buffer_len);

	delete[] icon_buffer;

#endif
	return true;
}

/*
	Draws a screen with a single text on it.
	Text will be removed when the screen is drawn the next time.
	Additionally, a progressbar can be drawn when percent is set between 0 and 100.
*/
void RenderingEngine::_draw_load_screen(const std::wstring &text,
		gui::IGUIEnvironment *guienv, ITextureSource *tsrc, float dtime,
		int percent)
{
#ifdef __IOS__
	if (m_device->isWindowMinimized() && m_load_screen_drawn)
		return;
#else
	if (!m_device->isWindowFocused() && m_load_screen_drawn)
		return;
#endif

	float fps_max = std::min(g_settings->getFloat("fps_max"), 30.0f);
	u64 cur_time = porting::getTimeMs();
	m_load_screen_dtime += dtime;

	if (cur_time - m_last_time < 1000.0f / fps_max && percent == m_percent)
		return;

	dtime = m_load_screen_dtime;
	m_load_screen_dtime = 0;
	m_last_time = cur_time;
	m_percent = percent;

	if (percent >= 100 || percent < m_percent_shown) {
		m_percent_shown = percent;
	} else {
		m_percent_shown += (percent - m_percent_shown) * MYMIN(1.0f, dtime * 6.0f);
		if (std::fabs(percent - m_percent_shown) < 0.5f)
			m_percent_shown = percent;
	}

	const v2u32 window = getWindowSize();

	video::ITexture *bar_bg = tsrc->getTexture("progress_bar_bg.png");
	video::ITexture *bar_fg = tsrc->getTexture("progress_bar_fg.png");
	video::ITexture *bar_top = tsrc->getTexture("progress_bar.png");

	const core::dimension2d<u32> art = bar_bg ? bar_bg->getOriginalSize() :
			core::dimension2d<u32>(512, 64);
#ifdef HAVE_TOUCHSCREENGUI
	const s32 bar_w = MYMIN(window.X * 0.5f, window.Y * 2.0f);
#else
	const s32 bar_w = 7.5 * (MYMIN(window.X, window.Y) * 0.9 / 15.0);
#endif
	const s32 bar_h = art.Width > 0 ? bar_w * art.Height / art.Width : bar_w / 8;
	const s32 bar_x = (s32)window.X / 2 - bar_w / 2;
	const s32 bar_y = (s32)window.Y / 2 - bar_h / 2;

	_draw_load_bg(guienv, tsrc, dtime);

	// draw progress bar
	if (percent >= 0 && percent <= 100 && bar_bg) {
		const core::rect<s32> whole(0, 0, art.Width, art.Height);
		draw2DImageFilterScaled(driver, bar_bg,
				core::rect<s32>(bar_x, bar_y, bar_x + bar_w, bar_y + bar_h),
				whole, nullptr, nullptr, true);

		const s32 done = MYMAX(bar_w * m_percent_shown / 100.0f, bar_h);
		const s32 left = done / 2;
		const s32 right = done - left;

		const video::SColor tint(255, 255 - (u32)m_percent_shown * 2,
				(u32)m_percent_shown * 2, 25);
		const video::SColor tints[] = {tint, tint, tint, tint};

		for (video::ITexture *layer : {bar_fg, bar_top}) {
			if (!layer)
				continue;
			const video::SColor *colors = layer == bar_fg ? tints : nullptr;

			if (left > 0)
				draw2DImageFilterScaled(driver, layer,
						core::rect<s32>(bar_x, bar_y, bar_x + left, bar_y + bar_h),
						core::rect<s32>(0, 0, left * art.Width / bar_w, art.Height),
						nullptr, colors, true);

			if (right > 0)
				draw2DImageFilterScaled(driver, layer,
						core::rect<s32>(bar_x + left, bar_y,
								bar_x + done, bar_y + bar_h),
						core::rect<s32>(art.Width - right * (s32)art.Width / bar_w, 0,
								art.Width, art.Height),
						nullptr, colors, true);
		}
	}

	const core::rect<s32> textrect(bar_x, bar_y, bar_x + bar_w, bar_y + bar_h);
	// the text grows with the bar on large screens, but never shrinks below default
	const u32 font_size = MYMAX(g_fontengine->getDefaultFontSize(),
			bar_h * FORMSPEC_FONT_SHARE / getDisplayDensity());

	if (!m_load_text) {
		m_load_text = guienv->addStaticText(text.c_str(), textrect, false, false);
		m_load_text->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_CENTER);
		// hovering it would steal the focus away from the cancel button
		m_load_text->setSubElement(true);
	} else {
		m_load_text->setText(text.c_str());
		m_load_text->setRelativePosition(textrect);
	}
	m_load_text->setOverrideFont(g_fontengine->getFont(font_size));

#ifdef HAVE_TOUCHSCREENGUI
	if (m_load_cancel) {
		const s32 touch_min = 9.0f * getDisplayDensity();
		const s32 btn_w = MYMAX(bar_w * 0.4f, touch_min * 3);
		const s32 btn_h = MYMAX(bar_h * 0.6f, touch_min);
		const core::rect<s32> btn(window.X / 2 - btn_w / 2, bar_y + bar_h + bar_h / 4,
				window.X / 2 + btn_w / 2, bar_y + bar_h + bar_h / 4 + btn_h);

		if (!m_load_root) {
			m_load_root = new LoadScreenRoot(guienv, guiroot,
					core::rect<s32>(0, 0, window.X, window.Y));
			const wchar_t *caption = wgettext("Cancel");
			m_load_button = GUIButton::addButton(guienv, btn, tsrc, m_load_root,
					-1, caption);
			delete[] caption;
			m_load_button->setStyles(StyleSpec::getButtonStyle());
			m_load_root->drop();
		} else {
			m_load_root->setRelativePosition(
					core::rect<s32>(0, 0, window.X, window.Y));
			m_load_button->setRelativePosition(btn);
		}
	}
#endif

	guienv->drawAll();
	driver->endScene();

	m_load_screen_drawn = true;
}

/*
	Draws the menu scene including (optional) cloud background.
*/
void RenderingEngine::_draw_menu_scene(gui::IGUIEnvironment *guienv,
		ITextureSource *tsrc, float dtime)
{
	_draw_load_bg(guienv, tsrc, dtime);
	guienv->drawAll();
	get_video_driver()->endScene();
}

/*
	Draws the cloud background used by draw_load_screen and draw_menu_scene
*/

void RenderingEngine::_draw_load_bg(gui::IGUIEnvironment *guienv,
									ITextureSource *tsrc, float dtime)
{
	driver->beginScene(true, true, m_sky_color);

	const v2u32 screensize = driver->getScreenSize();

	const bool cloud_menu_background = m_load_bg_clouds && g_settings->getBool("menu_clouds");
	if (cloud_menu_background) {
		scene::ICameraSceneNode *camera = g_menucloudsmgr->getActiveCamera();
		camera->setAspectRatio((float)(screensize.X) / screensize.Y);

		video::SColor fog_color;
		video::E_FOG_TYPE fog_type = video::EFT_FOG_LINEAR;
		f32 fog_start = 0;
		f32 fog_end = 0;
		f32 fog_density = 0;
		bool fog_pixelfog = false;
		bool fog_rangefog = false;
		driver->getFog(fog_color, fog_type, fog_start, fog_end, fog_density,
				fog_pixelfog, fog_rangefog);

		const video::SColor sky_color = g_menusky ? g_menusky->getSkyColor() : video::SColor(255, 5, 155, 245);
		driver->setFog(sky_color, fog_type, fog_start, fog_end, fog_density,
				fog_pixelfog, fog_rangefog);

		if (g_menusky) {
			u32 daynight_ratio = time_to_daynight_ratio(GUIEngine::g_timeofday * 24000.0f, true);
			float time_brightness = decode_light_f((float)daynight_ratio / 1000.0);
			g_menusky->update(GUIEngine::g_timeofday, time_brightness, time_brightness, true, CAMERA_MODE_FIRST, 3, 0);
			g_menusky->render();
			g_menuclouds->update(v3f(0, 0, 0), g_menusky->getCloudColor());
		}
		g_menuclouds->step(dtime * 3);
		g_menuclouds->render();
		g_menucloudsmgr->drawAll();
	}

	video::ITexture *texture = m_load_bg_texture.empty() ? nullptr : driver->getTexture(m_load_bg_texture.c_str());
	if (texture == nullptr) {
		if (!cloud_menu_background) {
			video::ITexture *background_image = tsrc->getTexture("bg.png");

			driver->draw2DImage(background_image,
				irr::core::rect<s32>(0, 0, screensize.X * 4, screensize.Y * 4),
				irr::core::rect<s32>(0, 0, screensize.X, screensize.Y), 0, 0, false);
		}
		return;
	}

	const v2u32 sourcesize = texture->getOriginalSize();

	/* Draw background texture */
	float aspectRatioScreen = (float) screensize.X / screensize.Y;
	float aspectRatioSource = (float) sourcesize.X / sourcesize.Y;

	int sourceX = aspectRatioSource > aspectRatioScreen ? (sourcesize.X - sourcesize.Y * aspectRatioScreen) / 2 : 0;
	int sourceY = aspectRatioSource < aspectRatioScreen ? (sourcesize.Y - sourcesize.X / aspectRatioScreen) / 2 : 0;

	draw2DImageFilterScaled(driver, texture,
			core::rect<s32>(0, 0, screensize.X, screensize.Y),
			core::rect<s32>(sourceX, sourceY, sourcesize.X - sourceX, sourcesize.Y - sourceY),
			NULL, NULL, true);
}

bool RenderingEngine::handle_load_screen_touch(const SEvent &event)
{
	if (!s_singleton || !s_singleton->m_load_button)
		return false;

	SEvent mouse = {};
	mouse.EventType = EET_MOUSE_INPUT_EVENT;
	mouse.MouseInput.X = event.TouchInput.X;
	mouse.MouseInput.Y = event.TouchInput.Y;

	switch (event.TouchInput.Event) {
	case ETIE_PRESSED_DOWN:
		mouse.MouseInput.Event = EMIE_LMOUSE_PRESSED_DOWN;
		mouse.MouseInput.ButtonStates = EMBSM_LEFT;
		break;
	case ETIE_MOVED:
		mouse.MouseInput.Event = EMIE_MOUSE_MOVED;
		mouse.MouseInput.ButtonStates = EMBSM_LEFT;
		break;
	case ETIE_LEFT_UP:
		mouse.MouseInput.Event = EMIE_LMOUSE_LEFT_UP;
		break;
	default:
		return false;
	}

	return get_gui_env()->postEventFromUser(mouse);
}

void RenderingEngine::_draw_load_cleanup()
{
	if (m_load_text) {
		m_load_text->remove();
		m_load_text = nullptr;
	}

	if (m_load_root) {
		m_load_root->remove();
		m_load_root = nullptr;
		m_load_button = nullptr;
	}

	m_load_cancel = nullptr;
	m_percent_shown = 0.0f;

	if (!m_load_bg_texture.empty()) {
		video::ITexture *texture = driver->getTexture(m_load_bg_texture.c_str());

		if (texture)
			driver->removeTexture(texture);
	}

	m_load_screen_dtime = 0;
	m_load_screen_drawn = false;
	m_percent = 0;
}

std::vector<core::vector3d<u32>> RenderingEngine::getSupportedVideoModes()
{
	IrrlichtDevice *nulldevice = createDevice(video::EDT_NULL);
	sanity_check(nulldevice);

	std::vector<core::vector3d<u32>> mlist;
	video::IVideoModeList *modelist = nulldevice->getVideoModeList();

	s32 num_modes = modelist->getVideoModeCount();
	for (s32 i = 0; i != num_modes; i++) {
		core::dimension2d<u32> mode_res = modelist->getVideoModeResolution(i);
		u32 mode_depth = (u32)modelist->getVideoModeDepth(i);
		mlist.emplace_back(mode_res.Width, mode_res.Height, mode_depth);
	}

	nulldevice->drop();
	return mlist;
}

std::vector<irr::video::E_DRIVER_TYPE> RenderingEngine::getSupportedVideoDrivers()
{
	std::vector<irr::video::E_DRIVER_TYPE> drivers;

	for (int i = 0; i != irr::video::EDT_COUNT; i++) {
		if (irr::IrrlichtDevice::isDriverSupported((irr::video::E_DRIVER_TYPE)i))
			drivers.push_back((irr::video::E_DRIVER_TYPE)i);
	}

	return drivers;
}

void RenderingEngine::_initialize(Client *client, Hud *hud)
{
	const std::string &draw_mode = g_settings->get("3d_mode");
	core.reset(createRenderingCore(draw_mode, m_device, client, hud));
	core->initialize();
}

void RenderingEngine::_finalize()
{
	core.reset();
}

void RenderingEngine::_clear_irrlicht_texture_cache()
{
	for (u32 i = 0; i < driver->getTextureCount(); i++) {
		irr::video::ITexture *texture = driver->getTextureByIndex(i);

		if (texture) {
			std::string filename = texture->getName().getPath().c_str();

			if ((filename.find("TTFontGlyphPage") == 0) || (filename == "#DefaultFont"))
				continue;

			driver->removeTexture(texture);
		}
	}
}

void RenderingEngine::_draw_scene(video::SColor skycolor, bool show_hud,
		bool show_minimap, bool draw_wield_tool, bool draw_crosshair, bool draw_nametags)
{
	core->draw(skycolor, show_hud, show_minimap, draw_wield_tool, draw_crosshair, draw_nametags);
}

const char *RenderingEngine::getVideoDriverName(irr::video::E_DRIVER_TYPE type)
{
	static const char *driver_ids[] = {
			"null",
			"software",
			"burningsvideo",
			"direct3d8",
			"direct3d9",
			"opengl",
			"ogles1",
			"ogles2",
			"webgl1",
			"metal",
	};

	return driver_ids[type];
}

const char *RenderingEngine::getVideoDriverFriendlyName(irr::video::E_DRIVER_TYPE type)
{
	static const char *driver_names[] = {
			"NULL Driver",
			"Software Renderer",
			"Burning's Video",
			"Direct3D 8",
			"Direct3D 9",
			"OpenGL",
			"OpenGL ES1",
			"OpenGL ES2",
			"WebGL 1",
			"Metal",
	};

	return driver_names[type];
}

float RenderingEngine::getScreenScale()
{
#if defined(__ANDROID__) || defined(__APPLE__)
	// the platform knows its own scale, SDL only reports a window against a drawable
	const float scale = porting::getScreenScale();
#if defined(__MACH__) && defined(__APPLE__) && !defined(__IOS__)
	// pre-retina panels cluster around 128ppi, the middle of the legibility band
	if (scale <= 1.0f)
		return 128.0f / 96.0f;
#endif
	return scale;
#elif defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
	const SDL_DisplayID display = SDL_GetPrimaryDisplay();
	const SDL_DisplayMode *mode = SDL_GetDesktopDisplayMode(display);
	if (!mode)
		return 0.0f;

	return SDL_GetDisplayContentScale(display) * mode->pixel_density;
#else
	return 0.0f;
#endif
}

#if defined(__ANDROID__) || defined(__APPLE__)

float RenderingEngine::getDisplayDensity()
{
	const float scale = getScreenScale();
	return (scale > 0.0f ? scale : 1.0f) * g_settings->getFloat("display_density_factor");
}

#endif

float RenderingEngine::getHudScaling()
{
#if defined(HAVE_TOUCHSCREENGUI) || \
		(defined(__MACH__) && defined(__APPLE__) && !defined(__IOS__))
	// a value of its own is the user's, an inherited default is ours to fit
	if (!g_settings->existsLocal("hud_scaling")) {
		// this runs per frame while a menu is open
		static v2u32 fitted_size;
		static float fitted_scaling = 1.0f;

#ifdef HAVE_TOUCHSCREENGUI
		const v2u32 size = getDisplaySize();
#else
		const RenderingEngine *engine = get_instance();
		const v2u32 size = engine ? engine->getWindowSize() : v2u32(0, 0);
#endif
		if (size.X > 0 && size.Y > 0) {
			if (size != fitted_size) {
				fitted_size = size;
				fitted_scaling = getAutoHudScaling(size, getDisplayDensity());
			}
			return fitted_scaling;
		}
	}
#endif

	return g_settings->getFloat("hud_scaling");
}

#if !defined(__ANDROID__) && !defined(__IOS__)
#if defined(XORG_USED)

static float calcDisplayDensity(irr::video::IVideoDriver *driver)
{
#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
	const video::SExposedVideoData exposedData = driver->getExposedVideoData();

	SDL_Window *window = exposedData.OpenGLSDL.Window;

	if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") != 0)
		return g_settings->getFloat("screen_dpi") / 96.0;
#endif

	const char *current_display = getenv("DISPLAY");

	if (current_display != NULL) {
		Display *x11display = XOpenDisplay(current_display);

		if (x11display != NULL) {
			/* try x direct */
			int dh = DisplayHeight(x11display, 0);
			int dw = DisplayWidth(x11display, 0);
			int dh_mm = DisplayHeightMM(x11display, 0);
			int dw_mm = DisplayWidthMM(x11display, 0);
			XCloseDisplay(x11display);

			if (dh_mm != 0 && dw_mm != 0) {
				float dpi_height = floor(dh / (dh_mm * 0.039370) + 0.5);
				float dpi_width = floor(dw / (dw_mm * 0.039370) + 0.5);
				return std::max(dpi_height, dpi_width) / 96.0;
			}
		}
	}

	/* return manually specified dpi */
	return g_settings->getFloat("screen_dpi") / 96.0;
}

float RenderingEngine::getDisplayDensity()
{
	static float cached_display_density = calcDisplayDensity(get_video_driver());
	return cached_display_density * g_settings->getFloat("display_density_factor");
}

#elif defined(_WIN32)


static float calcDisplayDensity(irr::video::IVideoDriver *driver)
{
	HWND hWnd;
	if (getWindowHandle(driver, hWnd)) {
		HDC hdc = GetDC(hWnd);
		float dpi = GetDeviceCaps(hdc, LOGPIXELSX);
		ReleaseDC(hWnd, hdc);
		return dpi / 96.0f;
	}

	/* return manually specified dpi */
	return g_settings->getFloat("screen_dpi") / 96.0f;
}

float RenderingEngine::getDisplayDensity()
{
	static bool cached = false;
	static float display_density;
	if (!cached) {
		display_density = calcDisplayDensity(get_video_driver());
		cached = true;
	}
	return display_density * g_settings->getFloat("display_density_factor");
}

#elif !defined(__APPLE__)

float RenderingEngine::getDisplayDensity()
{
	return (g_settings->getFloat("screen_dpi") / 96.0) * g_settings->getFloat("display_density_factor");
}

#endif

v2u32 RenderingEngine::getDisplaySize()
{
	IrrlichtDevice *nulldevice = createDevice(video::EDT_NULL);

	core::dimension2d<u32> deskres =
			nulldevice->getVideoModeList()->getDesktopResolution();
	nulldevice->drop();

	return deskres;
}

#else // __ANDROID__/__IOS__

v2u32 RenderingEngine::getDisplaySize()
{
	const RenderingEngine *engine = RenderingEngine::get_instance();
	if (engine == nullptr)
		return v2u32(0, 0);
	return engine->getWindowSize();
}

int RenderingEngine::getWindowSafeArea()
{
#ifdef __IOS__
	// don't make it static
	return MultiCraft::getScreenRound();
#elif defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
	RenderingEngine *engine = RenderingEngine::get_instance();

	if (engine) {
		video::IVideoDriver* driver = engine->getVideoDriver();

		if (driver) {
			const video::SExposedVideoData exposedData = driver->getExposedVideoData();
			SDL_Window *window = exposedData.OpenGLSDL.Window;

			SDL_Rect safe;
			if (window && SDL_GetWindowSafeArea(window, &safe))
				return (safe.x > safe.y) ? safe.x : safe.y;
		}
	}

	return 0;
#else
	return 0;
#endif
}

#endif // __ANDROID__/__IOS__

#ifdef HAVE_TOUCHSCREENGUI
bool RenderingEngine::isTablet()
{
#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
	static const bool isTablet = SDL_IsTablet();
	return isTablet;
#else
	return false;
#endif
}
#endif

bool RenderingEngine::isHighDpi()
{
	float density = RenderingEngine::getDisplayDensity();
#if defined(__MACH__) && defined(__APPLE__) && !defined(__IOS__)
	return density >= 2.0f;
#elif defined(__IOS__)
	return isTablet() ? (density >= 2.0f) : (density >= 3.0f);
#else
	return density >= 3.0f;
#endif
}

void RenderingEngine::startTextInput()
{
#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
	RenderingEngine *engine = RenderingEngine::get_instance();

	SDL_SetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD, porting::hasRealKeyboard() ? "0" : "1");

	if (engine && porting::hasRealKeyboard()) {
		video::IVideoDriver* driver = engine->getVideoDriver();
		if (driver) {
			const video::SExposedVideoData exposedData = driver->getExposedVideoData();
			SDL_Window *window = exposedData.OpenGLSDL.Window;

			if (window)
				SDL_StartTextInput(window);
		}
	}
#endif
}

void RenderingEngine::stopTextInput()
{
#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
	RenderingEngine *engine = RenderingEngine::get_instance();

	if (engine && porting::hasRealKeyboard()) {
		video::IVideoDriver* driver = engine->getVideoDriver();

		if (driver) {
			const video::SExposedVideoData exposedData = driver->getExposedVideoData();
			SDL_Window *window = exposedData.OpenGLSDL.Window;

			if (window && SDL_TextInputActive(window))
				SDL_StopTextInput(window);
		}
	}
#endif
}
