#include "rast.h"
#include <cmath>

namespace rast
{
    core::core( HWND hwnd)
    {
        this->width = 0;
        this->height = 0;
        this->hwnd = nullptr;
        this->wndDc = nullptr;
        this->backbufferDc = nullptr;
        this->bmp = nullptr;
        this->frameData = nullptr;
        this->depthBufferCount = 2;

        this->hwnd = hwnd;
        this->wndDc = GetDC(this->hwnd);
        this->backbufferDc = CreateCompatibleDC(this->wndDc);
         RECT r;
        GetClientRect(this->hwnd, &r);
        this->width = r.right - r.left;
        this->height = r.bottom - r.top;
        this->frameData = new byte[this->width * this->height * BYTES_PER_PIXEL];
        this->depthBuffer = new float[this->width * this->height * depthBufferCount];

        HDC hdc =  GetDC(nullptr);
         BITMAPINFOHEADER bmih;
        memset(&bmih, 0, sizeof( BITMAPINFOHEADER));
        bmih.biWidth = this->width;
        bmih.biHeight = this->height;
        bmih.biBitCount = BYTES_PER_PIXEL * 8;
        bmih.biCompression =  BI_RGB;
        bmih.biSize = sizeof( BITMAPINFOHEADER);
        bmih.biPlanes = 1;
         BITMAPINFO* bmi = ( BITMAPINFO*)&bmih;
        this->bmp =  CreateDIBitmap(hdc, &bmih,  CBM_INIT, this->frameData, bmi,  DIB_RGB_COLORS);
         ReleaseDC(nullptr, hdc);
         SelectObject(this->backbufferDc, this->bmp);
    }

    pointf core::norm2screen(pointf& p)
    {
        return { (p.x + 1.0f) / 2.0f * this->width, (-p.y + 1.0f) / 2.0f * this->height };
    }

    void core::norm2screen(vertex2& p)
    {
        p.x = (p.x + 1.0f) / 2.0f * this->width;
        p.y = (-p.y + 1.0f) / 2.0f * this->height;
    }

    void core::drawLine(vertex2 start, vertex2 end)
    {
        this->drawLine(start, end, true);
    }

