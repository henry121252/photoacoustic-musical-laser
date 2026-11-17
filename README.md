# Photoacoustic Musical Laser

*V1.0 completed April 2026*

Handheld and battery powered, when pointed at an object while playing, that object will begin playing music. At the time of its creation in April of 2026, it was the first handheld laser capable of playing music through the photoacoustic effect.

<p>
  <img src="IMG_0697 (1).jpg" width="400" alt="Front of the finished laser">
  <img src="IMG_0698 (1).jpg" width="400" alt="Rear view of the finished laser, showcasing the user interface">
  <img src="IMG_0685 (1).jpg" width="400" alt="Inner circuitry of the laser">
</p>

https://github.com/user-attachments/assets/083f9dec-a40b-4af8-b077-9969d5db2595

## Specifications

* **Laser diode:** NUBB24 44W 455nm diode array
* TTL and analog modulation
* **Power supply:** 6S 3300mAh LiPo nominal voltage 22.2V bucked to +5V and +15V as well as an adjustable supply for the main cooling fan with 3x LM2596, also boosted to 40V with aliexpress boost module
* **PCBs:** 6 custom photoboards
* **Microcontroller:** STM32F103C8

## Architecture

<img src="Screenshot 2026-04-21 232331 (2).png" width="800" alt="Block circuit diagram">

* The laser's voltage is supplied by a TL494 boost converter at 40V. Its current is regulated by a linear regulator made of an IRF4468 mosfet and an LM358 op amp. The laser also contains an audio preamplifier. When in analog modulation mode, the audio signal is fed into the non-inverting input of the op-amp with a dc bias equal to half the maximum VPP of the audio. This is neccesary as it keeps the value of the current desired by the regulator positive, which stops the laser diode from being cut off in negative half cycles of the audio.

<img src="Screen Recording 2026-11-16 192127.gif" width="500" alt="Regulator demo">

* The device has a switch to swap between 3 modes:
  1. Manual power adjust via a potentiometer
  2. Analog modulation via aux port
  3. Low power mode (regulator supplies the least current it is capable of, beam is very dim. This is used for aiming.)
* A P-channel mosfet is used to cut the power to the regulator, allowing the laser to be turned fully off, since the regulator cannot regulate down to 0A, as the LM358 has a certain resolution on its inputs. The mosfet comes from a level shifted GPIO pin from the STM32, with an input to the microcontroller coming from a bush button switch for toggle on/off. The device defaults to fully off when powered on.

## Challenges

* **Constant current linear regulator:** A laser diode requires a constant current source to achieve precise output levels and to avoid damaging the diode. Because of the desired musical capabilities, a linear regulator was used to achieve a stable, noise free current through all modes. A suitable driver was built with the IRF4468 power mosfet, an LM358 op amp, and a 100 milliohm power resistor.
* **Accurate audio modulation:** There are not exactly guides on how to achieve crisp, clear audio from a laser beam. I had seen TTL-modulated lasers produce single frequency tones before, and deduced that by modulating the beam continuously instead of discretely, all sorts of tones could be reproduced. As noted above, to stop audio clipping, the audio waveform must ride on a DC bias, allowing the current to smoothly change from maximum to minimum.
* **Embedded controller:** Many of the functions of the device go through a microcontroller, notably the LCD screen displaying temperature, diode current, and battery voltage. There are many signals for the microcontroller to process and display, in addition to being responsible for toggling the laser on and off.
* **Thermal performance:** 
  1. The NUBB24 dissipates around 50 watts of heat at full power and must run cool enough to be mounted on 3D printed parts. I used a large copper heatsink approximately 90x65x42mm and a large cooling fan with an adjustable power supply 0-Vbat (22.2V nominal)
  2. The linear regulator drops the 40V from the boost converter to 36V while running 3A through at full power. This translates to 12W of heat, and requires significant cooling as well. I had to mill down a spare heatsink i retrieved from a broken amplifier. That in combination with the 2x 40mm 24V cooling fans at the front of the compartment provided ample cooling.

## Experiments

To maximize audio output at **455 nm**, an ideal photoacoustic target needs:
* **High optical absorption at 455 nm** to convert as much beam energy as possible into heat
* **Low heat capacity & high coefficient of thermal expansion** to maximize rapid surface/air pressure changes per modulation cycle

Wood was originally used for quick baseline tests. Once the laser was complete, multiple candidate targets were tested—mostly coated or mixed with **candle soot** due to its near-black optical absorption and very low heat capacity—to find the best "photoacoustic transducer."

### Materials Tested

| Target Material | Preparation | Observed Performance |
| :--- | :--- | :--- |
| **Copper plate (1 mm)** | Coated with candle soot | **Loudest and clearest overall** |
| **Glass** | Coated with candle soot | Better than aluminum foil, worse than 1 mm copper |
| **Aluminum foil** | Coated with candle soot | Very high volume (membrane behavior; beaten by rigid targets) |
| **Polyamide** | Coated with candle soot | Very high volume (membrane behavior; beaten by rigid targets) |
| **Silicone** | Mixed with candle soot | Quiet overall; burned under the intense light |
| **Wood** | Uncoated | Initial baseline used for quick testing |

### Key Findings

* **Test Method:** A Python script on a laptop generated a **0 Hz – 20 kHz** tone sweep fed directly into the laser's **AUX-in** port while logging acoustic gain over time for each target. *(The hand-drawn ink annotation on the Glass plot marks the point where the glass broke during testing.)*

<p>
  <img src="Screenshot 2026-11-16 213938.png" width="400" alt="Copper gain">
  <img src="Screenshot 2026-11-16 213915.png" width="400" alt="Glass gain">
  <img src="Screenshot 2026-11-16 213951.png" width="400" alt="Aluminum foil gain">
  <img src="Screenshot 2026-11-16 214007.png" width="400" alt="Polyamide gain">
  <img src="Screenshot 2026-11-16 214023.png" width="400" alt="Wood gain">
</p>

* **Best Overall Transducer (`1 mm` Copper + Candle Soot):** A 1 mm copper plate coated in candle soot produced the loudest, clearest audio across all tested materials.
* **Rigid Surfaces vs. Membranes:** Membrane-like materials (aluminum foil and polyamide coated in candle soot) were tested on the hypothesis that their low mass would allow them to behave like a drum. While they yielded some of the highest raw volumes, they were outperformed by rigid materials (such as glass and copper)—likely because rigid surfaces provide stronger acoustic directivity, acting as a reflector that concentrates sound waves back into the forward hemisphere toward the listener.
* **Silicone Failure Mode:** Silicone mixed with candle soot was tested for its high coefficient of thermal expansion, but failed because it burned under the intense light and was quiet overall.
