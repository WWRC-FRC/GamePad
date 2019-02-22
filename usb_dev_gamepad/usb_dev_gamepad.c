//#define USE_ADC

//D1	B0
//D2	B1
//D3	B2
//D4	B3
//D5	B4
//D6	B5
//D7	B6 <=> D0
//D8	B7 <=> D1
//D9	E0
//D10	E1 - ADC-X
//D11	E2 - ADC-Y
//D12	E3 - ADC-Z
//D13	E4
//D14	E5
//D15	D6
//D16	D7
//D17	A2
//D18	A3
//D19	A4
//D20	A5
//D21	A6
//D22	A7
//D23	F0 - SW2
//D24	F4 - SW1
//D25	C4
//D26	C5
//D27	C6
//D28	C7
//D29	D0 <=> B6
//D30	D1 <=> B7
//D31	D2
//D32	D3

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/adc.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidgamepad.h"
#include "usb_gamepad_structs.h"
#include "buttons.h"
#include "utils/uartstdio.h"

#define SYSTICKS_PER_SECOND 1000
#define SYSTICK_PERIOD_MS (1000 / SYSTICKS_PER_SECOND)

uint32_t ButtonStates = 0;

//*****************************************************************************
//
// The HID gamepad report that is returned to the host.
//
//*****************************************************************************
static tGamepadReport sReport;

//*****************************************************************************
//
// The HID gamepad polled ADC data for the X/Y/Z coordinates.
//
//*****************************************************************************
static uint32_t g_pui32ADCData[3];

//*****************************************************************************
//
// An activity counter to slow the LED blink down to a visible rate.
//
//*****************************************************************************
static uint32_t g_ui32Updates;

//*****************************************************************************
//
// This enumeration holds the various states that the gamepad can be in during
// normal operation.
//
//*****************************************************************************
volatile enum
{
    //
    // Not yet configured.
    //
    eStateNotConfigured,

    //
    // Connected and not waiting on data to be sent.
    //
    eStateIdle,

    //
    // Suspended.
    //
    eStateSuspend,

    //
    // Connected and waiting on data to be sent out.
    //
    eStateSending
}
g_iGamepadState;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// Macro used to convert the 12-bit unsigned values to an eight bit signed
// value returned in the HID report.  This maps the values from the ADC that
// range from 0 to 2047 over to 127 to -128.
//
//*****************************************************************************
#define Convert8Bit(ui32Value)  ((int8_t)((0x7ff - ui32Value) >> 4))

//*****************************************************************************
//
// Handles asynchronous events from the HID gamepad driver.
//
// \param pvCBData is the event callback pointer provided during
// USBDHIDGamepadInit().  This is a pointer to our gamepad device structure
// (&g_sGamepadDevice).
// \param ui32Event identifies the event we are being called back for.
// \param ui32MsgData is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the HID gamepad driver to inform the application
// of particular asynchronous events related to operation of the gamepad HID
// device.
//
// \return Returns 0 in all cases.
//
//*****************************************************************************
uint32_t
GamepadHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgData,
               void *pvMsgData)
{
    switch (ui32Event)
    {
        //
        // The host has connected to us and configured the device.
        //
        case USB_EVENT_CONNECTED:
        {
            g_iGamepadState = eStateIdle;

            break;
        }

        //
        // The host has disconnected from us.
        //
        case USB_EVENT_DISCONNECTED:
        {
            g_iGamepadState = eStateNotConfigured;


            break;
        }

        //
        // This event occurs every time the host acknowledges transmission
        // of a report.  It is to return to the idle state so that a new report
        // can be sent to the host.
        //
        case USB_EVENT_TX_COMPLETE:
        {
            //
            // Enter the idle state since we finished sending something.
            //
            g_iGamepadState = eStateIdle;

            ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);//Clear  USB activity LED

            break;
        }

        //
        // This event indicates that the host has suspended the USB bus.
        //
        case USB_EVENT_SUSPEND:
        {
            //
            // Go to the suspended state.
            //
            g_iGamepadState = eStateSuspend;

            ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);//Clear  USB activity LED

            break;
        }

        //
        // This event signals that the host has resumed signaling on the bus.
        //
        case USB_EVENT_RESUME:
        {
            //
            // Go back to the idle state.
            //
            g_iGamepadState = eStateIdle;

            break;
        }

        //
        // Return the pointer to the current report.  This call is
        // rarely if ever made, but is required by the USB HID
        // specification.
        //
        case USBD_HID_EVENT_GET_REPORT:
        {
            *(void **)pvMsgData = (void *)&sReport;

            break;
        }

        //
        // We ignore all other events.
        //
        default:
        {
            break;
        }
    }

    return(0);
}

