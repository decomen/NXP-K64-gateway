#ifndef __AD7689_H__
#define __AD7689_H__

#define CHANNEL_QUEUE_SZ        (200)
#define CHANNEL_CHANGE_TIME     (250)  //MS

typedef enum
{
	ADC_MODE_NORMAL, 	//����ģʽ
	ADC_MODE_TEST, 		//����ģʽ�»�ȡADCֵ��������ֵ����ADC���
	ADC_MODE_CALC, 		//��ȡУ������ֵ��������С��4MA����
}eAdcWorkMode;

typedef struct{
	int ChannelQueueSz;
	int AdcGroupSz;
	int ChannelSleepTime;
	eAdcWorkMode eMode;
}s_AdcCfgPram;

typedef struct {
	int usEngUnit;     //��������ԭʼADCֵ
	float fMeasure;             //�����ʵ��ֵ
	int usBinaryUnit;  //�����Ʋ��� �� Ŀǰ�����Ʋ������ԭʼADCֵ
	float fPercentUnit;         //�ٷֱ�
	float fMeterUnit;           //�Ǳ��������
}s_AdcValue_t;

typedef struct{
	float  fMeterValue;   //�Ǳ����ֵ��
	int usAdcValue;    //0-65535
}s_CalValue_t;

//����
typedef struct {
	s_CalValue_t xMin;
	s_CalValue_t xMiddle;
	s_CalValue_t xMax;
	float factor;
}s_CalEntry_t;

//����ϵ��
typedef struct{
	float factor;
}s_CorrectionFactor_t;

typedef enum{
	Unit_Eng = 0x00,
	Unit_Binary,
	Unit_Percent,
	Unit_Meter,
	
	Unit_Max,
}eUnitType_t;

typedef enum{
	Range_4_20MA = 0x00,
	Range_0_20MA,
	Range_0_5V,
	Range_1_5V,	
	
	RANG_TYPE_MAX
}eRangeType_t;

typedef enum{
	SET_MIN = 0x00,
	SET_MIDDLE ,
	SET_MAX ,
	SET_ALL,
}eCalCountType_t;

typedef enum {
	ADC_CHANNEL_0 = 0x00,
	ADC_CHANNEL_1,
	ADC_CHANNEL_2,
	ADC_CHANNEL_3,
	ADC_CHANNEL_4,
	ADC_CHANNEL_5,
	ADC_CHANNEL_6,
	ADC_CHANNEL_7, 
	ADC_CHANNEL_NUM, 
} ADC_CHANNEL_E;



void vAd7689Init(void);
void vAdcPowerEnable(void); //ADC��һ����Դʹ�ܽţ���ʹ��֮ǰ��ʹ�ܵ�Դ
void vAdcPowerDisable(void);


//��ȡADCֵ��Ŀǰfactor���Դ���0����ʱû��ʹ�øò��� ��ʹ�øú�����ȡADCֵ֮ǰ�������У׼��У׼���뱣�浽SPIFLASH�ڣ�ÿ���ϵ����¶�ȡ��
//û��У�飬����Ĭ�ϵ�У��ֵ����Ӱ�쾫�ȡ�У������� vSetCalValue ��������У��ֵ��

//У�鷽��: ��ADCͨ���ڣ�����һ���߾��ȹ̶�������ֵ��(4-20mA) ,�ֱ���� 4MA 12MA 20MA , ͨ�� vGetAdcValue��ȡ��ADCԭʼֵ���ֱ����ö�Ӧ��ֵ

// eHardwareRangeType  Ӳ�����̣���ʾӲ�����������������̷�Χ��Ŀǰ����Ӳ���ǿ��Բ���0-20ma
// eSoftwareType       ������̣���������������̷�Χ������ʵ�ʿ��Բ���0-20ma,����ҳ����ֻ�ܲ���4-20MA. ����4MA,��ҳ����ʾ�����̣��Ҵ�ʱ������ֵ�Ͳ���ֵ��Ϊ0

void vGetAdcValue(ADC_CHANNEL_E chnn , eRangeType_t eHardwareRangeType, eRangeType_t eSoftwareType, float ext_min, float ext_max, s_CorrectionFactor_t factor, s_AdcValue_t *pVal);
void vGetAdcValueTest(ADC_CHANNEL_E chnn , eRangeType_t eHardwareRangeType, eRangeType_t eSoftwareType, float ext_min, float ext_max, s_CorrectionFactor_t factor, s_AdcValue_t *pVal);
void vGetAdcValueCal(ADC_CHANNEL_E chnn , eRangeType_t eHardwareRangeType, eRangeType_t eSoftwareType, float ext_min, float ext_max, s_CorrectionFactor_t factor, s_AdcValue_t *pVal);


//void vGetAdcValueByDefaultParm(ADC_CHANNEL_E chnn , eRangeType_t eHardwareRangeType, eRangeType_t eSoftwareType, float ext_min, float ext_max, s_CorrectionFactor_t factor, s_AdcValue_t *pVal);

void vGetCalValue(ADC_CHANNEL_E chnn , s_CalEntry_t *pCalEntry);
void vSetCalValue(ADC_CHANNEL_E chnn ,eCalCountType_t eCalType, s_CalEntry_t *pCalEntry);
extern s_CalEntry_t gCalEntry[ADC_CHANNEL_NUM];
extern s_CalEntry_t gCalEntryBak[ADC_CHANNEL_NUM];
void SetAdcCalCfgDefault();

extern s_AdcCfgPram gAdcCfgPram;


#endif
