#include <stdio.h>

#include <list>
#include <string>

#include <shlwapi.h>
#include "registry.h"

namespace {

LONG openKey(HKEY root, const std::wstring& path, HKEY *p_key, REGSAM samDesired = KEY_ALL_ACCESS)
{
    LONG result;
    result = RegOpenKeyExW(root,
                           path.c_str(),
                           0L,
                           samDesired,
                           p_key);

    return result;
}

int removeRegKey(HKEY root, const std::wstring& path, const std::wstring& subkey)
{
    HKEY hKey;
    LONG result = RegOpenKeyExW(root,
                                path.c_str(),
                                0L,
                                KEY_ALL_ACCESS|KEY_WOW64_64KEY,
                                &hKey);

    if (result != ERROR_SUCCESS) {
        return -1;
    }

    result = SHDeleteKeyW(hKey, subkey.c_str());
    if (result != ERROR_SUCCESS) {
        return -1;
    }

    return 0;
}

std::string wstring2String(const std::wstring &wstr)
{
    std::string str;
    int nLen = (int)wstr.length();
    str.resize(nLen, ' ');
    int nResult = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)wstr.c_str(), nLen, (LPSTR)str.c_str(), nLen, NULL, NULL);
    if (nResult == 0)
    {
        return "";
    }
    return str;
}

std::wstring string2Wstring(std::string s)
{
    std::string curLocale = setlocale(LC_ALL, "");
    const char* _Source = s.c_str();
    size_t _Dsize = mbstowcs(NULL, _Source, 0) + 1;
    wchar_t *_Dest = new wchar_t[_Dsize];
    wmemset(_Dest, 0, _Dsize);
    mbstowcs(_Dest,_Source,_Dsize);
    std::wstring result = _Dest;
    delete []_Dest;
    setlocale(LC_ALL, curLocale.c_str());
    return result;
}

} //namespace

bool cleanRegistryItem(HKEY root, const std::wstring& path)
{
    HKEY parent_key;
    std::list<std::string> keylist;
    LONG result = openKey(root, path, &parent_key, KEY_READ|KEY_WOW64_64KEY);
    if(result != ERROR_SUCCESS)
    {
        printf("open register item error");
        return false;
    }

    DWORD lpcSubKeys = 0;
	DWORD lpcbMaxSubKeyLen = 0;
    // ReQuery registry subkey numbers.
    result=RegQueryInfoKeyW(parent_key,
                            NULL,
                            NULL,
                            NULL,
                            &lpcSubKeys,
                            &lpcbMaxSubKeyLen,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL);

    if(result != ERROR_SUCCESS)
    {
        printf("regquery info key failed");
        return false;
    }

    DWORD lpcchName = 1024;
    wchar_t lpName[1024] = L"";
    for(DWORD dwIndex = 0; dwIndex < lpcSubKeys; ++dwIndex)
    {
        RegEnumKeyExW(parent_key,
                      dwIndex,
                      lpName,
                      &lpcchName,
                      NULL,
                      NULL,
                      NULL,
                      NULL);
        std::string strlpname = wstring2String(lpName);
        keylist.push_back(strlpname);
        memset(lpName, 0, sizeof(lpName));
        lpcchName = 1024;
    }
    RegCloseKey(parent_key);
    std::list<std::string>::iterator keyiter;

    for(keyiter = keylist.begin(); keyiter != keylist.end(); keyiter++)
    {
        std::string keyname = *keyiter;
        printf("key item is %s \n", keyname.c_str());
        if(strstr(keyname.c_str(),"Seafile") || strstr(keyname.c_str(),"OneDrive")) {

        } else {
            std::wstring wstrkeyname = string2Wstring(keyname);
            int result = removeRegKey(root, path, wstrkeyname);
            printf("remove keylist %s, result is %d \n", (keyname).c_str(), result);
        }
    }
}
