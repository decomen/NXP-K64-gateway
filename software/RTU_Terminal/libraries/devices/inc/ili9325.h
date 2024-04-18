#ifndef __ILI9325_H__
#define __ILI9325_H__

#include <stdint.h>

//FlexBus���߶���
#define ILI9325_BASE                    (0x70000000)
#define ILI9325_COMMAND_ADDRESS         *(unsigned short *)0x70000000
#define ILI9325_DATA_ADDRESS            *(unsigned short *)0x78000000

//д���ݣ�д����
#define WMLCDCOM(cmd)	   {ILI9325_COMMAND_ADDRESS = cmd;}
#define WMLCDDATA(data)	   {ILI9325_DATA_ADDRESS = data;}

#define LCD_X_MAX   (240)
#define LCD_Y_MAX   (320)

//������ɫ
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000	  
#define BLUE         	 0x001F  
#define BRED             0XF81F
#define GRED 			 0XFFE0
#define GBLUE			 0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			 0XBC40 //��ɫ
#define BRRED 			 0XFC07 //�غ�ɫ
#define GRAY  			 0X8430 //��ɫ
//GUI��ɫ
#define DARKBLUE      	 0X01CF	//����ɫ
#define LIGHTBLUE      	 0X7D7C	//ǳ��ɫ  
#define GRAYBLUE       	 0X5458 //����ɫ
//������ɫΪPANEL����ɫ 
#define LIGHTGREEN     	 0X841F //ǳ��ɫ
#define LIGHTGRAY        0XEF5B //ǳ��ɫ(PANNEL)
#define LGRAY 			 0XC618 //ǳ��ɫ(PANNEL),���屳��ɫ
#define LGRAYBLUE        0XA651 //ǳ����ɫ(�м����ɫ)
#define LBBLUE           0X2B12 //ǳ����ɫ(ѡ����Ŀ�ķ�ɫ)

//!< API functions
void ili9325_init(void);
void ili9325_clear(int c);
void ili9325_write_pixel(int x, int y, int c);
void ili9325_set_window(int x, int y, int xlen, int ylen);
int ili9325_read_pixel(int x, int y);



#endif

