#ifndef SEAFILE_EXTENSION_APPLET_CONNECTION_H
#define SEAFILE_EXTENSION_APPLET_CONNECTION_H

#include <string>
#include "ext-utils.h"

namespace seafile {

/**
 * Connection to the seafile appplet, thourgh which the shell extension would
 * execute an `AppletCommand`.
 *
 * It connects to seafile appelt through a named pipe.
 */
class AppletConnection {
public:
    static AppletConnection *appletInstance();
    static AppletConnection *driveInstance();

    /**
     * Send the command in a separate thread, returns immediately
     */
    bool sendCommand(const std::string& data);

    /**
     * Send the command and blocking wait for response.
     */
    bool sendCommandAndWait(const std::string& data, std::string *resp);

private:
    AppletConnection(const wchar_t* pipe_name);
    bool connect();
    bool readResponse(std::string *out);
    bool writeRequest(const std::string& cmd);
    void onPipeError();

    /**
     * When sending request to seafile client, we would retry one
     * more time if we're sure the connection to seafile client is broken.
     * This normally happens when seafile client was restarted.
     */
    bool sendWithReconnect(const std::string& cmd);

    static AppletConnection *applet_singleton_;
    static AppletConnection *drive_singleton_;

    bool connected_;
    const wchar_t *pipe_name_;
    HANDLE pipe_;

    uint64_t last_conn_failure_;

    /**
     * We have only one connection for each explorer process, so when sending
     * a command to seafile client we need to ensure exclusive access.
     */
    utils::Mutex mutex_;
};

} // namespace seafile

#endif // SEAFILE_EXTENSION_APPLET_CONNECTION_H
