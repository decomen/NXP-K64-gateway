#include <board.h>
#include <ad7707.h>

/*
		S0  S1 S2
CHANN6		0   0   0
CHANN4		0   0   1 
CHANN5		0   1   0
chann2		0   1   1
chann7		1   0   0
chann1		1   0   1
chann8		1   1   0
chann3		1   1   1
*/

//#define AD7689_CS_0()       rt_pin_write( BOARD_GPIO_ADC_PCS0, PIN_LOW);
//#define AD7689_CS_1()       rt_pin_write( BOARD_GPIO_ADC_PCS0, PIN_HIGH);




#define AD_DIN_0()      rt_pin_write( BOARD_GPIO_ADC_SIN, PIN_LOW);
#define AD_DIN_1()      rt_pin_write( BOARD_GPIO_ADC_SIN, PIN_HIGH);


#define AD_SCLK_0()     rt_pin_write( BOARD_GPIO_ADC_SCK, PIN_LOW);
#define AD_SCLK_1()     rt_pin_write( BOARD_GPIO_ADC_SCK, PIN_HIGH);

#define AD_DOUT_READ()  rt_pin_read(BOARD_GPIO_ADC_SOUT)
#define AD_DRDY_READ()  rt_pin_read(BOARD_GPIO_ADC_DRDY)

//#define AD7689_SOUT_0()     rt_pin_write( BOARD_GPIO_ADC_SOUT, PIN_LOW);
//#define AD7689_SOUT_1()     rt_pin_write( BOARD_GPIO_ADC_SOUT, PIN_HIGH);

void SelectChannel(ADC_CHANNEL_E chnn)
{
	unsigned char ch = chnn;

	/*
	int s0 = 0 , s1 = 0 , s2 = 0;
	if(chnn & 0x01){
		s0 = 1;
	}
	if((chnn >> 1) & 0x01){
		s1 = 1;
	}
	if((chnn >> 2) & 0x01){
		s2 - 2;
	}




chann0		1   0   1
chann1		0   1   1
chann2		1   1   1
CHANN3		0   0   1 
CHANN4		0   1   0
CHANN5		0   0   0
chann6		1   0   0
chann7		1   1   0


	*/

	

	switch(chnn){
		case ADC_CHANNEL_0:
			rt_pin_write(BOARD_GPIO_74HC4051_S0 , 1);
			rt_pin_write(BOARD_GPIO_74HC4051_S1 , 0);
			rt_pin_write(BOARD_GPIO_74HC4051_S2 , 1);
			break;
		case ADC_CHANNEL_1:
			rt_pin_write(BOARD_GPIO_74HC4051_S0 , 0);
			rt_pin_write(BOARD_GPIO_74HC4051_S1 , 1);
			rt_pin_write(BOARD_GPIO_74HC4051_S2 , 1);
			break;
		case ADC_CHANNEL_2:
			rt_pin_write(BOARD_GPIO_74HC4051_S0 , 1);
			rt_pin_write(BOARD_GPIO_74HC4051_S1 , 1);
			rt_pin_write(BOARD_GPIO_74HC4051_S2 , 1);
			break;
		case ADC_CHANNEL_3:
			rt_pin_write(BOARD_GPIO_74HC4051_S0 , 0);
			rt_pin_write(BOARD_GPIO_74HC4051_S1 , 0);
			rt_pin_write(BOARD_GPIO_74HC4051_S2 , 1);
			break;
		case ADC_CHANNEL_4:
			rt_pin_write(BOARD_GPIO_74HC4051_S0 , 0);
			rt_pin_write(BOARD_GPIO_74HC4051_S1 , 1);
			rt_pin_write(BOARD_GPIO_74HC4051_S2 , 0);
			break;
		case ADC_CHANNEL_5:
			rt_pin_write(BOARD_GPIO_74HC4051_S0 , 0);
			rt_pin_write(BOARD_GPIO_74HC4051_S1 , 0);
			rt_pin_write(BOARD_GPIO_74HC4051_S2 , 0);
			break;
		case ADC_CHANNEL_6:
			rt_pin_write(BOARD_GPIO_74HC4051_S0 , 1);
			rt_pin_write(BOARD_GPIO_74HC4051_S1 , 0);
			rt_pin_write(BOARD_GPIO_74HC4051_S2 , 0);
			break;
		case ADC_CHANNEL_7:
			rt_pin_write(BOARD_GPIO_74HC4051_S0 , 1);
			rt_pin_write(BOARD_GPIO_74HC4051_S1 , 1);
			rt_pin_write(BOARD_GPIO_74HC4051_S2 , 0);
			break;
		
	}

	//rt_pin_write(BOARD_GPIO_74HC4051_S0 , s0);
	//rt_pin_write(BOARD_GPIO_74HC4051_S1 , s1);
	//rt_pin_write(BOARD_GPIO_74HC4051_S2 , s2);
	
}

