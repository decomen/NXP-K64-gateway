#ifndef __VAR_MANAGE_H__
#define __VAR_MANAGE_H__

#define VAR_MANAGE_MALLOC(_n)           rt_malloc((_n))
#define VAR_MANAGE_CALLOC(_cnt,_n)      rt_calloc((_cnt),(_n))
#define VAR_MANAGE_FREE(_p)             if((_p)) { rt_free((_p)); _p = 0; }
#define VAR_MANAGE_OFS(_t, _m)          (unsigned int)(&(((_t *)0)->_m))

#define VAR_NULL                        0
#define VAR_TRUE                        1
#define VAR_FALSE                       0
#define VAR_BIT_0                       0
#define VAR_BIT_1                       1

typedef char *var_str_t;      /**<  string type */
typedef signed   char                   var_char_t;     /**<  8bit integer type */
typedef signed   char                   var_int8_t;     /**<  8bit integer type */
typedef signed   short                  var_int16_t;    /**< 16bit integer type */
typedef signed   long                   var_int32_t;    /**< 32bit integer type */
typedef signed   long long              var_int64_t;    /**< 64bit integer type */
typedef unsigned char                   var_uint8_t;    /**<  8bit unsigned integer type */
typedef unsigned char                   var_uchar_t;    /**<  8bit unsigned integer type */
typedef unsigned char                   var_byte_t;     /**<  8bit unsigned integer type */
typedef unsigned char                   var_bit_t;      /**<  8bit unsigned integer type */
typedef unsigned short                  var_uint16_t;   /**< 16bit unsigned integer type */
typedef unsigned long                   var_uint32_t;   /**< 32bit unsigned integer type */
typedef unsigned long long              var_uint64_t;   /**< 64bit unsigned integer type */
typedef var_uint8_t                     var_bool_t;     /**< boolean type */

typedef int                             var_int_t;
typedef unsigned int                    var_uint_t;

typedef float                           var_float_t;
typedef double                          var_double_t;

/* 32bit CPU */
typedef long                            var_base_t;     /**< Nbit CPU related date type */
typedef unsigned long                   var_ubase_t;    /**< Nbit unsigned CPU related data type */

typedef enum {
    E_VAR_BIT,
    E_VAR_INT8,
    E_VAR_UINT8,
    E_VAR_INT16,
    E_VAR_UINT16,
    E_VAR_INT32,
    E_VAR_UINT32,
    E_VAR_FLOAT,
    E_VAR_DOUBLE,
    E_VAR_ARRAY,        // can be a utf8 string or binary buffer
} eVarType;

typedef enum {
    E_VAR_RO,
    E_VAR_WO,
    E_VAR_RW,
} eVarRWType;

typedef enum {
    E_ERR_OP_NONE       = 0x00,
    E_ERR_OP_CLEAR,
} eErrOpType;

#define VAR_TYPE_SIZE_ARRAY         { 1, 1, 1, 2, 2, 4, 4, 4, 8, 1 }

typedef union {
    var_bit_t       val_bit;
    var_int8_t      val_i8;
    var_uint8_t     val_u8;
    var_int16_t     val_i16;
    var_uint16_t    val_u16;
    var_int32_t     val_i32;
    var_uint32_t    val_u32;
    var_float_t     val_f;
    var_double_t    val_db;
    var_uint16_t    val_reg[1];         // ���16���Ĵ���
    var_uint8_t     val_ary[1];         // ���32��
} VarValue_v;

typedef struct {
    var_int_t       len;
    char *str;
} VarString_t;

typedef struct {
    var_uint32_t    time;           // time
    var_uint32_t    count;          // count
    var_double_t    val_avg;        // ��ֵ
    var_float_t     val_cur;        // ��ǰֵ
    var_float_t     val_min;        // ��Сֵ
    var_float_t     val_max;        // ���ֵ
} VarAvg_t;

typedef struct {
    char            szName[9];      // has '\0'
} VarName_t;

typedef struct {
    char            szAlias[7];     // has '\0'
} VarAlias_t;

#define VAR_EXT_DATA_SIZE           ( 64 )

// add by jay 2016/11/26
// ���ֽڶ���, ���Խ�ʡһ�����ֽ���, ��Ч�ʵ�Ӱ����Ժ���
//#pragma pack(1)

typedef struct {
    var_uint8_t addr[6];
} dlt645_addr_t;

typedef struct {
    var_uint8_t     btOpCode;       // ������
    var_int_t       nSyncFAddr;     // ͬ֡��ַ
    var_uint8_t     btSlaveAddr;    // �ӻ���ַ
    var_uint16_t    usExtAddr;      // �ⲿ������ַ
    var_uint16_t    usAddrOfs;      // �ⲿ������ַƫ��
} ExtData_IO_modbus_t;

typedef struct {
    dlt645_addr_t   addr;           // �豸��ַ
    var_uint32_t    op;             // ������
} ExtData_IO_dlt645_t;

typedef union {
    ExtData_IO_modbus_t     modbus; // mosbus �ṹ
    ExtData_IO_dlt645_t     dlt645; // dlt645 �ṹ, ����ר��Э��
} ExtData_IO_Param_t;

