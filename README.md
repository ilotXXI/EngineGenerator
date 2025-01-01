# The device purpose
In recent years, there is a lot of mechanical and electro-mechanical vehicles
which work very silently, e.g. usual and electro-bicycles, electro-kickers,
electro-unicycles and so on. It's a great property in general, but becomes
dangerous for busy cities where infrastructure is not suitable for such
transport. Riders have to use sidewalks there, but surely pedestrians don't
expect fast heavy machines to move nearby. The silence plays a bad role when
such vehicles slink from back.

In this project, an audio device to generate an internal combustion engine
sound is suggested. It's sound should be more or less similar to the sound of a
coming up car, familiar to everyone in city environment. Also, it depends on
the vehicle speed to help people with velocity estimation.

# The generation idea
The idea of how to generate the sound is got from [here](https://github.com/jgardner8/engine-sound-simulator).
In short, we emulate cylinder fire once per cycle. As we emulate a car
four-stroke engine, one cycle includes two emulated crankshaft revolutions. The
sound is primarily, but not fully, defined by the crankshaft revolutions per
minute (RPM).

Another important part of the sound forming is one fire sound. It should be a
"hitting" sound with high amplitude in beginning and relatively fast it's drop
to the end. In general, any wave sample may be used for it, e.g. a recording of
a stick hitting metal part. But for simplicity, here we used
just `exp(a t) sin(f t)` function from the above-mentioned project, where `t`
is time since the fire start, `a` is exponent scaler to define amplitude drop
speed, `f` is the frequency of the hit sound.

The parameters were picked during playing with the mentioned project.

# Audio output
To make an audio output, a digital approach was used, which is related to so
called D-class digital-to-analog converters (DAC). It is just a pulse-width
modulation (PWM) on a frequency, depending on output sample magnitude, with
following analogues filtration by simple RC filter.

Another approach with R2R ladder was tried out with no success, as it's
difficult to provide good levelling in home conditions. It worked, but with
unpredictable behavior of the output.

Since the audio output is converted to analogues, it may be connected to any
audio device with amplifier, e.g. speaker or computer audio input.

# Hardware
Arduino Nano with ATMega168/ATMega328 was used as a microprocessor for this
project. Only few advantages of the Arduino libraries are used, because work
with sound requires quite high frequency of operations and active interrupts
usage, and high level functions couldn't provide it. Nevertheless, the board is
still very popular and may be used by anyone.

To estimate RPM, any of following methods is available.
 1. Linearly changing RPM. It requires no additional devices, but provides the
    least control and is used for debug only. RPM linearly increases in time to
    some limit, then decreases back. Then the cycle is repeated.
 2. Potentiometer. Connect a potentiometer to control RPM by changing it's
    resistance.
 3. Hall sensor. The target use-case: connect a digital Hall sensor, attach it
    to your vehicle static part near wheel. Attach a strong enough magnet to the
    wheel in a way that the sensor can detect it being nearby and far away. The
    wheel RPM is estimated by the magnet and the sensor, then recalculated to
    a car engine RPM. The RPM mapping is done to provide some reasonable
    relation between the vehicle speed and desired sound.

To assemble the device, please refer to the Fritzing scheme in `Schemes/Main.fzz`.

# How can I try it?
 1. Assemble the device by the scheme in `Schemes/Main.fzz`. Please note following:
    * the potentiometer R4 is optional, it is needed only for debugging;
    * the potentiometer R2 is optional too, it regulates volume;
    * U2 is any almost digital Hall sensor, not a temperature sensor as
      displayed on the scheme (found no suitable details in Fritzing, sorry); it
      is also optional if you want only to play with the sound and not use it on
      real vehicle.
 2. Set the macro `RPM_ESTIMATOR_TYPE` in `EngineGenerator.ino` to one of
    `RPM_ESTIMATOR_LINEAR`, `RPM_ESTIMATOR_POTENTIOMETER`, `RPM_ESTIMATOR_HALL`,
    see the caused effects in Hardware section. The default is
    `RPM_ESTIMATOR_HALL`. It defines a class to use statically instead of real
    inheritance to avoid excessive code compilation.
 3. Compile it with Arduino IDE, flash the microprocessor with result.
 4. Try out.
 5. If you are not satisfied with the sound, you have to play with RPM
    estimators parameters. Also, you can improve the fire sound, e.g. load
    some realistic sample as a static array.
