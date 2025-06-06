#include "battery.h"

#include <Arduino.h>

#include "debug.h"

void BatteryMonitor::init(uint8_t pin, uint8_t batScale, uint8_t batAdd, Buzzer *buzzer, Led *l) {
    buz = buzzer;
    led = l;
    vbatPin = pin;
    scale = 84 + (84 * batScale / 100);
    add = batAdd;
    state = ALARM_OFF;
    memset(measurements, 0, sizeof(measurements));
    measurementIndex = 0;
    lastCheckTimeMs = millis();
    pinMode(vbatPin, INPUT);

    for (int i = 0; i < AVERAGING_SIZE; i++) {
        getBatteryVoltage(); // kick averaging sum up to speed.
    }
}

static uint16_t averageSum = 0;

uint8_t BatteryMonitor::getBatteryVoltage() {
    // 0-3.3V maps to 0-4095, 8.4V max battery voltage ÷4 (resistor divider)
    // so 4095 * 2.1/3.3 = 2606 is our maximum value of fully charged 2S battery
    // we match the voltage to the maximum possible and add the current draw
    volatile uint16_t raw = analogRead(vbatPin);
    uint8_t scaled = map(raw, 0, 2606, 0, scale) + add;
    averageSum = averageSum - measurements[measurementIndex]; // substract oldest val
    measurements[measurementIndex] = raw;                     // replace old with new val
    averageSum += raw;                                        // update averageSum
    measurementIndex = (measurementIndex + 1) % AVERAGING_SIZE;
    uint8_t average =
        map(round(averageSum / AVERAGING_SIZE), 0, 2606, 0, scale) + add; // 3.3v ref accuracy, divider + voltage drop
    DEBUG("Battery raw:%u, scaled:%u, average:%u\n", raw, scaled, average);
    return average;
}

void BatteryMonitor::checkBatteryState(uint32_t currentTimeMs, uint8_t alarmThreshold) {
    switch (state) {
    case ALARM_OFF:
        if ((alarmThreshold > 0) && ((currentTimeMs - lastCheckTimeMs) > MONITOR_CHECK_TIME_MS)) {
            lastCheckTimeMs = currentTimeMs;
            uint8_t voltage = getBatteryVoltage();
            if ((voltage > add) && (voltage <= alarmThreshold)) {
                state = ALARM_BEEPING;
                buz->beep(MONITOR_BEEP_TIME_MS);
                led->blink(MONITOR_BEEP_TIME_MS);
            }
        }
        break;
    case ALARM_BEEPING:
        if ((currentTimeMs - lastCheckTimeMs) > MONITOR_BEEP_TIME_MS) {
            lastCheckTimeMs = currentTimeMs;
            state = ALARM_IDLE;
        }
        break;
    case ALARM_IDLE:
        if ((currentTimeMs - lastCheckTimeMs) > MONITOR_BEEP_TIME_MS) {
            lastCheckTimeMs = currentTimeMs;
            if (getBatteryVoltage() <= alarmThreshold + 1) { // add 0.1V of histeresis
                state = ALARM_BEEPING;
                buz->beep(MONITOR_BEEP_TIME_MS);
            } else {
                led->off();
                state = ALARM_OFF;
            }
        }
        break;
    default:
        break;
    }
}
