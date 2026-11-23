# Unbent: microcontroller code

Unbent is a floor dock that helps people who can't bend down put their shoes on and take them off. You step into the shoe, the dock tightens the laces, and a shoehorn lifts out of the way so you can walk off. Shoes on, no bending.

This repo holds the Arduino code that runs the dock's bench prototype:

- A **pressure sensor** under the heel detects when your foot is in.
- A **stepper motor** turns the lace reel to tighten the laces.
- A **servo** lifts the shoehorn out of the heel.

## Folders

| Folder | What it is |
| --- | --- |
| `firmware/unbent_nano/` | Main sketch. Runs the full cycle (foot in, tighten, lift) and each part on its own for testing. |
| `firmware/sensor_test/` | Small sketch that only tests the pressure sensor. |

## Hardware

- Arduino Nano 33 BLE Sense Rev2 (3.3 V board)
- 28BYJ-48 stepper motor with a ULN2003 driver board
- SG90 servo
- Force-sensitive resistor (FSR) pressure sensor
- Breadboard and jumper wires

## Wiring

| Part | Part pin | Nano pin |
| --- | --- | --- |
| Pressure sensor | leg 1 | A0 |
| Pressure sensor | leg 2 | GND |
| Servo | orange (signal) | D9 |
| Servo | red | 5V |
| Servo | brown | GND |
| ULN2003 board | IN1, IN2, IN3, IN4 | D4, D5, D6, D7 |
| ULN2003 board | + | 5V |
| ULN2003 board | − | GND |
| Stepper | white plug | ULN2003 white socket |

The sensor needs no resistor. The sketch turns on the Nano's built-in pull-up, so the sensor reads as pressed or not pressed. If you add a 10k resistor (sensor between 3.3V and A0, resistor from A0 to GND), set `HAVE_RESISTOR = true` to get a 0 to 1023 force reading instead.

Never connect 5V to A0 or any other signal pin. This board runs on 3.3 V.

### Power

The Nano's 5V pin only has power while USB is plugged in. It's fine for testing one motor at a time. To run full cycles with both motors, power the servo and the ULN2003 board from a separate 5V supply, such as a USB power bank, and connect its GND to the Nano's GND.

## Setup

1. Install the Arduino IDE.
2. In the Boards Manager, install **Arduino Mbed OS Nano Boards**.
3. Choose **Tools > Board > Arduino Nano 33 BLE**.
4. Open `firmware/unbent_nano/unbent_nano.ino` and click Upload.

If the upload fails, double-press the white reset button on the Nano and upload again. On an Apple Silicon Mac, a "bad CPU type" error means Rosetta is missing. Install it with `softwareupdate --install-rosetta`.

## Using the main sketch

Open the Serial Monitor at **115200 baud** with the line ending set to **Newline**, then type one of these commands:

| Command | What it does |
| --- | --- |
| `k` | Calibrate the sensor: empty shoe first, then with your foot in |
| `r` | Print the sensor reading for 10 seconds |
| `s1500` | Move the servo to 1500 µs (try values from 500 to 2500) |
| `d700` / `u2300` | Save the servo's DOWN / UP positions |
| `t0.25` | Turn the reel a quarter turn to tighten (`t-0.25` turns it back) |
| `c10` | Run 10 full cycles: wait for the foot, tighten, lift, wait for the foot to leave |

During `c`, the sketch prints lines starting with `CSV`. Copy them into a file to keep your test data.

If the reel turns the wrong way, change `direction = 1` to `direction = -1` near the top of the sketch.

## Using the sensor test

Upload `firmware/sensor_test/sensor_test.ino` and open the Serial Monitor at 115200 baud. Press the sensor firmly: the orange "L" LED lights up and it prints `PRESSED`. Use **Tools > Serial Plotter** to see the presses as a graph.

## Credits

Written by Shamika Tissera, with help from Claude (Anthropic).
