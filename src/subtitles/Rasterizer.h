/*
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include <boost/smart_ptr.hpp>
#include <vector>
#include "../SubPic/ISubPic.h"
#include "xy_malloc.h"

#ifdef _OPENMP
#include <omp.h>
#endif

#define PT_MOVETONC 0xfe
#define PT_BSPLINETO 0xfc
#define PT_BSPLINEPATCHTO 0xfa

typedef struct {
    int left, top;
    int w, h;                   // width, height
    unsigned char *buffer;      // w x h buffer
} Bitmap;

struct PathData
{
public:
    PathData();
    ~PathData();

    PathData(const PathData&);//important! disable default copy constructor
    const PathData& operator=(const PathData&);

    void _TrashPath();
    bool BeginPath(HDC hdc);
    bool EndPath(HDC hdc);
    bool PartialBeginPath(HDC hdc, bool bClearPath);
    bool PartialEndPath(HDC hdc, long dx, long dy);

    BYTE* mpPathTypes;
    POINT* mpPathPoints;
    int mPathPoints;
};

typedef ::boost::shared_ptr<PathData> SharedPtrPathData;

struct Overlay
{
public:
    Overlay()
    {
        memset(this, 0, sizeof(*this));//todo: fix me. not safe.
    }
    ~Overlay()
    {
        CleanUp();       
    }

    void CleanUp()
    {
        xy_free(mpOverlayBuffer.base);
        memset(&mpOverlayBuffer, 0, sizeof(mpOverlayBuffer));        
        mOverlayWidth=mOverlayHeight=mOverlayPitch=0;
        mfWideOutlineEmpty = false;
    }

    void FillAlphaMash(byte* outputAlphaMask, bool fBody, bool fBorder, 
        int x, int y, int w, int h, 
        const byte* pAlphaMask, int pitch, DWORD color_alpha);

    struct {
        byte *base;
        byte *body;
        byte *border;
    } mpOverlayBuffer;
    int mOffsetX, mOffsetY;
        
    int mOverlayWidth, mOverlayHeight, mOverlayPitch;

    bool mfWideOutlineEmpty;//specially for blur
private:
    void _DoFillAlphaMash(byte* outputAlphaMask, const byte* pBody, const byte* pBorder,
        int x, int y, int w, int h,
        const byte* pAlphaMask, int pitch, DWORD color_alpha);
};

typedef ::boost::shared_ptr<Overlay> SharedPtrOverlay;

class Rasterizer
{
	bool fFirstSet;
	CPoint firstp, lastp;
    
private:
	int mWidth, mHeight;

	typedef std::pair<unsigned __int64, unsigned __int64> tSpan;
	typedef std::vector<tSpan> tSpanBuffer;

	tSpanBuffer mOutline;
	tSpanBuffer mWideOutline;
	int mWideBorder;

	struct Edge {
		int next;
		int posandflag;
	} *mpEdgeBuffer;
	unsigned mEdgeHeapSize;
	unsigned mEdgeNext;

	unsigned int* mpScanBuffer;

	typedef unsigned char byte;

protected:
	int mPathOffsetX, mPathOffsetY;	

private:
	void _ReallocEdgeBuffer(int edges);
	void _EvaluateBezier(const PathData& path_data, int ptbase, bool fBSpline);
	void _EvaluateLine(const PathData& path_data, int pt1idx, int pt2idx);
	void _EvaluateLine(int x0, int y0, int x1, int y1);	
	static void _OverlapRegion(tSpanBuffer& dst, tSpanBuffer& src, int dx, int dy);

public:
	Rasterizer();
	virtual ~Rasterizer();
    
	bool ScanConvert(SharedPtrPathData path_data);
	bool CreateWidenedRegion(int borderX, int borderY);
	void DeleteOutlines();
    bool Rasterize(int xsub, int ysub, SharedPtrOverlay overlay);
    static bool Blur(const Overlay& input_overlay, int fBlur, double fGaussianBlur, SharedPtrOverlay output_overlay);

    CRect Draw(SubPicDesc& spd, SharedPtrOverlay overlay, CRect& clipRect, byte* pAlphaMask, int xsub, int ysub, const DWORD* switchpts, bool fBody, bool fBorder);    
};

