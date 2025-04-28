#ifndef SEADRIVEEMBLEMICONPLUGIN_H
#define SEADRIVEEMBLEMICONPLUGIN_H

#include <dfm-extension/emblemicon/dfmextemblemiconplugin.h>
#include "gui-connection.h"

namespace SeaDrivePlugin {

class SeaDriveEmblemIconPlugin : public DFMEXT::DFMExtEmblemIconPlugin
{
public:
    SeaDriveEmblemIconPlugin(GuiConnection *conn);
    ~SeaDriveEmblemIconPlugin();

    DFMEXT::DFMExtEmblem locationEmblemIcons(const std::string &filePath, int systemIconCount) const DFM_FAKE_OVERRIDE;

private:
    GuiConnection *conn_;
};

}   // namespace SeaDrivePlugin

#endif   // SEADRIVEEMBLEMICONPLUGIN_H
