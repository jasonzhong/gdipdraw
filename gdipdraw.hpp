#pragma once
#include "gdiplus.h"
#pragma comment(lib, "gdiplus.lib")

#include <map>
#include <GdiplusTypes.h>
#include "singleton.h"
#include "gdiplock.hpp"

namespace gdipdraw
{
    class CInitGdiPlus
    {
    public:
        CInitGdiPlus() throw() {
            m_dwToken = 0;
            m_dwLastError = S_OK;

            Gdiplus::GdiplusStartupInput input;
            Gdiplus::GdiplusStartupOutput output;
            Gdiplus::Status status = Gdiplus::GdiplusStartup(&m_dwToken, &input, &output);
        }
        ~CInitGdiPlus() throw() {
            Gdiplus::GdiplusShutdown(m_dwToken);
        }

    private:
        ULONG_PTR m_dwToken;
        DWORD m_dwLastError;
    };

    class CGdiPlusImageData
    {
    public:
        CGdiPlusImageData() {}
        ~CGdiPlusImageData() {}

        Gdiplus::Image* LoadImageFromFile( LPCTSTR lpFilePath );
        Gdiplus::Image* LoadImageFromStream( LPCTSTR lpFilePath );

        BOOL CheckFileExists( LPCTSTR lpFilePath );
        
        VOID InsertBitmapInMap( LPCTSTR lpFilePath, Gdiplus::Image* pImage );
        BOOL BitmapHasLoaded( LPCTSTR lpFilePath, Gdiplus::Image* &pImage );
        VOID ClearAllImageData();

    private:
        std::map<std::wstring, Gdiplus::Image*> m_mapBitmapFromFile;
        std::map<std::wstring, Gdiplus::Image*> m_mapBitmapFromSteam;

        CLock m_lockData;
    };

    Gdiplus::Image* CGdiPlusImageData::LoadImageFromFile( LPCTSTR lpFilePath )
    {
        Gdiplus::Image* pImage = NULL;
        
        BOOL bHasFile = FALSE;
        bHasFile = CheckFileExists(lpFilePath);
        if (bHasFile) {
            return NULL;
        }

        BOOL bHasLoaded = FALSE;
        bHasLoaded = BitmapHasLoaded(lpFilePath, pImage);
        if (bHasLoaded) {
            return pImage;
        }

        pImage = (Gdiplus::Image*)Gdiplus::Bitmap::FromFile(lpFilePath);
        if(!pImage || pImage->GetLastStatus() != Gdiplus::Ok) {
            return NULL;
        }

        InsertBitmapInMap(lpFilePath, pImage);

        return pImage;
    }

    Gdiplus::Image* CGdiPlusImageData::LoadImageFromStream( LPCTSTR lpFilePath )
    {
        Gdiplus::Image* pImage = NULL;
        HANDLE hFile = INVALID_HANDLE_VALUE;
        LARGE_INTEGER liFileSize = {0};
        HGLOBAL hGlobal = NULL;
        CHAR *pData = NULL;
        DWORD dwReadedSize = 0;
        IStream *pStream = NULL;

        BOOL bHasLoaded = FALSE;
        bHasLoaded = BitmapHasLoaded(lpFilePath, pImage);
        if (bHasLoaded) {
            return pImage;
        }

        BOOL bHasFile = FALSE;
        bHasFile = CheckFileExists(lpFilePath);
        if (!bHasFile) {
            return NULL;
        }

        hFile = ::CreateFile(lpFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); 
        if(hFile == INVALID_HANDLE_VALUE) {
            goto Exit0;
        }

        if(!::GetFileSizeEx(hFile, &liFileSize)) {
            goto Clear0;
        }

        hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, liFileSize.QuadPart);
        if (!hGlobal) {
            goto Clear0;
        };

        pData = reinterpret_cast<char*>(::GlobalLock(hGlobal));
        if (!pData) {
            ::GlobalFree(hGlobal);
            goto Clear0;
        };

        if(!::ReadFile(hFile, pData, liFileSize.QuadPart, &dwReadedSize, NULL)) {
            ::GlobalUnlock(hGlobal);
            ::GlobalFree(hGlobal);

            goto Clear0;
        }

        ::GlobalUnlock(hGlobal);

