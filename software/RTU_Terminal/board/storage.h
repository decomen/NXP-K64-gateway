
#ifndef _STORAGE__H__
#define _STORAGE__H__

#define STORAGE_VER_STR         (0)

#define STORAGE_LINE_LIMIT      (2)

typedef enum {
    ST_P_SPIFLASH,
    ST_P_SD,

    ST_P_MAX,
} eStoragePath;

typedef enum {
    ST_T_RT_DATA,
    ST_T_MINUTE_DATA,
    ST_T_HOURLY_DATA,
    ST_T_DIDO_DATA,

    ST_T_LOG_DATA,

    // for CC
    ST_T_CC_BJDC_DATA,

    ST_T_COMMIT,

    ST_T_MAX,
} eStorageType;

typedef enum {
    LOG_LVL_ASSERT      = 0,
    LOG_LVL_ERROR       = 1,
    LOG_LVL_WARN        = 2,
    LOG_LVL_INFO        = 3,
    LOG_LVL_DEBUG       = 4,
    LOG_LVL_VERBOSE     = 5,

    LOG_LVL_MAX,
} eLogLvl_t;

typedef struct {
    double          value;
    char            ident[12+1];
    char            alias[8+1];
} StorageDataItem_t;

typedef struct {
    eLogLvl_t       lvl;
    char            log[256+1];
} StorageLogItem_t;

// for cc_bjdc
typedef struct {
    int             desc_id;
    double          value;
    rt_uint8_t      flag;
} Storage_CC_BJDC_Item_t;

typedef struct {
    eStorageType    type;
    rt_time_t       time;
    union {
        StorageDataItem_t       xData;
        Storage_CC_BJDC_Item_t  xCC_BJDC;
        StorageLogItem_t        xLog;
    } xItem;
} StorageItem_t;

typedef struct {
    rt_bool_t       bEnable;
    rt_uint32_t     ulStep;
    eStoragePath    ePath;
} storage_cfg_t;

extern storage_cfg_t g_storage_cfg;

void vStorageInit( void );
rt_bool_t bStorageAddData( eStorageType type, char *ident, double value, char *alias );
rt_bool_t bStorageAddLog(eLogLvl_t lvl, const char *fmt, ...);
rt_bool_t bStorageAdd_elog(int elvl, const char *log, int size);

rt_bool_t bStorageAdd_CC_BJDC_Data(int desc_id, double value);

rt_bool_t log_upload(int level, const char *tag, const char *log, int size, rt_uint32_t time);
rt_bool_t bStorageDoCommit(void);
rt_bool_t bStorageDoClose(void);

int nStorage_CC_BJDC_DataDesc(
    const char *meter_id,
    const char *func_idex
);

#define bStorageLogAssert( fmt, ... )   bStorageAddLog( LOG_LVL_ASSERT  , fmt, ##__VA_ARGS__ )
#define bStorageLogError( fmt, ... )    bStorageAddLog( LOG_LVL_ERROR   , fmt, ##__VA_ARGS__ )
#define bStorageLogWarn( fmt, ... )     bStorageAddLog( LOG_LVL_WARN    , fmt, ##__VA_ARGS__ )
#define bStorageLogInfo( fmt, ... )     bStorageAddLog( LOG_LVL_INFO    , fmt, ##__VA_ARGS__ )
#define bStorageLogVerbose( fmt, ... )  bStorageAddLog( LOG_LVL_VERBOSE , fmt, ##__VA_ARGS__ )
#define bStorageLogDebug( fmt, ... )    bStorageAddLog( LOG_LVL_DEBUG   , fmt, ##__VA_ARGS__ )

rt_err_t storage_cfg_init( void );

#endif

