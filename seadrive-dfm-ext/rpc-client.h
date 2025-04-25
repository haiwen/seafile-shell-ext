#ifndef SEADRIVERPCCLIENT_H
#define SEADRIVERPCCLIENT_H

#include <string>

enum LockState {
      FILE_NOT_LOCKED = 0,
      FILE_LOCKED_BY_OTHERS,
      FILE_LOCKED_BY_ME_MANUAL,
      FILE_LOCKED_BY_ME_AUTO
};

namespace SeaDrivePlugin {

class SeaDriveRpcClient {
public:
    SeaDriveRpcClient();
    ~SeaDriveRpcClient();
    void connectDaemon();

    bool writeRequest(const std::string& cmd);
    bool readResponse(std::string *out);
    std::string formatRequest (std::string name, std::string path) const { return name + "\t" + path; }

    std::string getMountDir() const { return mount_dir_; }
    bool isConnected() const { return connected_; }

    int lockFile (const char *path);
    int unlockFile (const char *path);
    int getFileLockState (const char *path);
    int getShareLink (const char *path);
    int getInternalLink (const char *path);
    int getUploadLink (const char *path);
    int showFileHistory (const char *path);
    bool isFileCached (const char *path);

private:
    pthread_mutex_t mutex_;
    int socket_fd_;

    std::string mount_dir_;
    std::string seadrive_dir_;
    bool connected_;
};

}

#endif // SEADRIVERPCCLIENT_H