//*****************************************************************************
//
// Initialize the ADC inputs used by the game pad device.  This example uses
// the ADC pins on Port E pins 1, 2, and 3(AIN0-2).
//
//*****************************************************************************
void
ADCInit(void)
{
    int32_t ui32Chan;

    //
    // Enable the GPIOs and the ADC used by this example.
    //
    //SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOE);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_ADC0);

    //
    // Select the external reference for greatest accuracy.
    //
    ROM_ADCReferenceSet(ADC0_BASE, ADC_REF_EXT_3V);

    //
    // Configure the pins which are used as analog inputs.
    //
    ROM_GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_2 |
                                            GPIO_PIN_1);

    //
    // Configure the sequencer for 3 steps.
    //
    for(ui32Chan = 0; ui32Chan < 2; ui32Chan++)
    {
        //
        // Configure the sequence step
        //
        ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, ui32Chan, ui32Chan);
    }

    ROM_ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_CTL_CH2 | ADC_CTL_IE |
                                                  ADC_CTL_END);
    //
    // Enable the sequence but do not start it yet.
    //
    ROM_ADCSequenceEnable(ADC0_BASE, 0);
}


//--------------------------------------------------------------------------------------------------
// Init_SystemInit
//--------------------------------------------------------------------------------------------------
void Init_SystemInit()
{
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    ROM_FPULazyStackingEnable();

    // Set the clocking to run from the PLL at 80MHz
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

//    // Enable the system tick.
//    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / SYSTICKS_PER_SECOND);
//    ROM_SysTickIntEnable();
//    ROM_SysTickEnable();
}


//--------------------------------------------------------------------------------------------------
// ConfigurePins
//--------------------------------------------------------------------------------------------------
void ConfigurePins(void)
{
    //
    // Enable the GPIO pin for the LEDs (PF1,2,3).
    //
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

}

uint32_t GetButtons()
{
    uint8_t A;
    uint8_t B;
    uint8_t C;
    uint8_t D;
    uint8_t E;
    uint8_t F;
    
    A = ROM_GPIOPinRead(GPIO_PORTA_BASE, 0xfc); //Bits 765432--
    B = ROM_GPIOPinRead(GPIO_PORTB_BASE, 0xff); //Bits 76543210
    C = ROM_GPIOPinRead(GPIO_PORTC_BASE, 0xf0); //Bits 7654----
    D = ROM_GPIOPinRead(GPIO_PORTD_BASE, 0xcf); //Bits 76--3210
#ifdef USE_ADC
    E = ROM_GPIOPinRead(GPIO_PORTE_BASE, 0x31); //Bits --54---0
#else
    E = ROM_GPIOPinRead(GPIO_PORTE_BASE, 0x3f); //Bits --543210
#endif
    
    F = ROM_GPIOPinRead(GPIO_PORTF_BASE, 0x1f); //Bits ---43210
    
    //Now merge in ADC readings as fake buttons
    if (sReport.i8XPos > 0)
        E = E | 0x2;
    if (sReport.i8YPos > 0)
        E = E | 0x4;
    if (sReport.i8ZPos > 0)
        E = E | 0x8;
    
    return ~(((D & 0x0f) << 28) | (C << 20) | ((F & 0x10) << 19) | ((F & 1) << 22) | (A << 14) | ((D & 0xc0) << 8) | (E << 8) | B);
}

