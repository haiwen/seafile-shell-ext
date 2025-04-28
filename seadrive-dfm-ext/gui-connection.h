#ifndef SEADRIVEGUICONNECTION_H
#define SEADRIVEGUICONNECTION_H

#include <string>

enum SyncStatus {
    SYNC_STATUS_CLOUD,
    SYNC_STATUS_SYNCING,
    SYNC_STATUS_SYNCED,
    SYNC_STATUS_LOCKED,
    SYNC_STATUS_LOCKED_BY_ME      
};

namespace SeaDrivePlugin {

class GuiConnection {
public:
    GuiConnection();
    ~GuiConnection();
    void connectDaemon();

    bool writeRequest(const std::string& cmd);
    bool readResponse(std::string *out);
    bool sendCommand(const std::string& cmd);
    std::string formatRequest (std::string name, std::string path) const { return name + "\t" + path; }

    std::string getMountDir() const { return mount_dir_; }
    bool isConnected() const { return connected_; }

    int lockFile (const char *path);
    int unlockFile (const char *path);
    int getFileStatus (const char *path);
    int getShareLink (const char *path);
    int getInternalLink (const char *path);
    int getUploadLink (const char *path);
    int showFileHistory (const char *path);
    bool isFileCached (const char *path);
    bool isFileInRepo(const char *path);

private:
    pthread_mutex_t mutex_;
    int socket_fd_;

    std::string mount_dir_;
    std::string seadrive_dir_;
    bool connected_;
};

}

#endif // SEADRIVERPCCLIENT_H
