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

std::unique_ptr<std::string> SeadriveThumbnailProvider::disk_letter_cache_;
utils::Mutex SeadriveThumbnailProvider::disk_letter_cache_mutex_;

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
    return S_OK;
}

bool SeadriveThumbnailProvider::isFileCached(const std::string& path)
{
    // Get file cache status
    seafile::GetCachedStatusCommand cache_status_cmd(path);

    bool cached;
    if (!cache_status_cmd.sendAndWait(&cached)) {
        seaf_ext_log("send get file cached status failed");
        return false;
    }

    seaf_ext_log("the file [%s] cached = %s", path.c_str(), cached ? "yes" : "no");
    return cached;
}

bool SeadriveThumbnailProvider::isImage (const std::string& path)
{
    std::string ext = path.substr(path.find_last_of('.') + 1);
    for (int i = 0; i < ext.size(); i++) {
        ext[i] = std::tolower(ext[i]);
    }
    if (!ext.compare ("png") || !ext.compare("jpeg") || !ext.compare("jpg")) {
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
    if (!isImage (current_file_)) {
        return E_FAIL;
    } else if (isFileCached(current_file_)) {
        return extractWithGDI(utils::utf8ToWString(current_file_), phbmp);
    } else {
        seaf_ext_log("file %s is not cached", current_file_.c_str());
        std::string png_path;
        seafile::FetchThumbnailCommand fetch_thumbnail_cmd(current_file_);
        if (!fetch_thumbnail_cmd.sendAndWait(&png_path) || png_path.empty()) {
            seaf_ext_log("send get thumbnail commmand failed");
            return E_FAIL;
        }

        return extractWithGDI(utils::utf8ToWString(png_path), phbmp);
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
    seaf_ext_log("extracting with GDI: %s", utils::wStringToUtf8(wpath).c_str());
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
