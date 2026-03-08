/**
 * Input Handler Implementation
 * All input handling code moved here from main.cpp
 * Behavior remains exactly the same
 */

#include "InputHandler.h"

InputHandler::InputHandler() 
    : lastEncoderPos(0),
      buttonPressTime(0),
      buttonPressed(false),
      longPressHandled(false),
      lastEncoderChangeTime(0),
      settingsMenuAccum(0),
      settingsEditAccum(0),
      idleAdjustAccum(0),
      lastUserActivityTime(0),
      screenDimmed(false) {
}

void InputHandler::init() {
    // Initialize encoder position
    lastEncoderPos = M5Dial.Encoder.read();
    lastUserActivityTime = millis();
    screenDimmed = false;
}

void InputHandler::processInput(TimerState& currentState,
                                PomodoroSettings& settings,
                                uint8_t& settingsMenuIndex,
                                bool& settingsEditing,
                                uint32_t& timerRemaining,
                                uint32_t& timerDuration,
                                bool& needsRedraw,
                                void (*startTimerCallback)(uint32_t),
                                void (*pauseTimerCallback)(),
                                void (*resumeTimerCallback)(),
                                void (*resetTimerCallback)()) {
    // Handle encoder input
    handleEncoderInput(currentState, settings, settingsMenuIndex, settingsEditing,
                      timerRemaining, timerDuration, needsRedraw);
    
    // Handle button input with callbacks
    handleButtonInput(currentState, settings, settingsMenuIndex, settingsEditing, needsRedraw,
                     startTimerCallback, pauseTimerCallback, resumeTimerCallback, resetTimerCallback);
    
    // Handle touch input
    handleTouchInput(currentState, settings, settingsMenuIndex, settingsEditing, needsRedraw);

    // Update idle-time dimming behavior
    updateScreenDimming(currentState, settings);
}

