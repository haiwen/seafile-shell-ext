#pragma once

#include <string>
#include <memory>

#include <windows.h>
#include <thumbcache.h>     // For IThumbnailProvider
#include <wincodec.h>       // Windows Imaging Codecs
#include <GdiPlus.h>

#include "ext-utils.h"

class SeadriveThumbnailProvider :
    public IInitializeWithFile,
    public IThumbnailProvider
{
public:
    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();

    // IInitializeWithFile
    IFACEMETHODIMP Initialize(LPCWSTR pfilePath, DWORD grfMode);

    // IThumbnailProvider
    IFACEMETHODIMP GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha);

    SeadriveThumbnailProvider();

    static std::unique_ptr<std::string> disk_letter_cache_;
    static seafile::utils::Mutex disk_letter_cache_mutex_;

protected:
    ~SeadriveThumbnailProvider();

private:
    bool isFileCached(const std::string& path);
    bool getDiskLetter(std::string *disk_letter);
    bool isFileLocalOrCached(const std::string& path);
    HRESULT extractWithGDI(LPCWSTR pfilePath, HBITMAP *hbmap);

    // Reference count of component.
    long m_cRef;

    // Provided during initialization.
    std::string current_file_;
};