    void core::clipLine(line& ln, bool denormalize)
    {
        if (denormalize)
        {
            this->norm2screen(ln.start);
            this->norm2screen(ln.end);
        }

        if (ln.start.x < ln.end.x)
        {
            ln.left = &ln.start;
            ln.right = &ln.end;
        }
        else
        {
            ln.right = &ln.start;
            ln.left = &ln.end;
        }

        if (ln.start.y > ln.end.y)
        {
            ln.bottom = &ln.start;
            ln.top = &ln.end;
        }
        else
        {
            ln.top = &ln.start;
            ln.bottom = &ln.end;
        }

        ln.dy = ln.right->y - ln.left->y;
        ln.dx = ln.right->x - ln.left->x;
        ln.delta = ln.dy / ln.dx;
        float t = 0;

        // line is outside (bounding box test = line bounding rectangle is outside frame)
        if (ln.bottom->y < 0 || ln.top->y >= this->height || ln.right->x < 0 || ln.left->x >= this->width)
        {
            ln.left->x = 0;
            ln.left->y = 0;
            ln.right->x = 0;
            ln.right->y = 0;
        }

        // line is inside (bounding box test = line bounding rectangle is inside frame)
        else if (ln.bottom->y < this->height && ln.top->y >= 0 && ln.left->x >= 0 && ln.right->x < this->width)
        {
            // do not clip anything
        }

        // line is horizontal (clipping is trivial)
        else if (ln.delta == 0)
        {
            if (ln.left->x < 0)
            {
                ln.left->u += (ln.right->u - ln.left->u) / (ln.right->x - ln.left->x) * -ln.left->x;
                ln.left->v += (ln.right->v - ln.left->v) / (ln.right->x - ln.left->x) * -ln.left->x;
                ln.left->x = 0;
            }
            if (ln.right->x >= this->width)
            {
                ln.right->u -= (ln.right->u - ln.left->u) / (ln.right->x - ln.left->x) * (ln.right->x - this->width);
                ln.right->v -= (ln.right->v - ln.left->v) / (ln.right->x - ln.left->x) * (ln.right->x - this->width);
                ln.right->x = this->width - 1.0f;
            }
        }

        // line is vertical (clipping is trivial)
        else if (ln.dx == 0)
        {
            if (ln.top->y < 0)
                ln.top->y = 0;
            if (ln.bottom->y >= this->height)
                ln.bottom->y = this->height - 1.0f;
        }

        // line has to be clipped (it still can be outside, bounding box overlaps with frame but line itself is outside)
        else
        {
            // clip left
            if (ln.left->x < 0)
            {
                t = (0 - ln.left->x) / ln.dx; // fabsf not necessary because dx is set to be positive
                ln.left->x = 0;
                ln.left->y = ln.left->y + t * ln.dy;
                ln.dy = ln.right->y - ln.left->y;
                ln.dx = ln.right->x - ln.left->x;
            }

            // line is outside
            if (ln.bottom->y < 0 || ln.top->y >= this->height || ln.right->x < 0 || ln.left->x >= this->width)
                return;

            // clip right
            if (ln.right->x >= this->width)
            {
                t = (this->width - ln.left->x) / ln.dx; // fabsf not necessary because dx is set to be positive
                ln.right->x = this->width - 1.0f;
                ln.right->y = ln.left->y + t * ln.dy;
                ln.dy = ln.right->y - ln.left->y;
                ln.dx = ln.right->x - ln.left->x;
            }

            // line is outside
            if (ln.bottom->y < 0 || ln.top->y >= this->height || ln.right->x < 0 || ln.left->x >= this->width)
                return;

            // dont need more outside checks after that

            // clip top
            if (ln.top->y < 0)
            {
                t = fabsf((ln.left->y) / ln.dy); // (0 - top->y) / fabsf(dy);
                ln.top->y = 0;
                ln.top->x = ln.left->x + t * ln.dx;
                ln.dy = ln.right->y - ln.left->y;
                ln.dx = ln.right->x - ln.left->x;
            }

            // clip bottom
            if (ln.bottom->y >= this->height)
            {
                t = fabsf((ln.left->y - this->height) / ln.dy); // (this->height - top->y) / fabsf(dy);
                ln.bottom->y = this->height - 1.0f;
                ln.bottom->x = ln.left->x + t * ln.dx;
                ln.dy = ln.right->y - ln.left->y;
                ln.dx = ln.right->x - ln.left->x;
            }
        }
    }

    void core::drawLine(vertex2 start, vertex2 end, bool denormalize)
    {
        line ln;
        ln.start = start;
        ln.end = end;
        this->clipLine(ln, denormalize);

        // draw line by advancing x (dy/dx <= 1)
        if (fabsf(ln.delta) <= 1)
        {
            float x2 = ln.right->x;
            float curY = ln.left->y;
            float curX = ln.left->x;
            float u = ln.left->u;
            float v = ln.left->v;
            float du = (ln.right->u - ln.left->u) / (ln.right->x - ln.left->x);
            float dv = (ln.right->v - ln.left->v) / (ln.right->x - ln.left->x);

            while (curX <= x2)
            {
                int x = this->tex.sz.width * u;
                int y = this->tex.sz.height * v;

                // TODO, quick fix for rounding error
                if (y >= this->tex.sz.height)
                    y = this->tex.sz.height - 1;

                int byteIndex = (this->tex.sz.width * y + x) * BYTES_PER_PIXEL;

                // TODO, have to copy byte per byte to insert colors in correct order
                // src is rgba, frame is bgra
                // how to change order of frame ?
                byte r = this->tex.data[byteIndex];
                byte g = this->tex.data[byteIndex + 1];
                byte b = this->tex.data[byteIndex + 2];
                byte a = this->tex.data[byteIndex + 3];

                int offset = ((int)(curY) * this->width + (int)(curX)) * BYTES_PER_PIXEL;
                int depthBufferOffset = ((int)(curY)* this->width + (int)(curX)) * this->depthBufferCount;

                if (this->depthBuffer[depthBufferOffset] >= start.z)
                {
                    this->depthBuffer[depthBufferOffset] = start.z;

                    this->frameData[offset] = b; //b
                    this->frameData[offset + 1] = g; //g
                    this->frameData[offset + 2] = r; //r
                    this->frameData[offset + 3] = a; //a
                }

                curX++;
                curY += ln.delta;
                u += du;
                v += dv;
            }
        }
        // draw line by advancing y (dy/dx > 1)
        else
        {
            float y2 = ln.bottom->y;
            float curY = ln.top->y;
            float curX = ln.top->x;
            ln.delta = 1.0f / ln.delta;

            while (curY <= y2)
            {
                int offset = ((int)curY * this->width + (int)curX) * BYTES_PER_PIXEL;
                this->frameData[offset] = 0;
                this->frameData[offset + 1] = 0;
                this->frameData[offset + 2] = 0;
                this->frameData[offset + 3] = 0xff;
                curY++;
                curX += ln.delta;
            }
        }
    }