static void vResetAd7707(void)
{
	rt_pin_write( BOARD_GPIO_ADC_RESET, PIN_LOW);
	DelayMs(100);
	rt_pin_write( BOARD_GPIO_ADC_RESET, PIN_HIGH);
	DelayMs(100);
}

#define delay_AD_us()        {int i = 10; while(i--);}

static unsigned char AD_rw(unsigned char data)
{
	unsigned char i;  
	
	for(i=0; i<8; i++)          // 循环8次  
	{  
		//SET_GPIO_SPI_SCK();
		AD_SCLK_0();
		
		if(data & 0x80){		// byte最高位输出到MOSI  
			//SET_GPIO_SPI_SI();
			AD_DIN_1();
		}
		else{ 
			//RESET_GPIO_SPI_SI();
			AD_DIN_0();
		}
		
		data <<= 1;             // 低一位移位到最高位  
                
                

		//RESET_GPIO_SPI_SCK();	// 拉高SCK，nRF24L01从MOSI读入1位数据，同时从MISO输出1位数据  
		AD_SCLK_1();
        delay_AD_us();

		if(AD_DOUT_READ()){
			data |= 1;           // 读MISO到byte最低位  
		}
		//SET_GPIO_SPI_SCK();               // SCK置低  
		AD_SCLK_0();  
        delay_AD_us();
	}  
	
	return(data);               // 返回读出的一字节  
}

static void vInit2(void)
{
	AD_rw(0X11); //下一操作为设置寄存器写操作。
	AD_rw(0X44);

	AD_rw(0X51); //将下一个操作设为对滤波低寄存器进行写操作。
    AD_rw(0Xff);
	
    AD_rw(0X21); //下一个操作设为对滤波高寄存器进行写操作。
    AD_rw(0X08);
}

void vInit1(void)
{
	AD_rw(0X10); //下一操作为设置寄存器写操作。
	AD_rw(0X44);

	AD_rw(0X50); //将下一个操作设为对滤波低寄存器进行写操作。
      AD_rw(0Xff);
	
      AD_rw(0X20); //下一个操作设为对滤波高寄存器进行写操作。
      AD_rw(0X08);
}




void vAdcPowerEnable(void)
{
	rt_pin_mode( BOARD_GPIO_ADC_POWER, PIN_MODE_OUTPUT );
	rt_pin_write( BOARD_GPIO_ADC_POWER, PIN_HIGH);
}

void vAdcPowerDisable(void)
{
	rt_pin_mode( BOARD_GPIO_ADC_POWER, PIN_MODE_OUTPUT );
	rt_pin_write( BOARD_GPIO_ADC_POWER, PIN_LOW);
}


void test_reg()
{
	char data = 0;
	AD_rw(0X28);
	data = AD_rw(0);

	AD_rw(0X18);
	data = AD_rw(0);

	AD_rw(0X68);
	data = AD_rw(0);
    data = AD_rw(0);
    data = AD_rw(0);
}

static const s_CalEntry_t gDefaultCalEntry[ADC_CHANNEL_NUM] = {
	[ADC_CHANNEL_0] = {
		.xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},	
		.xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},	
		.xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},	
		.factor = {0.0f},
	},
	[ADC_CHANNEL_1] = {
		.xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},	
		.xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},	
		.xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},	
		.factor = {0.0f},
	},
	[ADC_CHANNEL_2] = {
		.xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},	
		.xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},	
		.xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},	
		.factor = {0.0f},
	},
	[ADC_CHANNEL_3] = {
		.xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},	
		.xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},	
		.xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},	
		.factor = {0.0f},
	},
	[ADC_CHANNEL_4] = {
		.xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},	
		.xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},	
		.xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},	
		.factor = {0.0f},
	},
	[ADC_CHANNEL_5] = {
		.xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},	
		.xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},	
		.xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},	
		.factor = {0.0f},
	},
	[ADC_CHANNEL_6] = {
		.xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},	
		.xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},	
		.xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},	
		.factor = {0.0f},
	},
	[ADC_CHANNEL_7] = {
		.xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},	
		.xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},	
		.xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},	
		.factor = {0.0f},
	},
};

