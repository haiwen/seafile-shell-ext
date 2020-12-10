#include "ext-common.h"

#include "log.h"
#include "applet-connection.h"
#include "ext-utils.h"

#include "commands.h"

namespace seafile {

uint64_t reposInfoTimestamp = 0;

GetShareLinkCommand::GetShareLinkCommand(const std::string path)
    : SimpleAppletCommand("get-share-link"),
      path_(path)
{
}

std::string GetShareLinkCommand::serialize()
{
    return path_;
}

GetInternalLinkCommand::GetInternalLinkCommand(const std::string path)
    : SimpleAppletCommand("get-internal-link"),
      path_(path)
{
}

std::string GetInternalLinkCommand::serialize()
{
    return path_;
}

ListReposCommand::ListReposCommand()
    : AppletCommand<RepoInfoList>("list-repos")
{
}

std::string ListReposCommand::serialize()
{
    char buf[512];
    const char *version = "v1";
    snprintf (buf, sizeof(buf), "%I64u\t%s", reposInfoTimestamp, version);
    return buf;
}

std::string ListReposCommand::serializeForDrive()
{
    return "";
}

bool ListReposCommand::parseAppletResponse(const std::string& raw_resp,
                                     RepoInfoList* infos)
{
    std::vector<std::string> lines = utils::split(raw_resp, '\n');
    if (lines.empty()) {
        // seaf_ext_log("lines is empty");
        return true;
    }
    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = lines[i];
        // seaf_ext_log("lines = %s", line.c_str());
        std::vector<std::string> parts = utils::split(line, '\t');
        if (parts.size() < 6) {
            continue;
        }
        std::string repo_id, repo_name, worktree, status;
        SyncStatus st;
        bool support_file_lock;
        bool support_private_share;
        bool support_internal_link = false;

        repo_id = parts[0];
        repo_name = parts[1];
        worktree = utils::normalizedPath(parts[2]);
        status = parts[3];
        support_file_lock = parts[4] == "file-lock-supported";
        support_private_share = parts[5] == "private-share-supported";
        if (parts.size() > 6) {
            support_internal_link = parts[6] == "internal-link-supported";
        }
        if (status == "paused") {
            st = Paused;
        }
        else if (status == "syncing") {
            st = Syncing;
        }
        else if (status == "error") {
            st = Error;
        }
        else if (status == "normal") {
            st = Synced;
        }
        else {
            // impossible
            seaf_ext_log("bad repo status \"%s\"", status.c_str());
            continue;
        }
        // seaf_ext_log ("status for %s is \"%s\"", repo_name.c_str(),
        // status.c_str());
        infos->push_back(RepoInfo(repo_id, repo_name, worktree, st,
                                  support_file_lock, support_private_share,
                                  support_internal_link));
    }

    reposInfoTimestamp = utils::currentMSecsSinceEpoch();
    return true;
}

bool ListReposCommand::parseDriveResponse(const std::string& raw_resp,
                                          RepoInfoList* infos)
{
    std::vector<std::string> lines = utils::split(raw_resp, '\n');
    bool support_internal_link = false;
    if (lines.empty()) {
        return true;
    }
    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = lines[i];
        if (i == 0) {
          support_internal_link = line == "internal-link-supported";
        }
#if defined(_MSC_VER)
        else if (i == 1) {
            drive_letter_ = line;
            // seaf_ext_log ("seadrive Root: %s\n", drive_letter_.c_str());
        }
#endif
        else {
          std::string repo_dir = utils::normalizedPath(line);

          // seaf_ext_log ("repo dir: %s\n", repo_dir.c_str());
          infos->push_back(RepoInfo(repo_dir, support_internal_link));
#if !defined(_MSC_VER)
          drive_letter_ = repo_dir.substr(0, 3);
#endif
          // seaf_ext_log ("drive letter: %s\n", drive_letter_.c_str());
        }
    }
    return true;
}

void ListReposCommand::mergeResponse(RepoInfoList *resp,
                                     const RepoInfoList &appletResp,
                                     const RepoInfoList &driveResp)
{
    resp->insert(resp->end(), appletResp.begin(), appletResp.end());
    resp->insert(resp->end(), driveResp.begin(), driveResp.end());
}

