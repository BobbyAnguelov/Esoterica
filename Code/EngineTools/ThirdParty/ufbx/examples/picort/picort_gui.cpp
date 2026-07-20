#include "picort.h"
#include <mutex>

#ifndef GUI
	#define GUI 0
#endif

#if GUI && defined(_WIN32)
	#define NOMINMAX
	#include <Windows.h>
#endif

#if GUI && defined(_WIN32)

struct BmpHeaderRgba
{
	char data[122] =
		// magic  size    unused   data offset    DIB size   width  height
		"" "BM"  "????" "\0\0\0\0" "\x7a\0\0\0" "\x6c\0\0\0" "????" "????" 
		// 1-plane 32-bits  bitfields    data-size     print resolution
		"" "\x1\0" "\x20\0" "\x3\0\0\0"   "????"   "\x13\xb\0\0\x13\xb\0\0"
		//   palette counts            channel masks for RGBA            colors
		"" "\0\0\0\0\0\0\0\0" "\xff\0\0\0\0\xff\0\0\0\0\xff\0\0\0\0\xff" "sRGB";
			
	BmpHeaderRgba(size_t width=0, size_t height=0) {
		size_t size = width * height * 4;
		patch(0x02, 122 + size); // total size
		patch(0x12, width);      // width (left-to-right)
		patch(0x16, 0 - height); // height (top-to-bottom)
		patch(0x22, size);       // data size
	}

	void patch(size_t offset, size_t value) {
		data[offset+0] = (char)((value >>  0) & 0xff);
		data[offset+1] = (char)((value >>  8) & 0xff);
		data[offset+2] = (char)((value >> 16) & 0xff);
		data[offset+3] = (char)((value >> 24) & 0xff);
	}
};

struct GuiStateWin32
{
	HANDLE init_event = NULL;
	std::thread thread;
	HWND hwnd;

	UINT_PTR timer;
	std::recursive_mutex mutex;
	Framebuffer internal_framebuffer;
	const Framebuffer *framebuffer;
	std::unique_ptr<DebugTracerBase> tracer;
};

static GuiStateWin32 g_gui;

LRESULT CALLBACK win32_wndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	std::lock_guard<std::recursive_mutex> lg { g_gui.mutex };
	const Framebuffer *fb = g_gui.framebuffer;

	switch (uMsg)
	{
	case WM_CLOSE: DestroyWindow(hwnd); return 0;
	case WM_DESTROY: PostQuitMessage(0); return 0;
	case WM_TIMER: {
		if (fb) {
			RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_INVALIDATE);
		}
	} return 0;
	default: return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		if (fb) {
			RECT rect;
			GetClientRect(hwnd, &rect);
			int width = rect.right - rect.left, height = rect.bottom - rect.top;

			BmpHeaderRgba header { fb->width, fb->height };
			StretchDIBits(hdc, 0, 0, width, height, 0, 0, (int)fb->width, (int)fb->height,
				fb->pixels.data(), (BITMAPINFO*)(header.data + 0xe), DIB_RGB_COLORS, SRCCOPY);
		}

		EndPaint(hwnd, &ps);
	} return 0;
	case WM_LBUTTONDOWN: if (g_gui.tracer) {
		RECT rect;
		GetClientRect(hwnd, &rect);
		Vec2 window_pos { (Real)LOWORD(lParam), (Real)HIWORD(lParam) };
		Vec2 uv = window_pos / Vec2{ (Real)(rect.right - rect.left), (Real)(rect.bottom - rect.top) };
		Vec3 l = g_gui.tracer->trace(uv);
		printf("(%.6f, %.6f) -> (%.4f, %.4f, %.4f)\n",
			uv.x, uv.y, l.x, l.y, l.z);
	} return 0;
	}
}

void win32_gui_thread(const Framebuffer *fb)
{
	WNDCLASSW wc = { };
	wc.lpfnWndProc = &win32_wndproc;
	wc.hInstance = GetModuleHandleW(NULL);
	wc.lpszClassName = L"picort";
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	ATOM atom = RegisterClassW(&wc);

	DWORD style = WS_VISIBLE|WS_OVERLAPPEDWINDOW;
	RECT rect = { 0, 0, (int)fb->width, (int)fb->height };
	AdjustWindowRectEx(&rect, style, FALSE, 0);
	g_gui.hwnd = CreateWindowExW(0, MAKEINTATOM(atom), L"picort", style,
		CW_USEDEFAULT, CW_USEDEFAULT, rect.right-rect.left, rect.bottom-rect.top,
		NULL, NULL, wc.hInstance, NULL);

	SetEvent(g_gui.init_event);

	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0) != 0) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

void enable_gui(const Framebuffer *fb)
{
	if (!g_gui.init_event) {
		g_gui.internal_framebuffer = Framebuffer{ 1, 1 };
		g_gui.framebuffer = &g_gui.internal_framebuffer;
		g_gui.init_event = CreateEventW(NULL, FALSE, FALSE, NULL);
		g_gui.thread = std::thread{ win32_gui_thread, fb };
		WaitForSingleObject(g_gui.init_event, INFINITE);
	}

	std::lock_guard<std::recursive_mutex> lg { g_gui.mutex };
	g_gui.framebuffer = fb;
	if (!g_gui.timer) {
		g_gui.timer = SetTimer(g_gui.hwnd, NULL, 100, NULL);
	}
}

void disable_gui(Framebuffer &&fb, std::unique_ptr<DebugTracerBase> tracer)
{
	{
		std::lock_guard<std::recursive_mutex> lg { g_gui.mutex };
		g_gui.internal_framebuffer = std::move(fb);
		g_gui.framebuffer = &g_gui.internal_framebuffer;
		KillTimer(g_gui.hwnd, g_gui.timer);
		g_gui.timer = 0;
		g_gui.tracer = std::move(tracer);
	}

	RedrawWindow(g_gui.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_INVALIDATE);
}

void close_gui()
{
	PostMessageW(g_gui.hwnd, WM_CLOSE, 0, 0);
}

void wait_gui()
{
	g_gui.thread.join();
}

#else

void enable_gui(const Framebuffer *fb) { }
void disable_gui(Framebuffer &&fb, std::unique_ptr<DebugTracerBase> tracer) { }
void close_gui() { }
void wait_gui() { }

#endif
