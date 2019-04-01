#ifndef DEVICECONFIG_H
#define DEVICECONFIG_H

#include <Arduino.h>
#include "conf.h"

#define MAX_TOOLS 16

enum Mode { KEEP, UNLOCK };

struct Config {
    String deviceName;
    int deviceId;
    String bgImageUrl;
    String backendUrl = BACKEND_URL;
    String backupBackendUrl;

    int toolAmount;

    int toolIds[MAX_TOOLS];
    int toolPins[MAX_TOOLS];
    Mode toolModes[MAX_TOOLS];
    String toolNames[MAX_TOOLS];
};

#endif // DEVICECONFIG_H