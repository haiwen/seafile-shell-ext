#include "ext-common.h"
#include "repo-info.h"

namespace seafile {

std::string toString(SyncStatus st) {
    switch (st) {
    case NoStatus:
        return "none";
    case Syncing:
        return "syncing";
    case Error:
        return "error";
    case Synced:
        return "synced";
    case PartialSynced:
        return "partial synced";
    case Cloud:
        return "cloud";
    case ReadOnly:
        return "readonly";
    case LockedByOthers:
        return "locked by someone else";
    case LockedByMe:
        return "locked by me";
    case Paused:
        return "paused";
    case N_Status:
        return "";
    }
    return "";
}

}