        if (::CreateStreamOnHGlobal(hGlobal, TRUE, &pStream) != S_OK) {
            ::GlobalFree(hGlobal);
            goto Clear0;
        }

        pImage = Gdiplus::Bitmap::FromStream(pStream);
        if(!pImage || pImage->GetLastStatus() != Gdiplus::Ok) {
            goto Clear0;
        }

        InsertBitmapInMap(lpFilePath, pImage);

    Clear0:
        if(hFile != INVALID_HANDLE_VALUE) {
            ::CloseHandle(hFile);
            hFile = INVALID_HANDLE_VALUE;
        } 
        if(pStream) {
            pStream->Release();
            pStream = NULL;
        }

    Exit0:
        return pImage;
    }

    VOID CGdiPlusImageData::ClearAllImageData()
    {
        CLockContainer lock(&m_lockData);
        std::map<std::wstring, Gdiplus::Image*>::iterator itMap;
        itMap = m_mapBitmapFromFile.begin();
        for ( ; itMap != m_mapBitmapFromFile.end(); ++itMap) {
            delete itMap->second;
            itMap->second = NULL;
        }
        m_mapBitmapFromFile.clear();
    }

    BOOL CGdiPlusImageData::BitmapHasLoaded( LPCTSTR lpFilePath, Gdiplus::Image* &pImage )
    {
        CLockContainer lock(&m_lockData);
        std::map<std::wstring, Gdiplus::Image*>::iterator itMap;
        itMap = m_mapBitmapFromFile.find(lpFilePath);
        if (itMap != m_mapBitmapFromFile.end()) {
            pImage = itMap->second;
            return TRUE;
        }
        return FALSE;
    }

    VOID CGdiPlusImageData::InsertBitmapInMap( LPCTSTR lpFilePath, Gdiplus::Image* pImage )
    {
        CLockContainer lock(&m_lockData);
        m_mapBitmapFromFile.insert(std::pair<std::wstring, Gdiplus::Image*>(lpFilePath, pImage));
    }

    BOOL CGdiPlusImageData::CheckFileExists( LPCTSTR lpFilePath )
    {
        if (!lpFilePath) {
            return FALSE;
        }

        if(!::PathFileExists(lpFilePath)) {
            return FALSE;
        }
        return TRUE;
    }

    class CGdiPlusDrawImage
    {
    public:
        CGdiPlusDrawImage() {
            m_nRotate = 0;
        }
        ~CGdiPlusDrawImage() {}

        BOOL DrawImageStatic(IN HDC& dc, IN Gdiplus::Image* lpImage, IN const Gdiplus::Rect& destRect);
        BOOL DrawImageRotation(IN HDC& dc, IN Gdiplus::Image* lpImage, IN const Gdiplus::Rect& destRect, IN INT nRotate = 0);
        BOOL DrawImageZoom(IN HDC& dc, IN Gdiplus::Image* lpImage, IN const Gdiplus::Rect& destRect, IN InterpolationMode nZoomMode = InterpolationModeBicubic);

    private:
        INT m_nRotate;
        CGdiPlusImageData m_imageData;
    };

    BOOL CGdiPlusDrawImage::DrawImageStatic( IN HDC& dc, IN Gdiplus::Image* lpImage, IN const Gdiplus::Rect& destRect )
    {
        Gdiplus::Graphics graphics(dc);
        Gdiplus::Image* pDrawImg = lpImage;

        if(pDrawImg == NULL || 0 == pDrawImg->GetHeight() || pDrawImg->GetLastStatus() != Gdiplus::Ok) {
            return FALSE;
        }

        Status sRet = graphics.DrawImage(pDrawImg, 
                                  destRect,
                                  0, 0,
                                  pDrawImg->GetWidth(),
                                  pDrawImg->GetHeight(),
                                  Gdiplus::UnitPixel );
        if (sRet != Ok) {
            return FALSE;
        }
        return TRUE;
    }

    BOOL CGdiPlusDrawImage::DrawImageRotation( IN HDC& dc, IN Gdiplus::Image* lpImage, IN const Gdiplus::Rect& destRect, IN INT nRotate /*= 0*/ )
    {
        Gdiplus::Graphics graphics(dc);
        Gdiplus::Image* pDrawImg = lpImage;

        if(pDrawImg == NULL || 0 == pDrawImg->GetHeight() || pDrawImg->GetLastStatus() != Gdiplus::Ok) {
            return FALSE;
        }

        INT nX = destRect.X + destRect.Width / 2;
        INT nY = destRect.Y + destRect.Height / 2;

        m_nRotate += nRotate;
        m_nRotate = m_nRotate % 360;
        graphics.TranslateTransform(nX, nY);
        graphics.RotateTransform(m_nRotate);
        graphics.TranslateTransform(-nX, -nY);

        Status sRet = graphics.DrawImage(pDrawImg, 
                                    destRect,
                                    0, 0,
                                    pDrawImg->GetWidth(),
                                    pDrawImg->GetHeight(),
                                    Gdiplus::UnitPixel );
        graphics.ResetTransform();
        if (sRet != Ok) {
            return FALSE;
        }
        return TRUE;
    }

    BOOL CGdiPlusDrawImage::DrawImageZoom( IN HDC& dc, IN Gdiplus::Image* lpImage, IN const Gdiplus::Rect& destRect, IN InterpolationMode nZoomMode /*= InterpolationModeHighQuality*/ )
    {
        Gdiplus::Graphics graphics(dc);
        Gdiplus::Image* pDrawImg = lpImage;

        if(pDrawImg == NULL || 0 == pDrawImg->GetHeight() || pDrawImg->GetLastStatus() != Gdiplus::Ok) {
            return FALSE;
        }

        graphics.SetInterpolationMode(nZoomMode);
        Status sRet = graphics.DrawImage(pDrawImg, 
            destRect,
            0, 0,
            pDrawImg->GetWidth(),
            pDrawImg->GetHeight(),
            Gdiplus::UnitPixel );
        if (sRet != Ok) {
            return FALSE;
        }
        return TRUE;
    }

    class CGdipdraw
    {
        SINGLETON_DECLARE(CGdipdraw, GetInstance)
    public:
	    CGdipdraw() {}
	    virtual ~CGdipdraw() {
            ClearAllImageData();
        }

        BOOL DrawImage(IN HDC& dc, 
                       IN LPCTSTR lpFilePath, 
                       IN const Gdiplus::Rect& destRect);

        BOOL DrawImageRotation(IN HDC& dc, 
            IN LPCTSTR lpFilePath, 
            IN const Gdiplus::Rect& destRect,
            IN INT nRotate = 0);

        BOOL DrawImageZoom(IN HDC& dc, 
            IN LPCTSTR lpFilePath, 
            IN const Gdiplus::Rect& destRect,
            IN InterpolationMode nZoomMode = InterpolationModeBicubic);

    private:
        VOID ClearAllImageData();

    private:
        CInitGdiPlus m_gdipInit;
        CGdiPlusImageData m_imageData;
        CGdiPlusDrawImage m_drawImage;
    };

    BOOL CGdipdraw::DrawImage( IN HDC& dc, IN LPCTSTR lpFilePath, IN const Gdiplus::Rect& destRect )
    {
        Gdiplus::Image* lpImage = NULL;
        lpImage = m_imageData.LoadImageFromStream(lpFilePath);
        return m_drawImage.DrawImageStatic(dc, lpImage, destRect);
    }

    VOID CGdipdraw::ClearAllImageData()
    {
        m_imageData.ClearAllImageData();
    }

    BOOL CGdipdraw::DrawImageRotation( IN HDC& dc, IN LPCTSTR lpFilePath, IN const Gdiplus::Rect& destRect, IN INT nRotate /*= 0*/ )
    {
        Gdiplus::Image* lpImage = NULL;
        lpImage = m_imageData.LoadImageFromStream(lpFilePath);
        return m_drawImage.DrawImageRotation(dc, lpImage, destRect, nRotate);
    }

    BOOL CGdipdraw::DrawImageZoom( IN HDC& dc, IN LPCTSTR lpFilePath, IN const Gdiplus::Rect& destRect, IN InterpolationMode nZoomMode /*= InterpolationModeHighQuality*/ )
    {
        Gdiplus::Image* lpImage = NULL;
        lpImage = m_imageData.LoadImageFromStream(lpFilePath);
        return m_drawImage.DrawImageZoom(dc, lpImage, destRect, nZoomMode);
    }

}