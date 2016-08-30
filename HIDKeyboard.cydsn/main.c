/*******************************************************************************
* File Name: Main.c
* Version 1.0
*
* Description:
* Firmware for DIYGamingPad
*
* Note:
* Heavily based on AN58726
* http://www.cypress.com/documentation/application-notes/an58726-usb-hid-intermediate-psoc-3-and-psoc-5lp
*
* Their lisence applies
*/

#include <device.h>

typedef uint8(*keyReadCallback)(void);

#define EVER (;;)
#define LSHIFT 0x02 
#define ENTER 0x28 
#define CAPS 0x39
#define TAB 0x2B
#define CTRL 0x01 // 0xE0
#define ALT 0x04 // 0xE2
#define NO_KEY 0x00

#define NUMBER_OF_KEYS 26
#define NUMBER_OF_KEY_ROWS 5
#define NUMBER_OF_KEY_COLS 7

void Send_To_HOST (void);
void Receive_From_HOST (void);
uint8 Key_Is_Function_Modifier(char key);
uint8 Key_Needs_Shift(char key);

/* External variable where OUT Report data is stored */
extern uint8 USBFS_1_DEVICE0_CONFIGURATION0_INTERFACE0_ALTERNATE0_HID_OUT_BUF[USBFS_1_DEVICE0_CONFIGURATION0_INTERFACE0_ALTERNATE0_HID_OUT_BUF_SIZE];

/// The layout of your DIYGamingPad
const uint8 Keypad_Layout[NUMBER_OF_KEY_ROWS][NUMBER_OF_KEY_COLS] = {
    {'0', '1', '2', '3', '4', '5', NO_KEY},
    {TAB, 'q', 'w', 'e', 'r', 't', NO_KEY},
    {LSHIFT, 'a', 's', 'd', 'f', 'g', NO_KEY},
    {NO_KEY, 'z', 'x', 'c', 'v', 'b', NO_KEY},
    {NO_KEY, NO_KEY, NO_KEY, NO_KEY, ALT, ENTER, CTRL}
};

/// the callbacks for each key in the keypad
keyReadCallback Read_Keypad[NUMBER_OF_KEY_ROWS][NUMBER_OF_KEY_COLS] = {
    { Key_1_Read, Key_2_Read, Key_3_Read, Key_4_Read, Key_5_Read, Key_6_Read, NULL },
    { Key_7_Read, Key_8_Read, Key_9_Read, Key_10_Read, Key_11_Read, Key_12_Read, NULL },
    { Key_13_Read, Key_14_Read, Key_15_Read, Key_16_Read, Key_17_Read, Key_18_Read, NULL },
    { NULL, Key_19_Read, Key_20_Read, Key_21_Read, Key_22_Read, Key_23_Read, NULL },
    { NULL, NULL, NULL, NULL, Key_24_Read, Key_25_Read, Key_26_Read },
};

// Creates a Scan Code Look Up Table for the various ASCII values
// http://www.asciitable.com/
// http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
//
// BEWARE to modify if you have a nonregular keyboard layout, this should work for english keyboard input
const uint8 aASCII_ToScanCode[] = {0x2C, 0x1E, 0x34, 0x20, 0x21, 0x22, 0x24, 0x34, 0x26, 0x27, 0x25, 0x2E, 0x36, 
								   0x2D, 0x37, 0x38, 0x27, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 
								   0x33, 0x33, 0x36, 0x2E, 0x37, 0x38, 0x1F, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 
								   0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15 ,0x16, 
								   0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x2F, 0x31, 0x30, 0x23, 0x2D, 0x35, 
								   0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 
								   0x11, 0x12, 0x13, 0x14, 0x15 ,0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 
								   0x2F, 0x31, 0x30, 0x35, 0x4C};

static unsigned char Keyboard_Data[8] = {0, 0, 0, 0, 0, 0, 0, 0}; 
static unsigned char Out_Data[1] = {0};

static char keypadKey;
static uint8 key;
static uint8 row, col;
static uint8_t isCapsLocked = 0;
static uint8_t isNumLocked = 0;
static uint8_t isScrollLocked = 0;

void main()
{
    CYGlobalIntEnable; 
    
    DBG_LED_Write(1);
	USBFS_1_Start(0, USBFS_1_DWR_VDDD_OPERATION); //Start USBFS Operation and Device 0 and with 5V operation
	USBFS_1_EnableOutEP(2);
	while(!USBFS_1_bGetConfiguration());    // wait enumeration	
	USBFS_1_LoadInEP(1, Keyboard_Data, 8);  // start comms
    DBG_LED_Write(0);

    for EVER
    {
		/*Checks for ACK from host*/
		if(USBFS_1_bGetEPAckState(1)) 
		{
			Send_To_HOST(); 	
			Receive_From_HOST();
		}
    }
}

