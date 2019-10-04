#define RPM_ESTIMATOR_LINEAR        0
#define RPM_ESTIMATOR_POTENTIOMETER 1
#define RPM_ESTIMATOR_HALL          2

#define RPM_ESTIMATOR_TYPE          RPM_ESTIMATOR_HALL

#ifndef RPM_ESTIMATOR_TYPE
static_assert(false, "RPM estimator type is not set");
#endif


#include <math.h>
#include <Arduino.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "RpmEstimator.h"

inline unsigned char toOutValue(float value)
{
    const int maxOutValue = 255;
    const float maxOutValueF = static_cast<float>(maxOutValue);
    int intVal = static_cast<int>(
        (value + 1.f) / 2.f * maxOutValueF + 0.5f);
    if (intVal > maxOutValue)
    {
        intVal = maxOutValue;
    }
    else if (intVal < 0)
    {
        intVal = 0;
    }
    return static_cast<unsigned char>(intVal);
}

static const float sampleRate = 10000.f;

static const float fireSoundDur = 0.05f;
static const size_t fireSoundSize = int(sampleRate * fireSoundDur + 0.5);
static unsigned char fireSound[fireSoundSize];

size_t soundIndex = 0;
size_t cycleSize = fireSoundSize;
unsigned char outValue = 0;
static const unsigned char silenceSound = toOutValue(0.f);

volatile size_t cycleSizeToUpdate = cycleSize;

RpmEstimator rpmEstimator;

void calculateFireSound()
{
    const float sinFreq = 160.f;
    const float pi2 = 2.f * 3.14159265359f;
    const float sinPhaseDelta = pi2 * sinFreq / sampleRate;
    
    const float initialExpArg = 0.f;
    const float finalExpArg = log(1E-3f);
    const float expArgDelta = (finalExpArg - initialExpArg) /
        static_cast<float>(fireSoundSize - 1);
    
    float sinPhase = 0.f;
    float expArg = initialExpArg;
    for (auto &val:  fireSound)
    {
        float value = sin(sinPhase);
        value *= exp(expArg);
        val = toOutValue(value);
        
        sinPhase += sinPhaseDelta;
        expArg += expArgDelta;
    }
}

bool setupSamplingInterrupts()
{
    const float prescaler = 8.f;
    const float cpuFreq = 16E6f;
    const float ticksPerOutF = cpuFreq / prescaler / sampleRate;
    const int ticksPerOut = static_cast<int>(ticksPerOutF + 0.5f);
    
    if (ticksPerOut < 1)
    {
        Serial.println("The chosen sample rate is too slow. "
                       "Try another prescaler or greater rate.");
        return false;
    }
    if (255 - ticksPerOut < 0)
    {
        Serial.println("Too much ticks per sampling period for "
                       "8-bit counter. Try another prescaler or less rate.");
        return false;
    }
    if (abs(static_cast<float>(ticksPerOut) - ticksPerOutF) > 1E-7)
    {
        Serial.println("CPU frequency is not dividable to "
                       "prescaler * sample rate.");
        return false;
    }
    
    cli(); // Disable interrupts
    // Set timer0 interrupt at "ticksPerOut" Hz
    TCCR0A = 0; // Set entire TCCR0A register to 0
    TCCR0B = 0; // Same for TCCR0B
    TCNT0  = 0; // Initialize counter value to 0
    // Set compare match register for "ticksPerOut" Hz increments
    OCR0A = static_cast<unsigned char>(ticksPerOut - 1);
    // Turn on CTC mode
    TCCR0A |= (1 << WGM01);
    TCCR0B |= (1 << CS01);      // Set CS01 bit for 8 prescaler
    TIMSK0 |= (1 << OCIE0A);    // Enable timer compare interrupt
    sei();  // Enable interrupts
    
    return true;
}

void setupPwm()
{
    cli(); // Disable interrupts
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
    DDRB |= (1 << DDB3);
    sei();  // Enable interrupts
}

void setup()
{
    Serial.begin(9600);
    
    calculateFireSound();
    const bool setupSuccess = setupSamplingInterrupts();
    if (!setupSuccess)
    {
        Serial.println("Setup errors occured. Cannot continue.");
        digitalWrite(LED_BUILTIN, HIGH);
        for (;;)
            ;
    }
    setupPwm();
    rpmEstimator.setup();
}

void loop()
{
    const int rpm = rpmEstimator.rpm();
    const float rps = static_cast<float>(rpm) / 60.f;
    // 4-stroke engine fires once on 2 revolutions of crankshaft.
    const float cyclesPerSec = rps / 2.f;
    const float cycleDuration = 1.f / cyclesPerSec;
    const int newCycleSize = static_cast<int>(
        cycleDuration * sampleRate + 0.5f);
    
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        cycleSizeToUpdate = newCycleSize;
    }
}

ISR(TIMER0_COMPA_vect)
{ // interrupt routine
    OCR2A = outValue;
    
    outValue = (soundIndex < fireSoundSize) ?
        fireSound[soundIndex] : silenceSound;
    ++soundIndex;
    if (soundIndex >= cycleSize)
    {
        soundIndex = 0;
        cycleSize = cycleSizeToUpdate;
    }
}
