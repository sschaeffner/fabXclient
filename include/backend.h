#ifndef BACKEND_H
#define BACKEND_H

#include <Arduino.h>

class Backend {
    public:
        bool onlineRequest(int deviceId, String cardId);
};

#endif //BACKEND_H