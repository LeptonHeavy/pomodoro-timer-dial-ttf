/**
 * Display Module - All UI drawing functions
 * Handles all visual rendering for the Pomodoro Timer
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <M5Dial.h>
#include "config.h"
#include "types.h"

class Display {
public:
    // Constructor
    Display();
    
    void drawClockScreen(const m5::rtc_datetime_t& dateTime,
                     bool forceRedraw = false);

    // Main drawing functions
    void drawTimerDisplay(uint32_t seconds, uint16_t color, TimerState state, 
                         uint32_t duration, uint32_t remaining, 
                         TimerState lastState, float& lastProgress);
    void drawStatusText(const char* text, uint16_t color, TimerState state, TimerState lastState);
    void drawPomodoroCounter(uint8_t completedPomodoros, TimerState state);
    void drawTomatoIcon(TimerState state);
    void drawSettingsMenu(const PomodoroSettings& settings, uint8_t menuIndex, 
                         bool editing, TimerState lastState);
    
    // Helper functions
    String formatTime(uint32_t seconds);
    uint16_t getStateColor(TimerState state);
    uint16_t getStateBackgroundColor(TimerState state, TimerState stateBeforePause);
    
private:
    // Internal drawing helpers
    void drawCircularProgress(float progress, uint16_t color, TimerState state);
    void drawCurvedText(const char* text, int16_t centerX, int16_t centerY, 
                       int16_t radius, float startAngle, uint16_t color);
    bool loadUiFont(const char* path);

    // Timer sprite
    lgfx::LGFX_Sprite timerSprite;
    bool timerSpriteReady = false;
    bool timerFontLoaded = false;
    void ensureTimerSprite();

    // Settings menu sprite
    lgfx::LGFX_Sprite settingsSprite;
    bool settingsSpriteReady = false;
    bool settingsFontLoaded = false;
    void ensureSettingsSprite();

    // Screen saver clock sprites
    lgfx::LGFX_Sprite clockTimeSprite;
    lgfx::LGFX_Sprite clockDateSprite;

    bool clockSpritesReady = false;
    bool clockTimeFontLoaded = false;
    bool clockDateFontLoaded = false;

    uint32_t lastClockMinute = UINT32_MAX;
    uint32_t lastClockDate = UINT32_MAX;

    void ensureClockSprites();
};

#endif // DISPLAY_H

