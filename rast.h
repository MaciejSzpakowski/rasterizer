#pragma once
#include <Windows.h>

namespace rast
{
    typedef unsigned char byte;
    const int BYTES_PER_PIXEL = 4;

    struct color
    {
        byte r, g, b;
    };

    struct color2
    {
        byte r, g, b, a;
    };

    struct pointf
    {
        float x, y;
    };

    struct rectf
    {
        float left, top, right, bottom;
    };

    struct sizef
    {
        float width, height;
    };

    struct vertex
    {
        float x, y, z;
    };

    struct vertex2
    {
        float x, y, z, u, v, f6, f7, f8, f9, f10;
    };

    enum class topology : int
    {
        POINTLIST = 1,
        LINELIST = 2,
        LINESTRIP = 3,
        TRIANGLELIST = 4,
        TRIANGLESTRIP = 5,
        QUADLIST = 6,
    };

    enum class fillMode : int
    {
        WIREFRAME = 1,
        SOLID = 2
    };

    struct texture
    {
        union
        {
            byte* data;
            color2* col;
        };
        sizef sz;
    };

    struct line
    {
        vertex2 start, end;
        vertex2* left, *right, *top, *bottom;
        float delta, dx, dy;
    };

    class core
    {
    private:
        int width;
        int height;
         HWND hwnd;
         HDC wndDc;
         HDC backbufferDc;
         HBITMAP bmp;
        byte* frameData;
        float* depthBuffer;
        fillMode mode;
        texture tex;
        int depthBufferCount;

        void test();
        void drawLine(vertex2 start, vertex2 end);
        void drawLine(vertex2 start, vertex2 end, bool denormalize);
        void clipLine(line& ln, bool denormalize);
        pointf norm2screen(pointf& p);
        void norm2screen(vertex2& p);
        void drawPrimitiveFill(topology topo, int vertexCount, vertex2* data);
        void drawPrimitiveWire(topology topo, int vertexCount, vertex2* data);
        void drawTriangleFill(vertex2* data, bool denormalize);
        void drawTriangleFlatTop(vertex2* p);
        void drawTriangleFlatBottom(vertex2* p);
    public:
        core( HWND hwnd);

        void clear(color c);

        void present();

        void drawPrimitive(topology topo, int vertexCount, vertex2* data);

        void setFillMode(fillMode mode);

        void setTexture(const texture& tex);

        // put data in frame buffer directly
        void blt(rectf& rect, byte* data);
    };
}