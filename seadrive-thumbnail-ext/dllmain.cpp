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

// GUID for extension appid.
// {7AF9D9DB-4805-4F04-BCA4-BA3F0BC0D7CD}
const CLSID CLSID_SeadriveExtensionAppID =
{ 0x7AF9D9DB, 0x4805, 0x4F04, {0xBC, 0xA4, 0xBA, 0x3F, 0x0B, 0xC0, 0xD7, 0xCD}};

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
    HRESULT hr = S_OK;

    wchar_t szModule[MAX_PATH];
    if (GetModuleFileName(g_hmodThisDll, szModule, ARRAYSIZE(szModule)) == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    char buf[MAX_PATH] = {'\0'};
    GetEnvironmentVariableA("SEADRIVE_EXT_DEBUG", buf, MAX_PATH);
    // When installing seadrive, InprocServer will be registered, so only when debugging, register it when registing dll.
    if (buf[0] != '\0') {
        // Register the extension appid.
        hr = RegisterShellApp(CLSID_SeadriveExtensionAppID, L"SeaDrive Shell Extensions");
        if (!SUCCEEDED(hr)) {
            return hr;
        }
        // Register the component.
        hr = RegisterInprocServer(szModule, CLSID_SeadriveThumbnailProvider,
                                  CLSID_SeadriveExtensionAppID,
                                  L"CppShellExtThumbnailHandler.SeadriveThumbnailProvider Class",
                                  L"Apartment");
        if (SUCCEEDED(hr)) {
            SHChangeNotify (SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
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
    if (GetModuleFileName(g_hmodThisDll, szModule, ARRAYSIZE(szModule)) == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    char buf[MAX_PATH] = {'\0'};
    GetEnvironmentVariableA("SEADRIVE_EXT_DEBUG", buf, MAX_PATH);
    if (buf[0] != '\0') {
        // Unregister the component.
        hr = UnregisterShellApp(CLSID_SeadriveExtensionAppID);
        if (!SUCCEEDED(hr)) {
            return hr;
        }
        hr = UnregisterInprocServer(CLSID_SeadriveThumbnailProvider);
    }

    return hr;
}
