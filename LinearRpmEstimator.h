#pragma once
#if RPM_ESTIMATOR_TYPE == RPM_ESTIMATOR_LINEAR

#include "RpmEstimatorSettings.h"

class LinearRpmEstimator
{
public:
    void setup();
    
    int rpm();
    
private:
    static const int    _minOut = RpmEstimatorSettings::minRpm;
    static const int    _maxOut = RpmEstimatorSettings::maxRpm;
    static const int    _step = 100;
    
    int                 _rpm = _minRpm;
    bool                _increase = true;
    volatile float      _nothing = 0.f;
};


void LinearRpmEstimator::setup()
{
}

int LinearRpmEstimator::rpm()
{
    const int res = _rpm;
    
    if (_increase)
    {
        _rpm += _step;
        if (_rpm >= _maxRpm)
        {
            _rpm = _maxRpm;
            _increase = false;
        }
    }
    else
    {
        _rpm -= _step;
        if (_rpm <= _minRpm)
        {
            _rpm = _minRpm;
            _increase = true;
        }
    }
    
    // delay() doesn't work properly because of counters and
    // interrupt flag manual use.
    for (unsigned int i = 1; i < 1000; ++i)
    {
        _nothing = sqrt(1.f / static_cast<float>(i));
    }
    
    return res;
}

#endif