GetSyncStatusCommand::GetSyncStatusCommand(const std::string& path,
                                           const std::string& repo_id,
                                           const std::string& path_in_repo,
                                           bool isdir)
    : AppletCommand<SyncStatus>("get-file-status"),
    path_(path),
    repo_id_(repo_id),
    path_in_repo_(path_in_repo),
    isdir_(isdir)
{
}

std::string GetSyncStatusCommand::serialize()
{
    char buf[512];
    snprintf (buf, sizeof(buf), "%s\t%s\t%s",
              repo_id_.c_str(), path_in_repo_.c_str(), isdir_ ? "true" : "false");
    return buf;
}

std::string GetSyncStatusCommand::serializeForDrive()
{
    return path_;
}

bool GetSyncStatusCommand::parseAppletResponse(const std::string& raw_resp,
                                               SyncStatus *status)
{
    // seaf_ext_log ("raw_resp is %s\n", raw_resp.c_str());
    if (raw_resp == "syncing") {
        *status = Syncing;
    } else if (raw_resp == "error") {
        *status = Error;
    } else if (raw_resp == "synced") {
        *status = Synced;
    } else if (raw_resp == "partial_synced") {
        *status = PartialSynced;
    } else if (raw_resp == "cloud") {
        *status = Cloud;
    } else if (raw_resp == "readonly") {
        *status = ReadOnly;
    } else if (raw_resp == "locked") {
        *status = LockedByOthers;
    } else if (raw_resp == "locked_by_me") {
        *status = LockedByMe;
    } else if (raw_resp == "paused") {
        *status = Paused;
    } else if (raw_resp == "ignored") {
        *status = NoStatus;
    } else {
        *status = NoStatus;

        // seaf_ext_log ("[GetStatusCommand] status for %s is %s, raw_resp is %s\n",
        //               path_.c_str(),
        //               seafile::toString(*status).c_str(), raw_resp.c_str());
    }

    return true;
}

bool GetSyncStatusCommand::parseDriveResponse(const std::string& raw_resp,
                                              SyncStatus *status)
{
    return parseAppletResponse(raw_resp, status);
}

void GetSyncStatusCommand::mergeResponse(SyncStatus *resp,
                                         const SyncStatus &appletResp,
                                         const SyncStatus &driveResp)
{
    *resp = appletResp != NoStatus ? appletResp : driveResp;
}

LockFileCommand::LockFileCommand(const std::string& path)
    : SimpleAppletCommand("lock-file"),
    path_(path)
{
}

std::string LockFileCommand::serialize()
{
    return path_;
}

UnlockFileCommand::UnlockFileCommand(const std::string& path)
    : SimpleAppletCommand("unlock-file"),
    path_(path)
{
}

std::string UnlockFileCommand::serialize()
{
    return path_;
}

PrivateShareCommand::PrivateShareCommand(const std::string& path, bool to_group)
    : SimpleAppletCommand(to_group ? "private-share-to-group"
                                   : "private-share-to-user"),
      path_(path)
{
}

std::string PrivateShareCommand::serialize()
{
    return path_;
}

ShowHistoryCommand::ShowHistoryCommand(const std::string& path)
    : SimpleAppletCommand("show-history"),
      path_(path)
{
}

std::string ShowHistoryCommand::serialize()
{
    return path_;
}

DownloadCommand::DownloadCommand(const std::string path)
    : SimpleAppletCommand("download"),
      path_(path)
{
}

std::string DownloadCommand::serialize()
{
    return path_;
}

ShowLockedByCommand::ShowLockedByCommand(const std::string path)
    : SimpleAppletCommand("show-locked-by"),
      path_(path)
{
}

std::string ShowLockedByCommand::serialize()
{
    return path_;
}

GetUploadLinkByCommand::GetUploadLinkByCommand(const std::string path)
    : SimpleAppletCommand("get-upload-link"),
      path_(path)
{
}

std::string GetUploadLinkByCommand::serialize()
{
    return path_;
}

} // namespace seafile
