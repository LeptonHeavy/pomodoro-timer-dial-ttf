/**
 * Display Module Implementation
 * All drawing functions moved here from main.cpp
 * UI/visuals remain exactly the same
 */

#include "Display.h"

Display::Display()
  : timerSprite(&M5Dial.Display),
    settingsSprite(&M5Dial.Display) {
    // Constructor - nothing to initialize
}

bool Display::loadUiFont(const char* path) {
    if (!SPIFFS.exists(path)) {
        Serial.print("Missing font file: ");
        Serial.println(path);
        return false;
    }

    return M5Dial.Display.loadFont(SPIFFS, path);
}

// Timer is rendered into an LGFX_Sprite before being pushed to the display.
// This eliminates visible flicker caused by clearing and redrawing large
// smooth fonts directly on the LCD each second.
void Display::ensureTimerSprite() {
    if (!timerSpriteReady) {
        timerSprite.setColorDepth(16);
        timerSprite.createSprite(220, 80);
        timerSprite.setTextDatum(middle_center);
        timerSpriteReady = true;
    }
    // Keep the timer font loaded in the sprite to avoid reloading it from SPIFFS
    // on every 1-second refresh.
    if (timerSpriteReady && !timerFontLoaded) {
        timerFontLoaded = timerSprite.loadFont(SPIFFS, "/fonts/RobotoMonoBold72.vlw");
        if (!timerFontLoaded) {
            Serial.println("Failed to load /fonts/RobotoMonoBold72.vlw");
        }
    }
}

// Similar to above the settings menu is rendered into an LGFX_Sprite before being pushed to the display.
// This eliminates visible flicker caused by clearing and redrawing a large area.
void Display::ensureSettingsSprite() {
    if (!settingsSpriteReady) {
        settingsSprite.setColorDepth(16);
        settingsSprite.createSprite(SCREEN_WIDTH, 165);
        settingsSprite.setTextDatum(top_center);
        settingsSpriteReady = true;
    }

    if (settingsSpriteReady && !settingsFontLoaded) {
        settingsFontLoaded = settingsSprite.loadFont(SPIFFS, "/fonts/RobotoRegular14.vlw");
        if (!settingsFontLoaded) {
            Serial.println("Failed to load /fonts/RobotoRegular14.vlw");
        }
    }
}

void Display::drawCircularProgress(float progress, uint16_t color, TimerState state) {
    // Draw a static full white circle ring (no progress updates)
    uint16_t bgColor = getStateBackgroundColor(state, state);
    int16_t outerRadius = CIRCLE_RADIUS + CIRCLE_THICKNESS/2;
    int16_t innerRadius = CIRCLE_RADIUS - CIRCLE_THICKNESS/2;
    
    // Draw a full solid white ring by filling the outer circle and clearing the inner circle
    // First, draw filled outer circle
    M5Dial.Display.fillCircle(CENTER_X, CENTER_Y, outerRadius, color);
    // Then, clear the inner circle to create the ring
    M5Dial.Display.fillCircle(CENTER_X, CENTER_Y, innerRadius, bgColor);
}

