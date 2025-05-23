#include "seadrive-menu-plugin.h"

#include <sys/types.h>
#include <sys/xattr.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <cassert>
#include <iostream>

#include "log.h"

#include <dfm-extension/menu/dfmextmenu.h>
#include <dfm-extension/menu/dfmextmenuproxy.h>
#include <dfm-extension/menu/dfmextaction.h>

namespace SeaDrivePlugin {
USING_DFMEXT_NAMESPACE

SeaDriveMenuPlugin::SeaDriveMenuPlugin(GuiConnection *conn)
    : DFMEXT::DFMExtMenuPlugin()
{
    conn_ = conn;

    registerInitialize([this](DFMEXT::DFMExtMenuProxy *proxy) {
        initialize(proxy);
    });
    registerBuildNormalMenu([this](DFMExtMenu *main, const std::string &currentPath,
                                   const std::string &focusPath, const std::list<std::string> &pathList,
                                   bool onDesktop) {
        return buildNormalMenu(main, currentPath, focusPath, pathList, onDesktop);
    });
    registerBuildEmptyAreaMenu([this](DFMExtMenu *main, const std::string &currentPath, bool onDesktop) {
        return buildEmptyAreaMenu(main, currentPath, onDesktop);
    });
}

SeaDriveMenuPlugin::~SeaDriveMenuPlugin()
{
}

void SeaDriveMenuPlugin::initialize(DFMExtMenuProxy *proxy)
{
    proxy_ = proxy;
}

bool SeaDriveMenuPlugin::buildNormalMenu(DFMExtMenu *main, const std::string &currentPath,
                                   const std::string &focusPath, const std::list<std::string> &pathList,
                                   bool onDesktop)
{
    if (onDesktop) {
        return true;
    }

    if (pathList.size() != 1) {
        return true;
    }

    if (!conn_->isConnected()) {
        conn_->connectDaemon(); 
    }
    if (!conn_->isConnected()) {
        return true;
    }

    // If the file is not in the mount dir, do not set the menu.
    if (pathList.front().rfind(conn_->getMountDir(), 0) != 0) {
        return true;
    }

    // Set first-level menu.
    auto rootAction { proxy_->createAction() };
    rootAction->setText("SeaDrive");

    auto menu { proxy_->createMenu() };

    rootAction->setMenu(menu);
    // Set second-level menu.
    rootAction->registerHovered([this, pathList](DFMExtAction *action) {
        std::string path = pathList.front();
        if (path.empty()) {
            return;
        }
        if (!action->menu()->actions().empty())
            return;

        struct stat st;
        if (stat(pathList.front().c_str(), &st)!= 0) {
            seaf_ext_log ("Failed to stat path %s: %s.\n", pathList.front().c_str(), strerror(errno));
            return;
        }

        bool in_repo = conn_->isFileInRepo(path.c_str());
        if (!in_repo) {
            return;
        }

        if (S_ISDIR(st.st_mode)) {
            auto uploadLinkAct { proxy_->createAction() };
            uploadLinkAct->setText("获取上传链接");
            uploadLinkAct->registerTriggered([this, path](DFMExtAction *, bool) {
                conn_->getUploadLink (path.c_str());
            });
            action->menu()->addAction(uploadLinkAct);
            return;
        }

        int status = conn_->getFileStatus (path.c_str());
        if (status == SYNC_STATUS_LOCKED_BY_ME) {
            auto unlockFileAct { proxy_->createAction() };
            unlockFileAct->setText("解锁该文件");
            unlockFileAct->registerTriggered([this, path](DFMExtAction *, bool) {
                conn_->unlockFile (path.c_str());
            });
            action->menu()->addAction(unlockFileAct);
        } else if (status != SYNC_STATUS_LOCKED) {
            auto lockFileAct { proxy_->createAction() };
            lockFileAct->setText("锁定该文件");
            lockFileAct->registerTriggered([this, path](DFMExtAction *, bool) {
                conn_->lockFile (path.c_str());
            });
            action->menu()->addAction(lockFileAct);
        }

        auto shareLinkAct { proxy_->createAction() };
        shareLinkAct->setText("获取共享链接");
        shareLinkAct->registerTriggered([this, path](DFMExtAction *, bool) {
            conn_->getShareLink (path.c_str());
        });

        auto internalLinkAct { proxy_->createAction() };
        internalLinkAct->setText("获取内部链接");
        internalLinkAct->registerTriggered([this, path](DFMExtAction *, bool) {
            conn_->getInternalLink (path.c_str());
        });

        auto fileHistoryAct { proxy_->createAction() };
        fileHistoryAct->setText("查看文件历史");
        fileHistoryAct->registerTriggered([this, path](DFMExtAction *, bool) {
            conn_->showFileHistory (path.c_str());
        });

        action->menu()->addAction(shareLinkAct);
        action->menu()->addAction(internalLinkAct);
        action->menu()->addAction(fileHistoryAct);
    });

    main->addAction(rootAction);
    return true;
}

bool SeaDriveMenuPlugin::buildEmptyAreaMenu(DFMExtMenu *main, const std::string &currentPath, bool onDesktop)
{
    return false;
}

}   // namespace SeaDrivePlugin