void ButtonsInit()
{
    ROM_GPIODirModeSet(GPIO_PORTA_BASE, 0xfc, GPIO_DIR_MODE_IN);
    ROM_GPIODirModeSet(GPIO_PORTB_BASE, 0xff, GPIO_DIR_MODE_IN);
    ROM_GPIODirModeSet(GPIO_PORTC_BASE, 0xf0, GPIO_DIR_MODE_IN);
    ROM_GPIODirModeSet(GPIO_PORTD_BASE, 0xcf, GPIO_DIR_MODE_IN);
#ifdef USE_ADC    
    ROM_GPIODirModeSet(GPIO_PORTE_BASE, 0x31, GPIO_DIR_MODE_IN);//ADC on 1, 2, 3. Digitize them for dual purpose
#else
    ROM_GPIODirModeSet(GPIO_PORTE_BASE, 0x3f, GPIO_DIR_MODE_IN);
#endif
    
    ROM_GPIODirModeSet(GPIO_PORTF_BASE, 0x11, GPIO_DIR_MODE_IN);
    
    ROM_GPIOPadConfigSet(GPIO_PORTA_BASE, 0xfc, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    ROM_GPIOPadConfigSet(GPIO_PORTB_BASE, 0xff, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    ROM_GPIOPadConfigSet(GPIO_PORTC_BASE, 0xf0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    ROM_GPIOPadConfigSet(GPIO_PORTD_BASE, 0xcf, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
#ifdef USE_ADC    
    ROM_GPIOPadConfigSet(GPIO_PORTE_BASE, 0x31, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
#else
    ROM_GPIOPadConfigSet(GPIO_PORTE_BASE, 0x3F, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
#endif
    
    ROM_GPIOPadConfigSet(GPIO_PORTF_BASE, 0x11, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)));
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)));
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC)));
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD)));
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)));
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)));
    
    //Set the initial state as per the buttons currently inverted toforce an initial update
    ButtonStates = ~GetButtons();
}

void USBHIDInit()
{
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    
//    SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOD);
//    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTD_AHB_BASE, GPIO_PIN_4 | GPIO_PIN_5);
    
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTD_BASE, GPIO_PIN_5 | GPIO_PIN_4);
    

    //
    // Set the USB stack mode to Device mode.
    //
    USBStackModeSet(0, eUSBModeForceDevice, 0);

    //
    // Pass the device information to the USB library and place the device
    // on the bus.
    //
    USBDHIDGamepadInit(0, &g_sGamepadDevice);
}

//--------------------------------------------------------------------------------------------------
// Init_PeripheralInit
//--------------------------------------------------------------------------------------------------
void Init_PeripheralInit(void)
{
    //
    // Enable the peripherals
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  
    //Unlock the GPIO inputs which are locked
    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTD_BASE + GPIO_O_CR) |= GPIO_PIN_7;

    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= GPIO_PIN_0;

    ConfigurePins();

    //
    // Configure the GPIOS for the buttons.
    //
    ButtonsInit();

#ifdef USE_ADC
    //
    // Initialize the ADC channels.
    //
    ADCInit();
#endif

    USBHIDInit();
}

void Test2()
{
    volatile uint8_t D;

    ROM_FPULazyStackingEnable();

    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

//    //Unlock the GPIO inputs which are locked
//    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
//    HWREG(GPIO_PORTD_BASE + GPIO_O_CR) |= GPIO_PIN_7;
    
    GPIOPinTypeGPIOInput (GPIO_PORTE_BASE, 0x31);
    ROM_GPIODirModeSet(GPIO_PORTE_BASE, 0x31, GPIO_DIR_MODE_IN);
    ROM_GPIOPadConfigSet(GPIO_PORTE_BASE, 0x31, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)));
    
    ADCInit();

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    //SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOD);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);
    
    while(1)
    {
        D = ROM_GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_0);
    }
    
}

