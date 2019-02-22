# USB Button GamePad
## Tiva Launchpad as USB controller
This example application turns a Tiva Launchpad into a USB game pad device using the Human Interface Device gamepad class. 

The code was developed on the Keil C compiler for Windows but should be fairly simple to port to the free Code Composer Studio 6 available from Texas Instruments. 

A binary file is available in the .\Gamepad\usb_dev_gamepad\rvmdk directory which can be programmed directly on to a Tiva Launchpad with the freely available LM Flash Programmer application available for free from Texas Instruments

In total 32 buttons and 3 analog channels are create, although the 3 analog channels are multiplexed on 3 of the digital inputs. This allows the 3 channels to be used for either analog input or digital inputs, with the result being applied to both analog and digital outputs.

In this mode the internal digital pull up resistors cannot be applied to the 3 dual purpose inputs so must be provided axternally if needed.

Alternatively the code can be compiled to only utilize digital inputs. In this case the analog channels are still created but will always return a value of zero.

In this mode the internal pull ups are applied to all digital pins and no external resistors are needed for the button channels.

The Tiva Launchpad, by default, has 2 resistors connecting Tiva B6 with D0 and also B7 with D1. To use all 32 buttons independently it is necessary to remove R9 and R10 on the Tiva Launchpad (located just below the actual Tiva on the PCB)

Once attached to a computer the Tiva Launchpad will enumerate as a RoboCats Gamepad and expose 32 buttons and 3 analog channels as shown below.

Note that the current version of the WPILib interface will only allow access to the first 16 buttons even though all 32 trigger in Driver Station.

| Controller   | Tiva IO pin |
|--------------|-------------|
| D1           | B0          |
| D2           | B1          |
| D3           | B2          |
| D4           | B3          |
| D5           | B4          |
| D6           | B5          |
| D7           | B6 <=> D0   |
| D8           | B7 <=> D1   |
| D9           | E0          |
| D10          | E1 - ADC-X  |
| D11          | E2 - ADC-Y  |
| D12          | E3 - ADC-Z  |
| D13          | E4          |
| D14          | E5          |
| D15          | D6          |
| D16          | D7          |
| D17          | A2          |
| D18          | A3          |
| D19          | A4          |
| D20          | A5          |
| D21          | A6          |
| D22          | A7          |
| D23          | F0 - SW2    |
| D24          | F4 - SW1    |
| D25          | C4          |
| D26          | C5          |
| D27          | C6          |
| D28          | C7          |
| D29          | D0 <=> B6   |
| D30          | D1 <=> B7   |
| D31          | D2          |
| D32          | D3          |

To trigger a button simply connect a button from the Tiva IO pin to ground.

The analog inputs will register -128 to +127 for an input voltage of 0V to 3.3V

## Enclosure
A customizable enclosure has been designed to house the Tiva Launchpad and buttons. The enclosure was designed in the freely available OpenSCAD application. Open SCAD is a CAD package which uses a descriptive language to describe primitives and objects rather than a traditional graphical CAD package.
The dimensions of the enclosure and button hole size and positions can easily be modified and laser cuttable DXF files generated for any custom enclosure.
