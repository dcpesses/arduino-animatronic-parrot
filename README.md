# Arduino Animatronic Parrot

Create your own talking animatronic parrot! Uses a Wii nunchuck to control body movement 
via servo motors as well as a MSGEQ7 chip to convert any audio source to beak movement.  

This is still a work in progress, so please bear with me on the lack of details.

## Parts & Components Needed
- Arduino Uno R3
- Pololu Micro Maestro 6-Channel USB Servo Controller
- Nintendo Wii Nunchuck
- WiiChuck Adapter
- MSGEQ7 IC (See https://github.com/NicoHood/MSGEQ7 for required components & breadboard setup)
- 4+ Turnigy TGY-9018MG (or TGY-9025MG) Metal Gear Servo motors
- LM8UU Linear Ball Bearing (8mm)
- Great Planes Threaded 2-56 Ball Link Set (Set of 6)
- XTC-3D 
- Misc hardware

## Arduino Sketches
- AnimatronicParrot-v1 - Controls the beak and body movement using only the Wii nunchuck.
- AnimatronicParrot-v2 - Refactor of above sketch, with beak movement derived from MSGEQ7 values

