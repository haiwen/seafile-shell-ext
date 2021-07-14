#include <windows.h>
#include <Guiddef.h>
#include <vector>
#include <string>

#include <shlobj.h>                 // For SHChangeNotify
#include "class-factory.h"           // For the class factory
#include "Reg.h"
#include "log.h"
#include "ext-utils.h"

// GUID for the thumbnail extension.
// {AD201912-A383-4CB1-9654-F2FC32EA0000}
const CLSID CLSID_SeadriveThumbnailProvider =
{ 0xAD201912, 0xA383, 0x4CB1, {0x96, 0x54, 0xF2, 0xFC, 0x32, 0xEA, 0x00, 0x00}};

// Customization Thumbnail file type list.
std::vector<std::wstring> imageformatlist = {L"png", L"jpeg", L"jpg"};

HINSTANCE   g_hmodThisDll     = NULL;
long        g_cDllRef   = 0;


BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        // Hold the instance of this DLL module, we will use it to get the
        // path of the DLL to register the component.
        g_hmodThisDll = hModule;
        DisableThreadLibraryCalls(hModule);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


//
//   FUNCTION: DllGetClassObject
//
//   PURPOSE: Create the class factory and query to the specific interface.
//
//   PARAMETERS:
//   * rclsid - The CLSID that will associate the correct data and code.
//   * riid - A reference to the identifier of the interface that the caller
//     is to use to communicate with the class object.
//   * ppv - The address of a pointer variable that receives the interface
//     pointer requested in riid. Upon successful return, *ppv contains the
//     requested interface pointer. If an error occurs, the interface pointer
//     is NULL.
//
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    // seaf_ext_log("DllGetClassObject called\n");
    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
    if (IsEqualCLSID(CLSID_SeadriveThumbnailProvider, rclsid))
    {
        hr = E_OUTOFMEMORY;
        ClassFactory *pClassFactory = new ClassFactory();
        if (pClassFactory)
        {
            hr = pClassFactory->QueryInterface(riid, ppv);
            pClassFactory->Release();
        }
    }

    return hr;
}


//
//   FUNCTION: DllCanUnloadNow
//
//   PURPOSE: Check if we can unload the component from the memory.
//
//   NOTE: The component can be unloaded from the memory when its reference
//   count is zero (i.e. nobody is still using the component).
//
STDAPI DllCanUnloadNow(void)
{
    // seaf_ext_log("DllCanUnloadNow called\n");
    return g_cDllRef > 0 ? S_FALSE : S_OK;
}


//
//   FUNCTION: DllRegisterServer
//
//   PURPOSE: Register the COM server and the thumbnail handler.
//
STDAPI DllRegisterServer(void)
{
    seaf_ext_log("register dll server");
    HRESULT hr;

    wchar_t szModule[MAX_PATH];
    if (GetModuleFileNameW(g_hmodThisDll, szModule, ARRAYSIZE(szModule)) == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    // Register the component.
    hr = RegisterInprocServer(szModule, CLSID_SeadriveThumbnailProvider,
        L"CppShellExtThumbnailHandler.SeadriveThumbnailProvider Class",
        L"Apartment");
    if (SUCCEEDED(hr))
    {
        // Register the thumbnail handler. The thumbnail handler is associated
        // with the .recipe file class.

        for (std::wstring imageformat : imageformatlist) {
            hr = RegisterShellExtThumbnailHandler(imageformat.c_str(), CLSID_SeadriveThumbnailProvider);
            if (FAILED(hr)) {
                seaf_ext_log("unable to register the file format is %s", seafile::utils::wStringToUtf8(imageformat.c_str()).c_str());
            }
        }

        if (SUCCEEDED(hr))
        {
            // This tells the shell to invalidate the thumbnail cache. It is
            // important because any .recipe files viewed before registering
            // this handler would otherwise show cached blank thumbnails.
            SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
        }
    }

    return hr;
}


//
//   FUNCTION: DllUnregisterServer
//
//   PURPOSE: Unregister the COM server and the thumbnail handler.
//
STDAPI DllUnregisterServer(void)
{
    seaf_ext_log("dll unregister server");
    HRESULT hr = S_OK;

    wchar_t szModule[MAX_PATH];
    if (GetModuleFileNameW(g_hmodThisDll, szModule, ARRAYSIZE(szModule)) == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    // Unregister the component.
    hr = UnregisterInprocServer(CLSID_SeadriveThumbnailProvider);
    if (SUCCEEDED(hr))
    {
        // Unregister the thumbnail handler.
        for (std::wstring imageformat : imageformatlist) {
            hr = UnregisterShellExtThumbnailHandler(imageformat.c_str());
            if (FAILED(hr)) {
                seaf_ext_log("unable to unregister the file format is %s", seafile::utils::wStringToUtf8(imageformat.c_str()).c_str());
            }
        }
    }

    return hr;
}
