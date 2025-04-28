#ifndef SEAFILE_EXTENSION_APPLET_COMMANDS_H
#define SEAFILE_EXTENSION_APPLET_COMMANDS_H

#include "commands-base.h"
#include "repo-info.h"

namespace seafile
{

class GetShareLinkCommand : public SimpleAppletCommand
{
public:
    GetShareLinkCommand(const std::string path);

protected:
    std::string serialize();

private:
    std::string path_;
};

class GetInternalLinkCommand : public SimpleAppletCommand
{
public:
    GetInternalLinkCommand(const std::string path);

protected:
    std::string serialize();

private:
    std::string path_;
};


class LockFileCommand : public SimpleAppletCommand
{
public:
    LockFileCommand(const std::string &path);

protected:
    std::string serialize();

private:
    std::string path_;
};

class UnlockFileCommand : public SimpleAppletCommand
{
public:
    UnlockFileCommand(const std::string &path);

protected:
    std::string serialize();

private:
    std::string path_;
};

class PrivateShareCommand : public SimpleAppletCommand
{
public:
    PrivateShareCommand(const std::string &path, bool to_group);

protected:
    std::string serialize();

private:
    std::string path_;
    bool to_group;
};

class ShowHistoryCommand : public SimpleAppletCommand
{
public:
    ShowHistoryCommand(const std::string &path);

protected:
    std::string serialize();

private:
    std::string path_;
};


class ListReposCommand : public AppletCommand<RepoInfoList>
{
public:
    ListReposCommand();
    const std::string& driveLetter() { return drive_letter_; }

protected:
    std::string serialize();
    std::string serializeForDrive();

    bool parseAppletResponse(const std::string &raw_resp,
                             RepoInfoList *worktrees);
    bool parseDriveResponse(const std::string &raw_resp,
                            RepoInfoList *worktrees);
    void mergeResponse(RepoInfoList *resp,
                       const RepoInfoList &appletResp,
                       const RepoInfoList &driveResp);

private:
    std::string drive_letter_;
};

class GetSyncStatusCommand : public AppletCommand<SyncStatus>
{
public:
    GetSyncStatusCommand(const std::string& path,
                         const std::string &repo_id,
                         const std::string &path_in_repo,
                         bool isdir);

protected:
    std::string serialize();
    std::string serializeForDrive();
    bool shouldSendToDrive() { return repo_id_.empty(); }
    bool shouldSendToApplet() { return !shouldSendToDrive(); }

    bool parseAppletResponse(const std::string &raw_resp,
                             SyncStatus *status);
    bool parseDriveResponse(const std::string &raw_resp,
                            SyncStatus *status);
    void mergeResponse(SyncStatus *resp,
                       const SyncStatus &appletResp,
                       const SyncStatus &driveResp);

private:
    std::string path_;
    std::string repo_id_;
    std::string path_in_repo_;
    bool isdir_;
};

class DownloadCommand : public SimpleAppletCommand {
public:
    DownloadCommand(const std::string path);

protected:
    std::string serialize();

private:
    std::string path_;
};

class ShowLockedByCommand : public SimpleAppletCommand {
public:
    ShowLockedByCommand(const std::string path);

protected:
    std::string serialize();

private:
    std::string path_;
};

class GetUploadLinkByCommand : public SimpleAppletCommand {
public:
    GetUploadLinkByCommand(const std::string path);

protected:
    std::string serialize();

private:
    std::string path_;
};

}

#endif // SEAFILE_EXTENSION_APPLET_COMMANDS_H