void Send_To_HOST(void)
{
	if(BTN_Read() == 0) 
	{
        while (BTN_Read() == 0); //wait for btn to be released
		Keyboard_Data[0] = 0x00;
		Keyboard_Data[2] = aASCII_ToScanCode['a' - 0x20];
		USBFS_1_LoadInEP(1, Keyboard_Data, 8);
		while(!USBFS_1_bGetEPAckState(1));
		Keyboard_Data[0] = 0x00; 
		Keyboard_Data[2] = 0x00; 
		USBFS_1_LoadInEP(1, Keyboard_Data, 8);
		while(!USBFS_1_bGetEPAckState(1));
	}
    Keyboard_Data[0] = 0x00; 
    for (row=0; row<NUMBER_OF_KEY_ROWS; row++) {
        for (col=0; col<NUMBER_OF_KEY_COLS; col++) {
            keypadKey = Keypad_Layout[row][col];
            if (Key_Is_Function_Modifier(keypadKey) && Read_Keypad[row][col]() == 0) {
				Keyboard_Data[0] = keypadKey; 
            }
        }
    }
    for (row=0; row<NUMBER_OF_KEY_ROWS; row++) {
        for (col=0; col<NUMBER_OF_KEY_COLS; col++) {
            keypadKey = Keypad_Layout[row][col];
            if (!Key_Is_Function_Modifier(keypadKey)) {
                if (Read_Keypad[row][col] != NULL && Read_Keypad[row][col]() == 0) {
                    DBG_LED_Write(DBG_LED_Read());
                    if (keypadKey == ENTER || keypadKey == TAB) {
    				    Keyboard_Data[2] = keypadKey; 
                    }
                    else if((keypadKey >= 0x20) && (keypadKey <= 0x7F))
    				{
    					key = keypadKey - 0x20; 
    					if(Key_Needs_Shift(key))
    					{
    						Keyboard_Data[0] = LSHIFT; 
    					}
    				    Keyboard_Data[2] = aASCII_ToScanCode[key]; 
    				}
    				USBFS_1_LoadInEP(1, Keyboard_Data, 8);
    				while(!USBFS_1_bGetEPAckState(1));
    				Keyboard_Data[0] = 0x00; 
    				Keyboard_Data[2] = 0x00; 
    				USBFS_1_LoadInEP(1, Keyboard_Data, 8);
    				while(!USBFS_1_bGetEPAckState(1));
                    DBG_LED_Write(DBG_LED_Read());
                }
            }
        }
    }
}
	
/*
    Hook onto messages and display status for things that intesterests you
*/
void Receive_From_HOST(void)
{
	/*Reads the OUT Report data */
	Out_Data[0] = USBFS_1_DEVICE0_CONFIGURATION0_INTERFACE0_ALTERNATE0_HID_OUT_BUF[0];


	/*If the Num Lock is enabled, display on LCD and LED*/
	if ((Out_Data[0] & 0x01)!= 0)
	{
        isNumLocked = 1;
		Keyboard_Data[2] = 0x00;
		USBFS_1_LoadInEP(1, Keyboard_Data, 8);
		LED1_Write(1);
	} else {
        isNumLocked = 0;
		LED1_Write(2);
	}
		
	/*If the Caps Lock is enabled, display on LCD and LED*/
	if((Out_Data[0] & 0x02)!= 0)
	{
        isCapsLocked = 1;
		Keyboard_Data[2] = 0x00;
		USBFS_1_LoadInEP(1, Keyboard_Data, 8);
		DBG_LED_Write(1);
	}else{
        isCapsLocked = 0;
		DBG_LED_Write(0);
	}
			
				
	if((Out_Data[0] & 0x04)!= 0)
	{
        isScrollLocked = 1;
		Keyboard_Data[2] = 0x00;
		USBFS_1_LoadInEP(1, Keyboard_Data, 8);
		LED2_Write(1);
	} 
	else
	{
        isScrollLocked = 0;
		LED2_Write(0);
	}
}

uint8 Key_Needs_Shift(char key) {
    return ((key >= 1) && (key <= 6)) || ((key >= 8) && (key <=11)) || (key == 26) || (key == 28) || ((key >= 30) && (key <=58)) || (key == 62) || (key == 63) || ((key >= 91) && (key <= 94));
}

uint8 Key_Is_Function_Modifier(char key){
    return key == LSHIFT || key == CTRL || key == ALT;
}
/* End of File */