    void core::clear(color c)
    {
        for (int i = 0; i < this->height; i++)
        {
            for (int j = 0; j < this->width; j++)
            {
                int offset = (i * this->width + j) * BYTES_PER_PIXEL;
                int depthBufferOffset = (i * this->width + j) * this->depthBufferCount;
                this->frameData[offset] = c.b;
                this->frameData[offset + 1] = c.g;
                this->frameData[offset + 2] = c.r;
                this->frameData[offset + 3] = 0xff;

                this->depthBuffer[depthBufferOffset] = 1.0f;
                this->depthBuffer[depthBufferOffset + 1] = 1.0f;
            }
        }
    }

    // put data in frame buffer
    void core::blt(rectf& rect, byte* data)
    {
        int width = rect.right - rect.left;

        for (int i = rect.top; i < rect.bottom; i++)
        {
            for (int j = rect.left; j < rect.right; j++)
            {
                int offset = (i * width + j) * BYTES_PER_PIXEL;
                this->frameData[offset] = *data++;
                this->frameData[offset + 1] = *data++;
                this->frameData[offset + 2] = *data++;
                this->frameData[offset + 3] = *data++;
            }
        }
    }

    void core::drawPrimitive(topology topo, int vertexCount, vertex2* data)
    {
        if (this->mode == fillMode::WIREFRAME)
            this->drawPrimitiveWire(topo, vertexCount, data);
        else
            this->drawPrimitiveFill(topo, vertexCount, data);
    }

    void core::drawTriangleFlatTop(vertex2* p)
    {
        float x1 = p[2].x;
        float x2 = p[2].x;

        float dx1 = (p[2].x - p[0].x) / (p[2].y - p[0].y);
        float dx2 = (p[2].x - p[1].x) / (p[2].y - p[1].y);

        float u1 = p[2].u;
        float u2 = p[2].u;
        float v1 = p[2].v;
        float v2 = p[2].v;

        float du1 = (p[2].u - p[0].u) / (p[2].y - p[0].y);
        float du2 = (p[2].u - p[1].u) / (p[2].y - p[1].y);
        float dv1 = (p[2].v - p[0].v) / (p[2].y - p[0].y);
        float dv2 = (p[2].v - p[1].v) / (p[2].y - p[1].y);

        for (float y = p[2].y; y > p[0].y; y--)
        {
            this->drawLine({ x1, y, p[0].z, u1, v1 }, { x2, y, p[0].z, u2, v2 }, false);
            x1 -= dx1;
            x2 -= dx2;
            u1 -= du1;
            u2 -= du2;
            v1 -= dv1;
            v2 -= dv2;
        }
    }

