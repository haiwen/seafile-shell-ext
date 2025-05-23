#include "ext-common.h"
#include "ext-utils.h"
#include "shell-ext.h"
#include "log.h"
#include "repo-info.h"

#include <string>

namespace utils = seafile::utils;

// "The Shell calls IShellIconOverlayIdentifier::GetOverlayInfo to request the
//  location of the handler's icon overlay. The icon overlay handler returns
//  the name of the file containing the overlay image, and its index within
//  that file. The Shell then adds the icon overlay to the system image list."

STDMETHODIMP ShellExt::GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int* pIndex, DWORD* pdwFlags)
{
    std::string dll = utils::getThisDllPath();

    std::unique_ptr<wchar_t[]> ico(utils::utf8ToWString(dll));
    int wlen = wcslen(ico.get());
    if (wlen + 1 > cchMax)
        return S_FALSE;

    wmemcpy(pwszIconFile, ico.get(), wlen + 1);

    *pdwFlags = ISIOI_ICONFILE | ISIOI_ICONINDEX;

    *pIndex = (int)status_ - 1;

    return S_OK;
}

STDMETHODIMP ShellExt::GetPriority(int *priority)
{
    /* The priority value can be 0 ~ 100, with 0 be the highest */
    *priority = seafile::N_Status - status_;
    return S_OK;
}


// "Before painting an object's icon, the Shell passes the object's name to
//  each icon overlay handler's IShellIconOverlayIdentifier::IsMemberOf
//  method. If a handler wants to have its icon overlay displayed,
//  it returns S_OK. The Shell then calls the handler's
//  IShellIconOverlayIdentifier::GetOverlayInfo method to determine which icon
//  to display."

STDMETHODIMP ShellExt::IsMemberOf(LPCWSTR path_w, DWORD attr)
{
    if (!seafile::utils::isShellExtEnabled()) {
        return S_FALSE;
    }

    std::string path = utils::wStringToUtf8(path_w);
    if (!path.size()) {
        seaf_ext_log ("convert to char failed");
        return S_FALSE;
    }

    /* If length of path is shorter than 3, it should be a drive path,
     * such as C:\ , which should not be a repo folder ; And the
     * current folder being displayed must be "My Computer". If we
     * don't return quickly, it will lag the display.
     */
    if (path.size() <= 3) {
        return S_FALSE;
    }

    std::string path_in_repo;
    seafile::RepoInfo repo;
    bool is_seadrive_category_dir = isSeaDriveCategoryDir(path);
    if (!is_seadrive_category_dir && !pathInRepo(path, &path_in_repo, &repo)) {
        return S_FALSE;
    } else if (is_seadrive_category_dir) {
        // seadrive 2x, not use icon overlay
        return S_FALSE;
    }

    if (path_in_repo.size() <= 1) {
        // it's a repo top folder
        path_in_repo = "";
    }

    // Then check the file status.
    seafile::SyncStatus status = getRepoSyncStatus(
            path, repo.repo_id, path_in_repo, attr & FILE_ATTRIBUTE_DIRECTORY);

    if (status == seafile::Paused && !path_in_repo.empty()) {
        return S_FALSE;
    }
    if (status == status_ || (status_ == seafile::Paused && status == seafile::ReadOnly)) {
        return S_OK;
    }

    return S_FALSE;
}
