/**
 * Input Handler Module
 * Handles encoder, button, and touch input
 */

#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <Arduino.h>
#include <M5Dial.h>
#include "config.h"
#include "types.h"

class InputHandler {
public:
    // Constructor
    InputHandler();
    
    // Initialize input handler
    void init();
    
    // Main input processing function (call from loop)
    void processInput(TimerState& currentState, 
                     PomodoroSettings& settings,
                     uint8_t& settingsMenuIndex,
                     bool& settingsEditing,
                     uint32_t& timerRemaining,
                     uint32_t& timerDuration,
                     bool& needsRedraw,
                     void (*startTimerCallback)(uint32_t),
                     void (*pauseTimerCallback)(),
                     void (*resumeTimerCallback)(),
                     void (*resetTimerCallback)());
    
private:
    // Input state tracking
    long lastEncoderPos;
    uint32_t buttonPressTime;
    bool buttonPressed;
    bool longPressHandled;

    
    // Performance optimization - encoder debouncing
    uint32_t lastEncoderChangeTime;
    static constexpr uint32_t ENCODER_DEBOUNCE_MS = 10;  // Balanced for smoothness and responsiveness
    static constexpr int32_t ENCODER_THRESHOLD = 1;       // Minimum encoder delta to process

    // Settings menu navigation should move one item per physical detent
    int32_t settingsMenuAccum;
    static constexpr int32_t MENU_NAV_COUNTS_PER_DETENT = 4;

    // Settings value editing should also move one step per physical detent
    int32_t settingsEditAccum;
    static constexpr int32_t MENU_EDIT_COUNTS_PER_DETENT = 4;

    // Idle time adjust
    int32_t idleAdjustAccum;
    static constexpr int32_t IDLE_ADJUST_COUNTS_PER_DETENT = 4;

    // Inactivity dimming
    uint32_t lastUserActivityTime;
    bool screenDimmed;

    static constexpr uint32_t DIM_TIMEOUT_MS = 5000;
    static constexpr uint8_t DIM_BRIGHTNESS_LEVEL = 1;

    void noteUserActivity(const PomodoroSettings& settings);
    void updateScreenDimming(TimerState currentState, const PomodoroSettings& settings);

    // Internal handlers
    void handleEncoderInput(TimerState& currentState,
                           PomodoroSettings& settings,
                           uint8_t& settingsMenuIndex,
                           bool& settingsEditing,
                           uint32_t& timerRemaining,
                           uint32_t& timerDuration,
                           bool& needsRedraw);
    
    void handleButtonInput(TimerState& currentState,
                          PomodoroSettings& settings,
                          uint8_t& settingsMenuIndex,
                          bool& settingsEditing,
                          bool& needsRedraw,
                          void (*startTimerCallback)(uint32_t),
                          void (*pauseTimerCallback)(),
                          void (*resumeTimerCallback)(),
                          void (*resetTimerCallback)());
    
    void handleTouchInput(TimerState& currentState,
                         PomodoroSettings& settings,
                         uint8_t& settingsMenuIndex,
                         bool& settingsEditing,
                         bool& needsRedraw);
    
    void handleButtonPress(TimerState& currentState,
                          PomodoroSettings& settings,
                          uint8_t& settingsMenuIndex,
                          bool& settingsEditing,
                          bool& needsRedraw,
                          void (*startTimerCallback)(uint32_t),
                          void (*pauseTimerCallback)(),
                          void (*resumeTimerCallback)(),
                          void (*resetTimerCallback)());
};

#endif // INPUT_HANDLER_H

