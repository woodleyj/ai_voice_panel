#pragma once

enum AppState {
    STATE_IDLE,
    STATE_LISTENING,
    STATE_PROCESSING,
    STATE_SPEAKING,
    STATE_ERROR
};

class AppController {
private:
    AppState currentState = STATE_IDLE;
public:
    void begin();
    void setState(AppState newState);
    AppState getState();
    void loop();
};