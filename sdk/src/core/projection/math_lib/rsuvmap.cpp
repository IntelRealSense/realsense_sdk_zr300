// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "rs_math.h"
#include <math.h>
#include <stdlib.h>


//Added
status REFCALL rsUVmapFilter_32f_C2IR( float *pSrcDst, int srcDstStep, rs::core::sizeI32 roiSize, const unsigned short *pDepth, int depthStep, unsigned short invalidDepth )
{
    pointF32 *uvTest;
    int y, x;
    for( y = 0; y < roiSize.height; y ++ )
    {
        if (pDepth != NULL)
        {
            for ( x = 0; x < roiSize.width; x++ )
            {
                uvTest = ((pointF32*)pSrcDst) + x;
                if( ((unsigned short*)pDepth)[x] > 0 && ((unsigned short*)pDepth)[x] != invalidDepth
                        && uvTest->x >= 0.f && uvTest->x < 1.f && uvTest->y >= 0.f && uvTest->y < 1.f )
                {
                    continue;
                }
                uvTest->x = uvTest->y = -1.f;
            }
            pDepth  = (const unsigned short*)((const unsigned char*)pDepth + depthStep);
        }
        else
        {
            for ( x = 0; x < roiSize.width; x++ )
            {
                uvTest = ((pointF32*)pSrcDst) + x;
                if( uvTest->x >= 0.f && uvTest->x < 1.f && uvTest->y >= 0.f && uvTest->y < 1.f )
                {
                    continue;
                }
                uvTest->x = uvTest->y = -1.f;
            }
        }
        pSrcDst = (float*)((unsigned char*)pSrcDst + srcDstStep);
    }

    return status_no_error;
}

static double MinOfArray(double *v, int n)
{
    double minV = v[0];
    int k;
    for (k=1; k<n; k++)
    {
        if (v[k] < minV) minV=v[k];
    }
    return minV;
}

static double MaxOfArray(double *v, int n)
{
    double maxV = v[0];
    int k;
    for (k=1; k<n; k++)
    {
        if (v[k] > maxV) maxV=v[k];
    }
    return maxV;
}

#define SWAP(a, b) { tmpPtr = (a); (a) = (b); (b) = tmpPtr; }

typedef struct
{
    double x;
    double y;
} point_64f;

