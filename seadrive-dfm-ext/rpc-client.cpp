#include "rpc-client.h"
#include <unistd.h>
#include <pwd.h>
#include "log.h"
#include "utils.h"
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <cstring>
#include <iostream>

namespace SeaDrivePlugin {

// USING_DFMEXT_NAMESPACE

const char *kSeadriveSockName = "seadrive_ext.sock";

SeaDriveRpcClient::SeaDriveRpcClient()
    : socket_fd_(-1),
      connected_(false)
{
    pthread_rwlock_init (&mutex_, NULL);

    struct passwd *pw = getpwuid(getuid());
    std::string homePath {pw->pw_dir}; 
    mount_dir_ = homePath + "/SeaDrive";
    seadrive_dir_ = homePath + "/.seadrive";
}

SeaDriveRpcClient::~SeaDriveRpcClient()
{
    if (socket_fd_ != -1) {
        close (socket_fd_);
        socket_fd_ = -1;
    }
      connected_ = false;
}

void SeaDriveRpcClient::connectDaemon()
{
    MutexLocker lock(&mutex_);

    if (socket_fd_ != -1) {
        close(socket_fd_);
        socket_fd_ = -1;
    }

    socket_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd_ == -1) {
        perror("socket");
        seaf_ext_log ("Failed to create unix socket fd: %s\n", strerror(errno));
        connected_ = false;
        return;
    }

    std::string rpc_pipe_path = seadrive_dir_ + std::string(kSeadriveSockName);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, rpc_pipe_path.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(socket_fd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        seaf_ext_log ("Failed to connect unix socket: %s\n", strerror(errno));
        close(socket_fd_);
        socket_fd_ = -1;
        connected_ = false;
        return;
    }

    connected_ = true;
}

bool SeaDriveRpcClient::writeRequest(const std::string& cmd)
{
  uint32_t len = cmd.size();
  if (!pipe_write_n(socket_fd_, &len, sizeof(len))) {
      seaf_ext_log ("Failed to send command: %s", strerror(errno));
      return false;
  }

  if (!pipe_write_n (socket_fd_, cmd.c_str(), len)) {
      seaf_ext_log ("Failed to send command: %s", strerror(errno));
      return false;
  }

  return true;
}

bool SeaDriveRpcClient::readResponse(std::string *out)
{
  uint32_t len = 0;
  if (!pipe_read_n(socket_fd_, &len, sizeof(len))) {
      seaf_ext_log ("Failed to read response length: %s\n", strerror(errno));
      return false;
  }

  if (len == UINT32_MAX) {
      return false;
  }

  if (len == 0) {
      return true;
  }

  std::unique_ptr<char[]> buf(new char[len + 1]);
  buf.get()[len] = 0;
  if (!pipe_read_n (socket_fd_, buf.get(), len)) {
      seaf_ext_log ("Failed to read response message: %s\n", strerror (errno));
      return false;
  }

  if (out != NULL) {
      *out = buf.get();
  }

  return true;
}

int SeaDriveRpcClient::lockFile(const char *path)
{
    MutexLocker lock(&mutex_);

    if (!connected_) {
        return -1;
    }

    std::string cmd = formatRequest ("lock-file", path);
    if (!writeRequest (cmd)) {
        return -1;
    }

    return 0;
}

int SeaDriveRpcClient::unlockFile(const char *path)
{
    MutexLocker lock(&mutex_);

    if (!connected_) {
        return -1;
    }

    std::string cmd = formatRequest ("unlock-file", path);
    if (!writeRequest (cmd)) {
        return -1;
    }

    return 0;
}

int SeaDriveRpcClient::getFileLockState (const char *path)
{
    MutexLocker lock(&mutex_);

    if (!connected_) {
        return -1;
    }

    std::string cmd = formatRequest ("get-file-status", path);
    if (!writeRequest (cmd)) {
        return -1;
    }

    std::string r;
    if (!readResponse(&r)) {
        return -1;
    }

    if (r == "locked") {
        return FILE_LOCKED_BY_OTHERS;
    } else if (r == "locked_by_me") {
        return FILE_LOCKED_BY_ME_MANUAL;
    } else if (r == "locked_auto") {
        return FILE_LOCKED_BY_ME_AUTO;
    }

    return FILE_NOT_LOCKED;
}

int SeaDriveRpcClient::getShareLink (const char *path)
{
    MutexLocker lock(&mutex_);

    if (!connected_) {
        return -1;
    }

    std::string cmd = formatRequest ("get-share-link", path);
    if (!writeRequest (cmd)) {
        return -1;
    }

    return 0;
}

int SeaDriveRpcClient::getInternalLink (const char *path)
{
    MutexLocker lock(&mutex_);

    if (!connected_) {
        return -1;
    }

    std::string cmd = formatRequest ("get-internal-link", path);
    if (!writeRequest (cmd)) {
        return -1;
    }

    return 0;
}

int SeaDriveRpcClient::getUploadLink (const char *path)
{
    MutexLocker lock(&mutex_);

    if (!connected_) {
        return -1;
    }

    std::string cmd = formatRequest ("get-upload-link", path);
    if (!writeRequest (cmd)) {
        return -1;
    }

    return 0;
}

int SeaDriveRpcClient::showFileHistory (const char *path)
{
    MutexLocker lock(&mutex_);

    if (!connected_) {
        return -1;
    }

    std::string cmd = formatRequest ("show-history", path);
    if (!writeRequest (cmd)) {
        return -1;
    }

    return 0;
}

bool SeaDriveRpcClient::isFileCached (const char *path)
{
    MutexLocker lock(&mutex_);

    if (!connected_) {
        return false;
    }

    std::string cmd = formatRequest ("is-file-cached", path);
    if (!writeRequest (cmd)) {
        return false;
    }

    std::string r;
    if (!readResponse(&r)) {
        return false;
    }

    return r == "cached";
}

}
