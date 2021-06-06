#include "game_window.h"

#include <chrono>
#include <iostream>

GameWindow::~GameWindow() {
	if (m_handle) {
		DestroyWindow(m_handle);
		m_handle = nullptr;
	}
}

void GameWindow::run() {
	if (!createWindow()) return;
	m_shouldClose = false;

	auto t = std::thread(&GameWindow::coreLoop, this);
	eventLoop();

	t.join();
}

void GameWindow::resize(int width, int height, bool center) {
	RECT wect;
	GetWindowRect(m_handle, &wect);

	if (center) {
		wect.left = GetSystemMetrics(SM_CXSCREEN) / 2 - width / 2;
		wect.top = GetSystemMetrics(SM_CYSCREEN) / 2 - height / 2;
	}

	SetWindowPos(m_handle, nullptr, wect.left, wect.top, width, height, 0);

	RECT nect;
	nect.left = wect.left;
	nect.top = wect.top;
	nect.right = nect.left + width;
	nect.bottom = nect.top + height;

	m_width = width;
	m_height = height;

	AdjustWindowRectEx(&nect, m_dwStyle, 0, 0);

	m_resizeMutex.lock();
	onResize();
	m_resizeMutex.unlock();
}

bool GameWindow::createWindow() {
	WNDCLASS wc = {};
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.lpfnWndProc = GameWindow::WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.lpszMenuName = nullptr;
	wc.hbrBackground = nullptr;
	wc.lpszClassName = L"GameWindow";

	DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	m_dwStyle = WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_THICKFRAME | WS_MINIMIZEBOX;

	if (!RegisterClass(&wc)) {
		MessageBoxA(NULL, "Failed to initialize window.", "Error", MB_OK);
		return false;
	}

	RECT wect;
	wect.left = GetSystemMetrics(SM_CXSCREEN) / 2 - m_width / 2;
	wect.top = GetSystemMetrics(SM_CYSCREEN) / 2 - m_height / 2;
	wect.right = wect.left + m_width;
	wect.bottom = wect.top + m_height;

	AdjustWindowRectEx(&wect, m_dwStyle, 0, 0);

	m_handle = CreateWindowEx(
		dwExStyle,
		L"GameWindow",
		L"Window",
		m_dwStyle,
		wect.left, wect.top, wect.right - wect.left, wect.bottom - wect.top,
		NULL, NULL,
		GetModuleHandle(nullptr),
		this
	);
	SetWindowLongPtrA(m_handle, GWLP_USERDATA, (LONG_PTR)this);

	ShowWindow(m_handle, SW_SHOW);
	UpdateWindow(m_handle);

	return true;
}

bool GameWindow::preCreate() {
	gl::initializeGL();

	int pixel_format_attribs[] = {
		WGL_DRAW_TO_WINDOW_ARB,     GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB,     GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB,      GL_TRUE,
		WGL_ACCELERATION_ARB,       WGL_FULL_ACCELERATION_ARB,
		WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB,         32,
		WGL_DEPTH_BITS_ARB,         m_config.depthBits,
		WGL_STENCIL_BITS_ARB,       m_config.stencilBits,
		0
	};

	m_dc = GetDC(m_handle);

	int pixel_format;
	UINT num_formats;
	if (!gl::wglChoosePixelFormatARB(m_dc, pixel_format_attribs, 0, 1, &pixel_format, &num_formats)) {
		MessageBoxA(nullptr, "Failed to choose pixel format.", "Error", MB_OK);
		return false;
	}

	PIXELFORMATDESCRIPTOR pfd = {};
	DescribePixelFormat(m_dc, pixel_format, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	pfd.cRedBits = m_config.redBits;
	pfd.cGreenBits = m_config.greenBits;
	pfd.cBlueBits = m_config.blueBits;
	pfd.cAlphaBits = m_config.alphaBits;

	if (!SetPixelFormat(m_dc, pixel_format, &pfd)) {
		MessageBoxA(nullptr, "Failed to set pixel format.", "Error", MB_OK);
		return false;
	}

	int gl_attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, m_config.majorVersion,
		WGL_CONTEXT_MINOR_VERSION_ARB, m_config.minorVersion,
		WGL_CONTEXT_PROFILE_MASK_ARB,  m_config.profile,
		0,
	};

	m_context = gl::wglCreateContextAttribsARB(m_dc, 0, gl_attribs);
	if (!wglMakeCurrent(m_dc, m_context)) {
		MessageBoxA(nullptr, "Failed to make OpenGL context as current.", "Error", MB_OK);
		return false;
	}

	gladLoadGL();
	//glEnable(GL_FRAMEBUFFER_SRGB);

	std::cout << "OpenGL v" << glGetString(GL_VERSION) << " - " << glGetString(GL_VENDOR) << std::endl;
	return true;
}

void GameWindow::eventLoop() {
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void GameWindow::coreLoop() {
	if (!preCreate()) return;

	onCreate();
	auto prevTime = std::chrono::system_clock::now();

	while (!m_shouldClose) {
		auto currTime = std::chrono::system_clock::now();
		std::chrono::duration<float> elapsed = currTime - prevTime;
		prevTime = currTime;

		onDraw(elapsed.count());
		swapBuffers();
	}

	wglDeleteContext(m_context);

	onDestroy();
}

LRESULT GameWindow::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	GameWindow* gw = (GameWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (uMsg) {
		case WM_DESTROY: DestroyWindow(hwnd); return 0;
		case WM_CLOSE: PostQuitMessage(0); gw->quit(); return 0;
		default: break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void gl::initializeGL() {
	if (wglCreateContextAttribsARB) return;

	WNDCLASSEX cls = {};
	cls.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	cls.lpfnWndProc = DefWindowProcA;
	cls.hInstance = GetModuleHandle(0);
	cls.lpszClassName = L"Dummy_WGL";
	cls.cbSize = sizeof(WNDCLASSEX);

	if (!RegisterClassEx(&cls)) {
		error("Failed to register OpenGL window.");
		return;
	}

	HWND dummy_window = CreateWindowEx(
		0,
		cls.lpszClassName,
		L"Dummy OpenGL",
		0,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		0,
		cls.hInstance,
		0
	);

	if (!dummy_window) {
		error("Failed to create OpenGL window.");
	}

	HDC dummy_dc = GetDC(dummy_window);

	PIXELFORMATDESCRIPTOR pfd = {};
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.cColorBits = 32;
	pfd.cAlphaBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;

	int pixel_format = ChoosePixelFormat(dummy_dc, &pfd);
	if (!pixel_format) {
		error("Failed to find a suitable pixel format.");
	}
	if (!SetPixelFormat(dummy_dc, pixel_format, &pfd)) {
		error("Failed to set the pixel format.");
	}

	HGLRC dummy_context = wglCreateContext(dummy_dc);
	if (!dummy_context) {
		error("Failed to create a dummy OpenGL rendering context.");
	}

	if (!wglMakeCurrent(dummy_dc, dummy_context)) {
		error("Failed to activate dummy OpenGL rendering context.");
	}

	wglCreateContextAttribsARB = (wglCreateContextAttribsARB_type*)wglGetProcAddress(
		"wglCreateContextAttribsARB");
	wglChoosePixelFormatARB = (wglChoosePixelFormatARB_type*)wglGetProcAddress(
		"wglChoosePixelFormatARB");

	gladLoadGL();

	wglMakeCurrent(dummy_dc, 0);
	wglDeleteContext(dummy_context);
	ReleaseDC(dummy_window, dummy_dc);
	DestroyWindow(dummy_window);
}