void Display::drawTimerDisplay(uint32_t seconds, uint16_t color, TimerState state,
                               uint32_t duration, uint32_t remaining,
                               TimerState lastState, float& lastProgress) {
    // Calculate progress
    float progress = duration > 0 ? 1.0 - ((float)remaining / (float)duration) : 0.0;
    
    // Get background color based on state
    uint16_t bgColor = getStateBackgroundColor(state, state);
    
    // Redraw everything if state changed, or on first draw
    bool fullRedraw = (lastState != state) || (lastProgress < 0);
    
    if (fullRedraw) {
        // Full screen clear with state background color
        M5Dial.Display.fillScreen(bgColor);
        // Draw white circle only if enabled
        if (SHOW_WHITE_CIRCLE) {
            drawCircularProgress(progress, COLOR_TEXT, state); // Draw static white circle
        }
        // Draw tomato icon after screen clear
        // drawTomatoIcon(state); now omitting the tomato
        lastProgress = progress;
    }
    // Circle is static - no need to update it based on progress changes
    
    // Always redraw time text in white (it changes every second, or when adjusting)
    // Draw into an off-screen sprite, then push the finished image to the display
    int16_t timerY = CENTER_Y - 10; // Move timer up slightly to make room for status text below

    ensureTimerSprite();

    timerSprite.fillSprite(bgColor);
    timerSprite.setTextColor(COLOR_TEXT, bgColor);
    timerSprite.setTextDatum(middle_center);

    if (timerFontLoaded) {
        timerSprite.drawString(formatTime(seconds).c_str(), 110, 40);
    } else {
        timerSprite.setTextSize(5);
        timerSprite.drawString(formatTime(seconds).c_str(), 110, 50);
    }

    // Push the completed timer image to the physical display in one operation
    timerSprite.pushSprite(CENTER_X - 110, timerY - 50);

    // Draw status text inside the circle, below the timer
    const char* statusText = "";
    if (state == STATE_IDLE) {
        statusText = "Ready";
    } else if (state == STATE_PAUSED) {
        statusText = "Paused";
    } else if (state == STATE_RUNNING) {
        statusText = "Focusing";
    } else if (state == STATE_SHORT_BREAK) {
        statusText = "Short Break";
    } else {
        statusText = "Long Break";
    }
    

    // Clear area for status text inside circle - positioned below centered timer
    // Only redraw status text when the screen is fully redrawn
    if (fullRedraw) {
        M5Dial.Display.fillRect(CENTER_X - 80, CENTER_Y + 25, 160, 30, bgColor);
        M5Dial.Display.setTextColor(TFT_WHITE);
        M5Dial.Display.setTextDatum(middle_center);

        if (loadUiFont("/fonts/RobotoRegularBold20.vlw")) {
            M5Dial.Display.drawString(statusText, CENTER_X, CENTER_Y + 35);
            M5Dial.Display.unloadFont();
        } else {
            M5Dial.Display.setTextSize(2);
            M5Dial.Display.drawString(statusText, CENTER_X, CENTER_Y + 35);
        }
    }
    // No moving indicator dot - static circle only
}

void Display::drawStatusText(const char* text, uint16_t color, TimerState state, TimerState lastState) {
    // Status text is now drawn inside the circle in drawTimerDisplay
    // This function draws the instructions and settings gear at the bottom
    
    // Get background color based on state
    uint16_t bgColor = getStateBackgroundColor(state, state);
    
    // Draw instructions at bottom (moved higher to avoid gear icon)
    int16_t instructionY = SCREEN_HEIGHT - 48;
    M5Dial.Display.fillRect(0, instructionY - 10, SCREEN_WIDTH, 20, bgColor);
    M5Dial.Display.setTextColor(COLOR_TEXT);
    M5Dial.Display.setTextSize(1);
    const char* instruction = "";
    if (state == STATE_IDLE) {
        instruction = "Press: Start | Hold: Reset";
    } else if (state == STATE_PAUSED) {
        instruction = "Press: Resume | Hold: Reset";
    } else {
        instruction = "Press: Pause | Hold: Reset";
    }
    if (loadUiFont("/fonts/RobotoRegularBold12.vlw")) {
        M5Dial.Display.drawString(instruction, CENTER_X, instructionY-10);
        M5Dial.Display.unloadFont();
    } else {
        // Fallback to current built-in font behavior
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.drawString(instruction, CENTER_X, instructionY);
    } 
    
    
    // Draw settings gear icon at bottom center (only when not in settings)
    // Simple approach: always draw when state changed from what was displayed last frame
    if (state != STATE_SETTINGS && lastState != state) {
        Serial.println(">>> Drawing gear icon (state changed)!");
        int16_t iconY = SCREEN_HEIGHT - 20;
        int16_t iconSize = 24;
        int16_t iconX = CENTER_X - iconSize/2;
        int16_t iconYPos = (iconY - iconSize/2)-3; // Adjusted to align better with text
        
        // Clear area for icon
        M5Dial.Display.fillRect(CENTER_X - 15, iconY - 15, 30, 30, bgColor);
        
        // Load and draw gear icon from SPIFFS
        File gearFile = SPIFFS.open("/gear.png", "r");
        if (gearFile) {
            bool result = M5Dial.Display.drawPng(&gearFile, iconX, iconYPos);
            gearFile.close();
            if (!result) {
                Serial.println("PNG draw failed - using fallback");
                M5Dial.Display.setTextColor(TFT_WHITE);
                M5Dial.Display.setTextDatum(middle_center);
                M5Dial.Display.setTextSize(2);
                M5Dial.Display.drawString("\xE2\x9A\x99", CENTER_X, iconY);
            } else {
                Serial.println("Gear PNG drawn successfully");
            }
        } else {
            Serial.println("Failed to open gear.png - using fallback");
            M5Dial.Display.setTextColor(TFT_WHITE);
            M5Dial.Display.setTextDatum(middle_center);
            M5Dial.Display.setTextSize(2);
            M5Dial.Display.drawString("\xE2\x9A\x99", CENTER_X, iconY);
        }
    }
}

