#include "ext-common.h"
#include "provider.h"
#include <Shlwapi.h>
#include <Wincrypt.h>   // For CryptStringToBinary.
#include <msxml6.h>
#include <shobjidl.h>   //For IShellItemImageFactory

#include <windows.h>
#include <algorithm>

#include "ext-utils.h"
#include "commands.h"
#include "log.h"

namespace utils = seafile::utils;
namespace gdi = Gdiplus;

extern HINSTANCE g_hmodThisDll;
extern long g_cDllRef;

SeadriveThumbnailProvider::SeadriveThumbnailProvider() : m_cRef(1)
{
    InterlockedIncrement(&g_cDllRef);
}

SeadriveThumbnailProvider::~SeadriveThumbnailProvider()
{
    InterlockedDecrement(&g_cDllRef);
}

// Query to the interface the component supported.
IFACEMETHODIMP SeadriveThumbnailProvider::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(SeadriveThumbnailProvider, IThumbnailProvider),
        QITABENT(SeadriveThumbnailProvider, IInitializeWithItem),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

// Increase the reference count for an interface on an object.
IFACEMETHODIMP_(ULONG) SeadriveThumbnailProvider::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

// Decrease the reference count for an interface on an object.
IFACEMETHODIMP_(ULONG) SeadriveThumbnailProvider::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef) {
        delete this;
    }

    return cRef;
}

IFACEMETHODIMP SeadriveThumbnailProvider::Initialize(_In_ IShellItem *item, DWORD mode)
{
    HRESULT hresult = item->QueryInterface(__uuidof(_item), reinterpret_cast<void **>(&_item));
    if (FAILED(hresult)) {
        return hresult;
    }
    LPWSTR pszName = NULL;
    hresult = _item->GetDisplayName (SIGDN_FILESYSPATH, &pszName);
    if (FAILED(hresult)) {
        return hresult;
    }
    current_file_ = utils::normalizedPath(utils::wStringToUtf8(pszName));
    if (pszName)
        CoTaskMemFree (pszName);
    return S_OK;
}

bool SeadriveThumbnailProvider::isFileCached(const std::string& path)
{
    // Get file cache status
    seafile::IsFileCachedCommand is_cached_cmd(path);

    bool cached;
    if (!is_cached_cmd.sendAndWait(&cached)) {
        seaf_ext_log("send get file cached status failed");
        return false;
    }

    return cached;
}

bool SeadriveThumbnailProvider::isImage (const std::string& path)
{
    std::string ext = path.substr(path.find_last_of('.') + 1);
    for (int i = 0; i < ext.size(); i++) {
        ext[i] = std::tolower(ext[i]);
    }
    if (!ext.compare ("png") || !ext.compare("jpeg") || !ext.compare("jpg") ||
        !ext.compare("ico") || !ext.compare("bmp") || !ext.compare("tif") ||
        !ext.compare("tiff") || !ext.compare("gif")) {
        return true;
    }

    return false;
}

// Gets a thumbnail image and alpha type. The GetThumbnail is called with the
// largest desired size of the image, in pixels. Although the parameter is
// called cx, this is used as the maximum size of both the x and y dimensions.
// If the retrieved thumbnail is not square, then the longer axis is limited
// by cx and the aspect ratio of the original image respected. On exit,
// GetThumbnail provides a handle to the retrieved image. It also provides a
// value that indicates the color format of the image and whether it has
// valid alpha information.
IFACEMETHODIMP SeadriveThumbnailProvider::GetThumbnail(UINT cx, HBITMAP *phbmp,
    WTS_ALPHATYPE *pdwAlpha)
{
    HRESULT hresult;
    // When the file has beed cached, use the default thumbnail handler.
    if (isFileCached(current_file_)) {
        IThumbnailProvider *pThumbProvider;
        hresult = _item->BindToHandler (NULL, BHID_ThumbnailHandler, IID_PPV_ARGS(&pThumbProvider));
        if (hresult != S_OK) {
            seaf_ext_log ("Failed to bind to thumbnail handler\n", );
            return hresult;
        }
        return pThumbProvider->GetThumbnail (cx, phbmp, pdwAlpha);
    } else if (!isImage (current_file_)) {
        return E_FAIL;
    } else {
        std::string png_path;
        seafile::FetchThumbnailCommand fetch_thumbnail_cmd(current_file_, cx);
        if (!fetch_thumbnail_cmd.sendAndWait(&png_path) || png_path.empty()) {
            seaf_ext_log("send get thumbnail commmand failed");
            return E_FAIL;
        }

        wchar_t *png_path_w = utils::utf8ToWString(png_path);
        hresult = extractWithGDI(png_path_w, phbmp);
        free (png_path_w);
        return hresult;
    }
}

// Helper class to init/shutdown GDI using RAII.
class GDIResource {
public:
    GDIResource() {
        gdi::GdiplusStartup(&token_, &input_, NULL);
    }

    ~GDIResource() {
        gdi::GdiplusShutdown(token_);
    }

private:
    gdi::GdiplusStartupInput input_;
    ULONG_PTR token_;
};

HRESULT SeadriveThumbnailProvider::extractWithGDI(LPCWSTR wpath, HBITMAP* hbmap)
{
    GDIResource resource;

    std::unique_ptr<gdi::Bitmap> bitmap(gdi::Bitmap::FromFile(wpath));
    if (!bitmap) {
        seaf_ext_log("failed to load bitmap from file %s", utils::wStringToUtf8(wpath).c_str());
        return E_FAIL;
    }

    gdi::Status status = bitmap->GetHBITMAP(NULL, hbmap);
    if (status != gdi::Ok) {
        return E_FAIL;
    }

    return S_OK;
}
