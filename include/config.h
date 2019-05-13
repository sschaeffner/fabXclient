#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define MAX_TOOLS 16

enum Mode { KEEP, UNLOCK };

struct Config {
    String deviceName;
    int deviceId;
    int toolAmount;

    int toolIds[MAX_TOOLS];
    int toolPins[MAX_TOOLS];
    Mode toolModes[MAX_TOOLS];
    String toolNames[MAX_TOOLS];
};

#endif