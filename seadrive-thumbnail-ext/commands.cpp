#include "ext-common.h"

#include "ext-utils.h"

#include "commands.h"

namespace seafile {

GetCachedStatusCommand::GetCachedStatusCommand(const std::string &path)
    : SeaDriveCommand<bool>("is-file-cached"),
    path_(path)
{
}

std::string GetCachedStatusCommand::serialize()
{
    return path_;
}

std::string GetCachedStatusCommand::serializeForDrive()
{
    return serialize();
}

bool GetCachedStatusCommand::parseDriveResponse(const std::string& raw_resp,
                                                bool *status)
{
    if (raw_resp == "cached") {
        *status = true;
    } else {
        *status = false;
    }
    return true;
}

/**
 * Get seadrive mount point
 */
GetSeadriveMountLetterCommand::GetSeadriveMountLetterCommand()
    : SeaDriveCommand <std::string>("get-mount-point")
{
}

std::string GetSeadriveMountLetterCommand::serialize() {
    return "";
}

std::string GetSeadriveMountLetterCommand::serializeForDrive(){
    return serialize();
}

bool GetSeadriveMountLetterCommand::parseDriveResponse(const std::string &raw_resp,
                                                std::string *letter)
{
    if (raw_resp.empty()) {
        return false;
    } else {
        *letter = raw_resp;
    }
    return true;
}


// Get thumbnail from server command
FetchThumbnailCommand::FetchThumbnailCommand(const std::string &path)
    : SeaDriveCommand <std::string>("get-thumbnail-from-server"),
     path_(path)
{
}

std::string FetchThumbnailCommand::serialize() {
    return path_;
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
