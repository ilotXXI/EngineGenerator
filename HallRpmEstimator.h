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
    static constexpr float  _maxSpeedKmh = 25.f;
    static constexpr float  _minDetectedSpeedKmh = 2.f;
    static constexpr float  _wheelDiameterM = 24.f * 2.54E-2f;
    static constexpr float  _prescaler = 1024.f;
    
    static constexpr int    _pin = 2;
    
    int                     _rpm = _minOut;
    uint32_t                _lastOnCounter = 0;
    unsigned int            _counterOverflows = 0;
    const uint32_t          _maxWaitCounter = calcMaxWaitCounter();
    bool                    _magnetWasNear = false;
    
    int calcRpm(uint32_t onCounter) const;
    void checkCounterOverflow();
    void checkDeadPeriod();
    uint32_t fullCounter() const;
    
    static uint32_t calcMaxWaitCounter();
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

    checkCounterOverflow();
    checkDeadPeriod();
    
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
    
    const float wayKm = _wheelDiameterM * 1E-3f * pi;
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

inline void HallRpmEstimator::checkCounterOverflow()
{
    if (TIFR1 & (1 << TOV1))
    {
        ++_counterOverflows;
        // Reset the overflow flag.
        TIFR1 |= (1 << TOV1);
    }
}

inline void HallRpmEstimator::checkDeadPeriod()
{
    if (fullCounter() - _lastOnCounter > _maxWaitCounter)
    {
        _rpm = _minOut;
        Serial.print("Dead period ");
        Serial.print(_maxWaitCounter);
        Serial.println(" is met");
    }
}

inline uint32_t HallRpmEstimator::fullCounter() const
{
    uint16_t bit16 = 0;
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        bit16 = TCNT1;
    }
    const uint32_t res = uint32_t(_counterOverflows) * (1ULL << 16ULL) + bit16;
    return res;
}

uint32_t HallRpmEstimator::calcMaxWaitCounter()
{
    static_assert(_minDetectedSpeedKmh > 0.f,
        "Minimal detected speed must be set");
    static_assert(_minDetectedSpeedKmh < _maxSpeedKmh,
        "Minimal detected speed must less then the maximal one");
    
    constexpr float cpuFreq = 16E6f;
    constexpr float pi = 3.14159265359f;
    constexpr float wheelLenKm = _wheelDiameterM * 1E-3f * pi;
    constexpr float maxPeriodH = wheelLenKm / _minDetectedSpeedKmh;
    constexpr float maxPeriodSec = maxPeriodH * 60.f * 60.f;
    constexpr float maxPeriodClocks = maxPeriodSec * cpuFreq / _prescaler;
    
    constexpr float safePeriodMax = float(1ULL << 31ULL) - 1.f;
    static_assert(0.5f <= maxPeriodClocks,
        "Wrong wheel \"dead\" period");
    static_assert(maxPeriodClocks < safePeriodMax,
        "The wheel \"dead\" period is too long to detect it. Try to raise "
        "the _minDetectedSpeedKmh");
    
    return uint32_t(maxPeriodClocks + 0.5f);
}

#endif
