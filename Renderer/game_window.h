#pragma once

#include <cstdint>
#include <Windows.h>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>

#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")

#include "glad.h"
#pragma comment (lib, "opengl32.lib")

#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

#define WGL_DRAW_TO_WINDOW_ARB                    0x2001
#define WGL_ACCELERATION_ARB                      0x2003
#define WGL_SUPPORT_OPENGL_ARB                    0x2010
#define WGL_DOUBLE_BUFFER_ARB                     0x2011
#define WGL_PIXEL_TYPE_ARB                        0x2013
#define WGL_COLOR_BITS_ARB                        0x2014
#define WGL_DEPTH_BITS_ARB                        0x2022
#define WGL_STENCIL_BITS_ARB                      0x2023
#define WGL_SAMPLE_BUFFERS_ARB                    0x2041
#define WGL_SAMPLES_ARB                           0x2042

#define WGL_FULL_ACCELERATION_ARB                 0x2027
#define WGL_TYPE_RGBA_ARB                         0x202B

#include "renderdoc_app.h"

struct ContextConfig {
	uint8_t depthBits{ 24 },
		stencilBits{ 8 },
		redBits{ 8 },
		greenBits{ 8 },
		blueBits{ 8 },
		alphaBits{ 8 },
		majorVersion{ 1 },
		minorVersion{ 1 };
	enum Profile {
		ProfileCore = WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		ProfileCompat = WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB
	} profile{ ProfileCompat };
};

namespace gl {
	typedef HGLRC WINAPI wglCreateContextAttribsARB_type(HDC hdc, HGLRC hShareContext, const int* attribList);
	static wglCreateContextAttribsARB_type* wglCreateContextAttribsARB;

	typedef BOOL WINAPI wglChoosePixelFormatARB_type(HDC hdc, const int* piAttribIList,
													 const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);
	static wglChoosePixelFormatARB_type* wglChoosePixelFormatARB;

	static void error(const std::string& text) {
		MessageBoxA(nullptr, text.c_str(), "OpenGL Error", MB_OK | MB_ICONERROR);
	}

	static void initializeGL();
};

class GameWindow {
public:
	GameWindow() = default;
	~GameWindow();

	GameWindow(const ContextConfig& config) : m_config(config) {}

	void run();
	void resize(int width, int height, bool center = false);

	virtual void onCreate() {}
	virtual void onDraw(float elapsedTime) {}
	virtual void onResize() {}
	virtual void onDestroy() {}

	void swapBuffers() { SwapBuffers(m_dc); /*DwmFlush();*/ }
	void quit() { m_shouldClose = true; }

	void setTitle(const std::string& title) { SetWindowTextA(m_handle, title.c_str()); }

	uint32_t width() const { return m_width; }
	uint32_t height() const { return m_height; }

private:
	HWND m_handle{ nullptr };
	HDC m_dc{ nullptr };
	ContextConfig m_config;
	HGLRC m_context{ nullptr };
	DWORD m_dwStyle{ 0 };

	uint32_t m_width{ 640 }, m_height{ 480 };

	std::atomic<bool> m_shouldClose{ false };
	std::mutex m_resizeMutex{};

	bool createWindow();
	bool preCreate();
	void eventLoop();
	void coreLoop();

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
