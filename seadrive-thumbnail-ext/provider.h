#pragma once

#include <string>
#include <memory>

#include <windows.h>
#include <thumbcache.h>     // For IThumbnailProvider
#include <wincodec.h>       // Windows Imaging Codecs
#include <GdiPlus.h>

#include "ext-utils.h"

class SeadriveThumbnailProvider :
    public IInitializeWithItem,
    public IThumbnailProvider
{
public:
    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();

    // IInitializeWithItem
    IFACEMETHODIMP Initialize(_In_ IShellItem *item, DWORD mode);

    // IThumbnailProvider
    IFACEMETHODIMP GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha);

    SeadriveThumbnailProvider();

protected:
    ~SeadriveThumbnailProvider();

private:
    bool isImage(const std::string& path);
    bool isFileCached(const std::string& path);
    HRESULT extractWithGDI(LPCWSTR pfilePath, HBITMAP *hbmap);

    // Reference count of component.
    long m_cRef;

    // Provided during initialization.
    IShellItem *_item;
    std::string current_file_;
};
