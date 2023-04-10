#include "ext-common.h"

#include "ext-utils.h"

#include "commands.h"

namespace seafile {

IsFileCachedCommand::IsFileCachedCommand(const std::string &path)
    : SeaDriveCommand<bool>("is-file-cached"),
    path_(path)
{
}

std::string IsFileCachedCommand::serialize()
{
    return path_;
}

std::string IsFileCachedCommand::serializeForDrive()
{
    return serialize();
}

bool IsFileCachedCommand::parseDriveResponse(const std::string& raw_resp,
                                                bool *status)
{
    if (raw_resp == "cached") {
        *status = true;
    } else {
        *status = false;
    }
    return true;
}

// Get thumbnail from server command
FetchThumbnailCommand::FetchThumbnailCommand(const std::string &path, UINT cx)
    : SeaDriveCommand <std::string>("get-thumbnail-from-server"),
     path_(path),
     cx_(cx)
{
}

std::string FetchThumbnailCommand::serialize() {
    char buf[4096 + 64];
    snprintf (buf, sizeof(buf), "%s\t%s", path_.c_str(), std::to_string(cx_));
    return buf;
}

std::string FetchThumbnailCommand::serializeForDrive() {
    return serialize();
}

bool FetchThumbnailCommand::parseDriveResponse(const std::string &raw_resp, std::string *cached_thumbnail_path) {
    if(raw_resp.empty()) {
        return false;
    } else {
        *cached_thumbnail_path = raw_resp;
    }
    return true;
}

} // namespace seafile