    void core::drawTriangleFlatBottom(vertex2* p)
    {
        float x1 = p[0].x;
        float x2 = p[0].x;

        float dx1 = (p[0].x - p[1].x) / (p[0].y - p[1].y);
        float dx2 = (p[0].x - p[2].x) / (p[0].y - p[2].y);

        float u1 = p[0].u;
        float u2 = p[0].u;
        float v1 = p[0].v;
        float v2 = p[0].v;

        float du1 = (p[0].u - p[1].u) / (p[0].y - p[1].y);
        float du2 = (p[0].u - p[2].u) / (p[0].y - p[2].y);
        float dv1 = (p[0].v - p[1].v) / (p[0].y - p[1].y);
        float dv2 = (p[0].v - p[2].v) / (p[0].y - p[2].y);

        for (float y = p[0].y; y < p[1].y; y++)
        {
            this->drawLine({ x1, y, p[0].z, u1, v1 }, { x2, y, p[0].z, u2, v2 }, false);
            x1 += dx1;
            x2 += dx2;
            u1 += du1;
            u2 += du2;
            v1 += dv1;
            v2 += dv2;
        }
    }

    void core::drawTriangleFill(vertex2* data, bool denormalize)
    {
        // it uses first algorithm from http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html

        vertex2 p[4];
        vertex2 temp;
        p[0] = data[0];
        p[1] = data[1];
        p[2] = data[2];

        if (denormalize)
        {
            this->norm2screen(p[0]);
            this->norm2screen(p[1]);
            this->norm2screen(p[2]);
        }

        // sort top to bottom
        // actually it's sorted ascending since top is smaller y
        if (p[1].y < p[0].y)
        {
            temp = p[0];
            p[0] = p[1];
            p[1] = temp;
        }

        if (p[2].y < p[1].y)
        {
            temp = p[1];
            p[1] = p[2];
            p[2] = temp;
        }

        if (p[1].y < p[0].y)
        {
            temp = p[0];
            p[0] = p[1];
            p[1] = temp;
        }

        // case 1, triangle has horizontal edge top (top is actually smaller y )
        if (p[0].y == p[1].y)
            this->drawTriangleFlatTop(p);

        // case 2, triangle has horizontal edge bottom (bottom is larger y)
        else if (p[1].y == p[2].y)
            this->drawTriangleFlatBottom(p);

        // case 3, non of the above
        // this splits triangle into 2 where one has flat bottom and other, flat top
        else
        {
            float x4 = p[0].x + ((p[1].y - p[0].y)/(p[2].y - p[0].y)) * (p[2].x - p[0].x);
            float u4 = p[0].u + ((p[1].y - p[0].y) / (p[2].y - p[0].y)) * (p[2].u - p[0].u);
            float v4 = p[0].v + ((p[1].y - p[0].y) / (p[2].y - p[0].y)) * (p[2].v - p[0].v);
            p[3] = p[2];
            p[2] = { x4 ,p[1].y, p[0].z, u4, v4 };

            this->drawTriangleFlatBottom(p);
            this->drawTriangleFlatTop(p + 1);
        }
    }

    void core::drawPrimitiveFill(topology topo, int vertexCount, vertex2* data)
    {
        switch (topo)
        {
        case rast::topology::POINTLIST:
            break;
        case rast::topology::LINELIST:
            break;
        case rast::topology::LINESTRIP:
            break;
        case rast::topology::TRIANGLELIST:
        {
            for (int i = 0; i < vertexCount; i += 3)
                this->drawTriangleFill(data + i, true);
            break;
        }
        case rast::topology::TRIANGLESTRIP:
        {
            for (int i = 0; i < vertexCount - 2; i += 1)
                this->drawTriangleFill(data + i, true);
            break;
        }
        case rast::topology::QUADLIST:
            break;
        default:
            break;
        }
    }

