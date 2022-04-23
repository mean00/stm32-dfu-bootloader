/*
MIT License

Copyright (c) 2020 Avra Mitra

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <stdlib.h>
#include <ili9341_stm32_parallel8.h>

//TFT width and height default global variables
uint16_t ili_tftwidth = 320;
uint16_t ili_tftheight = 240;
#define FAKE_DELAY_COMMAND 0x55
#define ILI9341_INVERTOFF  0x20

static const uint8_t dso_resetOff[] __attribute__((used))= {
	0x01, 0,            //Soft Reset
	FAKE_DELAY_COMMAND, 150,  // .kbv will power up with ONLY reset, sleep out, display on
	0x28, 0,            //Display Off
	0x3A, 1, 0x55,      //Pixel read=565, write=565.
    0
} ;
static const uint8_t dso_wakeOn[] __attribute__((used))= {
	0x11, 0,            //Sleep Out
	FAKE_DELAY_COMMAND, 150,
	0x29, 0,            //Display On
	//additional settings
	ILI9341_INVERTOFF, 0,			// invert off
	0x36, 1, 0x48,      //Memory Access
	0xB0, 1, 0x40,      //RGB Signal [40] RCM=2
    0
} ;

extern void delay(int ms);

/**
 * Initialize the display driver
 */

static void writeCmdParam( const uint8_t cmd, int size, const uint8_t *data)
{
	_ili_write_command_8bit(cmd);
	ILI_DC_DAT;
	for(int i=0;i<size;i++)
	{
		ILI_WRITE_8BIT(data[i]);
	}
}

static void sendSequence( const uint8_t *data)
{
	while (*data ) 
        {
            uint8_t cmd = data[0];
            uint8_t len = data[1];
            data+=2;
            if (cmd == FAKE_DELAY_COMMAND) 
            {			
                delay(len);
                continue;
            }                 
            writeCmdParam(cmd, len, data);
            data += len;		
	}        
}

/**
 * Set an area for drawing on the display with start row,col and end row,col.
 * User don't need to call it usually, call it only before some functions who don't call it by default.
 * @param x1 start column address.
 * @param y1 start row address.
 * @param x2 end column address.
 * @param y2 end row address.
 */
void ili_set_address_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	uint8_t xx[4]={x1>>8,x1,x2>>8,x2};
	uint8_t yy[4]={y1>>8,y1,y2>>8,y2};

	writeCmdParam(ILI_CASET,4,xx);
	writeCmdParam(ILI_PASET,4,yy);
	_ili_write_command_8bit(ILI_RAMWR);
	return;
}


void ili_dataBegin()
{
	ILI_DC_DAT;
}
void ili_dataEnd()
{
	ILI_DC_CMD;
}
void ili_sendWords(int nb, const uint16_t *data)
{
	for(int i=0;i<nb;i++)
	{
		uint8_t color_high = data[i] >> 8;
		uint8_t color_low = data[i] ;
		ILI_WRITE_8BIT(color_high); 
		ILI_WRITE_8BIT(color_low);
	}
}

/**
 * Fills `len` number of pixels with `color`.
 * Call ili_set_address_window() before calling this function.
 * @param color 16-bit RGB565 color value
 * @param len 32-bit number of pixels
 */
void ili_fill_color(uint16_t color, uint32_t len)
{
	uint8_t color_high = color >> 8;
	uint8_t color_low = color;
	ILI_DC_DAT;	
	while(len--)
	{
			ILI_WRITE_8BIT(color_high); 
			ILI_WRITE_8BIT(color_low); 	
	}
}

/**
 * Fill the entire display (screen) with `color`
 * @param color 16-bit RGB565 color
 */
void ili_fill_screen(uint16_t color)
{
	ili_set_address_window(0, 0, ili_tftwidth - 1, ili_tftheight - 1);
	ili_fill_color(color, (uint32_t)ili_tftwidth * (uint32_t)ili_tftheight);
}



/**
 * Rotate the display clockwise or anti-clockwie set by `rotation`
 * @param rotation Type of rotation. Supported values 0, 1, 2, 3
 */
void ili_rotate_display(uint8_t rotation)
{
	/*
	* 	(uint8_t)rotation :	Rotation Type
	* 					0 : Default landscape
	* 					1 : Potrait 1
	* 					2 : Landscape 2
	* 					3 : Potrait 2
	*/

#ifdef USER_DEFAULT_PLATFORM
    uint16_t new_height = 240;
    uint16_t new_width = 320;
#elif DSO138_PLATFORM
    uint16_t new_height = 320;
    uint16_t new_width = 240;
#endif

	switch (rotation)
	{
		case 0:
			_ili_write_command_8bit(ILI_MADCTL);		//Memory Access Control
			_ili_write_data_8bit(0x40);				//MX: 1, MY: 0, MV: 0	(Landscape 1. Default)
			ili_tftheight = new_height;
			ili_tftwidth = new_width;
			break;
		case 1:
			_ili_write_command_8bit(ILI_MADCTL);		//Memory Access Control
			_ili_write_data_8bit(0x20);				//MX: 0, MY: 0, MV: 1	(Potrait 1)
			ili_tftheight = new_width;
			ili_tftwidth = new_height;
			break;
		case 2:
			_ili_write_command_8bit(ILI_MADCTL);		//Memory Access Control
			_ili_write_data_8bit(0x80);				//MX: 0, MY: 1, MV: 0	(Landscape 2)
			ili_tftheight = new_height;
			ili_tftwidth = new_width;
			break;
		case 3:
			_ili_write_command_8bit(ILI_MADCTL);		//Memory Access Control
			_ili_write_data_8bit(0xE0);				//MX: 1, MY: 1, MV: 1	(Potrait 2)
			ili_tftheight = new_width;
			ili_tftwidth = new_height;
			break;
	}
}


void ili_init()
{
	// Set gpio clock
	// MEANX 
	//ILI_CONFIG_GPIO_CLOCK();
	// Configure gpio output dir and mode
	ILI_CONFIG_GPIO();

	ILI_CS_ACTIVE;

	ILI_RST_IDLE;
	ILI_RST_ACTIVE;
	ILI_RST_IDLE;

	delay(10);
	sendSequence(dso_resetOff);
	sendSequence(dso_wakeOn);
}