void Display::drawCurvedText(const char* text, int16_t centerX, int16_t centerY, int16_t radius, float startAngle, uint16_t color) {
    // Draw text along a circular arc
    M5Dial.Display.setTextColor(color);
    M5Dial.Display.setTextSize(1);
    
    int len = strlen(text);
    float angleStep = (2.0 * PI) / len; // Distribute characters evenly around the arc
    
    for (int i = 0; i < len; i++) {
        float angle = startAngle + (i * angleStep);
        int16_t x = centerX + (int16_t)(radius * cos(angle));
        int16_t y = centerY - (int16_t)(radius * sin(angle));
        
        // Draw character at this position
        char ch[2] = {text[i], '\0'};
        M5Dial.Display.drawString(ch, x, y);
    }
}

void Display::drawPomodoroCounter(uint8_t completedPomodoros, TimerState state) {
    // Get background color based on state
    uint16_t bgColor = getStateBackgroundColor(state, state);
    
    // Clear area at the top (use state background)
    M5Dial.Display.fillRect(0, 0, SCREEN_WIDTH, 45, bgColor);
    
    // Draw pomodoro count text at the top center (slightly lower)
    char pomoText[25];
    snprintf(pomoText, sizeof(pomoText), "Sessions: %d", completedPomodoros);
    
    // Draw text at the top center - simple and visible
    M5Dial.Display.setTextColor(COLOR_TEXT);
    M5Dial.Display.setTextDatum(middle_center);

    if (loadUiFont("/fonts/RobotoRegularBold18.vlw")) {
        M5Dial.Display.drawString(pomoText, CENTER_X, 40); // Positioned lower (was +35)
        M5Dial.Display.unloadFont();
    } else {
        // Fallback to current built-in font behavior
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.drawString(pomoText, CENTER_X, 40); // Lowered from 15 to 20
    }


}

void Display::drawTomatoIcon(TimerState state) {
    // Draw tomato icon between pomodoro counter (y=20) and timer (y=120)
    // Position it slightly higher: around y=60
    int16_t iconY = 60; // Slightly higher than middle (was 70)
    int16_t iconSize = 32; // PNG is 32x32
    int16_t iconX = CENTER_X - iconSize/2;
    int16_t iconYPos = iconY - iconSize/2;
    
    // Use M5GFX's drawPng via Stream to load PNG from SPIFFS with transparency support
    File tomatoFile = SPIFFS.open("/pomodoro.png", "r");
    if (tomatoFile) {
        bool result = M5Dial.Display.drawPng(&tomatoFile, iconX, iconYPos);
        tomatoFile.close();
        if (!result) {
            Serial.println("Failed to draw PNG");
            M5Dial.Display.fillRect(iconX, iconYPos, iconSize, iconSize, TFT_RED);
        } else {
            Serial.println("PNG drawn successfully");
        }
    } else {
        Serial.println("Failed to open /pomodoro.png from SPIFFS");
        M5Dial.Display.fillRect(iconX, iconYPos, iconSize, iconSize, TFT_RED);
    }
}

