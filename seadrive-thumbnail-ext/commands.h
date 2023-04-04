#ifndef SEAFILE_EXTENSION_APPLET_COMMANDS_H
#define SEAFILE_EXTENSION_APPLET_COMMANDS_H

#include <string>
#include <vector>

#include "applet-connection.h"
#include "commands-base.h"

namespace seafile
{

template <class T>
class SeaDriveCommand : public AppletCommand<T>
{
public:
    SeaDriveCommand(std::string name) : AppletCommand(name)
    {
    }

protected:
    virtual bool shouldSendToApplet() {
        return false;
    }
};

class GetCachedStatusCommand : public SeaDriveCommand <bool> {

public:
    GetCachedStatusCommand(const std::string &path);

protected:
    std::string serialize();
    std::string serializeForDrive();

    bool parseDriveResponse(const std::string &raw_resp,
                            bool *status);

private:
    std::string path_;
};

class FetchThumbnailCommand : public SeaDriveCommand <std::string> {
public:
    FetchThumbnailCommand(const std::string &path, UINT cx);
protected:
    std::string serialize();
    std::string serializeForDrive();
    bool parseDriveResponse(const std::string &raw_resp, std::string *cached_thumbnail_path);

private:
    std::string path_;
    UINT cx_;
};

} // namespace seafile


#endif // SEAFILE_EXTENSION_APPLET_COMMANDS_H
