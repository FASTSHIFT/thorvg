/*
 * Copyright (c) 2023 the ThorVG project. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifdef TEXMAP_INT_MASK
{
    float _dudx = dudx, _dvdx = dvdx;
    float _dxdya = dxdya, _dxdyb = dxdyb, _dudya = dudya, _dvdya = dvdya;
    float _xa = xa, _xb = xb, _ua = ua, _va = va;
    auto sbuf = image->buf32;
    int32_t sw = static_cast<int32_t>(image->stride);
    int32_t sh = image->h;
    int32_t x1, x2, ar, ab, iru, irv, px, ay;
    int32_t vv = 0, uu = 0;
    int32_t minx = INT32_MAX, maxx = INT32_MIN;
    float dx, u, v, iptr;
    auto cbuffer = surface->compositor->image.buf32;
    SwSpan* span = nullptr;         //used only when rle based.

    if (!_arrange(image, region, yStart, yEnd)) return;

    //Clear out of the Polygon vertical ranges
    auto size = surface->compositor->bbox.max.x - surface->compositor->bbox.min.x;
    if (dirFlag == 1) {     //left top case.
        for(int y = surface->compositor->bbox.min.y; y < yStart; ++y) {
            rasterRGBA32(surface->compositor->image.buf32 + y * surface->compositor->image.stride, 0, surface->compositor->bbox.min.x, size);
        }
    }
    if (dirFlag == 4) {     //right bottom case.
        for(int y = yEnd; y < surface->compositor->bbox.max.y; ++y) {
            rasterRGBA32(surface->compositor->image.buf32 + y * surface->compositor->image.stride, 0, surface->compositor->bbox.min.x, size);
        }
    }

    //Loop through all lines in the segment
    uint32_t spanIdx = 0;

    if (region) {
        minx = region->min.x;
        maxx = region->max.x;
    } else {
        span = image->rle->spans;
        while (span->y < yStart) {
            ++span;
            ++spanIdx;
        }
    }

    for (int32_t y = yStart; y < yEnd; ++y) {
        auto cmp = &cbuffer[y * surface->compositor->image.stride];
        x1 = (int32_t)_xa;
        x2 = (int32_t)_xb;

        if (!region) {
            minx = INT32_MAX;
            maxx = INT32_MIN;
            //one single row, could be consisted of multiple spans.
            while (span->y == y && spanIdx < image->rle->size) {
                if (minx > span->x) minx = span->x;
                if (maxx < span->x + span->len) maxx = span->x + span->len;
                ++span;
                ++spanIdx;
            }
        }

        if (x1 < minx) x1 = minx;
        if (x2 > maxx) x2 = maxx;

        //Anti-Aliasing frames
        //FIXME: this aa must be applied before masking op
        ay = y - aaSpans->yStart;
        if (aaSpans->lines[ay].x[0] > x1) aaSpans->lines[ay].x[0] = x1;
        if (aaSpans->lines[ay].x[1] < x2) aaSpans->lines[ay].x[1] = x2;

        //Range allowed
        if ((x2 - x1) >= 1 && (x1 < maxx) && (x2 > minx)) {
            for (int32_t x = surface->compositor->bbox.min.x; x < surface->compositor->bbox.max.x; ++x) {
                //Range allowed
                if (x >= x1 && x < x2) {                                            
                    //Perform subtexel pre-stepping on UV
                    dx = 1 - (_xa - x1);
                    u = _ua + dx * _dudx;
                    v = _va + dx * _dvdx;
                    if ((uint32_t)v >= image->h) {
                        cmp[x] = 0;
                    } else {
                        if (opacity == 255) {
                            uu = (int) u;
                            vv = (int) v;
                            ar = (int)(255 * (1 - modff(u, &iptr)));
                            ab = (int)(255 * (1 - modff(v, &iptr)));
                            iru = uu + 1;
                            irv = vv + 1;

                            if (vv >= sh) continue;
                                    
                            px = *(sbuf + (vv * sw) + uu);

                            /* horizontal interpolate */
                            if (iru < sw) {
                                /* right pixel */
                                int px2 = *(sbuf + (vv * sw) + iru);
                                px = INTERPOLATE(px, px2, ar);
                            }
                            /* vertical interpolate */
                            if (irv < sh) {
                                /* bottom pixel */
                                int px2 = *(sbuf + (irv * sw) + uu);

                                /* horizontal interpolate */
                                if (iru < sw) {
                                    /* bottom right pixel */
                                    int px3 = *(sbuf + (irv * sw) + iru);
                                    px2 = INTERPOLATE(px2, px3, ar);
                                }
                                px = INTERPOLATE(px, px2, ab);
                            }       
                            cmp[x] = ALPHA_BLEND(cmp[x], ALPHA(px));

                            //Step UV horizontally
                            u += _dudx;
                            v += _dvdx;
                        } else {
                            uu = (int) u;
                            vv = (int) v;
                            ar = (int)(255 * (1 - modff(u, &iptr)));
                            ab = (int)(255 * (1 - modff(v, &iptr)));
                            iru = uu + 1;
                            irv = vv + 1;

                            if (vv >= sh) continue;
                                    
                            px = *(sbuf + (vv * sw) + uu);

                            /* horizontal interpolate */
                            if (iru < sw) {
                                /* right pixel */
                                int px2 = *(sbuf + (vv * sw) + iru);
                                px = INTERPOLATE(px, px2, ar);
                            }
                            /* vertical interpolate */
                            if (irv < sh) {
                                /* bottom pixel */
                                int px2 = *(sbuf + (irv * sw) + uu);

                                /* horizontal interpolate */
                                if (iru < sw) {
                                    /* bottom right pixel */
                                    int px3 = *(sbuf + (irv * sw) + iru);
                                    px2 = INTERPOLATE(px2, px3, ar);
                                }
                                px = INTERPOLATE(px, px2, ab);
                            }
                            cmp[x] = ALPHA_BLEND(cmp[x], MULTIPLY(ALPHA(px), opacity));

                            //Step UV horizontally
                            u += _dudx;
                            v += _dvdx;
                        }
                    }
                } else {
                    //Clear out of polygon horizontal range
                    if (x < x1 && (dirFlag == 1 || dirFlag == 2)) cmp[x] = 0;
                    else if (x >= x2 && (dirFlag == 3 || dirFlag == 4)) cmp[x] = 0;                    
                }
            }
        }
        //Step along both edges
        _xa += _dxdya;
        _xb += _dxdyb;
        _ua += _dudya;
        _va += _dvdya;
    }
    xa = _xa;
    xb = _xb;
    ua = _ua;
    va = _va;
}
#else
{
    float _dudx = dudx, _dvdx = dvdx;
    float _dxdya = dxdya, _dxdyb = dxdyb, _dudya = dudya, _dvdya = dvdya;
    float _xa = xa, _xb = xb, _ua = ua, _va = va;
    auto sbuf = image->buf32;
    int32_t sw = static_cast<int32_t>(image->stride);
    int32_t sh = image->h;
    int32_t x1, x2, x, y, ar, ab, iru, irv, px, ay;
    int32_t vv = 0, uu = 0;
    int32_t minx = INT32_MAX, maxx = INT32_MIN;
    float dx, u, v, iptr;
    SwSpan* span = nullptr;         //used only when rle based.

    if (!_arrange(image, region, yStart, yEnd)) return;

    //Loop through all lines in the segment
    uint32_t spanIdx = 0;

    if (region) {
        minx = region->min.x;
        maxx = region->max.x;
    } else {
        span = image->rle->spans;
        while (span->y < yStart) {
            ++span;
            ++spanIdx;
        }
    }

    y = yStart;

    while (y < yEnd) {
        x1 = (int32_t)_xa;
        x2 = (int32_t)_xb;

        if (!region) {
            minx = INT32_MAX;
            maxx = INT32_MIN;
            //one single row, could be consisted of multiple spans.
            while (span->y == y && spanIdx < image->rle->size) {
                if (minx > span->x) minx = span->x;
                if (maxx < span->x + span->len) maxx = span->x + span->len;
                ++span;
                ++spanIdx;
            }
        }
        if (x1 < minx) x1 = minx;
        if (x2 > maxx) x2 = maxx;

        //Anti-Aliasing frames
        ay = y - aaSpans->yStart;
        if (aaSpans->lines[ay].x[0] > x1) aaSpans->lines[ay].x[0] = x1;
        if (aaSpans->lines[ay].x[1] < x2) aaSpans->lines[ay].x[1] = x2;

        //Range allowed
        if ((x2 - x1) >= 1 && (x1 < maxx) && (x2 > minx)) {

            //Perform subtexel pre-stepping on UV
            dx = 1 - (_xa - x1);
            u = _ua + dx * _dudx;
            v = _va + dx * _dvdx;

            x = x1;

            auto cmp = &surface->compositor->image.buf32[y * surface->compositor->image.stride + x1];

            if (opacity == 255) {
                //Draw horizontal line
                while (x++ < x2) {
                    uu = (int) u;
                    vv = (int) v;

                    ar = (int)(255 * (1 - modff(u, &iptr)));
                    ab = (int)(255 * (1 - modff(v, &iptr)));
                    iru = uu + 1;
                    irv = vv + 1;

                    if (vv >= sh) continue;

                    px = *(sbuf + (vv * sw) + uu);

                    /* horizontal interpolate */
                    if (iru < sw) {
                        /* right pixel */
                        int px2 = *(sbuf + (vv * sw) + iru);
                        px = INTERPOLATE(px, px2, ar);
                    }
                    /* vertical interpolate */
                    if (irv < sh) {
                        /* bottom pixel */
                        int px2 = *(sbuf + (irv * sw) + uu);

                        /* horizontal interpolate */
                        if (iru < sw) {
                            /* bottom right pixel */
                            int px3 = *(sbuf + (irv * sw) + iru);
                            px2 = INTERPOLATE(px2, px3, ar);
                        }
                        px = INTERPOLATE(px, px2, ab);
                    }
#ifdef TEXMAP_ADD_MASK
                    *cmp = px + ALPHA_BLEND(*cmp, IALPHA(px));
#elif defined(TEXMAP_SUB_MASK)
                    *cmp = ALPHA_BLEND(*cmp, IALPHA(px));
#elif defined(TEXMAP_DIF_MASK)
                    *cmp = ALPHA_BLEND(px, IALPHA(*cmp)) + ALPHA_BLEND(*cmp, IALPHA(px));
#endif
                    ++cmp;
    
                    //Step UV horizontally
                    u += _dudx;
                    v += _dvdx;
                    //range over?
                    if ((uint32_t)v >= image->h) break;
                }
            } else {
                //Draw horizontal line
                while (x++ < x2) {
                    uu = (int) u;
                    vv = (int) v;

                    ar = (int)(255 * (1 - modff(u, &iptr)));
                    ab = (int)(255 * (1 - modff(v, &iptr)));
                    iru = uu + 1;
                    irv = vv + 1;

                    if (vv >= sh) continue;

                    px = *(sbuf + (vv * sw) + uu);

                    /* horizontal interpolate */
                    if (iru < sw) {
                        /* right pixel */
                        int px2 = *(sbuf + (vv * sw) + iru);
                        px = INTERPOLATE(px, px2, ar);
                    }
                    /* vertical interpolate */
                    if (irv < sh) {
                        /* bottom pixel */
                        int px2 = *(sbuf + (irv * sw) + uu);

                        /* horizontal interpolate */
                        if (iru < sw) {
                            /* bottom right pixel */
                            int px3 = *(sbuf + (irv * sw) + iru);
                            px2 = INTERPOLATE(px2, px3, ar);
                        }
                        px = INTERPOLATE(px, px2, ab);
                    }
#ifdef TEXMAP_ADD_MASK
                    *cmp = INTERPOLATE(px, *cmp, opacity);
#elif defined(TEXMAP_SUB_MASK)
                    *cmp = ALPHA_BLEND(*cmp, IALPHA(ALPHA_BLEND(px, opacity)));
#elif defined(TEXMAP_DIF_MASK)
                    auto src = ALPHA_BLEND(px, opacity);
                    *cmp = ALPHA_BLEND(src, IALPHA(*cmp)) + ALPHA_BLEND(*cmp, IALPHA(src));
#endif
                    ++cmp;

                    //Step UV horizontally
                    u += _dudx;
                    v += _dvdx;
                    //range over?
                    if ((uint32_t)v >= image->h) break;
                }
            }
        }

        //Step along both edges
        _xa += _dxdya;
        _xb += _dxdyb;
        _ua += _dudya;
        _va += _dvdya;

        if (!region && spanIdx >= image->rle->size) break;

        ++y;
    }
    xa = _xa;
    xb = _xb;
    ua = _ua;
    va = _va;
}
#endif