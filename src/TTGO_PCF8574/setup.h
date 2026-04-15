#ifndef SETUP_H
#define SETUP_H

// --- Display ---
#define TFT_ROTATION 1
#define COLOR_BACKGROUND TFT_BLACK
#define COLOR_TEXT TFT_WHITE
#define COLOR_HIGHLIGHT TFT_CYAN
#define COLOR_SUCCESS TFT_GREEN
#define COLOR_ERROR TFT_RED

// --- Battery ADC (ESP32-C3, 100k+100k divider → VBAT to GPIO3 to GND) ---
#define BATTERY_PIN 3
#define BATTERY_DIVIDER 2.0f // matches your resistor ratio
#define BATTERY_VMAX 4200    // mV
#define BATTERY_VMIN 3300    // mV

// --- Intervals ---
#define SENSOR_INTERVAL_MS 2000

#endif