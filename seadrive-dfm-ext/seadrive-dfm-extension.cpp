#include <dfm-extension/dfm-extension.h>

#include "rpc-client.h"
#include "seadrive-menu-plugin.h"
#include "seadrive-emblemicon-plugin.h"

static DFMEXT::DFMExtMenuPlugin *seadriveMenu { nullptr };
static DFMEXT::DFMExtEmblemIconPlugin *seadriveEmblemIcon { nullptr };

using namespace SeaDrivePlugin;
static SeaDriveRpcClient *rpc_client { nullptr };

extern "C" void dfm_extension_initiliaze()
{
    rpc_client = new SeaDrivePlugin::SeaDriveRpcClient;
    seadriveMenu = new SeaDrivePlugin::SeaDriveMenuPlugin(rpc_client);
    seadriveEmblemIcon = new SeaDrivePlugin::SeaDriveEmblemIconPlugin(rpc_client);
}

extern "C" void dfm_extension_shutdown()
{
    if (rpc_client) {
        delete rpc_client;
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
