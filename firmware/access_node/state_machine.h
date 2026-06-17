#pragma once

enum class State {
    BOOT,
    IDLE,
    CARD_READING,
    ACCESS_DENIED,
    SESSION_ACTIVE,
    TIMEOUT_WARNING,
    TIMEOUT_EXPIRED,
    SESSION_ENDED,
    MACHINE_DISABLED,
    OTA_UPDATE,
    ERROR_STATE,
};

namespace StateMachine {
    void begin();
    void tick();         // call every main loop iteration
    State current();
}
