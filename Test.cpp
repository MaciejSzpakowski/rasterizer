#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <cmath>
#include <Windows.h>
#include "../Rasterizer/rast.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#ifdef _DEBUG
#pragma comment(lib, "../x64/Debug/Rasterizer.lib")
#else
#pragma comment(lib, "../x64/Release/Rasterizer.lib")
#endif

#define PI 3.14159265f

rast::vertex2 verts[] = { {0.5f,0.5f, 0.0f, 0.3f, 0},{-0.5f,0.5f, 0.0f, 0, 0},{0.5f, -0.5f, 0.0f, 0.3f, 1},{-0.5f, -0.5f, 0.0f, 0, 1} };
float t = 0.0f;
float x = 0.0f;
float y = 0.0f;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN:
        if (wParam == 'Q')
            t -= 0.01f;
        if (wParam == 'E')
            t += 0.01f;
        if (wParam == VK_LEFT)
            x -= 0.01f;
        if (wParam == VK_RIGHT)
            x += 0.01f;
        if (wParam == VK_UP)
            y += 0.01f;
        if (wParam == VK_DOWN)
            y -= 0.01f;
        break;
    case WM_CLOSE:
        exit(0);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void loadImage(const char* filename, unsigned char** dst, int* sizex, int* sizey)
{
    int x = -1, y = -1, n = -1;
    const int components = 4; // components means how many elements from 'RGBA'
                              // you want to return, I want 4 (RGBA) even in not all 4 are present
    unsigned char* data = stbi_load(filename, sizex, sizey, &n, components);
    *dst = data;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wcex;
    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = "myclass";

    RegisterClassEx(&wcex);

    HWND hwnd = CreateWindowEx(0, "myclass", "Nothing", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1300, 1300, nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, nCmdShow);

    /////
    rast::core vi(hwnd);
    rast::vertex2 verts2[] = { {0.51f,0.14f, 1.0f, 1, 0},{-0.08,0.14f, 1.0f, 0, 0},{0.51f, -0.45f, 1.0f, 1, 1},{-0.08f, -0.45f, 1.0f, 0, 1} };
    rast::texture brickTex;
    rast::texture whiteTex;
    int whiteTexData = 0xff0000ff;
    whiteTex.data = (byte*)&whiteTexData;
    whiteTex.sz = { 1,1 };

    int x, y;
    loadImage("brick.jpg", (unsigned char**)&brickTex.data, &x, &y);
    brickTex.sz.width = x;
    brickTex.sz.height = y;

    vi.setTexture(brickTex);

    /////
    MSG msg = {};

    int fps = 0;
    int fpsLast = time(0);

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            vi.clear({ 255,255,255 });
            vi.setFillMode(rast::fillMode::SOLID);
            vi.setTexture(brickTex);
            vi.drawPrimitive(rast::topology::TRIANGLESTRIP, 4, verts);
            vi.setTexture(whiteTex);
            vi.drawPrimitive(rast::topology::TRIANGLESTRIP, 4, verts2);
            vi.present();
            fps++;

            if (fpsLast != time(0))
            {
                fpsLast = time(0);
                char str[10];
                sprintf_s(str, 10, "%d", fps);
                fps = 0;
                SetWindowText(hwnd, str);
            }

            // transform square
            verts[0].x = sin(t + PI / 4.0f) * 0.5f + ::x;
            verts[0].y = cos(t + PI / 4.0f) * 0.5f + ::y;
            verts[1].x = sin(t + PI / 4.0f * 3.0f) * 0.5f + ::x;
            verts[1].y = cos(t + PI / 4.0f * 3.0f) * 0.5f + ::y;
            verts[2].x = sin(t + PI / 4.0f * 7.0f) * 0.5f + ::x;
            verts[2].y = cos(t + PI / 4.0f * 7.0f) * 0.5f + ::y;
            verts[3].x = sin(t + PI / 4.0f * 5.0f) * 0.5f + ::x;
            verts[3].y = cos(t + PI / 4.0f * 5.0f) * 0.5f + ::y;
        }
    }

    return 0;
}