s_CalEntry_t gCalEntry[ADC_CHANNEL_NUM];
s_CalEntry_t gCalEntryBak[ADC_CHANNEL_NUM];

void SetAdcCalCfgDefault()
{
	for(int i = 0; i < ADC_CHANNEL_NUM;i++){
		gCalEntry[i] = gDefaultCalEntry[i];
		gCalEntryBak[i]= gDefaultCalEntry[i];
	}
}

void vAd7689Init(void)
{
    vAdcPowerEnable();
    rt_pin_mode( BOARD_GPIO_ADC_RESET, PIN_MODE_OUTPUT );
    rt_pin_mode( BOARD_GPIO_ADC_SIN, PIN_MODE_OUTPUT );
    rt_pin_mode( BOARD_GPIO_ADC_SCK, PIN_MODE_OUTPUT );
    rt_pin_mode( BOARD_GPIO_ADC_SOUT, PIN_MODE_INPUT );
	
    rt_pin_mode( BOARD_GPIO_ADC_DRDY, PIN_MODE_INPUT );
	//rt_pin_mode( BOARD_GPIO_ADC_SOUT, PIN_MODE_INPUT );

	vResetAd7707();
	//配置74HC4051

	rt_pin_mode( BOARD_GPIO_74HC4051_S0, PIN_MODE_OUTPUT );
	rt_pin_mode( BOARD_GPIO_74HC4051_S1, PIN_MODE_OUTPUT );
    rt_pin_mode( BOARD_GPIO_74HC4051_S2, PIN_MODE_OUTPUT );
	rt_pin_mode( BOARD_GPIO_74HC4051_E, PIN_MODE_OUTPUT );
    rt_pin_write( BOARD_GPIO_74HC4051_E, PIN_LOW);
	rt_pin_write( BOARD_GPIO_74HC4051_E, PIN_HIGH); //使能74HC4051
        

	//AD7689_CS_1(); 
	AD_DIN_1();
	AD_SCLK_1();
    test_reg(); //测试一下时序是否OK。此函数可以注释掉
	vInit1();
	
}


/*

static unsigned short AD7689_spi(unsigned char chn)
{
	unsigned char i = 0;
	unsigned short reg = 0;
	unsigned short data = 0;

	reg = (1<<13)|(7<<10)|(chn<<7)|(1<<6)|(6<<3)|(1<<0);
	reg <<=2; 
	
	 AD7689_CS_0();
	 AD7689_SCLK_0();
	 DelayUs(10);
      
	for(i=0; i<16; i++){
		
         if(reg&0x8000){
                AD7689_DIN_1();
         }else{
                AD7689_DIN_0();
         }       
         DelayUs(4);
         reg <<= 1;
         AD7689_SCLK_1();
         DelayUs(4);
         data<<=1;
         
         if(AD7689_DOUT_READ()){
		data|=1;
         }
                 
         AD7689_SCLK_0();
         DelayUs(4);
		
      } //最高位先入
      
       AD7689_CS_1();
       DelayUs(10);
	return data;        
}*/

static unsigned long getAdcChnn2(ADC_CHANNEL_E chnn)
{
	SelectChannel(chnn);
	rt_thread_delay( CHANNEL_CHANGE_TIME * RT_TICK_PER_SECOND / 1000 );
	//DelayMs(CHANNEL_CHANGE_TIME);
	
	unsigned long TM_result = 0;
	//while(!AD_DRDY_READ());
	int rety = 10;
	while( !AD_DRDY_READ() && rety-- ) rt_thread_delay( RT_TICK_PER_SECOND / 100 );
	AD_rw(0x38);   //向通信寄存器写入数据，选择通道2作为有效，将下一个操作设为读数据寄存器。
    int i =0;
	unsigned char buf[3] = {0};
    for(i = 0 ; i< 3; i++){
        buf[i] = AD_rw(0);
    }
    TM_result=buf[2]+(buf[1]*256)+(buf[0]*65536);
     return TM_result;
}




