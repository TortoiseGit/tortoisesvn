#include "stdafx.h"
#include "D2D.h"
#include "SmartHandle.h"

CD2D* CD2D::m_pInstance = nullptr;
ID2D1Factory * CD2D::m_pDirect2dFactory = nullptr;
IDWriteFactory * CD2D::m_pWriteFactory = nullptr;;
HMODULE CD2D::m_hD2D1Lib = NULL;
HMODULE CD2D::m_hWriteLib = NULL;

CD2D::CD2D(void)
{
    CoInitialize(NULL);
    if (m_pDirect2dFactory == nullptr)
    {
        m_hD2D1Lib = LoadLibrary(L"D2D1.DLL");
        if (m_hD2D1Lib)
        {
            typedef HRESULT(STDAPICALLTYPE* FPCF)(D2D1_FACTORY_TYPE, REFIID, CONST D2D1_FACTORY_OPTIONS *, void**);
            FPCF pfn = (FPCF) GetProcAddress(m_hD2D1Lib, "D2D1CreateFactory");
            if (pfn == NULL || FAILED(pfn(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), NULL, (void**)&m_pDirect2dFactory)))
            {
                m_pDirect2dFactory = nullptr;
            }
        }
        m_hWriteLib = LoadLibrary(L"DWRITE.DLL");
        if (m_hWriteLib)
        {
            typedef HRESULT(STDAPICALLTYPE* FPCWF)(DWRITE_FACTORY_TYPE, REFIID, void**);
            FPCWF pfn = (FPCWF) GetProcAddress(m_hWriteLib, "DWriteCreateFactory");
            if (pfn == NULL || FAILED(pfn(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (void**)&m_pWriteFactory)))
            {
                m_pWriteFactory = nullptr;
            }
        }
        // note: both handles to the d2d1 and dwrite dlls must be kept open,
        // otherwise the COM objects can not be used.
    }
}


CD2D::~CD2D(void)
{
    if (m_pInstance)
    {
        if (m_pDirect2dFactory)
            m_pDirect2dFactory->Release();
        if (m_pWriteFactory)
            m_pWriteFactory->Release();
        delete m_pInstance;
        FreeLibrary(m_hD2D1Lib);
        FreeLibrary(m_hWriteLib);
        CoUninitialize();
    }
}

CD2D& CD2D::Instance()
{
    if (m_pInstance == NULL)
        m_pInstance = new CD2D();
    return *m_pInstance;
}

HRESULT CD2D::CreateHwndRenderTarget( HWND hWnd, ID2D1HwndRenderTarget** target )
{
    HRESULT hr = E_FAIL;
    (*target) = nullptr;
    if (m_pDirect2dFactory)
    {
        RECT rc;
        GetClientRect(hWnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(
            rc.right - rc.left,
            rc.bottom - rc.top
            );

        hr = m_pDirect2dFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                                                        D2D1::HwndRenderTargetProperties(hWnd, size),
                                                        target);
    }

    return hr;
}

HRESULT CD2D::CreateTextFormat( const WCHAR * fontFamilyName, DWRITE_FONT_WEIGHT fontWeight, DWRITE_FONT_STYLE fontStyle, DWRITE_FONT_STRETCH fontStretch, int fontSize, IDWriteTextFormat ** textFormat )
{
    HRESULT hr = E_FAIL;
    (*textFormat) = nullptr;
    if (m_pWriteFactory)
    {
        FLOAT fs = (fontSize/72.0f)*96.0f; // convert points to DIP
        hr = m_pWriteFactory->CreateTextFormat(fontFamilyName, NULL, fontWeight, fontStyle, fontStretch, fs, L"en-us", textFormat);
    }

    return hr;
}

HRESULT CD2D::CreateTextLayout( const WCHAR * string, UINT32 stringLength, IDWriteTextFormat * textFormat, FLOAT maxWidth, FLOAT maxHeight, IDWriteTextLayout ** textLayout )
{
    HRESULT hr = E_FAIL;
    (*textLayout) = nullptr;
    if (m_pWriteFactory)
    {
        hr = m_pWriteFactory->CreateTextLayout(string, stringLength, textFormat, maxWidth, maxHeight, textLayout);
    }

    return hr;
}

HRESULT CD2D::LoadResourceBitmap(
    ID2D1RenderTarget *pRT,
    IWICImagingFactory *pIWICFactory,
    PCWSTR resourceName,
    PCWSTR resourceType,
    __deref_out ID2D1Bitmap **ppBitmap
    )
{
    HRESULT hr = S_OK;

    CComPtr<IWICBitmapDecoder> pDecoder = NULL;
    CComPtr<IWICBitmapFrameDecode> pSource = NULL;
    CComPtr<IWICStream> pStream = NULL;
    CComPtr<IWICFormatConverter> pConverter = NULL;

    HRSRC imageResHandle = NULL;
    HGLOBAL imageResDataHandle = NULL;
    void *pImageFile = NULL;
    DWORD imageFileSize = 0;

    // Locate the resource handle in our dll
    imageResHandle = FindResourceW(
        NULL,
        resourceName,
        resourceType
        );

    hr = imageResHandle ? S_OK : E_FAIL;

    // Load the resource
    imageResDataHandle = LoadResource(
        NULL,
        imageResHandle
        );

    if (SUCCEEDED(hr))
    {
        hr = imageResDataHandle ? S_OK : E_FAIL;
    }

    // Lock it to get a system memory pointer
    pImageFile = LockResource(
        imageResDataHandle
        );

    if (SUCCEEDED(hr))
    {
        hr = pImageFile ? S_OK : E_FAIL;
    }

    // Calculate the size
    imageFileSize = SizeofResource(
        NULL,
        imageResHandle
        );

    if (SUCCEEDED(hr))
    {
        hr = imageFileSize ? S_OK : E_FAIL;
    }

    // Create a WIC stream to map onto the memory
    if (SUCCEEDED(hr))
    {
        hr = pIWICFactory->CreateStream(&pStream);
    }

    // Initialize the stream with the memory pointer and size
    if (SUCCEEDED(hr))
    {
        hr = pStream->InitializeFromMemory(
            reinterpret_cast<BYTE*>(pImageFile),
            imageFileSize
            );
    }

    // Create a decoder for the stream
    if (SUCCEEDED(hr))
    {
        hr = pIWICFactory->CreateDecoderFromStream(
            pStream,
            NULL,
            WICDecodeMetadataCacheOnLoad,
            &pDecoder
            );
    }

    // Create the initial frame
    if (SUCCEEDED(hr))
    {
        hr = pDecoder->GetFrame(
            0,
            &pSource
            );
    }

    // Format convert to 32bppPBGRA -- which Direct2D expects
    if (SUCCEEDED(hr))
    {
        hr = pIWICFactory->CreateFormatConverter(&pConverter);
    }

    if (SUCCEEDED(hr))
    {
        hr = pConverter->Initialize(
            pSource,
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.f,
            WICBitmapPaletteTypeMedianCut
            );
    }

    // Create a Direct2D bitmap from the WIC bitmap.
    if (SUCCEEDED(hr))
    {
        hr = pRT->CreateBitmapFromWicBitmap(
            pConverter,
            NULL,
            ppBitmap
            );
    }

    return hr;
}

