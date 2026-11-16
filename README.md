# Photoacoustic Musical Laser

Handheld and battery powered, when pointed at an object while playing, that object will begin playing music. At the time of its creation at least, it was the first handheld laser capable of playing music through the photoacoustic effect.

# Specifications
* Laser diode: NUBB24 44W 455nm diode array
* TTL and analog modulation
* Power supply: 6S 3300mAh LiPo bucked to +5V and +15V with 2x LM2596, also boosted to 40V with aliexpress boost module
* PCBs: 6 custom photoboards
* Microcontroller: STM32F103C8

# Architecture

* The laser's voltage is supplied by a TL494 boost converter at 40V. Its current is regulated by a linear regulator made of an IRF4468 mosfet and an LM358 op amp. The laser also contains an audio preamplifier. When in analog modulation mode, the audio signal is fed into the non-inverting input of the op-amp with a dc bias equal to half the maximum VPP of the audio. This is neccesary as it keeps the value of the current desired by the regulator positive, which stops the laser diode from being cut off in negative half cycles of the audio.
* The device has a switch to swap between 3 modes:
1.Manual power adjust via a potentiometer
2.Analog modulation via aux port
3.Low power mode (regulator supplies the least current it is capable of, beam is very dim. This is used for aiming.)
* A P-channel mosfet is used to cut the power to the regulator, allowing the laser to be turned fully off, since the regulator cannot regulate down to 0A, as the LM358 has a certain resolution on its inputs. The mosfet comes from a level shifted GPIO pin from the STM32, with an input to the microcontroller coming from a bush button switch for toggle on/off. The device defaults to fully off when powered on.

# Challenges

* Constant current linear regulator: A laser diode requires a constant current source to achieve precise output levels and to avoid damaging the diode. Because of the desired musical capabilities, a linear regulator was used to achieve a stable, noise free current through all modes. A suitable driver was built with the IRF4468 power mosfet, an LM358 op amp, and a 100 milliohm power resistor.
* Accurate audio modulation: There are not exactly guides on how to achieve crisp, clear audio from a laser beam. I had seen TTL-modulated lasers produce single frequency tones before, and deduced that by modulating the beam continuously instead of discretely, all sorts of tones could be reproduced. As noted above, to stop audio clipping, the audio waveform must ride on a DC bias, allowing the current to smoothly change from maximum to minimum.
* Embedded controller: Many of the functions of the device go through a microcontroller, notably the LCD screen displaying temperature, diode current, and battery voltage. There are many signals for the microcontroller to process and display, in addition to being responsible for toggling the laser on and off.