static status rowniUVmapInvertor ( const pointF32 *uvMap, int uvMapStep, rs::core::sizeI32 uvMapSize, rect uvMapRoi,
                                   pointF32 *uvInv, int uvInvStep, rs::core::sizeI32 uvInvSize, rect uvInvRoi, int uvInvUnitsIsRelative )
{
    point_64f pValidPix[4];
    int r, c, xmin,xmax,ymin,ymax, numPix;
    double a0, b0, c0, a1, b1, c1, d1, e1, d0, e0;
    double dx0, dy0, dx1, dy1, dx2, dy2, dx3, dy3, dx4, dy4;
    double *p0, *p1, *p2, *p3, *tmpPtr;
    double widthC = (double)uvInvSize.width;
    double heightC = (double)uvInvSize.height;
    int  xmin_uvInvRoi = uvInvRoi.x;
    int  ymin_uvInvRoi = uvInvRoi.y;
    int  xmax_uvInvRoi = uvInvRoi.x + uvInvRoi.width  - 1;
    int  ymax_uvInvRoi = uvInvRoi.y + uvInvRoi.height - 1;
    int iY, iX;

    double xNorming = uvInvUnitsIsRelative?(1. / (double)uvMapSize.width):1.f;
    double yNorming = uvInvUnitsIsRelative?(1. / (double)uvMapSize.height):1.f;

    unsigned char *uvMapPtr0 = (unsigned char*)uvMap + uvMapRoi.x * sizeof(pointF32) + uvMapRoi.y * uvMapStep;
    unsigned char *uvMapPtr1 = uvMapPtr0 + uvMapStep;
    unsigned char *pUVInv    = (unsigned char*)uvInv;
    pointF32 *uvPtr0, *uvPtr1, *uvInvPtr;
    // Since RGB has smaller FOV than depth, we can sweep through a subset of the depth pixels
    for ( r=0; r < uvMapRoi.height-1; r++ )
    {
        uvPtr0 = (pointF32*)uvMapPtr0;
        uvPtr1 = (pointF32*)uvMapPtr1;
        for ( c=0; c < uvMapRoi.width-1; c++ )
        {
            // Get the number of valid pixels (around (c,r))
            const double posC = (uvMapRoi.x + c + 0.5f) * xNorming;
            const double posR = (uvMapRoi.y + r + 0.5f) * yNorming;

            numPix = 0;
            if( uvPtr0[c].x >= 0.f )
            {
                pValidPix[numPix].x = uvPtr0[c].x;
                pValidPix[numPix].y = uvPtr0[c].y;
                numPix ++;
            }
            if( uvPtr0[c+1].x >= 0.f )
            {
                pValidPix[numPix].x = uvPtr0[c+1].x;
                pValidPix[numPix].y = uvPtr0[c+1].y;
                numPix ++;
            }
            if (numPix == 0)
                continue;

            if( uvPtr1[c].x >= 0.f )
            {
                pValidPix[numPix].x = uvPtr1[c].x;
                pValidPix[numPix].y = uvPtr1[c].y;
                numPix ++;
            }
            if( uvPtr1[c+1].x >= 0.f )
            {
                pValidPix[numPix].x = uvPtr1[c+1].x;
                pValidPix[numPix].y = uvPtr1[c+1].y;
                numPix ++;
            }

            if (numPix < 3)
                continue;

            pValidPix[0].x *= widthC; pValidPix[0].y *= heightC;
            pValidPix[1].x *= widthC; pValidPix[1].y *= heightC;
            pValidPix[2].x *= widthC; pValidPix[2].y *= heightC;
            p0=(double*)&pValidPix[0];
            p1=(double*)&pValidPix[1];
            p2=(double*)&pValidPix[2];

            if ( numPix == 4 )
            {
                // Use pointers instead of datacopy
                double v_x[4];
                double v_y[4];

                pValidPix[3].x *= widthC; pValidPix[3].y *= heightC;
                p3=(double*)&pValidPix[3];

                v_x[0] = p0[0];
                v_x[1] = p1[0];
                v_x[2] = p2[0];
                v_x[3] = p3[0];

                xmin = (int) ceil(MinOfArray(v_x, 4));
                xmax = (int)MaxOfArray(v_x, 4);
                if (xmin < xmin_uvInvRoi) xmin = xmin_uvInvRoi;
                //if (xmax < xmin_uvInvRoi) continue;
                if (xmax > xmax_uvInvRoi) xmax = xmax_uvInvRoi;
                //if (xmin > xmax_uvInvRoi) continue;

                v_y[0] = p0[1];
                v_y[1] = p1[1];
                v_y[2] = p2[1];
                v_y[3] = p3[1];

                ymin = (int)ceil(MinOfArray(v_y, 4));
                ymax = (int)MaxOfArray(v_y, 4);
                if (ymin < ymin_uvInvRoi) ymin = ymin_uvInvRoi;
                //if (ymax < ymin_uvInvRoi) continue;
                if (ymax > ymax_uvInvRoi) ymax = ymax_uvInvRoi;
                //if (ymin > ymax_uvInvRoi) continue;

                // TriangleInteriorTest
                if (p0[0] > p1[0]) SWAP( p0, p1 );
                if (p0[1] > p2[1]) SWAP( p0, p2 );
                if (p2[0] > p3[0]) SWAP( p2, p3 );
                if (p1[1] > p3[1]) SWAP( p1, p3 );

                dx0 = p1[0] - p0[0];
                dy0 = p1[1] - p0[1];
                dx1 = p2[0] - p1[0];
                dy1 = p2[1] - p1[1];
                dx2 = p0[0] - p2[0];
                dy2 = p0[1] - p2[1];
                dx3 = p3[0] - p2[0];
                dy3 = p3[1] - p2[1];
                dx4 = p1[0] - p3[0];
                dy4 = p1[1] - p3[1];

                a0 = dy0 * (p0[0] - xmin + 1) - dx0 * (p0[1] - ymin);
                b0 = dy1 * (p1[0] - xmin + 1) - dx1 * (p1[1] - ymin);
                c0 = dy2 * (p2[0] - xmin + 1) - dx2 * (p2[1] - ymin);
                d0 = dy3 * (p2[0] - xmin + 1) - dx3 * (p2[1] - ymin);
                e0 = dy4 * (p3[0] - xmin + 1) - dx4 * (p3[1] - ymin);

                pUVInv = ((unsigned char*)uvInv + ymin * uvInvStep);
                for (iY = ymin ; iY <= ymax ; ++iY)
                {
                    a1 = a0;
                    b1 = b0;
                    c1 = c0;
                    d1 = d0;
                    e1 = e0;

                    uvInvPtr = (pointF32*)pUVInv;
                    for (iX=xmin ; iX <= xmax ; ++iX)
                    {
                        a1 -= dy0;
                        b1 -= dy1;
                        c1 -= dy2;
                        d1 -= dy3;
                        e1 -= dy4;
                        if (uvInvPtr[iX].x==-1)
                        {
                            if( b1 >= 0 )
                            {
                                if( a1 >= 0 )
                                {
                                    if( c1 >= 0 )
                                    {
                                        uvInvPtr[iX].x = (float)posC; uvInvPtr[iX].y = (float)posR;

                                        if (iX == 34 && iY == 167)
                                        {
                                            int h = 0;
                                        }

                                        continue;
                                    }
                                }
                                if( d1 >= 0 )
                                {
                                    if( e1 >= 0 )
                                    {
                                        uvInvPtr[iX].x = (float)posC; uvInvPtr[iX].y = (float)posR;

                                        if (iX == 34 && iY == 167)
                                        {
                                            int h = 0;
                                        }

                                        continue;
                                    }
                                }
                            }
                            else
                            {
                                if( a1 < 0 )
                                {
                                    if( c1 < 0 )
                                    {
                                        uvInvPtr[iX].x = (float)posC; uvInvPtr[iX].y = (float)posR;

                                        if (iX == 34 && iY == 167)
                                        {
                                            int h = 0;
                                        }

                                        continue;
                                    }
                                }
                                if( d1 < 0 )
                                {
                                    if( e1 < 0 )
                                    {
                                        uvInvPtr[iX].x = (float)posC; uvInvPtr[iX].y = (float)posR;

                                        if (iX == 34 && iY == 167)
                                        {
                                            int h = 0;
                                        }

                                        continue;
                                    }
                                }
                            }
                        }
                    }
                    a0 += dx0;
                    b0 += dx1;
                    c0 += dx2;
                    d0 += dx3;
                    e0 += dx4;
                    pUVInv += uvInvStep;
                }
            }
            else if ( numPix == 3 )
            {
                // only 3 elements detected
                double v_x[] = {p0[0], p1[0], p2[0]};
                double v_y[] = {p0[1], p1[1], p2[1]};
                xmin = (int) ceil(MinOfArray(v_x, 3));
                xmax = (int)MaxOfArray(v_x, 3);
                if (xmin < xmin_uvInvRoi) xmin = xmin_uvInvRoi;
                //if (xmax < xmin_uvInvRoi) continue;
                if (xmax > xmax_uvInvRoi) xmax = xmax_uvInvRoi;
                //if (xmin > xmax_uvInvRoi) continue;

                ymin = (int)ceil(MinOfArray(v_y, 3));
                ymax = (int)MaxOfArray(v_y, 3);
                if (ymin < ymin_uvInvRoi) ymin = ymin_uvInvRoi;
                //if (ymax < ymin_uvInvRoi) continue;
                if (ymax > ymax_uvInvRoi) ymax = ymax_uvInvRoi;
                //if (ymin > ymax_uvInvRoi) continue;

                // TriangleInteriorTest
                dx0 = p1[0] - p0[0];
                dy0 = p1[1] - p0[1];
                dx1 = p2[0] - p1[0];
                dy1 = p2[1] - p1[1];
                dx2 = p0[0] - p2[0];
                dy2 = p0[1] - p2[1];

                a0 = dy0 * (p0[0] - xmin + 1) - dx0 * (p0[1] - ymin);
                b0 = dy1 * (p1[0] - xmin + 1) - dx1 * (p1[1] - ymin);
                c0 = dy2 * (p2[0] - xmin + 1) - dx2 * (p2[1] - ymin);

                pUVInv = ((unsigned char*)uvInv + ymin * uvInvStep);
                for (iY = ymin ; iY <= ymax ; ++iY)
                {
                    a1 = a0;
                    b1 = b0;
                    c1 = c0;

                    uvInvPtr = (pointF32*)pUVInv;
                    for (iX=xmin ; iX <= xmax ; ++iX)
                    {
                        a1 -= dy0;
                        b1 -= dy1;
                        c1 -= dy2;
                        if (uvInvPtr[iX].x==-1)
                        {
                            if( a1 >= 0 )
                            {
                                if( b1 >= 0 )
                                {
                                    if( c1 >= 0 )
                                    {
                                        uvInvPtr[iX].x = (float)posC; uvInvPtr[iX].y = (float)posR;

                                        if (iX == 34 && iY == 167)
                                        {
                                            int h = 0;
                                        }

                                        continue;
                                    }
                                }
                            }
                            else
                            {
                                if( b1 < 0 )
                                {
                                    if( c1 < 0 )
                                    {
                                        uvInvPtr[iX].x = (float)posC; uvInvPtr[iX].y = (float)posR;

                                        if (iX == 34 && iY == 167)
                                        {
                                            int h = 0;
                                        }

                                        continue;
                                    }
                                }
                            }
                        }
                    }
                    a0 += dx0;
                    b0 += dx1;
                    c0 += dx2;
                    pUVInv += uvInvStep;
                }


            } // endif numPix
        }
        uvMapPtr0 += uvMapStep;
        uvMapPtr1 += uvMapStep;
    }

    return status_no_error;
}

