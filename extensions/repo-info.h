#ifndef SEAFILE_REPO_INFO_H
#define SEAFILE_REPO_INFO_H

#include <string>

namespace seafile
{
class RepoInfo
{
public:
    enum Status {
        NoStatus = 0,
        Paused,
        Normal,
        Syncing,
        Error,
        LockedByMe,
        LockedByOthers,
        ReadOnly,
        N_Status,
    };

    bool is_seadrive;
    std::string worktree;

    // The following members are only available if this is an applet repo
    std::string repo_id;
    std::string repo_name;
    Status status;
    bool support_file_lock;
    bool support_private_share;

    RepoInfo() : status(NoStatus)
    {
    }

    RepoInfo(const std::string worktree)
        : is_seadrive(true),
          worktree(worktree),
          status(NoStatus),
          support_file_lock(false),
          support_private_share(false)
    {
    }

    RepoInfo(const std::string &repo_id,
             const std::string repo_name,
             const std::string &worktree,
             Status status,
             bool support_file_lock,
             bool support_private_share)
        : is_seadrive(false),
          worktree(worktree),
          repo_id(repo_id),
          repo_name(repo_name),
          status(status),
          support_file_lock(support_file_lock),
          support_private_share(support_private_share)
    {
    }

    bool isValid()
    {
        return !repo_id.empty();
    }
};

std::string toString(RepoInfo::Status st);

typedef std::vector<RepoInfo> RepoInfoList;
}

#endif // SEAFILE_REPO_INFO_H