void Display::drawSettingsMenu(const PomodoroSettings& settings, uint8_t menuIndex,
                               bool editing, TimerState lastState) {
    // Clear screen if we just entered settings (transitioning from another state)
    if (lastState != STATE_SETTINGS) {
        M5Dial.Display.fillScreen(COLOR_BG);
        Serial.println("Clearing screen for Settings entry");
    }
    
    M5Dial.Display.setTextColor(COLOR_TEXT);
    M5Dial.Display.setTextDatum(top_center);

    if (loadUiFont("/fonts/RobotoRegularBold18.vlw")) {
        M5Dial.Display.drawString("Settings", CENTER_X, 15); 
        M5Dial.Display.unloadFont();
    } else {
        // Fallback to current built-in font behavior
        M5Dial.Display.setTextSize(2);
        M5Dial.Display.drawString("Settings", CENTER_X, 15);
    }

   
    
    M5Dial.Display.setTextSize(1);
    
    ensureSettingsSprite();
    settingsSprite.fillSprite(COLOR_BG);
    settingsSprite.setTextDatum(top_center);

    if (!settingsFontLoaded) {
        settingsSprite.setTextSize(1);
    }

    int16_t yPos = 10;

    const char* menuItems[] = {
        "Work Duration",
        "Short Break",
        "Long Break",
        "Sessions/Long",
        "Brightness",
        "Back"
    };

    for (uint8_t i = 0; i < 6; i++) {
                if (i == menuIndex) {
            settingsSprite.fillRect(10, yPos - 4, SCREEN_WIDTH - 20, 22, COLOR_PROGRESS_BG);

            if (editing) {
                settingsSprite.setTextColor(TFT_ORANGE, COLOR_PROGRESS_BG);
            } else {
                settingsSprite.setTextColor(COLOR_WORK, COLOR_PROGRESS_BG);
            }
        } else {
            settingsSprite.setTextColor(COLOR_TEXT, COLOR_BG);
        }

        char baseLine[60];
        char line[70];

        if (i == 0) {
            snprintf(baseLine, sizeof(baseLine), "%s: %s", menuItems[i], formatTime(settings.workDuration).c_str());
        } else if (i == 1) {
            snprintf(baseLine, sizeof(baseLine), "%s: %s", menuItems[i], formatTime(settings.shortBreakDuration).c_str());
        } else if (i == 2) {
            snprintf(baseLine, sizeof(baseLine), "%s: %s", menuItems[i], formatTime(settings.longBreakDuration).c_str());
        } else if (i == 3) {
            snprintf(baseLine, sizeof(baseLine), "%s: %d", menuItems[i], settings.pomodorosUntilLongBreak);
        } else if (i == 4) {
            snprintf(baseLine, sizeof(baseLine), "%s: Level %d/6", menuItems[i], settings.brightnessLevel);
        } else {
            snprintf(baseLine, sizeof(baseLine), "%s", menuItems[i]);
        }

        if (editing && i == menuIndex && i != 5) {
            snprintf(line, sizeof(line), "-- %s --", baseLine);
        } else {
            snprintf(line, sizeof(line), "%s", baseLine);
        }

        settingsSprite.drawString(line, CENTER_X, yPos);
        yPos += 25;
    }

    settingsSprite.pushSprite(0, 40);
    
    // Instructions (clear area first) - moved higher to be fully visible
    M5Dial.Display.fillRect(0, SCREEN_HEIGHT - 45, SCREEN_WIDTH, 45, COLOR_BG);
    M5Dial.Display.setTextColor(COLOR_TEXT);
    M5Dial.Display.drawLine(0, SCREEN_HEIGHT - 47, SCREEN_WIDTH, SCREEN_HEIGHT - 47, TFT_WHITE);

    if (loadUiFont("/fonts/RobotoRegular12.vlw")) {
        M5Dial.Display.drawString("Dial: Navigate/Adjust", CENTER_X, SCREEN_HEIGHT - 40);
        M5Dial.Display.drawString("Press: Select/Edit", CENTER_X, SCREEN_HEIGHT - 26);
        M5Dial.Display.unloadFont();
    } else {
        // Fallback to current built-in font behavior
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.drawString("Dial: Navigate/Adjust", CENTER_X, SCREEN_HEIGHT - 40);
        M5Dial.Display.drawString("Press: Select/Edit", CENTER_X, SCREEN_HEIGHT - 26);
    }   


    
}

String Display::formatTime(uint32_t seconds) {
    uint32_t minutes = seconds / 60;
    uint32_t secs = seconds % 60;
    
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu", minutes, secs);
    return String(buffer);
}

uint16_t Display::getStateColor(TimerState state) {
    switch (state) {
        case STATE_RUNNING:
            return COLOR_WORK;
        case STATE_SHORT_BREAK:
            return COLOR_BREAK;
        case STATE_LONG_BREAK:
            return COLOR_LONG_BREAK;
        case STATE_PAUSED:
            return 0x7BEF; // Yellow
        default:
            return COLOR_TEXT;
    }
}

uint16_t Display::getStateBackgroundColor(TimerState state, TimerState stateBeforePause) {
    switch (state) {
        case STATE_RUNNING:
            return COLOR_WORK_BG; // Red background
        case STATE_SHORT_BREAK:
            return COLOR_SHORT_BREAK_BG; // Green background
        case STATE_LONG_BREAK:
            return COLOR_LONG_BREAK_BG; // Orange background for long break
        case STATE_PAUSED:
            // When paused, keep the color of the state that was paused
            // Use stateBeforePause to determine the background color
            switch (stateBeforePause) {
                case STATE_RUNNING:
                    return COLOR_WORK_BG; // Red background
                case STATE_SHORT_BREAK:
                    return COLOR_SHORT_BREAK_BG; // Green background
                case STATE_LONG_BREAK:
                    return COLOR_LONG_BREAK_BG; // Orange background
                default:
                    return COLOR_BG; // Default to black
            }
        default:
            return COLOR_WORK_BG; // Red background for idle (default)
    }
}