void InputHandler::handleEncoderInput(TimerState& currentState,
                                     PomodoroSettings& settings,
                                     uint8_t& settingsMenuIndex,
                                     bool& settingsEditing,
                                     uint32_t& timerRemaining,
                                     uint32_t& timerDuration,
                                     bool& needsRedraw) {
    long currentPos = M5Dial.Encoder.read();
    long delta = currentPos - lastEncoderPos;

    // Performance optimization: Ignore small changes and debounce
    if (abs(delta) < ENCODER_THRESHOLD) return;

    uint32_t now = millis();
    if (now - lastEncoderChangeTime < ENCODER_DEBOUNCE_MS) return;

    lastEncoderPos = currentPos;
    lastEncoderChangeTime = now;
    noteUserActivity(settings); // Reset dimming timer on encoder activity

    if (currentState == STATE_SETTINGS) {
        if (settingsEditing) {
            // Prevent stale navigation counts from affecting edit mode
            settingsMenuAccum = 0;

            // Edit setting: one physical detent = one value step
            settingsEditAccum += delta;

            int32_t step = 0;

            while (settingsEditAccum >= MENU_EDIT_COUNTS_PER_DETENT) {
                step++;
                settingsEditAccum -= MENU_EDIT_COUNTS_PER_DETENT;
            }

            while (settingsEditAccum <= -MENU_EDIT_COUNTS_PER_DETENT) {
                step--;
                settingsEditAccum += MENU_EDIT_COUNTS_PER_DETENT;
            }

            if (step != 0) {
                if (settingsMenuIndex == 0) {
                    int32_t newVal = (int32_t)settings.workDuration + (step * 60);
                    if (newVal < 60) newVal = 60;
                    if (newVal > 3600) newVal = 3600;
                    settings.workDuration = newVal;

                    // Keep Short Break in sync at 1/5 of Work Duration
                    settings.shortBreakDuration = settings.workDuration / 5;
                    settings.longBreakDuration = settings.workDuration;

                } else if (settingsMenuIndex == 1) {
                    int32_t newVal = (int32_t)settings.shortBreakDuration + (step * 60);
                    if (newVal < 60) newVal = 60;
                    if (newVal > 3600) newVal = 3600;
                    settings.shortBreakDuration = newVal;

                } else if (settingsMenuIndex == 2) {
                    int32_t newVal = (int32_t)settings.longBreakDuration + (step * 60);
                    if (newVal < 60) newVal = 60;
                    if (newVal > 3600) newVal = 3600;
                    settings.longBreakDuration = newVal;

                } else if (settingsMenuIndex == 3) {
                    int16_t newVal = (int16_t)settings.pomodorosUntilLongBreak + step;
                    if (newVal < 1) newVal = 1;
                    if (newVal > 10) newVal = 10;
                    settings.pomodorosUntilLongBreak = newVal;

                } else if (settingsMenuIndex == 4) {
                    int16_t newVal = (int16_t)settings.brightnessLevel + step;
                    if (newVal < 1) newVal = 1;
                    if (newVal > 6) newVal = 6;
                    settings.brightnessLevel = newVal;
                    M5Dial.Display.setBrightness((settings.brightnessLevel * 255) / 6);
                }

                needsRedraw = true;
            }

        } else {
            // Prevent stale edit counts from affecting navigation mode
            settingsEditAccum = 0;

            // Navigate menu: one physical detent = one item move
            settingsMenuAccum += delta;

            bool moved = false;

            while (settingsMenuAccum >= MENU_NAV_COUNTS_PER_DETENT) {
                settingsMenuIndex = (settingsMenuIndex + 1) % 6;
                settingsMenuAccum -= MENU_NAV_COUNTS_PER_DETENT;
                moved = true;
            }

            while (settingsMenuAccum <= -MENU_NAV_COUNTS_PER_DETENT) {
                settingsMenuIndex = (settingsMenuIndex + 5) % 6;
                settingsMenuAccum += MENU_NAV_COUNTS_PER_DETENT;
                moved = true;
            }

            if (moved) {
                needsRedraw = true;
            }
        }

        } else if (currentState == STATE_IDLE) {
        // Clear settings accumulators when not in settings
        settingsMenuAccum = 0;
        settingsEditAccum = 0;

        // Idle timer adjustment should also be one step per physical detent
        idleAdjustAccum += delta;

        int32_t step = 0;

        while (idleAdjustAccum >= IDLE_ADJUST_COUNTS_PER_DETENT) {
            step++;
            idleAdjustAccum -= IDLE_ADJUST_COUNTS_PER_DETENT;
        }

        while (idleAdjustAccum <= -IDLE_ADJUST_COUNTS_PER_DETENT) {
            step--;
            idleAdjustAccum += IDLE_ADJUST_COUNTS_PER_DETENT;
        }

        if (step != 0) {
            uint16_t currentMinutes = settings.workDuration / 60;
            int16_t newMinutes = currentMinutes + step;

            if (newMinutes < 1) newMinutes = 1;
            if (newMinutes > 25) newMinutes = 25;

            if (newMinutes != currentMinutes) {
                settings.workDuration = newMinutes * 60;
                timerRemaining = settings.workDuration;
                timerDuration = settings.workDuration;

                // When dial is used, automatically calculate breaks using 1/5 rule
                settings.shortBreakDuration = settings.workDuration / 5;
                settings.longBreakDuration = settings.workDuration;

                M5Dial.Speaker.tone(800, 30);
                needsRedraw = true;
            }
        }
    } else {
        // Clear accumulators in other states
        settingsMenuAccum = 0;
        settingsEditAccum = 0;
        idleAdjustAccum = 0;
    }
}

void InputHandler::handleButtonInput(TimerState& currentState,
                                    PomodoroSettings& settings,
                                    uint8_t& settingsMenuIndex,
                                    bool& settingsEditing,
                                    bool& needsRedraw,
                                    void (*startTimerCallback)(uint32_t),
                                    void (*pauseTimerCallback)(),
                                    void (*resumeTimerCallback)(),
                                    void (*resetTimerCallback)()) {
    if (M5Dial.BtnA.isPressed()) {
        if (!buttonPressed) {
            buttonPressed = true;
            longPressHandled = false;
            buttonPressTime = millis();
            noteUserActivity(settings);
        } else {
            uint32_t pressDuration = millis() - buttonPressTime;
            
            // Check for long press (2+ seconds) = Reset to ready
            if (pressDuration > 2000 && !longPressHandled) {
                longPressHandled = true;
                if (currentState == STATE_SETTINGS) {
                    // In settings, long press does nothing
                    Serial.println("In Settings - use Back to exit");
                } else if (currentState == STATE_RUNNING || currentState == STATE_PAUSED || 
                           currentState == STATE_SHORT_BREAK || currentState == STATE_LONG_BREAK) {
                    // Long press when timer is active = reset to ready state
                    if (resetTimerCallback) {
                        resetTimerCallback();
                    }
                    needsRedraw = true;
                    Serial.println("Reset to Ready (2s+ press)");
                } else if (currentState == STATE_IDLE) {
                    // Long press when already idle = do nothing
                    Serial.println("Already in Ready state");
                }
            }
        }
    } else {
        if (buttonPressed) {
            buttonPressed = false;
            uint32_t pressDuration = millis() - buttonPressTime;
            // Only handle short press if long press was NOT handled
            if (pressDuration < 2000 && !longPressHandled) {
                // Short press (< 2 seconds) = normal actions
                handleButtonPress(currentState, settings, settingsMenuIndex, settingsEditing,
                                needsRedraw, startTimerCallback, pauseTimerCallback, 
                                resumeTimerCallback, resetTimerCallback);
            }
        }
    }
}

