#include <stdio.h>

#include <vector>
#include <string>

#include <shlwapi.h>

#include "registry.h"
#include "log.h"

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

bool removeRegKey(HKEY root, const std::wstring& path, const std::wstring& subkey)
{
    HKEY hKey;
    LONG result = RegOpenKeyExW(root,
                                path.c_str(),
                                0L,
                                KEY_ALL_ACCESS|KEY_WOW64_64KEY,
                                &hKey);

    if (result != ERROR_SUCCESS) {
        return false;
    }

    result = SHDeleteKeyW(hKey, subkey.c_str());
    if (result != ERROR_SUCCESS) {
        return false;
    }

    return true;
}

std::string wstring2String(const std::wstring& wstr)
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
    const char *source = s.c_str();
    size_t size = mbstowcs(NULL, source, 0) + 1;
    wchar_t *buf = new wchar_t[size];
    wmemset(buf, 0, size);
    mbstowcs(buf, source, size);
    std::wstring result = buf;
    delete []buf;
    setlocale(LC_ALL, curLocale.c_str());

    return result;
}

std::vector<std::string> collectRegisteredIconExts(HKEY root, const std::wstring& path)
{
    HKEY parent_key;
    std::vector<std::string> subkey_list;

    LONG result = openKey(root, path, &parent_key, KEY_READ|KEY_WOW64_64KEY);
    if (result != ERROR_SUCCESS)
    {
        shellfix_log("open registry item error");
        return subkey_list;
    }

    DWORD subkeys_count = 0;
    DWORD subkeys_max_len = 0;
    // Query registry subkey numbers.
    result = RegQueryInfoKeyW(parent_key,
                            NULL,
                            NULL,
                            NULL,
                            &subkeys_count,
                            &subkeys_max_len,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL);

    if (result != ERROR_SUCCESS)
    {
        shellfix_log("query registry info failed");
        return subkey_list;
    }

    DWORD subkey_len = 1024;
    wchar_t subkey_name[1024] = L"";
    for (DWORD subkey_index = 0; subkey_index < subkeys_count; ++subkey_index)
    {
        RegEnumKeyExW(parent_key,
                      subkey_index,
                      subkey_name,
                      &subkey_len,
                      NULL,
                      NULL,
                      NULL,
                      NULL);
        std::string strlpname = wstring2String(subkey_name);
        subkey_list.push_back(strlpname);
        memset(subkey_name, 0, sizeof(subkey_name));
        subkey_len = 1024;
    }

    RegCloseKey(parent_key);

    return subkey_list;
}

} // anonymouse namespace

bool removeIconExts(HKEY root, const std::wstring& path)
{
    std::vector<std::string>::iterator keyiter;
    bool is_delsubkey_success = true;
    std::vector<std::string> subkey_list = collectRegisteredIconExts(root, path);

    if(subkey_list.size() == 0)
    {
        return false;
    }

    for (keyiter = subkey_list.begin(); keyiter != subkey_list.end(); keyiter++)
    {
        std::string keyname = *keyiter;
        shellfix_log("key item is %s \n", keyname.c_str());

        // EnhancedStorageShell registry canot remove so we not try to remove it
        if (!(strstr(keyname.c_str(), "Seafile") || strstr(keyname.c_str(), "OneDrive") || strstr(keyname.c_str(), "EnhancedStorageShell"))) {
            std::wstring wstr_keyname = string2Wstring(keyname);
            is_delsubkey_success = removeRegKey(root, path, wstr_keyname);
            shellfix_log("remove subkey_list %s, is successful: %s\n", (keyname).c_str(), is_delsubkey_success ? "true" : "false");

            if (!is_delsubkey_success) {
                return is_delsubkey_success;
            }
        }
    }

    return is_delsubkey_success;
}

