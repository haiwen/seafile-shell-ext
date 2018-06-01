#ifndef SEAFILE_EXTENSION_APPLET_COMMANDS_H
#define SEAFILE_EXTENSION_APPLET_COMMANDS_H

#include <string>
#include <vector>

#include "log.h"
#include "repo-info.h"
#include "applet-connection.h"

namespace seafile
{

/**
 * Abstract base class for all one-way commands, e.g. don't require a response
 * from seafile/seadrive client.
 */
class SimpleAppletCommand
{
public:
    SimpleAppletCommand(std::string name) : name_(name)
    {
    }

    /**
     * send the command to seafile client, don't need the response
     */
    void send(bool is_seadrive_command)
    {
        if (is_seadrive_command) {
            AppletConnection::driveInstance()->sendCommand(formatDriveRequest());
        } else {
            AppletConnection::appletInstance()->sendCommand(formatRequest());
        }
    }

    std::string formatRequest()
    {
        std::string body = serialize();
        if (body.empty()) {
            return name_;
        } else {
            return name_ + "\t" + body;
        }
    }

    std::string formatDriveRequest()
    {
        std::string body = serializeForDrive();
        if (body.empty()) {
            return name_;
        } else {
            return name_ + "\t" + body;
        }
    }

protected:
    /**
     * Prepare this command for sending through the pipe
     */
    virtual std::string serialize() = 0;
    virtual std::string serializeForDrive()
    {
        return serialize();
    }

    virtual bool shouldSendToApplet() {
        return true;
    }

    virtual bool shouldSendToDrive() {
        return true;
    }

    std::string name_;
};


/**
 * Abstract base class for all commands that also requires response from
 * seafile/seadrive client.
 */
template <class T>
class AppletCommand : public SimpleAppletCommand
{
public:
    AppletCommand(std::string name) : SimpleAppletCommand(name)
    {
    }

    /**
     * send the command to seafile client & seadrive client, and wait for the
     * response, then merge the response
     */
    bool sendAndWait(T *resp)
    {
        std::string raw_resp;
        T appletResp;
        T driveResp;
        bool appletSuccess = false;
        bool driveSuccess = false;
        if (shouldSendToApplet()) {
            appletSuccess =
                AppletConnection::appletInstance()->sendCommandAndWait(
                    formatRequest(), &raw_resp);
            if (appletSuccess) {
            appletSuccess = parseAppletResponse(raw_resp, &appletResp);
            }
        }
        if (shouldSendToDrive()) {
            driveSuccess =
                AppletConnection::driveInstance()->sendCommandAndWait(
                    formatDriveRequest(), &raw_resp);
            if (driveSuccess) {
                driveSuccess = parseDriveResponse(raw_resp, &driveResp);
            }
        }
        if (appletSuccess && driveSuccess) {
            mergeResponse(resp, appletResp, driveResp);
            return true;
        } else if (appletSuccess) {
            *resp = appletResp;
            return true;
        } else if (driveSuccess) {
            *resp = driveResp;
            return true;
        } else {
            return false;
        }
    }

protected:
    virtual std::string serialize() = 0;
    virtual std::string serializeForDrive()
    {
        return serialize();
    }

    /**
     * Parse response from seafile applet. Commands that don't need the
     * respnse can inherit the implementation of the base class, which does
     * nothing.
     */
    virtual bool parseAppletResponse(const std::string &raw_resp, T *resp)
    {
        return false;
    }

    virtual bool parseDriveResponse(const std::string &raw_resp, T *resp)
    {
        return false;
    }

    virtual void mergeResponse(T *resp, const T &appletResp, const T &driveResp)
    {
    }

    virtual bool shouldSendToApplet() {
        return true;
    }

    virtual bool shouldSendToDrive() {
        return true;
    }
};


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

}

#endif // SEAFILE_EXTENSION_APPLET_COMMANDS_H
