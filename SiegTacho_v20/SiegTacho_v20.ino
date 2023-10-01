// Modified by: Nicolai Shlapunov. Redesigned receiving engine. INT0 used to catch LCDCS signal after each data frame
// as result, we can check how many bits received data frame has. If it isn't multiple of 17 (8 + 9 bits), we can reset
// receiving sequence. In this case we don't care about any additional "headers" that has less than 17 bit per data frame.
// Date Modified: Sep 2023

// Author: Andrew McWhirter based initial work by Jeff Nelson (https://macpod.net)
// Direct link to tachometer: https://macpod.net/misc/sx2_tachometer/sx2_tachometer.php

/*
  Display the tacho reading from SIEG SX2-P or SC3 lathe

  Tachometer Port Interface (looking at the socket on the machines, notch at the top)
  Physical connector is known as GX16 7 pole.
  The pin numbering can be seen moulded into the plug.

        | |
    1   \ /   6
   2     7     5
    3         4

  1 = 5V (Grey)
  2 = Ground (Pink)
  3 = 5V (Blue)
  4 = Ground (Green)
  5 = Data (White)            (labelled LCDDI)
  6 = Clock (Red)             (labelled LCDCL)
  7 = Frame indicator (Brown) (labelled LCDCS)

  Data format:
  Every 750ms seconds a packet is sent over the port.
  The data is transmitted very quickly - less than one ms

  Each packet consists of:
        On newer machines only (post ~2014): a 36 bit header consisting of 3 frames of 12 bits
        Following this are 4 data frames consisting of 17 bits each.
          The first 8 bits represent an address, and the other bits represent RPM and other data.

  Frame 0: Represents 7-segment data used for 1000's place of rpm readout.
    Address: 0xA0
    Data: First bit is always 0
      Next 7 bits indicate which of the 7-segments to light up.
      Last bit is ignored
  Frame 1: Represents 7-segment data used for 100's place of rpm readout.
    Address: 0xA1
    Data: First bit is ignored
      Next 7 bits indicate which of the 7-segments to light up.
      Last bit is ignored
  Frame 2: Represents 7-segment data used for 10's place of rpm readout.
    Address: 0xA2
    Data: First bit is always 0
      Next 7 bits indicate which of the 7-segments to light up.
      Last bit is ignored
  Frame 3: Supposedly provides additional information:
    Address: 0xA3
    Data: This is normally 0x020, but the SIEG tachos reportedly use it to show other info:
    bit         8 7654 3210
    Usually     0 0010 0000 (0x20 - bit 5 is set which mean add forth digit as 0)

    Bit 0 - Always 0 (but sometimes can be 1)
    Bit 1 - Always 0
    Bit 2 - Always 0
    Bit 3 - Always 0
    Bit 4 - Light “forward” indicating that the spindle is running forward
    Bit 5 - Light up the 4th digit as 0 (e.g., multiply speed by 10) (this is the 0x20 you found, without it you only get 3 digits on the display)
    Bit 6 - Light “tapping” (doesn’t appear as if there is any dedicated functionality for tapping in the mill; not in SX2)
    Bit 7 - Light “reverse” indicating that the spindle is running in reverse
    Bit 8 - Always 0

   SIEG address the 7-segments like this:
    --d--
   |     |
   c     g
   |     |
    --f--
   |     |
   b     e
   |     |
    --a--

  digit =  bin(abcdefg)
    0  = 1111101 = 0x7D
    1  = 0000101 = 0x05
    2  = 1101011 = 0x6B
    3  = 1001111 = 0x4F
    4  = 0010111 = 0x17
    5  = 1011110 = 0x5E
    6  = 1111110 = 0x7E
    7  = 0001101 = 0x0D
    8  = 1111111 = 0x7F
    9  = 1011111 = 0x5F

  4 digit LED display
  ===================
  We can display custom messages on the 4 digit LED
  Note the LED segments are NOT THE SAME as the Sieg definition!
    --a--
   |     |
   f     b
   |     |
    --g--
   |     |
   e     c
   |     |
    --d--   (h=decimal or colons or nothing - depends on display)
*/

// Multiplication by factor 2. If your mill hads hight speed kit
// and spindle max RPM 5000 instead 2500, uncomment next line
//#define MULTIPLY_X2

// Data pins that connect to the machine's tacho port
#define TACH_FRM 2 // INT0
#define TACH_CLK 3 // Must be PORTD
#define TACH_DAT 4 // INT1

// For the TM1367 4 Digit display
// For convenience, we'll use the MISO(12) and MOSI(11) pins that are
// available on the 6-pin header as data and clock for the display.
#define LED_CLK 12
#define LED_DIO 11

