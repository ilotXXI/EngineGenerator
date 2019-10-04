#pragma once
#if RPM_ESTIMATOR_TYPE == RPM_ESTIMATOR_HALL

#include <Arduino.h>
#include <stdint.h>

#include "RpmEstimatorSettings.h"

class HallRpmEstimator
{
public:
    void setup();
    
    int rpm();
    
private:
    static constexpr int    _minOut = RpmEstimatorSettings::minRpm;
    static constexpr int    _maxOut = RpmEstimatorSettings::maxRpm;
    static constexpr int    _maxSpeedKmh = 25;
    static constexpr float  _wheelDiameter = 24.f * 2.54E-2f;
    static constexpr float  _prescaler = 1024.f;
    
    static constexpr int    _pin = 2;
    
    int                     _rpm = _minOut;
    uint32_t                _lastOnCounter = 0;
    unsigned int            _counterOverflows = 0;
    bool                    _magnetWasNear = false;
    
    int calcRpm(uint32_t onCounter) const;
    uint32_t fullCounter() const;
};


void HallRpmEstimator::setup()
{
    pinMode(_pin, INPUT);
    _magnetWasNear = digitalRead(_pin) == LOW;
    
    cli(); // Disable interrupts
    // Setup 16-bit timer1.
    TCCR1A = 0; // Set entire TCCR1A register to 0
    TCCR1B = 0; // Same for TCCR1B
    TCNT1  = 0; // Initialize counter value to 0
    // Set normal mode.
    TCCR1A |= 0;
    TCCR1B |= (1 << CS12) | (1 << CS10); // Set 1024 prescaler.
    static_assert(_prescaler == 1024.f,
        "The prescaler is hard-coded in several pieces");
    TIFR1 &= ~(1 << TOV1);
    sei();  // Enable interrupts
}

int HallRpmEstimator::rpm()
{
    const bool magnetIsNear = digitalRead(_pin) == LOW;
    if (_magnetWasNear && !magnetIsNear)
    {
        Serial.println("Go down");
        _magnetWasNear = false;
    }
    else if (!_magnetWasNear && magnetIsNear)
    {
        Serial.println("Go on");
        _magnetWasNear = true;
        const uint32_t onCounter = fullCounter();
        _rpm = calcRpm(onCounter);
        _lastOnCounter = onCounter;
    }

    if (TIFR1 & (1 << TOV1))
    {
        // TODO: 0 RPM when waiting is too long.
        ++_counterOverflows;
        // Reset the overflow flag.
        TIFR1 |= (1 << TOV1);
    }
    
    return _rpm;
}

int HallRpmEstimator::calcRpm(uint32_t onCounter) const
{
    const float cpuFreq = 16E6f;
    const float pi = 3.14159265359f;
    
    const uint32_t periodClocks = uint32_t(onCounter - _lastOnCounter);
    if (periodClocks == 0)
    {
        return _maxOut;
    }
    const float periodSec = float(periodClocks) * _prescaler / cpuFreq;
    const float periodHour = periodSec / 60.f / 60.f;
    
    const float wayKm = _wheelDiameter * 1E-3f * pi;
    float speedKmh = wayKm / periodHour;
    if (speedKmh > _maxSpeedKmh)
    {
        speedKmh = _maxSpeedKmh;
    }
    
    const float rpm = speedKmh / _maxSpeedKmh * (_maxOut - _minOut) + _minOut;
    Serial.print(periodClocks);
    Serial.print('\t');
    Serial.print(periodHour * 60 * 60);
    Serial.print('\t');
    Serial.print(wayKm * 1E-3);
    Serial.print('\t');
    Serial.print(speedKmh);
    Serial.print('\t');
    Serial.println(rpm);
    return int(rpm + 0.5f);
}

inline uint32_t HallRpmEstimator::fullCounter() const
{
    uint16_t bit16 = 0;
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        bit16 = TCNT1;
    }
    const uint32_t res = uint32_t(_counterOverflows) * (1ULL << 16ULL) + bit16;
    Serial.print("Read counter: ");
    Serial.println(res);
    return res;
}

#endif