void Test()
{
    volatile uint8_t D;

    ROM_FPULazyStackingEnable();

    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    //Unlock the GPIO inputs which are locked
    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTD_BASE + GPIO_O_CR) |= GPIO_PIN_7;
    
    ROM_GPIODirModeSet(GPIO_PORTD_BASE, 0xcf, GPIO_DIR_MODE_IN);
    ROM_GPIOPadConfigSet(GPIO_PORTD_BASE, 0xcf, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD)));

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTD_BASE, GPIO_PIN_5 | GPIO_PIN_4);

    GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, 0xcf);
    
    while(1)
    {
        D = ROM_GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_0);
    }
    
}


//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************


int
main(void)
{
    bool bUpdate;
    uint8_t ActiveToggle = 0;
    uint32_t ToggleCounter = 0;

//    Test();

    Init_SystemInit();

    Init_PeripheralInit();

    //
    // Not configured initially.
    //
    g_iGamepadState = eStateNotConfigured;


    //
    // Zero out the initial report.
    //
    sReport.ui32Buttons = 0;
    sReport.i8XPos = 0;
    sReport.i8YPos = 0;
    sReport.i8ZPos = 0;

#ifdef USE_ADC
    //
    // Trigger an initial ADC sequence.
    //
    ADCProcessorTrigger(ADC0_BASE, 0);
#endif

    //
    // The main loop starts here.  We begin by waiting for a host connection
    // then drop into the main gamepad handling section.  If the host
    // disconnects, we return to the top and wait for a new connection.
    //
    while(1)
    {
        //
        // Limit the blink rate of the ALIVE LED.
        //
        if(ToggleCounter++ == 200000)
        {
            ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, ActiveToggle);
            ActiveToggle ^= GPIO_PIN_3;

            //
            // Reset the update count.
            //
            ToggleCounter = 0;
        }

        //
        // Wait here until USB device is connected to a host.
        //
        if(g_iGamepadState == eStateIdle)
        {
            //
            // No update by default.
            //
            bUpdate = false;

            //
            // See if the buttons updated.
            // Note, no debounce at the moment
            //

            sReport.ui32Buttons = GetButtons();
            if (ButtonStates != sReport.ui32Buttons)
            {
                ButtonStates = sReport.ui32Buttons;
                bUpdate = true;
            }

#ifdef USE_ADC
            //
            // See if the ADC updated.
            //
            if(ADCIntStatus(ADC0_BASE, 0, false) != 0)
            {
                //
                // Clear the ADC interrupt.
                //
                ADCIntClear(ADC0_BASE, 0);

                //
                // Read the data and trigger a new sample request.
                //
                ADCSequenceDataGet(ADC0_BASE, 0, &g_pui32ADCData[0]);
                ADCProcessorTrigger(ADC0_BASE, 0);

                //
                // Update the report.
                //
                sReport.i8XPos = Convert8Bit(g_pui32ADCData[0]);
                sReport.i8YPos = Convert8Bit(g_pui32ADCData[1]);
                sReport.i8ZPos = Convert8Bit(g_pui32ADCData[2]);
                bUpdate = true;
            }
#endif
            //
            // Send the report if there was an update.
            //
            if(bUpdate)
            {
                USBDHIDGamepadSendReport(&g_sGamepadDevice, &sReport,
                                         sizeof(sReport));

                //
                // Now sending data but protect this from an interrupt since
                // it can change in interrupt context as well.
                //
                IntMasterDisable();
                g_iGamepadState = eStateSending;
                IntMasterEnable();

                //
                // Limit the blink rate of the USB ACTIVE LED.
                //
                if(g_ui32Updates++ == 200)
                {
                    ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);

                    //
                    // Reset the update count.
                    //
                    g_ui32Updates = 0;
                }
            }
        }
    }
}

