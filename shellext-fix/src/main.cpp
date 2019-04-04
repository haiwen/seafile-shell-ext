#include <string>

#include "registry.h"

namespace {
    const std::wstring kRGISTRYPATH = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers";
} //namespace

std::string shellfix_log_path = "";

int main(int argc, char *argv[])
{
    if (argc == 2) {
        shellfix_log_path = argv[1];
    }

    bool is_removed_subkey = removeIconExts(HKEY_LOCAL_MACHINE, kRGISTRYPATH);
    if (is_removed_subkey) {
        return 0;
    } else {
        return -1;
    }
}

