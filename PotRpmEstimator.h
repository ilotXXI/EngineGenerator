#pragma once
#if RPM_ESTIMATOR_TYPE == RPM_ESTIMATOR_POTENTIOMETER

#include <Arduino.h>

class PotRpmEstimator
{
public:
    void setup();
    
    int rpm();
    
private:
    static const int    _minOut = 1000;
    static const int    _maxOut = 10500;
    static const int    _minIn = 935;
    static const int    _maxIn = (1 << 10) - 1;
    
    static const int    _pin = A5;
    
    float               _mult = 0.f;
};


void PotRpmEstimator::setup()
{
    pinMode(_pin, INPUT);
    _mult =
        static_cast<float>(_maxOut - _minOut) /
        static_cast<float>(_maxIn - _minIn);
}

int PotRpmEstimator::rpm()
{
    const float in = static_cast<float>(analogRead(_pin));
    const float out = (in - _minIn) * _mult + _minOut;
    Serial.println(out);
    return static_cast<int>(out + 0.5f);
}

#endif
