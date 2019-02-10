#pragma once

class RpmEstimator
{
public:
    void setup();
    
    int rpm();
    
private:
    static const int    _minRpm = 1000;
    static const int    _maxRpm = 10500;
    static const int    _step = 100;
    
    int                 _rpm = _minRpm;
    bool                _increase = true;
    volatile float      _nothing = 0.f;
};


void RpmEstimator::setup()
{
}

int RpmEstimator::rpm()
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
