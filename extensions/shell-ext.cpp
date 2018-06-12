// Initialize GUIDs (should be done only and at-least once per DLL/EXE)
#include <initguid.h>
#include "guids.h"

#include "ext-common.h"
#include "ext-utils.h"
#include "commands.h"
#include "log.h"
#include "shell-ext.h"

namespace utils = seafile::utils;

namespace {

const int kWorktreeCacheExpireMSecs = 7 * 1000;
const int kSyncStatusCacheExpireMSecs = 3 * 1000;
const int kSyncStatusCacheMaxEntries = 10000;

} // namespace

std::unique_ptr<seafile::RepoInfoList> ShellExt::repos_cache_;
uint64_t ShellExt::cache_ts_;
seafile::utils::Mutex ShellExt::repos_cache_mutex_;

std::map<std::string, ShellExt::SyncStatusCacheEntry> ShellExt::sync_status_cache_;
seafile::utils::Mutex ShellExt::sync_status_cache_mutex_;
std::string ShellExt::drive_letter_;

// *********************** ShellExt *************************
ShellExt::ShellExt(seafile::SyncStatus status)
  : main_menu_(0),
    index_(0),
    first_(0),
    last_(0)
{
    m_cRef = 0L;
    InterlockedIncrement(&g_cRefThisDll);

    sub_menu_ = CreateMenu();
    next_active_item_ = 0;
    status_ = status;

    // INITCOMMONCONTROLSEX used = {
    //     sizeof(INITCOMMONCONTROLSEX),
    //         ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES
    // };
    // InitCommonControlsEx(&used);
}

ShellExt::~ShellExt()
{
    InterlockedDecrement(&g_cRefThisDll);
}

STDMETHODIMP ShellExt::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    if (ppv == 0)
        return E_POINTER;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown)) {
        *ppv = static_cast<LPSHELLEXTINIT>(this);
    }
    else if (IsEqualIID(riid, IID_IContextMenu)) {
        *ppv = static_cast<LPCONTEXTMENU>(this);
    }
    else if (IsEqualIID(riid, IID_IShellIconOverlayIdentifier)) {
        *ppv = static_cast<IShellIconOverlayIdentifier*>(this);
    }
    // else if (IsEqualIID(riid, IID_IContextMenu3))
    // {
    //     *ppv = static_cast<LPCONTEXTMENU3>(this);
    // }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) ShellExt::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) ShellExt::Release()
{
    if (--m_cRef)
        return m_cRef;

    delete this;

    return 0L;
}

bool ShellExt::getReposList(seafile::RepoInfoList *wts)
{
    seafile::utils::MutexLocker lock(&repos_cache_mutex_);

    uint64_t now = utils::currentMSecsSinceEpoch();
    if (repos_cache_ && now < cache_ts_ + kWorktreeCacheExpireMSecs) {
        *wts = *(repos_cache_.get());
        // seaf_ext_log("use cached repos list!");
        return true;
    }

    // no cached worktree list, send request to seafile client
    seafile::ListReposCommand cmd;
    seafile::RepoInfoList repos;
    if (!cmd.sendAndWait(&repos)) {
        // seaf_ext_log("ListReposCommand returned false!");
        return false;
    }
    drive_letter_ = cmd.driveLetter();

    cache_ts_ = utils::currentMSecsSinceEpoch();
    repos_cache_.reset(new seafile::RepoInfoList(repos));

    *wts = repos;
    return true;
}

bool ShellExt::pathInRepo(const std::string& path,
                          std::string *path_in_repo,
                          seafile::RepoInfo *repo)
{
    seafile::RepoInfoList repos;
    if (!getReposList(&repos)) {
        return false;
    }
    std::string p = utils::normalizedPath(path);

    for (size_t i = 0; i < repos.size(); i++) {
        std::string wt = repos[i].worktree;
        // seaf_ext_log ("work tree is %s, path is %s\n", wt.c_str(), p.c_str());
        if (p.size() >= wt.size() && p.substr(0, wt.size()) == wt) {
            if (p.size() > wt.size() && p[wt.size()] != '/') {
                continue;
            }
            if (path_in_repo) {
                *path_in_repo = p.substr(wt.size(), p.size() - wt.size());
            }
            if (repo) {
                *repo = repos[i];
            }
            return true;
        }
    }

    return false;
}

bool ShellExt::isRepoTopDir(const std::string& path)
{
    seafile::RepoInfoList repos;
    if (!getReposList(&repos)) {
        return false;
    }

    std::string p = utils::normalizedPath(path);
    for (size_t i = 0; i < repos.size(); i++) {
        std::string wt = repos[i].worktree;
        if (p == wt) {
            return true;
        }
    }

    return false;
}

seafile::RepoInfo ShellExt::getRepoInfoByPath(const std::string& path)
{
    seafile::RepoInfoList repos;
    if (!getReposList(&repos)) {
        return seafile::RepoInfo();
    }

    std::string p = utils::normalizedPath(path);
    for (size_t i = 0; i < repos.size(); i++) {
        std::string wt = repos[i].worktree;
        if (p == wt) {
            return repos[i];
        }
    }

    return seafile::RepoInfo();
}

seafile::SyncStatus
ShellExt::getRepoSyncStatus(const std::string& _path,
                            const std::string& repo_id,
                            const std::string& path_in_repo,
                            bool isdir)
{
    seafile::utils::MutexLocker lock(&sync_status_cache_mutex_);

    const std::string path = utils::normalizedPath(_path);
    uint64_t now = utils::currentMSecsSinceEpoch();

    auto cached = sync_status_cache_.find(path);
    if (cached != sync_status_cache_.end() && now <= (cached->second.ts + kSyncStatusCacheExpireMSecs)) {
        return cached->second.status;
    }

    // Poor man's cache eviction. The main purpose is to prevent the sync status
    // cache from using too much memory.
    if (sync_status_cache_.size() > kSyncStatusCacheMaxEntries) {
        sync_status_cache_.clear();
    }

    seafile::GetSyncStatusCommand cmd(path, repo_id, path_in_repo, isdir);
    seafile::SyncStatus status;
    if (!cmd.sendAndWait(&status)) {
        return seafile::NoStatus;
    }

    SyncStatusCacheEntry entry;
    entry.ts = utils::currentMSecsSinceEpoch();
    entry.status = status;
    sync_status_cache_[path] = entry;

    return status;
}

bool
ShellExt::isSeaDriveCategoryDir(const std::string &path)
{
    // seaf_ext_log ("1x isSeaDriveCategoryDir: path = %s, drive_letter_ = %s", path.c_str(), drive_letter_.c_str());
    seafile::RepoInfoList repos;
    getReposList(&repos);
    if (drive_letter_.empty()) {
        // seaf_ext_log ("0x isSeaDriveCategoryDir: path = %s, drive_letter_ is empty", path.c_str());
        return false;
    }
    // seaf_ext_log ("2x isSeaDriveCategoryDir: path = %s, drive_letter_ = %s", path.c_str(), drive_letter_.c_str());

    std::string p = utils::normalizedPath(path);
    return p.size() > drive_letter_.size() &&
           p.substr(0, drive_letter_.size()) == drive_letter_;
}