//Added
status REFCALL rsUVmapInvertor_32f_C2R( const float *pSrc, int srcStep, rs::core::sizeI32 srcSize, rect srcRoi, float *pDst, int dstStep,
                                        rs::core::sizeI32 dstSize, int unitsIsRelative )
{
    rect uvInvRoi = {0, 0, dstSize.width, dstSize.height};
    int i, j;
    float *dst = pDst;

    for (i = 0; i < dstSize.height; ++i)
    {
        for (j = 0; j < dstSize.width * 2; ++j)
        {
            dst[j] = -1.f;
        }
        dst = (float*)((unsigned char*)dst + dstStep);
    }

    return rowniUVmapInvertor ( (pointF32*)pSrc, srcStep, srcSize, srcRoi, (pointF32*)pDst, dstStep, dstSize, uvInvRoi, unitsIsRelative );
}

status REFCALL rsDepthToRGB_16u8u_C1C4R(unsigned short* pSrc, int srcStep, unsigned char* pDst, int dstStep, rs::core::sizeI32 roiSize, unsigned short invalidDepth, unsigned char alpha)
{
    unsigned short* pDepth = (unsigned short*)pSrc;
    float mean = 0.f;
    double disp = 0.;
    int x, y, num_depth_points = 0;
    unsigned char* pRGB = pDst;

    for ( y = 0; y < roiSize.height; y++)
    {
        for ( x = 0; x < roiSize.width; x++)
        {
            float d = (pDepth[x]);
            if (d > 0 && d != invalidDepth)
            {
                num_depth_points++;
                mean += d;
                disp += d * d;
            }
        }
        pDepth = (unsigned short*)((char*)pDepth + srcStep);
    }
    if (num_depth_points)
    {
        mean /= num_depth_points;
        disp = sqrtf( fabs( (float)(disp / num_depth_points - (double)mean * (double)mean) ) );
        if (disp != 0) disp = 128./disp; else disp = 1e30;
    }

    pDepth = (unsigned short*)pSrc;

    for ( y = 0; y < roiSize.height; y++)
    {
        for ( x = 0; x < roiSize.width; x++)
        {
            int hist = 0;
            float d = (pDepth[x]);
            if ( 0 < d && d != invalidDepth)
            {
                hist = (int)(128.5f + disp * ( mean - d ));
                //hist = 255 - log10((float)d)/log10f(1.01);
                hist = MAX(hist, 0);
                hist = MIN(hist, 255);
            }

            pRGB[4*x + 0] = hist;
            pRGB[4*x + 1] = hist;
            pRGB[4*x + 2] = hist;
            pRGB[4*x + 3] = alpha;
        }

        pDepth = (unsigned short*)((char*)pDepth + srcStep);
        pRGB += dstStep;
    }

    return status_no_error;
}
