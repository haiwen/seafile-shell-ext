#include "seadrive-emblemicon-plugin.h"
#include "log.h"

#include <sys/types.h>
#include <sys/xattr.h>
#include <ulimit.h>
#include <unistd.h>
#include <error.h>
#include <string.h>
#include <sys/stat.h>

namespace SeaDrivePlugin {

USING_DFMEXT_NAMESPACE

SeaDriveEmblemIconPlugin::SeaDriveEmblemIconPlugin(GuiConnection *conn)
    : DFMEXT::DFMExtEmblemIconPlugin()
{
    conn_ = conn;

    registerLocationEmblemIcons([this](const std::string &filePath, int systemIconCount) {
        return locationEmblemIcons(filePath, systemIconCount);
    });
}

SeaDriveEmblemIconPlugin::~SeaDriveEmblemIconPlugin()
{
}

DFMExtEmblem SeaDriveEmblemIconPlugin::locationEmblemIcons(const std::string &filePath, int systemIconCount) const
{
    DFMExtEmblem emblem;

    // A file can have a maximum of four icons. When the number of system iconshas already reached 4, additional custom icons cannot be added.
    // Furthermore, if a custom icon is placed in a position occupied by a system icon, it will not be displayed.
    if (systemIconCount >= 4)
        return emblem;

    if (!conn_->isConnected()) {
        conn_->connectDaemon(); 
    }
    if (!conn_->isConnected()) {
        return emblem;
    }

    // If the file is not in the mount dir, do not set the emblem.
    if (filePath.rfind(conn_->getMountDir(), 0) != 0) {
        return emblem;
    }

    std::string repoPath = filePath.substr(conn_->getMountDir().size());

    if (!repoPath.empty() && repoPath[0] == '/') {
        repoPath = repoPath.substr(1);
    }

    if (repoPath.empty()) {
        return emblem;
    }


    struct stat st;
    if (stat(filePath.c_str(), &st)!= 0) {
        seaf_ext_log ("Failed to stat path %s: %s.\n", filePath.c_str(), strerror(errno));
        return emblem;
    }

    if (!S_ISREG(st.st_mode)) {
        return emblem;
    }

    bool in_repo = conn_->isFileInRepo(filePath.c_str());
    if (!in_repo) {
        return emblem;
    }

    // Set a badge elemb on the file sync status, the status may be 'emblem-seadrive-done', 
    // 'emblem-seadrive-syncing', 'emblem-seadrive-locked-by-me' or 'emblem-seadrive-locked-by-others'.
    std::string strBuffer;
    int status = conn_->getFileStatus (filePath.c_str());
    if (status == SYNC_STATUS_LOCKED) {
        strBuffer = "emblem-seadrive-locked-by-others";
    } else if (status == SYNC_STATUS_LOCKED_BY_ME) {
        strBuffer = "emblem-seadrive-locked-by-me";
    } else if (status == SYNC_STATUS_SYNCING){
        strBuffer = "emblem-seadrive-syncing";
    } else if (status == SYNC_STATUS_SYNCED) {
        strBuffer = "emblem-seadrive-done";
    }

    // Add a elemb icon to the bottom-left corner of the file icon.
    if (!strBuffer.empty()) {
        std::vector<DFMExtEmblemIconLayout> layouts;
        DFMExtEmblemIconLayout iconLayout { DFMExtEmblemIconLayout::LocationType::BottomLeft, strBuffer };
        layouts.push_back(iconLayout);
        emblem.setEmblem(layouts);
    }

    return emblem;
}

}   // namespace SeaDrivePlugin
