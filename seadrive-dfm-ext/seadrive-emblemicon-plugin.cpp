#include "seadrive-emblemicon-plugin.h"
#include "log.h"

#include <sys/types.h>
#include <sys/xattr.h>
#include <ulimit.h>

namespace SeaDrivePlugin {

USING_DFMEXT_NAMESPACE

SeaDriveEmblemIconPlugin::SeaDriveEmblemIconPlugin(SeaDriveRpcClient *rpc_client)
    : DFMEXT::DFMExtEmblemIconPlugin()
{
    rpc_client_ = rpc_client;

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

    if (!rpc_client_->isConnected()) {
        rpc_client_->connectDaemon(); 
    }
    if (!rpc_client_->isConnected()) {
        return emblem;
    }

    // If the file is not in the mount dir, do not set the emblem.
    if (filePath.rfind(rpc_client_->getMountDir(), 0) != 0) {
        return emblem;
    }

    std::string repoPath = filePath.substr(rpc_client_->getMountDir().size());

    if (!repoPath.empty() && repoPath[0] == '/') {
        repoPath = repoPath.substr(1);
    }

    if (repoPath.empty()) {
        return emblem;
    }

    // Set a badge elemb on the file cache state, the state may be 'elemb-seadrive-done', 
    // 'elemb-seadrive-locked-by-me' or 'elemb-seadrive-locked-by-others'.
    std::string strBuffer;
    int state = rpc_client_->getFileLockState (filePath.c_str());
    if (state == FILE_LOCKED_BY_OTHERS) {
        strBuffer = "elemb-seadrive-locked-by-others";
    } else if (state == FILE_LOCKED_BY_ME_MANUAL || state == FILE_LOCKED_BY_ME_AUTO) {
        strBuffer = "elemb-seadrive-locked-by-me";
    } else {
        bool cached = rpc_client_->isFileCached (filePath.c_str());
        if (cached) {
            strBuffer = "elemb-seadrive-done";
        }
    }

    // Add a elemb icon to the bottom-left corner of the file icon.
    std::string strBuffer { state};
    if (!strBuffer.empty()) {
        std::vector<DFMExtEmblemIconLayout> layouts;
        DFMExtEmblemIconLayout iconLayout { DFMExtEmblemIconLayout::LocationType::BottomLeft, strBuffer };
        layouts.push_back(iconLayout);
        emblem.setEmblem(layouts);
    }

    g_free (state);
    return emblem;
}

}   // namespace SeaDrivePlugin