// Brightness values 1-7
#define LED_BRIGHT 7

// Lib for the 4 digit display
#include <TM1637Display.h>

const byte SEG_INIT[] =
{
  SEG_E | SEG_F,                                   // I
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_E,                                           // i
  SEG_D | SEG_E | SEG_F | SEG_G                    // t
};

const uint8_t SEG_STOP[] =
{
 SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,           // S
 SEG_D | SEG_E | SEG_F | SEG_G,                   // t
 SEG_C | SEG_D | SEG_E | SEG_F | SEG_A | SEG_B,   // O
 SEG_A | SEG_B | SEG_E | SEG_F | SEG_G            // P
};

const uint8_t SEG_DASHES[] =
{
  SEG_G,                                           // -
  SEG_G,                                           // -
  SEG_G,                                           // -
  SEG_G                                            // -
};

const uint8_t SEG_ERR[] =
{
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,           // E
  SEG_E | SEG_G,                                   // r
  SEG_E | SEG_G,                                   // r
  0x00                                             // <blank>
};

const uint8_t SEG_ERR1[] =
{
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,           // E
  SEG_E | SEG_G,                                   // r
  SEG_E | SEG_G,                                   // r
  SEG_B | SEG_C                                    // 1
};

const uint8_t SEG_ERR2[] =
{
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,           // E
  SEG_E | SEG_G,                                   // r
  SEG_E | SEG_G,                                   // r
  SEG_A | SEG_B | SEG_G | SEG_E | SEG_D            // 2
};

#define DATA 4          // Number of data frames in a packet
#define DATA_ADDRESS 8  // Data address 8 bits(A0, A1, A2 and A3)
#define DATA_LED_DATA 9 // Data bits(only lower 8 bits processed in this program)
#define DATA_FRAME (DATA_ADDRESS + DATA_LED_DATA) // Data frame size(address size + data size)

// Packet length. This length should contain ONLY data packet that contain tacho data - no additional headers length needed.
#define PKT_LEN (DATA * DATA_FRAME)

// Index of received bit number
volatile uint8_t data_idx = 0u;
// We will read PORTD into the data buffer, even though only 1 bit (data) is needed
volatile uint8_t data_samples[PKT_LEN*2u] = {0}; // Twice data - just in case if INT0 will be missing once
// Flag indicates that data received
volatile uint8_t data_received = false;

// 4 digit LED display
TM1637Display display(LED_CLK, LED_DIO);

// *****************************************************************************
// ***   Setup loop   **********************************************************
// *****************************************************************************
void setup()
{
  // The only setup we need for the LED display is brightness
  display.setBrightness(LED_BRIGHT);
  display.setSegments(SEG_INIT);

  // Setup pins
  pinMode(TACH_FRM, INPUT_PULLUP);
  pinMode(TACH_CLK, INPUT_PULLUP);
  pinMode(TACH_DAT, INPUT_PULLUP);

  // TACH_FRM must be on pin 2, data pin needs to be somewhere else on pins 0-1 or 4-7
  // Trigger on rising edge of TACH_FRM
  EICRA |= (1 << ISC01);
  EICRA |= (1 << ISC00);

  // TACH_CLK must be on pin 3, data pin needs to be somewhere else on pins 0-1 or 4-7
  // Trigger on falling edge of TACH_CLK
  EICRA |= (1 << ISC11);
  EICRA &= ~(1 << ISC10);

  // Disable timer 0 else it interferes with the clock interrupt/data collection
  TIMSK0 &= ~(1 << TOIE0); // Disable timer0
}

// *****************************************************************************
// ***   Main loop   ***********************************************************
// *****************************************************************************
void loop()
{
  // Clear flag to wait new data
  data_received = false;
  // Enable interrupts on INT0 & INT1
  EIMSK |= ((1 << INT1) | (1 << INT0));

  // Wait until we have the full packet
  while (data_received != true)
  {
    asm("nop");
  }

  // Convert the received data frames to digits
  int16_t rpm = get_rpm();

  //  Update the display
  if (rpm == -1) // Err1 - address error
  {
    display.setSegments(SEG_ERR1);
  }
  else if (rpm == -2) // Err2 - data error
  {
    display.setSegments(SEG_ERR2);
  } 
  else if (rpm < 0) // Err - should not happen... yet
  {
    display.setSegments(SEG_ERR);
  } 
  else if (rpm == 0) // Zero RPM - show STOP text
  {
    //display.setSegments(SEG_STOP);
    display.setSegments(SEG_DASHES);
  } 
  else
  {
  #if defined(MULTIPLY_X2)
    // Multiply if needed
    rpm *= 2;
  #endif
    // Show RPM on display
    display.showNumberDec(rpm, false);
  }
}