    void core::drawPrimitiveWire(topology topo, int vertexCount, vertex2* data)
    {
        switch (topo)
        {
        case rast::topology::POINTLIST:
            for (int i = 0; i < vertexCount; i += 1)
            {
                /*int offset = (i * width + j) * BYTES_PER_PIXEL;
                this->frameData[offset] = *data++;
                this->frameData[offset + 1] = *data++;
                this->frameData[offset + 2] = *data++;
                this->frameData[offset + 3] = *data++;*/
            }
            break;
        case rast::topology::LINELIST:
            for (int i = 0; i < vertexCount; i += 2)
            {
                this->drawLine({ data[i].x,data[i].y }, { data[i + 1].x,data[i + 1].y });
            }
            break;
        case rast::topology::LINESTRIP:
            for (int i = 0; i < vertexCount - 1; i += 1)
            {
                this->drawLine({ data[i].x,data[i].y }, { data[i + 1].x,data[i + 1].y });
            }
            break;
        case rast::topology::TRIANGLELIST:
            for (int i = 0; i < vertexCount; i += 3)
            {
                this->drawLine({ data[i].x,data[i].y }, { data[i + 1].x,data[i + 1].y });
                this->drawLine({ data[i + 1].x,data[i + 1].y }, { data[i + 2].x,data[i + 2].y });
                this->drawLine({ data[i + 2].x,data[i + 2].y }, { data[i].x,data[i].y });
            }
            break;
        case rast::topology::TRIANGLESTRIP:
            for (int i = 0; i < vertexCount - 2; i += 1)
            {
                this->drawLine({ data[i].x,data[i].y }, { data[i + 1].x,data[i + 1].y });
                this->drawLine({ data[i + 1].x,data[i + 1].y }, { data[i + 2].x,data[i + 2].y });
                this->drawLine({ data[i + 2].x,data[i + 2].y }, { data[i].x,data[i].y });
            }
            break;
        case rast::topology::QUADLIST:
            for (int i = 0; i < vertexCount; i += 4)
            {
                this->drawLine({ data[i].x,data[i].y }, { data[i + 1].x,data[i + 1].y });
                this->drawLine({ data[i + 1].x,data[i + 1].y }, { data[i + 2].x,data[i + 2].y });
                this->drawLine({ data[i + 2].x,data[i + 2].y }, { data[i + 3].x,data[i + 3].y });
                this->drawLine({ data[i + 3].x,data[i + 3].y }, { data[i].x,data[i].y });
            }
            break;
        default:
            break;
        }
    }

    void core::test()
    {
        //srand(0);
        //for (int i = 0; i < 250; i++)
        //{
        //    float a = (rand() - 15000.0f) / 30.0f;
        //    float b = (rand() - 15000.0f) / 30.0f;
        //    float c = (rand() - 15000.0f) / 30.0f;
        //    float d = (rand() - 15000.0f) / 30.0f;
        //    //printf("v %f %f 0\n", a / 100.0f, b / 100.0f);
        //    //printf("v %f %f 0\n", c / 100.0f, d / 100.0f);

        //    drawLine({ a,b }, { c,d }, 0, 0, 0);
        //}
        //for (int i = 1; i <= 2000; i+=2)
        //    printf("l %d %d\n", i, i + 1);

        //getchar();

        /*BYTE data[100 * 100 * 4];
        for (int i = 0; i < 40000; i += 4)
        {
            data[i] = 0xff;
            data[i + 1] = 0;
            data[i + 2] = 0;
            data[i + 3] = 0xff;
        }

        blt({ 100, 100, 200, 200 }, data);*/
    }

    void core::setFillMode(fillMode mode)
    {
        this->mode = mode;
    }

    void core::setTexture(const texture& tex)
    {
        this->tex = tex;
    }

    void core::present()
    {
        test();

         SetBitmapBits(this->bmp, this->width * this->height * BYTES_PER_PIXEL, this->frameData);
         BitBlt(this->wndDc, 0, 0, this->width, this->height, this->backbufferDc, 0, 0,  SRCCOPY);
        //const BITMAPINFO bmi = { sizeof(BITMAPINFOHEADER), this->width, this->height, 1, this->width * this->height * BYTES_PER_PIXEL, BI_RGB, 0, 0, 0, 0, 0 };
        //SetDIBitsToDevice(this->wndDc, 0, 0, this->width, this->height, 0, 0, 0, this->height, this->frameData, &bmi, 0);
    }
}