void InputHandler::handleTouchInput(TimerState& currentState,
                                   PomodoroSettings& settings,
                                   uint8_t& settingsMenuIndex,
                                   bool& settingsEditing,
                                   bool& needsRedraw) {
    auto touch = M5Dial.Touch.getDetail();
    if (touch.wasPressed()) {
        noteUserActivity(settings);

        int16_t touchX = touch.x;
        int16_t touchY = touch.y;

        if (touchX >= CENTER_X - 20 && touchX <= CENTER_X + 20 &&
            touchY >= SCREEN_HEIGHT - 45 && touchY <= SCREEN_HEIGHT) {
            if (currentState != STATE_SETTINGS) {
                currentState = STATE_SETTINGS;
                settingsMenuIndex = 0;
                settingsEditing = false;
                needsRedraw = true;
                Serial.println("Opening Settings (gear icon touched)");
            }
        }
    }
}

void InputHandler::handleButtonPress(TimerState& currentState,
                                    PomodoroSettings& settings,
                                    uint8_t& settingsMenuIndex,
                                    bool& settingsEditing,
                                    bool& needsRedraw,
                                    void (*startTimerCallback)(uint32_t),
                                    void (*pauseTimerCallback)(),
                                    void (*resumeTimerCallback)(),
                                    void (*resetTimerCallback)()) {
    needsRedraw = true; // Mark that we need to redraw
    
    switch (currentState) {
        case STATE_IDLE:
            // Start work timer
            if (startTimerCallback) {
                startTimerCallback(settings.workDuration);
            }
            break;
            
        case STATE_RUNNING:
        case STATE_SHORT_BREAK:
        case STATE_LONG_BREAK:
            // Pause timer
            if (pauseTimerCallback) {
                pauseTimerCallback();
            }
            break;
            
        case STATE_PAUSED:
            // Resume timer
            if (resumeTimerCallback) {
                resumeTimerCallback();
            }
            break;
            
        case STATE_SETTINGS:
            if (settingsMenuIndex == 5) {
                // Back to main screen - force full screen clear to prevent overlap
                uint16_t bgColor = COLOR_WORK_BG; // Red background for idle
                M5Dial.Display.fillScreen(bgColor);
                currentState = STATE_IDLE;
                settingsMenuAccum = 0;
                settingsEditAccum = 0;
                lastEncoderPos = M5Dial.Encoder.read();
                if (resetTimerCallback) {
                    resetTimerCallback();
                }
                needsRedraw = true;
                Serial.println("Exiting Settings -> Idle");
            } else if (settingsMenuIndex >= 0 && settingsMenuIndex <= 4) {
                // Allow editing all settings: Work Duration, Short Break, Long Break, Pomodoros/Long, Brightness
                settingsEditing = !settingsEditing;
                settingsMenuAccum = 0;
                settingsEditAccum = 0;
                lastEncoderPos = M5Dial.Encoder.read();
                needsRedraw = true;
            }
            break;
    }
}
void InputHandler::noteUserActivity(const PomodoroSettings& settings) {
    lastUserActivityTime = millis();

    if (screenDimmed) {
        M5Dial.Display.setBrightness((settings.brightnessLevel * 255) / 6);
        screenDimmed = false;
    }
}

void InputHandler::updateScreenDimming(TimerState currentState, const PomodoroSettings& settings) {
    // Timer-related screens should always stay at user-selected brightness
    bool forceNormalBrightness =
        (currentState == STATE_RUNNING) ||
        (currentState == STATE_PAUSED) ||
        (currentState == STATE_SHORT_BREAK) ||
        (currentState == STATE_LONG_BREAK);

    if (forceNormalBrightness) {
        if (screenDimmed) {
            M5Dial.Display.setBrightness((settings.brightnessLevel * 255) / 6);
            screenDimmed = false;
        }
        lastUserActivityTime = millis();
        return;
    }

    // Idle and Settings screens may dim after inactivity
    if (currentState == STATE_IDLE || currentState == STATE_SETTINGS) {
        uint32_t now = millis();
        if (!screenDimmed && (now - lastUserActivityTime >= DIM_TIMEOUT_MS)) {
            M5Dial.Display.setBrightness((DIM_BRIGHTNESS_LEVEL * 255) / 6);
            screenDimmed = true;
        }
    }
}