// *****************************************************************************
// ***   Read the data frames and turn into an RPM reading   *******************
// *****************************************************************************
int16_t get_rpm()
{
  uint8_t tmp_dat = 0u;
  uint8_t tmp_digit = 0u;
  int16_t ret = 0;

  for (uint8_t frm = 0; frm < 4u; frm++)
  {
    // Check if address correct for received byte
    if (build_address(frm) != (0xA0 + frm))
    {
      ret = -1;
      break;
    }
    else
    {
      // Get data
      tmp_dat = build_data(frm);
      // For the first three frames
      if (frm < 3)
      {
        // We convert to a digit the data from the fourth frame is not used
        tmp_digit = get_digit_from_byte(tmp_dat);
        // Check if convertation is successful
        if (tmp_digit != 0xFF)
        {
          // Add digit to number
          ret = ret * 10 + tmp_digit;
        }
        else
        {
          ret = -2;
          break;
        }
      } // For the last frame
      else
      {
        // Check if multiplication flag is set and multiply number
        if(tmp_dat & 0x20)
        {
          ret = ret * 10;
        }
      }
    }
  }

  return ret;
}

// *****************************************************************************
// ***   Frame address is 8 bits long - just get the applicable byte   *********
// *****************************************************************************
uint8_t build_address(uint8_t frame_num)
{
  return get_byte(DATA_FRAME * frame_num);
}

// *****************************************************************************
// ***   Get data by frame number   ********************************************
// *****************************************************************************
// LED data is 9 bits long; the first bit is always 0
// The last bit isn't used (except for reportedly indicating stopped...which is 0 RPM anyway)
// So just get the first 8 data bits into a byte
uint8_t build_data(uint8_t frame_num)
{
  // Build data using only 8 last bits
  return get_byte((DATA_FRAME * frame_num) + DATA_ADDRESS + (DATA_LED_DATA - 8u));
}

// *****************************************************************************
// ***   Rebuild data byte from data stream   **********************************
// *****************************************************************************
uint8_t get_byte(byte start)
{
  uint8_t ret = 0;
  // Create data byte
  for (uint8_t i = start; i < (start + 8u); i++)
  {
    ret = (ret << 1) | ((data_samples[i] & (1u << TACH_DAT)) ? 1u : 0u);
  }
  return ret;
}

// *****************************************************************************
// ***   Convert digit from 7 segment to decimal   *****************************
// *****************************************************************************
byte get_digit_from_byte(byte data)
{
  byte ret = 0;
  // Clear last bit
  data &= 0b11111110;
  // Find digit
  switch (data)
  {
    case 0xFA:
      ret = 0;
      break;
    case 0x0A:
      ret = 1;
      break;
    case 0xD6:
      ret = 2;
      break;
    case 0x9E:
      ret = 3;
      break;
    case 0x2E:
      ret = 4;
      break;
    case 0xBC:
      ret = 5;
      break;
    case 0xFC:
      ret = 6;
      break;
    case 0x1A:
      ret = 7;
      break;
    case 0xFE:
      ret = 8;
      break;
    case 0xBE:
      ret = 9;
      break;
    default:
      ret = 0xFF;
      break;
  }
  return ret;
}

// *****************************************************************************
// ***   LCDCS signal inerrupt. Used to receive bytes.   ***********************
// *****************************************************************************
SIGNAL(INT0_vect)
{
  // Check data size. This code should looks like:
  // if((data_idx % DATA_FRAME) == 0u)
  // Unfortenatly AVR doesn't have HW divider and SW is probably too slow.
  if( (data_idx ==       DATA_FRAME)  ||
      (data_idx == (2u * DATA_FRAME)) ||
      (data_idx == (3u * DATA_FRAME)) )
  {
    ;// All good - do nothing
  }
  else if(data_idx == (DATA * DATA_FRAME)) // If it is the last data byte
  {
    // Clear data size
    data_idx = 0u;
    // Set flag that data is received
    data_received = true;
    // Disable the interrupts while we process the data
    EIMSK &= ~((1 << INT1) | (1 << INT0));
  }
  else // Received unexpected number of bits in data frame
  {
    // Clear data index to restart receive procedure
    data_idx = 0u;
  }
}

// *****************************************************************************
// ***   LCDCL signal inerrupt. Used to receive bits.   ************************
// *****************************************************************************
SIGNAL(INT1_vect)
{
  // Using PIND, assumes the data is on one of pins 0-7
  data_samples[data_idx++] = PIND;  // read PORTD
}
