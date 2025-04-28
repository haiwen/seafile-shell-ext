#include <dfm-extension/dfm-extension.h>

#include "gui-connection.h"
#include "seadrive-menu-plugin.h"
#include "seadrive-emblemicon-plugin.h"
#include "log.h"

static DFMEXT::DFMExtMenuPlugin *seadriveMenu { nullptr };
static DFMEXT::DFMExtEmblemIconPlugin *seadriveEmblemIcon { nullptr };

using namespace SeaDrivePlugin;
const char *log_file_name = "seadrive_ext.log";

static GuiConnection *conn { nullptr };

extern "C" void dfm_extension_initiliaze()
{
    conn = new SeaDrivePlugin::GuiConnection;
    seadriveMenu = new SeaDrivePlugin::SeaDriveMenuPlugin(conn);
    seadriveEmblemIcon = new SeaDrivePlugin::SeaDriveEmblemIconPlugin(conn);
}

extern "C" void dfm_extension_shutdown()
{
    if (conn) {
        delete conn;
    }
    delete seadriveMenu;
    delete seadriveEmblemIcon;
}

extern "C" DFMEXT::DFMExtMenuPlugin *dfm_extension_menu()
{
    return seadriveMenu;
}

extern "C" DFMEXT::DFMExtEmblemIconPlugin *dfm_extension_emblem()
{
    return seadriveEmblemIcon;
}