// A = (D - D0) *　(Am - A0) / (Dm-D0) + A0
// A 表示测量值
// D 表示ADC值

// eHardwareRangeType  硬件量程，表示硬件上面可以输入的量程范围，目前我们硬件是可以测量0-20ma
// eSoftwareType       软件量程，软件上面限制量程范围，比如实际可以测量0-20ma,但网页上面只能测量4-20MA. 低于4MA,网页就提示超量程，且此时工程量值和测量值都为0



void vGetAdcValue(ADC_CHANNEL_E chnn , eRangeType_t eHardwareRangeType, eRangeType_t eSoftwareType, float ext_min, float ext_max, s_CorrectionFactor_t factor, s_AdcValue_t *pVal)
{
	unsigned long usAdcVal = ((getAdcChnn2(chnn)) >> 4);


	float fPercent = 0 , fMeterVal = 0;

    switch(eSoftwareType) {
		case Range_0_20MA:{
		    if( Range_0_20MA == eHardwareRangeType ) {
		    }
	        break;
		}
		case Range_4_20MA:{
			 if( Range_0_20MA == eHardwareRangeType ) {
			 	if(usAdcVal < (gCalEntry[chnn].xMiddle.usAdcValue/gCalEntry[chnn].xMiddle.fMeterValue) * 4.0 ){
					usAdcVal = 0;
				}
		     }
		    }
	        break;
		
		case Range_1_5V:{
			/*
		    if( Range_0_5V == eHardwareRangeType ) {
		        if(usAdcVal < (gCalEntry[chnn].xMiddle.usAdcValue/gCalEntry[chnn].xMiddle.fMeterValue) * 4.0 ){
					usAdcVal = 0;
				}
		    }*/
	        break;
		}
		case Range_0_5V:{
		    if( Range_0_5V == eHardwareRangeType ) {
		    }
	        break;
		}
    }
    
    
	pVal->usEngUnit = usAdcVal;
	pVal->usBinaryUnit = usAdcVal;
	
    switch(eSoftwareType){
	case Range_4_20MA: {
			if (usAdcVal > 0 && usAdcVal <= (0x7FFFF)) {
				unsigned int TmpVal = 0;
				if (usAdcVal >= gCalEntry[chnn].xMin.usAdcValue) {
					TmpVal = usAdcVal - gCalEntry[chnn].xMin.usAdcValue;
					
					fMeterVal =  gCalEntry[chnn].xMin.fMeterValue +	
					   ( (TmpVal) * (gCalEntry[chnn].xMiddle.fMeterValue -  gCalEntry[chnn].xMin.fMeterValue) / 
					    ( gCalEntry[chnn].xMiddle.usAdcValue - gCalEntry[chnn].xMin.usAdcValue ) );
					
				} else {
					TmpVal =  gCalEntry[chnn].xMin.usAdcValue - usAdcVal;

					fMeterVal =  gCalEntry[chnn].xMin.fMeterValue -	
					   ( (TmpVal) * (gCalEntry[chnn].xMiddle.fMeterValue -  gCalEntry[chnn].xMin.fMeterValue) / 
					    ( gCalEntry[chnn].xMiddle.usAdcValue - gCalEntry[chnn].xMin.usAdcValue ) );
				}
				
				
				           
			} else if(usAdcVal > (0x7FFFF)) {
				unsigned int TmpVal = 0;
				if( usAdcVal >= gCalEntry[chnn].xMiddle.usAdcValue){
					TmpVal = usAdcVal - gCalEntry[chnn].xMiddle.usAdcValue;
					
					fMeterVal =  gCalEntry[chnn].xMiddle.fMeterValue +	
					   ( (TmpVal) *(gCalEntry[chnn].xMax.fMeterValue -  gCalEntry[chnn].xMiddle.fMeterValue) / 
					    ( gCalEntry[chnn].xMax.usAdcValue - gCalEntry[chnn].xMiddle.usAdcValue ) );
					
				}else {
					TmpVal =  gCalEntry[chnn].xMiddle.usAdcValue - usAdcVal;

					fMeterVal =  gCalEntry[chnn].xMiddle.fMeterValue -	
					   ( (TmpVal) *(gCalEntry[chnn].xMax.fMeterValue -  gCalEntry[chnn].xMiddle.fMeterValue) / 
					    ( gCalEntry[chnn].xMax.usAdcValue - gCalEntry[chnn].xMiddle.usAdcValue ) );
				}
				            
			} else {
                fMeterVal = 0.0f;
            }
            
			//fMeterVal -= gCalEntry[chnn].factor;
			pVal->fMeasure = fMeterVal;
			if (fMeterVal>=4) {
			    pVal->fPercentUnit = (fMeterVal-4)/(20-4);
			    pVal->fMeterUnit = (ext_max-ext_min)*(fMeterVal-4)/(20-4)+ext_min+factor.factor;
			} else {
			    pVal->fPercentUnit = 0;
			    pVal->fMeterUnit = ext_min+factor.factor;
			}
			break;
		}
        
         case Range_0_5V:
		 	break;
        case Range_1_5V:
			break;
		case Range_0_20MA:{
			break;
			break;
		}
	}

	
}

