#include <math.h>
#include <Arduino.h>

#include <avr/io.h>
#include <avr/interrupt.h>

static const int maxOutValue = 255;

static unsigned char    _value = 1;
static const int        _bufSize = 250;
static unsigned char    _buf[_bufSize];
static int              _bufInd = 0;

void setup()
{
    const float maxOutValueF = static_cast<float>(maxOutValue);
    const float sampleRateHz = 40000.f;
    const float sinFreq = 160.f;
    const float sampPeriodMcs = static_cast<unsigned long>(
        1.f / sampleRateHz * 1E6f + 0.5f);
    const float pi2 = 2.f * 3.14159265359f;
    const float sinPhaseDelta = pi2 * sinFreq / sampleRateHz;
    
    float phase = 0.f;
    for (auto &val:  _buf)
    {
        int intVal = static_cast<int>(
            (sin(phase) + 1.f) / 2.f * maxOutValueF + 0.5f);
        if (intVal > maxOutValue)
        {
            intVal = maxOutValue;
        }
        else if (intVal < 0)
        {
            intVal = 0;
        }
        val = static_cast<unsigned char>(intVal);
        phase += sinPhaseDelta;
    }
    
    const float prescaler = 8.f;
    const float cpuFreqMHz = 16.f;
    
    // Setup sampling rate interupts.
    cli(); // Disable interrupts
    // Set timer0 interrupt at 40kHz
    TCCR0A = 0; // Set entire TCCR0A register to 0
    TCCR0B = 0; // Same for TCCR0B
    TCNT0  = 0; // Initialize counter value to 0
    // Set compare match register for 40khz increments
    OCR0A = static_cast<unsigned char>(
        cpuFreqMHz / prescaler * sampPeriodMcs + 0.5f) - 1;
    // Turn on CTC mode
    TCCR0A |= (1 << WGM01);
    TCCR0B |= (1 << CS01);      // Set CS01 bit for 8 prescaler
    TIMSK0 |= (1 << OCIE0A);    // Enable timer compare interrupt
    sei();  // Enable interrupts
    
    // Setup PWM
    TCCR2A = 0;
    TCCR2B = 0;
    // Clear on compare match (1 << COM2A1),
    // fast PWM mode ((1 << WGM21) | (1 << WGM20))
    // or PWM with phase correction (1 << WGM20)
    TCCR2A |=
        (1 << COM2A1) |
        (1 << WGM21) | (1 << WGM20);
//         (1 << WGM20);
    TCCR2B |= (1 << CS20);  // No prescaler
    // Port B 3rd pin is set as output
    DDRB = (1 << DDB3);
}

void loop()
{
    // Do nothing. All processing is done by interrupts.
}

ISR(TIMER0_COMPA_vect)
{ // interrupt routine
    OCR2A = _value;
    
    _value = _buf[_bufInd];
    ++_bufInd;
    if (_bufInd >= _bufSize)
    {
        _bufInd = 0;
    }
}