typedef struct {
    var_uint8_t     btRWType;       // ��д����
    var_uint8_t     btVarType;      // ��������
    var_uint8_t     btVarSize;      // ���ݴ�С, val_aryʱ, ��ʾ�����С
    var_uint8_t     btVarRuleType;  // ������ʽ
    VarValue_v      xValue;         // ֵ
    var_float_t     fMax, fMin;     // ���/Сֵ
    var_float_t     fInit;          // ��ʼֵ
    var_float_t     fRatio;         // ���
    var_bool_t      bUseMax,bUseMin;
    var_bool_t      bUseInit,bUseRatio;

    var_uint16_t    usDevType;      // ����Ӳ��
    var_uint16_t    usDevNum;       // ����Ӳ�����
    var_uint16_t    usProtoType;    // ����Э����
    char            *szProtoName;   // ������Э������(�磺LUAЭ������)

    var_uint16_t    usAddr;         // �ڲ�������ַ

    var_uint8_t     btErrOp;
    var_uint8_t     btErrCnt;
    var_uint8_t     btErrNum;       // **�ڲ�ʹ��

    ExtData_IO_Param_t      param;

    char            *szExp;         // ���ʽ
} ExtData_IO_t;

typedef struct {
    var_bool_t      bEnable;        // �Ƿ񱨾�
} ExtData_Alarm_t;

typedef struct {
    var_bool_t      bEnable;        // �Ƿ����
    var_uint8_t     btType;         // ���̷�ʽ
    var_uint32_t    ulStep;         // ���̼��

    VarAvg_t        xAvgReal;       // ʵʱ��ֵ��
    VarAvg_t        xAvgMin;        // ���Ӿ�ֵ��
    VarAvg_t        xAvgHour;       // Сʱ��ֵ��

    var_bool_t      bCheck;
    var_float_t     fMax, fMin;
} ExtData_Storage_t;

typedef struct _ExtDataUp {
    var_bool_t      bEnable;        // ��Ч��־

    VarAvg_t        xAvgUp;         // �ϴ���ֵ��
    VarAvg_t        xAvgMin;        // ���Ӿ�ֵ��
    VarAvg_t        xAvgHour;       // Сʱ��ֵ��

    char            *szNid;         // nid
    char            *szFid;         // fid
    char            *szUnit;        // unit

    var_uint16_t    usDevType;      // ����Ӳ��
    var_uint16_t    usDevNum;       // ����Ӳ�����
    var_uint16_t    usProtoType;    // ����Э����
    char            *szProtoName;   // ������Э������(�磺LUAЭ������)

    char            *szDesc;        // ����
} ExtData_Up_t;

#define EXT_DATA_GET_NID(_nid)      ((_nid && _nid[0])?_nid:g_host_cfg.szId)

typedef struct _ExtData {
    var_bool_t      bEnable;        // ��Ч��־, ��Чʱ, ��������ݾ�������

    VarName_t       xName;          // ����
    VarAlias_t      xAlias;         // ����

    ExtData_IO_t        xIo;        // IO��������
    ExtData_Alarm_t     xAlarm;     // ��������
    ExtData_Storage_t   xStorage;   // ��������
    ExtData_Up_t        xUp;

    struct _ExtData *next, *prev;   // �������
} ExtData_t;

//#pragma pack()

typedef struct {
    int             n;              // ��Ŀ <= VAR_EXT_DATA_SIZE
    ExtData_t       *pList;         // ����
} ExtDataList_t;

var_uint16_t var_htons(var_uint16_t n);
var_uint32_t var_htonl(var_uint32_t n);
void vVarManage_ExtDataInit(void);
var_bool_t bVarManage_CheckExtValue(ExtData_t *data, VarValue_v *var);
var_bool_t bVarManage_GetExtValue(ExtData_t *data, var_double_t *value);
var_bool_t bVarManage_GetExtValueWithName(const char *name, var_double_t *value);
var_bool_t bVarManage_SetExtDataValue(ExtData_t *data, VarValue_v *pValue);
var_bool_t bVarManage_SetExtDataValueWithName(const char *name, VarValue_v *pValue);
ExtData_t* pVarManage_GetExtDataWithAddr(var_uint16_t usAddr, ExtData_t *data);
ExtData_t* pVarManage_GetExtDataWithSlaveAddr(var_uint16_t usExtAddr, var_uint8_t btSlaveAddr, ExtData_t *data);
var_int32_t lVarManage_GetExtDataUpInterval(var_uint16_t usDevType, var_uint16_t usDevNum);
void vVarManage_SetExtDataUpInterval(var_uint16_t usDevType, var_uint16_t usDevNum, var_uint32_t interval);
ExtData_t* pVarManage_GetExtDataWithUpProto(
    ExtData_t       *first,
    var_uint16_t    usDevType,
    var_uint16_t    usDevNum,
    var_uint16_t    usProtoType
    );
void vVarManage_ExtDataRefreshTask(void *parameter);
void vVarManage_ExtDataUpTask(void *parameter);

void vVarManage_TakeMutex( void );
void vVarManage_ReleaseMutex( void );

void bVarManage_UpdateAvgValue(VarAvg_t *varavg, var_double_t value);

extern var_uint16_t        g_xExtDataRegs[];
extern var_bool_t          g_xExtDataRegsFlag[];

#endif