//用于测试模式下，获取校准过后的ADC值

void vGetAdcTestValueInfo(ADC_CHANNEL_E chnn , eRangeType_t eHardwareRangeType, eRangeType_t eSoftwareType, float ext_min, float ext_max, s_CorrectionFactor_t factor, s_AdcValue_t *pVal)
{
	unsigned long usAdcVal = ((getAdcChnn2(chnn)) >> 4);


	float fPercent = 0 , fMeterVal = 0;

    switch(eSoftwareType) {
		case Range_0_20MA:{
		    if( Range_0_20MA == eHardwareRangeType ) {
		    }
	        break;
		}
		case Range_4_20MA:{
			 if( Range_0_20MA == eHardwareRangeType ) {
			 	
		     }
		    }
	        break;
		
		case Range_1_5V:{
			/*
		    if( Range_0_5V == eHardwareRangeType ) {
		        if(usAdcVal < (gCalEntry[chnn].xMiddle.usAdcValue/gCalEntry[chnn].xMiddle.fMeterValue) * 4.0 ){
					usAdcVal = 0;
				}
		    }*/
	        break;
		}
		case Range_0_5V:{
		    if( Range_0_5V == eHardwareRangeType ) {
		    }
	        break;
		}
    }
    
    
	pVal->usEngUnit = usAdcVal;
	pVal->usBinaryUnit = usAdcVal;
	
    switch(eSoftwareType){
	case Range_4_20MA: {
			if (usAdcVal > 0 && usAdcVal <= (0x7FFFF)) {
				unsigned int TmpVal = 0;
				if (usAdcVal >= gCalEntry[chnn].xMin.usAdcValue) {
					TmpVal = usAdcVal - gCalEntry[chnn].xMin.usAdcValue;
					
					fMeterVal =  gCalEntry[chnn].xMin.fMeterValue +	
					   ( (TmpVal) * (gCalEntry[chnn].xMiddle.fMeterValue -  gCalEntry[chnn].xMin.fMeterValue) / 
					    ( gCalEntry[chnn].xMiddle.usAdcValue - gCalEntry[chnn].xMin.usAdcValue ) );
					
				} else {
					TmpVal =  gCalEntry[chnn].xMin.usAdcValue - usAdcVal;

					fMeterVal =  gCalEntry[chnn].xMin.fMeterValue -	
					   ( (TmpVal) * (gCalEntry[chnn].xMiddle.fMeterValue -  gCalEntry[chnn].xMin.fMeterValue) / 
					    ( gCalEntry[chnn].xMiddle.usAdcValue - gCalEntry[chnn].xMin.usAdcValue ) );
				}
				
				
				           
			} else if(usAdcVal > (0x7FFFF)) {
				unsigned int TmpVal = 0;
				if( usAdcVal >= gCalEntry[chnn].xMiddle.usAdcValue){
					TmpVal = usAdcVal - gCalEntry[chnn].xMiddle.usAdcValue;
					
					fMeterVal =  gCalEntry[chnn].xMiddle.fMeterValue +	
					   ( (TmpVal) *(gCalEntry[chnn].xMax.fMeterValue -  gCalEntry[chnn].xMiddle.fMeterValue) / 
					    ( gCalEntry[chnn].xMax.usAdcValue - gCalEntry[chnn].xMiddle.usAdcValue ) );
					
				}else {
					TmpVal =  gCalEntry[chnn].xMiddle.usAdcValue - usAdcVal;

					fMeterVal =  gCalEntry[chnn].xMiddle.fMeterValue -	
					   ( (TmpVal) *(gCalEntry[chnn].xMax.fMeterValue -  gCalEntry[chnn].xMiddle.fMeterValue) / 
					    ( gCalEntry[chnn].xMax.usAdcValue - gCalEntry[chnn].xMiddle.usAdcValue ) );
				}
				            
			} else {
                fMeterVal = 0.0f;
            }
            
			//fMeterVal -= gCalEntry[chnn].factor;
			pVal->fMeasure = fMeterVal;
			if (fMeterVal >= 4) {
			    pVal->fPercentUnit = (fMeterVal-4)/(20-4);
			    pVal->fMeterUnit = (ext_max-ext_min)*(fMeterVal-4)/(20-4)+ext_min+factor.factor;
			} else {
			    pVal->fPercentUnit = 0;
			    pVal->fMeterUnit = ext_min+factor.factor;
			}
			break;
		}
        
         case Range_0_5V:
		 	break;
        case Range_1_5V:
			break;
		case Range_0_20MA:{
			break;
		}
	}

	
}

//用于校准，获取ADC值，校准获取ADC的值，使用理论值进行计算

void vGetAdcValueTest(ADC_CHANNEL_E chnn , eRangeType_t eHardwareRangeType, eRangeType_t eSoftwareType, float ext_min, float ext_max, s_CorrectionFactor_t factor, s_AdcValue_t *pVal)
{

	unsigned long TM_result = 0;
	int rety = 10;
	while( !AD_DRDY_READ() && rety-- ) rt_thread_delay( RT_TICK_PER_SECOND / 100 );
	AD_rw(0x38);   //向通信寄存器写入数据，选择通道2作为有效，将下一个操作设为读数据寄存器。
    int i =0;
	unsigned char buf[3] = {0};
    for(i = 0 ; i< 3; i++){
        buf[i] = AD_rw(0);
    }
    TM_result=buf[2]+(buf[1]*256)+(buf[0]*65536);
	
	unsigned long usAdcVal = (TM_result >> 4);

	float fPercent = 0 , fMeterVal = 0;

    switch(eSoftwareType) {
		case Range_0_20MA:{
		    if( Range_0_20MA == eHardwareRangeType ) {
		    }
	        break;
		}
		case Range_4_20MA:{
			 if( Range_0_20MA == eHardwareRangeType ) {
			 	if(usAdcVal < (gDefaultCalEntry[chnn].xMiddle.usAdcValue/gDefaultCalEntry[chnn].xMiddle.fMeterValue) * 4.0 ){
					usAdcVal = 0;
				}
		     }
		    }
	        break;
		
		case Range_1_5V:{
			/*
		    if( Range_0_5V == eHardwareRangeType ) {
		        if(usAdcVal < (gCalEntry[chnn].xMiddle.usAdcValue/gCalEntry[chnn].xMiddle.fMeterValue) * 4.0 ){
					usAdcVal = 0;
				}
		    }*/
	        break;
		}
		case Range_0_5V:{
		    if( Range_0_5V == eHardwareRangeType ) {
		    }
	        break;
		}
    }
    
    
	pVal->usEngUnit = usAdcVal;
	pVal->usBinaryUnit = usAdcVal;
	
    switch(eSoftwareType){
	case Range_4_20MA:/*{
			fPercent = (float)(usAdcVal / 1048576.0f);
			//fMeterVal = (float)(temp * (gRange[Range_0_20MA].ucMax - gRange[Range_0_20MA].ucMin) / 65535.0f);
			if(usAdcVal >= 0 && usAdcVal <= (0x7FFFF)){
				fMeterVal = (usAdcVal - gCalEntry[Range_4_20MA].xMin.usAdcValue) * 
					    (gCalEntry[Range_4_20MA].xMiddle.fMeterValue -  gCalEntry[Range_4_20MA].xMin.fMeterValue) / 
					    ( gCalEntry[Range_4_20MA].xMiddle.usAdcValue - gCalEntry[Range_4_20MA].xMin.usAdcValue ) + 
				            gCalEntry[Range_4_20MA].xMin.fMeterValue;	
			}else if(usAdcVal > (0x7FFFF)){
				fMeterVal = (usAdcVal - gCalEntry[Range_4_20MA].xMiddle.usAdcValue) * 
					    (gCalEntry[Range_4_20MA].xMax.fMeterValue -  gCalEntry[Range_4_20MA].xMiddle.fMeterValue) / 
					    ( gCalEntry[Range_4_20MA].xMax.usAdcValue - gCalEntry[Range_4_20MA].xMiddle.usAdcValue ) + 
				            gCalEntry[Range_4_20MA].xMiddle.fMeterValue;	
			}
			pVal->fPercentUnit = fPercent;
			//pVal->fPercentUnit = fMeterVal;
			pVal->fMeasure = fMeterVal;
			pVal->fMeterUnit = (ext_max-ext_min)*(fMeterVal-4)/(20-4)+ext_min+factor.factor;
			
			break;
		}*/
        
         case Range_0_5V:/*{
			fPercent = (float)(usAdcVal / 1048576.0f);
			
			if(usAdcVal >= 0 && usAdcVal <= (0x7FFFF)){
				fMeterVal = (usAdcVal - gCalEntry[Range_0_5V].xMin.usAdcValue) * 
					    (gCalEntry[Range_0_5V].xMiddle.fMeterValue -  gCalEntry[Range_0_5V].xMin.fMeterValue) / 
					    ( gCalEntry[Range_0_5V].xMiddle.usAdcValue - gCalEntry[Range_0_5V].xMin.usAdcValue ) + 
				            gCalEntry[Range_0_5V].xMin.fMeterValue;	
			}else if(usAdcVal > (0x7FFFF)){
				fMeterVal = (usAdcVal - gCalEntry[Range_0_5V].xMiddle.usAdcValue) * 
					    (gCalEntry[Range_0_5V].xMax.fMeterValue -  gCalEntry[Range_0_5V].xMiddle.fMeterValue) / 
					    ( gCalEntry[Range_0_5V].xMax.usAdcValue - gCalEntry[Range_0_5V].xMiddle.usAdcValue ) + 
				            gCalEntry[Range_0_5V].xMiddle.fMeterValue;	
			}
			
			pVal->fPercentUnit = fPercent;
			pVal->fMeasure = fMeterVal;
			pVal->fMeterUnit = (ext_max-ext_min)*(fMeterVal-0)/(5-0)+ext_min+factor.factor;
			break;
		}*/
        case Range_1_5V:/*{
			fPercent = (float)(usAdcVal / 1048576.0f);
			
			if(usAdcVal >= 0 && usAdcVal <= (0x7FFFF)){
				fMeterVal = (usAdcVal - gCalEntry[Range_1_5V].xMin.usAdcValue) * 
					    (gCalEntry[Range_1_5V].xMiddle.fMeterValue -  gCalEntry[Range_1_5V].xMin.fMeterValue) / 
					    ( gCalEntry[Range_1_5V].xMiddle.usAdcValue - gCalEntry[Range_1_5V].xMin.usAdcValue ) + 
				            gCalEntry[Range_1_5V].xMin.fMeterValue;	
			}else if(usAdcVal > (0x7FFFF)){
				fMeterVal = (usAdcVal - gCalEntry[Range_1_5V].xMiddle.usAdcValue) * 
					    (gCalEntry[Range_1_5V].xMax.fMeterValue -  gCalEntry[Range_1_5V].xMiddle.fMeterValue) / 
					    ( gCalEntry[Range_1_5V].xMax.usAdcValue - gCalEntry[Range_1_5V].xMiddle.usAdcValue ) + 
				            gCalEntry[Range_1_5V].xMiddle.fMeterValue;	
			}
			
			pVal->fPercentUnit = fPercent;
			pVal->fMeasure = fMeterVal;
			pVal->fMeterUnit = (ext_max-ext_min)*(fMeterVal-1)/(5-1)+ext_min+factor.factor;
			break;
		}*/
		case Range_0_20MA:{
			fPercent = (float)(usAdcVal / 1048576.0f);
			//fMeterVal = (float)(temp * (gRange[Range_4_20MA].ucMax - gRange[Range_4_20MA].ucMin) / 65535.0f);

			if(usAdcVal >= 0 && usAdcVal <= (0x7FFFF)){
				
				unsigned int TmpVal = 0;
				if (usAdcVal >= gDefaultCalEntry[chnn].xMin.usAdcValue) {
					TmpVal = usAdcVal - gDefaultCalEntry[chnn].xMin.usAdcValue;
					
					fMeterVal =  gDefaultCalEntry[chnn].xMin.fMeterValue +	
					   ( (TmpVal) * (gDefaultCalEntry[chnn].xMiddle.fMeterValue -  gDefaultCalEntry[chnn].xMin.fMeterValue) / 
					    ( gDefaultCalEntry[chnn].xMiddle.usAdcValue - gDefaultCalEntry[chnn].xMin.usAdcValue ) );
					
				} else {
					TmpVal =  gDefaultCalEntry[chnn].xMin.usAdcValue - usAdcVal;

					fMeterVal =  gDefaultCalEntry[chnn].xMin.fMeterValue -	
					   ( (TmpVal) * (gDefaultCalEntry[chnn].xMiddle.fMeterValue -  gDefaultCalEntry[chnn].xMin.fMeterValue) / 
					    ( gDefaultCalEntry[chnn].xMiddle.usAdcValue - gDefaultCalEntry[chnn].xMin.usAdcValue ) );
				}
				
			}else if(usAdcVal > (0x7FFFF)){

				unsigned int TmpVal = 0;
				if( usAdcVal >= gDefaultCalEntry[chnn].xMiddle.usAdcValue){
					TmpVal = usAdcVal - gDefaultCalEntry[chnn].xMiddle.usAdcValue;
					
					fMeterVal =  gDefaultCalEntry[chnn].xMiddle.fMeterValue +	
					   ( (TmpVal) *(gDefaultCalEntry[chnn].xMax.fMeterValue -  gDefaultCalEntry[chnn].xMiddle.fMeterValue) / 
					    ( gDefaultCalEntry[chnn].xMax.usAdcValue - gDefaultCalEntry[chnn].xMiddle.usAdcValue ) );
					
				}else {
					TmpVal =  gDefaultCalEntry[chnn].xMiddle.usAdcValue - usAdcVal;

					fMeterVal =  gDefaultCalEntry[chnn].xMiddle.fMeterValue -	
					   ( (TmpVal) *(gDefaultCalEntry[chnn].xMax.fMeterValue -  gDefaultCalEntry[chnn].xMiddle.fMeterValue) / 
					    ( gDefaultCalEntry[chnn].xMax.usAdcValue - gDefaultCalEntry[chnn].xMiddle.usAdcValue ) );
				}
				
			}else {
              fPercent = 0.0f;
            }
			
			//pVal->fPercentUnit = fPercent;
			//pVal->fMeasure = fMeterVal;
			//pVal->fMeterUnit = (ext_max-ext_min)*(fMeterVal-0)/(20-0)+ext_min+factor.factor;

			pVal->fMeasure = fMeterVal;
			if (fMeterVal >= 4) {
			    pVal->fPercentUnit = (fMeterVal-0)/(20-0);
			    pVal->fMeterUnit = (ext_max-ext_min)*(fMeterVal-0)/(20-0)+ext_min+factor.factor;
			} else {
			    pVal->fPercentUnit = 0;
			    pVal->fMeterUnit = ext_min+factor.factor;
			}
			
			break;
		}
	}
}


void vGetCalValue(ADC_CHANNEL_E chnn , s_CalEntry_t *pCalEntry)
{
	*pCalEntry = gCalEntry[chnn];
}

void vSetCalValue(ADC_CHANNEL_E chnn ,eCalCountType_t eCalType, s_CalEntry_t *pCalEntry)
{
	switch(eCalType){
		case SET_MIN:
		{
			gCalEntryBak[chnn].xMin = pCalEntry->xMin;
			//gCalEntryBak[chnn].factor = pCalEntry->factor;
			break;
		}
		case SET_MIDDLE:
		{
			gCalEntryBak[chnn].xMiddle = pCalEntry->xMiddle;
			//gCalEntryBak[chnn].factor = pCalEntry->factor;
			break;
		}
		case SET_MAX:
		{
			gCalEntryBak[chnn].xMax = pCalEntry->xMax;
			//gCalEntryBak[chnn].factor = pCalEntry->factor;
			break;
		}
		case SET_ALL:{
			gCalEntryBak[chnn] = *pCalEntry;
			break;
		}
		default:
			break;
			
	}
}

