
#include <board.h>
#include "sqlite3.h"
#include "time.h"

#define _DATA_VERSION           (2)

// 每次占满后删除记录数(建议>100, 避免频繁进行数据库删除操作影响速率)
#define _DEL_NUM                (100)

static sqlite3 *s_db = RT_NULL;
static rt_bool_t s_dbinit = RT_FALSE;

#define TABLE_DIDO_DATA                 "DIDO_data"
#define TABLE_RT_DATA                   "RT_DATA"
#define TABLE_MINUTE_DATA               "Minute_data"
#define TABLE_HOURLY_DATA               "Hourly_data"
#define TABLE_LOG_DATA                  "Log_data"

#define TABLE_CFG_VAREXT                "cfg_varext"

// CC Table
#define TABLE_CC_BJDC_DECS              "CC_BJDC_desc"
#define TABLE_CC_BJDC_DATA              "CC_BJDC_data"

#define CREATE_VER_TABLE_SQL            "CREATE TABLE IF NOT EXISTS data_version " \
                                        "(id INTEGER PRIMARY KEY NOT NULL DEFAULT 0, ver INTEGER NOT NULL DEFAULT 0 );"

// fixed : id, time
#define CREATE_TABLE_SQL(_table)        "CREATE TABLE IF NOT EXISTS "_table" " \
                                        "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL DEFAULT 0, " \
                                        "time INTEGER, ident TEXT(12), value REAL(10,3), alias TEXT(8) );"

// fixed : id, time
#define CREATE_DIDO_TABLE_SQL(_table)   "CREATE TABLE IF NOT EXISTS "_table" " \
                                        "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL DEFAULT 0, " \
                                        "time INTEGER, ident TEXT(12), value TINYINT );"

// fixed : id, time
#define CREATE_LOG_TABLE_SQL(_table)    "CREATE TABLE IF NOT EXISTS "_table" " \
                                        "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL DEFAULT 0, " \
                                        "time INTEGER, level INTEGER, log TEXT(256), flag TINYINT );"

#define CREATE_CC_BJDC_DESC_TABLE_SQL   "CREATE TABLE IF NOT EXISTS " TABLE_CC_BJDC_DECS " " \
                                        "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL DEFAULT 0, " \
                                        "meter_id TEXT, func_idex TEXT );"

// fixed : id, time
#define CREATE_CC_BJDC_DATA_TABLE_SQL   "CREATE TABLE IF NOT EXISTS " TABLE_CC_BJDC_DATA " " \
                                        "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL DEFAULT 0, " \
                                        "time INTEGER, desc_id INTEGER, value REAL, flag TINYINT );"

#define CREATE_INDEX_SQL(_table)        "CREATE INDEX IF NOT EXISTS "_table"_INDEX ON "_table" (time ASC);"
#define DROP_TABLE_SQL(_table)          "DROP TABLE IF EXISTS "_table

// fixed : id, time
#define CREATE_CFG_VAREXT_TABLE_SQL     "CREATE TABLE IF NOT EXISTS " TABLE_CFG_VAREXT " " \
                                        "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL DEFAULT 0, " \
                                        "time INTEGER, name TEXT, cfg TEXT );"

static rt_mutex_t s_db_mutex;
static rt_bool_t s_transaction = RT_FALSE;
static rt_uint32_t s_limit_count[ST_T_MAX] = { 0 };
/*static sqlite3_stmt* s_rt_data_insert_stmt = RT_NULL;
static sqlite3_stmt* s_minute_data_insert_stmt = RT_NULL;
static sqlite3_stmt* s_hourly_data_insert_stmt = RT_NULL;*/
static void __begin_transaction(void);
static void __commit_transaction(void);
static void __rollback_transaction(void);

static rt_mq_t s_stroage_mq;

void rt_storage_thread(void *parameter);

void vStorageInit(void)
{
    s_db_mutex = rt_mutex_create("sto_mtx", RT_IPC_FLAG_FIFO);

    s_stroage_mq = rt_mq_create("sto_msg", sizeof(StorageItem_t), STORAGE_LINE_LIMIT, RT_IPC_FLAG_PRIO);

    if (s_stroage_mq) {

        rt_thread_t storage_thread = rt_thread_create("storage", rt_storage_thread, RT_NULL, 0x1200, 24, 20);

        if (storage_thread != RT_NULL) {
            rt_thddog_register(storage_thread, 60);
            rt_thread_startup(storage_thread);
        }
    } else {
        rt_kprintf("vStorageInit : no mem\n");
    }
}

rt_bool_t bStorageAddData(eStorageType type, char *ident, double value, char *alias)
{
    if (s_db_mutex && s_dbinit) {
        StorageItem_t *item = rt_calloc(1,sizeof(StorageItem_t));
        if( item ) {
            item->type = type;
            item->time = time(0);
            item->xItem.xData.value = value;

            if (ident) {
                rt_strncpy(item->xItem.xData.ident, ident, 12);
            }
            if (alias) {
                rt_strncpy(item->xItem.xData.alias, alias, 8);
            }

            // wait if transaction
            rt_mutex_take(s_db_mutex, RT_WAITING_FOREVER);
            rt_mutex_release(s_db_mutex);

            rt_mq_send(s_stroage_mq, item, sizeof(StorageItem_t));

            rt_free(item);
        }
    }

    return RT_TRUE;
}

rt_bool_t bStorageAddLog(eLogLvl_t lvl, const char *fmt, ...)
{
    if (s_db_mutex && s_dbinit && fmt) {
    
        StorageItem_t *item = rt_calloc(1,sizeof(StorageItem_t));
        if( item ) {
            item->type = ST_T_LOG_DATA;
            item->time = time(0);
            item->xItem.xLog.lvl = lvl;
            
            va_list args;
            va_start(args, fmt);
            rt_vsnprintf(item->xItem.xLog.log, sizeof(item->xItem.xLog.log) - 1, fmt, args);
            va_end(args);
            // wait if transaction
            rt_mutex_take(s_db_mutex, RT_WAITING_FOREVER);
            rt_mutex_release(s_db_mutex);

            rt_mq_send(s_stroage_mq, item, sizeof(StorageItem_t));
            rt_free(item);
        }
    }

    return RT_TRUE;
}

rt_bool_t bStorageAdd_elog(int elvl, const char *log, int size)
{
    if (s_db_mutex && s_dbinit && log && size > 0) {
    
        StorageItem_t *item = rt_calloc(1,sizeof(StorageItem_t));
        if( item ) {
            item->type = ST_T_LOG_DATA;
            item->time = time(0);
            item->xItem.xLog.lvl = elvl;

            if(size > sizeof(item->xItem.xLog.log)-1) size = sizeof(item->xItem.xLog.log)-1;
            strncpy(item->xItem.xLog.log, log, size);
            item->xItem.xLog.log[size] = '\0';
            // wait if transaction
            rt_mutex_take(s_db_mutex, RT_WAITING_FOREVER);
            rt_mutex_release(s_db_mutex);

            rt_mq_send(s_stroage_mq, item, sizeof(StorageItem_t));
            rt_free(item);
        }
    }

    return RT_TRUE;
}

rt_bool_t bStorageCommit( int time )
{
    if (s_db_mutex && s_dbinit) {
        if (RT_EOK == rt_mutex_take(s_db_mutex, RT_WAITING_NO)) {
            static StorageItem_t commit_item = { ST_T_COMMIT };
            commit_item.time = time;
            rt_mutex_release(s_db_mutex);
            rt_mq_send(s_stroage_mq, (void *)&commit_item, sizeof(StorageItem_t));
        }
    }

    return RT_TRUE;
}

rt_bool_t bStorageDoCommit(void)
{
    if (s_db_mutex && s_dbinit) {
        rt_thddog_feed("bStorageDoCommit take");
        rt_mutex_take(s_db_mutex, RT_WAITING_FOREVER);
        rt_thddog_feed("__commit_transaction with bStorageDoCommit");
        __commit_transaction();
        rt_thddog_feed("__begin_transaction with bStorageDoCommit");
        __begin_transaction();
        rt_mutex_release(s_db_mutex);
        return RT_TRUE;
    }

    return RT_FALSE;
}

rt_bool_t bStorageDoClose(void)
{
    if (s_db_mutex && s_dbinit) {
        rt_mutex_take(s_db_mutex, RT_WAITING_FOREVER);
        sqlite3_close(s_db);
        s_dbinit = RT_FALSE;
        rt_mutex_release(s_db_mutex);
        return RT_TRUE;
    }

    return RT_FALSE;
}

static void __begin_transaction(void)
{
    if (s_dbinit) {
        s_transaction = (SQLITE_OK == sqlite3_exec(s_db, "begin;", 0, 0, 0));
    }

    rt_kprintf("__begin_transaction = %d\n", s_transaction);
}

static void __commit_transaction(void)
{
    if (s_dbinit && s_transaction) {
        if (sqlite3_exec(s_db, "commit;", 0, 0, 0) != SQLITE_OK) {
            __rollback_transaction();
        }
        s_transaction = RT_FALSE;
    }
}

static void __rollback_transaction(void)
{
    if (s_dbinit) {
        sqlite3_exec(s_db, "rollback;", 0, 0, 0);
    }
}

static int __get_data_version(void)
{
    int ver = 0;
    sqlite3_stmt *stmt = RT_NULL;

    if (SQLITE_OK == sqlite3_prepare_v2(s_db, "SELECT ver FROM data_version WHERE 1", -1, &stmt, 0)) {
        if (SQLITE_ROW == sqlite3_step(stmt)) {
            if (SQLITE_INTEGER == sqlite3_column_type(stmt, 0)) {
                ver = sqlite3_column_int(stmt, 0);
            }
        }
    }

    if (stmt) {
        sqlite3_finalize(stmt);
    }

    return ver;
}

static rt_bool_t __update_data_version(int ver)
{
    rt_bool_t ret = RT_TRUE;
    char sql[64];
    sprintf(sql, "REPLACE INTO data_version(id,ver) VALUES (0,%d);", ver);
    char *errmsg = RT_NULL;

    ret = (SQLITE_OK == sqlite3_exec(s_db, sql, NULL, NULL, &errmsg));

    if (errmsg) {
        rt_kprintf("sqlite_alter_table_flag ==> ERR: %s\n", errmsg);
        sqlite3_free(errmsg);
    }

    return ret;
}

static void __relimit_table(eStorageType type)
{
    rt_uint32_t maxid = sqlite3_last_insert_rowid(s_db);
    rt_uint32_t minid = maxid;
    char *table = "";
    char sql[128];

    if (maxid > s_limit_count[type]) {
        sqlite3_stmt *stmt = RT_NULL;
        switch (type) {
        case ST_T_RT_DATA:
            table = TABLE_RT_DATA;
            break;

        case ST_T_MINUTE_DATA:
            table = TABLE_MINUTE_DATA;
            break;

        case ST_T_HOURLY_DATA:
            table = TABLE_HOURLY_DATA;
            break;

        case ST_T_DIDO_DATA:
            table = TABLE_DIDO_DATA;
            break;

        case ST_T_LOG_DATA:
            table = TABLE_LOG_DATA;
            break;

        case ST_T_CC_BJDC_DATA:
            table = TABLE_CC_BJDC_DATA;
            break;
        }

        rt_sprintf(sql, "SELECT min(id) FROM %s WHERE 1", table);
        if (SQLITE_OK == sqlite3_prepare_v2(s_db, sql, -1, &stmt, 0)) {
            if (SQLITE_ROW == sqlite3_step(stmt)) {
                if (SQLITE_INTEGER == sqlite3_column_type(stmt, 0)) {
                    minid = sqlite3_column_int(stmt, 0);
                }
            }
        }

        if (stmt) {
            sqlite3_finalize(stmt);
        }

        if (maxid - minid > s_limit_count[type]) {
            rt_sprintf(sql, "DELETE FROM %s WHERE id<%d", table, maxid - s_limit_count[type] + _DEL_NUM);
            sqlite3_exec(s_db, sql, NULL, NULL, NULL);
        }
    }
}

void rt_storage_thread(void *parameter)
{
    rt_tick_t start_tick = rt_tick_get();
    rt_bool_t insert = RT_FALSE;

    int result = SQLITE_OK;
    char *errmsg = RT_NULL;

    rt_thddog_feed("sqlite3_config");
    sqlite3_config(SQLITE_CONFIG_LOOKASIDE, 0, 0);
    sqlite3_soft_heap_limit64(16 * 1024);

    rt_thddog_feed("sqlite3_open");
    switch (g_storage_cfg.ePath) {
    case ST_P_SPIFLASH:
        result = sqlite3_open("/data/data.db", &s_db);
        break;

    case ST_P_SD:
        result = sqlite3_open(BOARD_SDCARD_MOUNT"/data/data.db", &s_db);
        break;

    default:
        break;
    }

    if (result != SQLITE_OK) {
        goto _err;
    }

    rt_thddog_feed("sqlite3_exec");
    if (SQLITE_OK != sqlite3_exec(s_db, CREATE_TABLE_SQL(TABLE_RT_DATA), NULL, NULL, &errmsg) ||
        SQLITE_OK != sqlite3_exec(s_db, CREATE_VER_TABLE_SQL, NULL, NULL, &errmsg) ||
        SQLITE_OK != sqlite3_exec(s_db, CREATE_TABLE_SQL(TABLE_MINUTE_DATA), NULL, NULL, &errmsg) ||
        SQLITE_OK != sqlite3_exec(s_db, CREATE_TABLE_SQL(TABLE_HOURLY_DATA), NULL, NULL, &errmsg) ||
        SQLITE_OK != sqlite3_exec(s_db, CREATE_DIDO_TABLE_SQL(TABLE_DIDO_DATA), NULL, NULL, &errmsg) ||
        SQLITE_OK != sqlite3_exec(s_db, CREATE_LOG_TABLE_SQL(TABLE_LOG_DATA), NULL, NULL, &errmsg) ||
        SQLITE_OK != sqlite3_exec(s_db, CREATE_CC_BJDC_DESC_TABLE_SQL, NULL, NULL, &errmsg) ||
        SQLITE_OK != sqlite3_exec(s_db, CREATE_CC_BJDC_DATA_TABLE_SQL, NULL, NULL, &errmsg) ||
        SQLITE_OK != sqlite3_exec(s_db, CREATE_CFG_VAREXT_TABLE_SQL, NULL, NULL, &errmsg) ||

        SQLITE_OK != sqlite3_exec(s_db, CREATE_INDEX_SQL(TABLE_RT_DATA), NULL, NULL, &errmsg) ||
        SQLITE_OK != sqlite3_exec(s_db, CREATE_INDEX_SQL(TABLE_MINUTE_DATA), NULL, NULL, &errmsg) ||
        SQLITE_OK != sqlite3_exec(s_db, CREATE_INDEX_SQL(TABLE_HOURLY_DATA), NULL, NULL, &errmsg) ||
        SQLITE_OK != sqlite3_exec(s_db, CREATE_INDEX_SQL(TABLE_DIDO_DATA), NULL, NULL, &errmsg) ||
        SQLITE_OK != sqlite3_exec(s_db, CREATE_INDEX_SQL(TABLE_LOG_DATA), NULL, NULL, &errmsg)) {
        goto _err;
    }

    s_dbinit = RT_TRUE;

    {
        // 重要 log 暂存 1000 条上限
        s_limit_count[ST_T_LOG_DATA] = 1000;

        // CC BJDC 暂定 5万条(约1M空间)
        s_limit_count[ST_T_CC_BJDC_DATA] = 50000;
        long long remain = 0;
        switch (g_storage_cfg.ePath) {
        case ST_P_SPIFLASH:
            if (g_xCpuUsage.flash_size > 5 * 1024 && g_xCpuUsage.flash_use < g_xCpuUsage.flash_size) {
                remain = (long long)(g_xCpuUsage.flash_size - 5 * 1024) * 1024;
            }
            break;

        case ST_P_SD:
            if (g_xCpuUsage.sd_size > 5 * 1024 && g_xCpuUsage.sd_use < g_xCpuUsage.sd_size) {
                remain = (long long)(g_xCpuUsage.sd_size - 5 * 1024) * 1024;
            }
            break;
        }
        s_limit_count[ST_T_RT_DATA] = (rt_uint32_t)(remain * 60 / 100 / 40);
        s_limit_count[ST_T_MINUTE_DATA] = (rt_uint32_t)(remain * 20 / 100 / 40);
        s_limit_count[ST_T_HOURLY_DATA] = (rt_uint32_t)(remain * 10 / 100 / 40);
        s_limit_count[ST_T_DIDO_DATA] = (rt_uint32_t)(remain * 10 / 100 / 32);
    }

    rt_thddog_feed("__begin_transaction");
    __begin_transaction();

    for (;;) {
        StorageItem_t item;

        rt_thddog_feed("rt_mq_recv with 5 sec");
        if (RT_EOK == rt_mq_recv(s_stroage_mq, (void *)&item, sizeof(StorageItem_t), rt_tick_from_millisecond(5000))) {
            if (s_db && item.type < ST_T_MAX) {
                if (ST_T_COMMIT == item.type) {
                    rt_thread_delay(item.time * RT_TICK_PER_SECOND / 1000);
                    rt_thddog_feed("__commit_transaction");
                    __commit_transaction();
                    __begin_transaction();
                } else if (s_limit_count[item.type] > 0) {
                    sqlite3_stmt *stmt = RT_NULL;
                    int result = SQLITE_ERROR;
                    rt_mutex_take(s_db_mutex, RT_WAITING_FOREVER);
                    switch (item.type) {
                    case ST_T_RT_DATA:
                        result = sqlite3_prepare_v2(s_db, "INSERT INTO "TABLE_RT_DATA" (id,time,ident,value,alias) values(NULL,?,?,?,?)", -1, &stmt, 0);
                        break;

                    case ST_T_MINUTE_DATA:
                        result = sqlite3_prepare_v2(s_db, "INSERT INTO "TABLE_MINUTE_DATA" (id,time,ident,value,alias) values(NULL,?,?,?,?)", -1, &stmt, 0);
                        break;

                    case ST_T_HOURLY_DATA:
                        result = sqlite3_prepare_v2(s_db, "INSERT INTO "TABLE_HOURLY_DATA" (id,time,ident,value,alias) values(NULL,?,?,?,?)", -1, &stmt, 0);
                        break;

                    case ST_T_DIDO_DATA:
                        result = sqlite3_prepare_v2(s_db, "INSERT INTO "TABLE_DIDO_DATA" (id,time,ident,value) values(NULL,?,?,?)", -1, &stmt, 0);
                        break;

                    case ST_T_LOG_DATA:
                        result = sqlite3_prepare_v2(s_db, "INSERT INTO "TABLE_LOG_DATA" (id,time,level,log,flag) values(NULL,?,?,?,?)", -1, &stmt, 0);
                        break;

                    case ST_T_CC_BJDC_DATA:
                        result = sqlite3_prepare_v2(s_db, "INSERT INTO "TABLE_CC_BJDC_DATA" (id,time,desc_id,value,flag) values(NULL,?,?,?,?)", -1, &stmt, 0);
                        break;
                    }

                    rt_thddog_feed("sqlite3_bind");
                    if (SQLITE_OK == result) {
                        sqlite3_bind_int(stmt, 1, item.time);
                        if (ST_T_LOG_DATA == item.type) {
                            sqlite3_bind_int(stmt, 2, item.xItem.xLog.lvl);
                            sqlite3_bind_text(stmt, 3, item.xItem.xLog.log, strlen(item.xItem.xLog.log), NULL);
                            sqlite3_bind_int(stmt, 4, 0);
                        } else if (ST_T_CC_BJDC_DATA == item.type) {
                            sqlite3_bind_int(stmt, 2, item.xItem.xCC_BJDC.desc_id);
                            sqlite3_bind_double(stmt, 3, item.xItem.xCC_BJDC.value);
                            sqlite3_bind_int(stmt, 4, item.xItem.xCC_BJDC.flag);
                        } else {
                            sqlite3_bind_text(stmt, 2, item.xItem.xData.ident, strlen(item.xItem.xData.ident), NULL);
                            if (ST_T_DIDO_DATA == item.type) {
                                sqlite3_bind_int(stmt, 3, (int)item.xItem.xData.value);
                            } else {
                                sqlite3_bind_double(stmt, 3, item.xItem.xData.value);
                                sqlite3_bind_text(stmt, 4, item.xItem.xData.alias, strlen(item.xItem.xData.alias), NULL);
                            }
                        }
                        rt_thddog_feed("sqlite3_step");
                        if (SQLITE_DONE == sqlite3_step(stmt)) {
                            sqlite3_finalize(stmt); stmt = RT_NULL;
                            rt_thddog_feed("__relimit_table");
                            __relimit_table(item.type);
                        }
                        insert = RT_TRUE;
                    }

                    if (stmt) {
                        sqlite3_finalize(stmt);
                    }
                    rt_mutex_release(s_db_mutex);
                }
            }
        }

        if (rt_tick_get() - start_tick > rt_tick_from_millisecond(10000) && insert) {
            rt_mutex_take(s_db_mutex, RT_WAITING_FOREVER);
            rt_thddog_feed("__commit_transaction with 10 sec");
            __commit_transaction();
            __begin_transaction();
            insert = RT_FALSE;
            rt_mutex_release(s_db_mutex);
            start_tick = rt_tick_get();
        }
    }


    _err:
    s_dbinit = RT_FALSE;

    if (s_db) {
        sqlite3_close(s_db); s_db = RT_NULL;
    }

    if (errmsg) {
        rt_kprintf("sqlite ==> ERR: %s\n", errmsg);
        sqlite3_free(errmsg);
    }

    rt_thddog_exit();
}

#define _STORAGE_CFG_DEFAULT       { RT_FALSE, 5, ST_P_SPIFLASH }

const storage_cfg_t c_storage_default_cfg = _STORAGE_CFG_DEFAULT;
storage_cfg_t g_storage_cfg = _STORAGE_CFG_DEFAULT;

static void prvSetStorageCfgDefault(void)
{
    g_storage_cfg = c_storage_default_cfg;
}

static void prvReadStorageCfgFromFs(void)
{
    if (!board_cfg_read(STORAGE_CFG_OFS, &g_storage_cfg, sizeof(g_storage_cfg))) {
        prvSetStorageCfgDefault();
    }
}

static void prvSaveStorageCfgToFs(void)
{
    if (!board_cfg_write(STORAGE_CFG_OFS, &g_storage_cfg, sizeof(g_storage_cfg))) {
        ; //prvSetStorageCfgDefault();
    }
}

rt_err_t storage_cfg_init(void)
{
    prvReadStorageCfgFromFs();

    if (ST_P_SD == g_storage_cfg.ePath && !rt_sd_in()) {
        g_storage_cfg.ePath = ST_P_SPIFLASH;
    }

    return RT_EOK;
}

void jsonStorageCfg(int n,cJSON *pItem)
{
    cJSON_AddNumberToObject(pItem, "se", g_storage_cfg.bEnable ? 1 : 0);
    cJSON_AddNumberToObject(pItem, "ss", g_storage_cfg.ulStep);
    cJSON_AddNumberToObject(pItem, "path", g_storage_cfg.ePath);
}

// for webserver
DEF_CGI_HANDLER(getStorageCfg)
{
    char *szRetJSON = RT_NULL;
    cJSON *pItem = cJSON_CreateObject();
    if(pItem) {
        cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
        jsonStorageCfg(0, pItem);
        szRetJSON = cJSON_PrintUnformatted(pItem);
        if(szRetJSON) {
            WEBS_PRINTF( szRetJSON );
            rt_free( szRetJSON );
        }
    }
    cJSON_Delete( pItem );

    WEBS_DONE(200);
}

rt_err_t setStorageCfgWithJson(cJSON *pCfg)
{
    rt_err_t err = RT_EOK;
    int se = cJSON_GetInt(pCfg, "se", -1);
    int ss = cJSON_GetInt(pCfg, "ss", -1);
    int path = cJSON_GetInt(pCfg, "path", -1);

    storage_cfg_t cfg_bak = g_storage_cfg;
    if (se >= 0) g_storage_cfg.bEnable = se;
    if (ss >= 0) g_storage_cfg.ulStep = ss;
    if (path >= 0) g_storage_cfg.ePath = path;

    if (ST_P_SD == g_storage_cfg.ePath && !rt_sd_in()) {
        err = RT_ERROR;
        g_storage_cfg.ePath = ST_P_SPIFLASH;
    }

    if (memcmp(&cfg_bak, &g_storage_cfg, sizeof(g_storage_cfg)) != 0) {
        prvSaveStorageCfgToFs();
    }

    return err;
}

DEF_CGI_HANDLER(setStorageCfg)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        err = setStorageCfgWithJson(pCfg);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(delLogData)
{
    rt_err_t err = RT_EOK;
    if (s_db_mutex && s_dbinit) {
        rt_mutex_take(s_db_mutex, RT_WAITING_FOREVER);
        {
            char *errmsg = RT_NULL;
            if(
                SQLITE_OK != sqlite3_exec(s_db, DROP_TABLE_SQL(TABLE_LOG_DATA), NULL, NULL, &errmsg) ||
                SQLITE_OK != sqlite3_exec(s_db, CREATE_LOG_TABLE_SQL(TABLE_LOG_DATA), NULL, NULL, &errmsg) ||
                SQLITE_OK != sqlite3_exec(s_db, CREATE_INDEX_SQL(TABLE_LOG_DATA), NULL, NULL, &errmsg)
            ) {
                err = RT_ERROR;
            }
            if (errmsg) {
                rt_kprintf("sqlite ==> ERR: %s\n", errmsg);
                sqlite3_free(errmsg);
            }
        }
        rt_mutex_release(s_db_mutex);
    }
    WEBS_PRINTF( "{\"ret\":%d}", err);
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(getLogData)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        rt_uint32_t start_time = (rt_uint32_t)cJSON_GetInt(pCfg, "start", 0);
        rt_uint32_t end_time = (rt_uint32_t)cJSON_GetInt(pCfg, "end", -1);
        //int lvl = cJSON_GetInt(pCfg, "lvl", -1);
        cJSON_Delete(pCfg);
        WEBS_PRINTF("{\"ret\":0,\"logs\":[");
        if (s_db_mutex && s_dbinit) {
            rt_mutex_take(s_db_mutex, RT_WAITING_FOREVER);
            {
                sqlite3_stmt *stmt = RT_NULL;
                char sql[150] = "";
                rt_sprintf(sql, "SELECT id,time,level,log FROM "TABLE_LOG_DATA" WHERE time>=%u AND time<=%u ORDER BY time ASC", start_time, end_time);
                if (SQLITE_OK == sqlite3_prepare_v2(s_db, sql, -1, &stmt, 0)) {
                    rt_bool_t first = RT_TRUE;
                    while (SQLITE_ROW == sqlite3_step(stmt)) {
                        int id = sqlite3_column_int(stmt, 0);
                        rt_uint32_t htime = sqlite3_column_int(stmt, 1);
                        int level = sqlite3_column_int(stmt, 2);
                        const char *log = sqlite3_column_text(stmt, 3);
                        if (log) {
                            cJSON *pItem = cJSON_CreateObject();
                            if(pItem) {
                                char *szItem = RT_NULL;
                                if (!first) WEBS_PRINTF(",");
                                first = RT_FALSE;
                                {
                                    struct tm lt;
                                    char date[32] = { 0 };
                                    localtime_r(&htime, &lt);
                                    rt_sprintf(date, "%04d-%02d-%02d %02d:%02d:%02d",
                                               lt.tm_year + 1900, lt.tm_mon + 1,
                                               lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
                                    cJSON_AddNumberToObject(pItem, "n", id);
                                    cJSON_AddStringToObject(pItem, "t", date);
                                    cJSON_AddNumberToObject(pItem, "l", level);
                                    cJSON_AddStringToObject(pItem, "g", log);
                                }
                                szItem = cJSON_PrintUnformatted(pItem);
                                
                                if(szItem) {
                                    WEBS_PRINTF(szItem);
                                    rt_free(szItem);
                                }
                            }
                            cJSON_Delete(pItem);
                        }
                    }
                }

                if (stmt) {
                    sqlite3_finalize(stmt);
                }
            }
            rt_mutex_release(s_db_mutex);
        }
    } else {
        err = RT_ERROR;
    }
    WEBS_PRINTF("]}");
    WEBS_DONE(200);
}

rt_bool_t bStorageAdd_CC_BJDC_Data(int desc_id, double value)
{
    if (s_db_mutex && s_dbinit) {
        StorageItem_t item = { ST_T_CC_BJDC_DATA, time(0), .xItem.xCC_BJDC = { desc_id, value, 0 } };

        // wait if transaction
        rt_mutex_take(s_db_mutex, RT_WAITING_FOREVER);
        rt_mutex_release(s_db_mutex);

        rt_mq_send(s_stroage_mq, &item, sizeof(StorageItem_t));
    }

    return RT_TRUE;
}

int nStorage_CC_BJDC_DataDesc(
    const char *meter_id,
    const char *func_idex
    )
{
    int desc_id = -1;
    if (s_db_mutex && s_dbinit) {
        rt_mutex_take(s_db_mutex, RT_WAITING_FOREVER);
        {
            char *sql = rt_calloc(1, 1024);

            if (sql) {
                sqlite3_stmt *stmt = RT_NULL;
                sprintf(sql,
                        "SELECT id FROM "
                        TABLE_CC_BJDC_DECS
                        " WHERE meter_id='%s' AND func_idex='%s'",
                        meter_id, func_idex
                       );
                if (SQLITE_OK == sqlite3_prepare_v2(s_db, sql, -1, &stmt, 0)) {
                    if (SQLITE_ROW == sqlite3_step(stmt)) {
                        if (SQLITE_INTEGER == sqlite3_column_type(stmt, 0)) {
                            desc_id = sqlite3_column_int(stmt, 0);
                        }
                    } else {
                        sprintf(sql,
                                "INSERT INTO "
                                TABLE_CC_BJDC_DECS
                                "(id,meter_id,func_idex)"
                                " VALUES (NULL,'%s','%s');",
                                meter_id, func_idex
                               );
                        if (SQLITE_OK == sqlite3_exec(s_db, sql, NULL, NULL, NULL)) {
                            desc_id = sqlite3_last_insert_rowid(s_db);
                        }
                    }
                }

                if (stmt) {
                    sqlite3_finalize(stmt);
                }

                rt_free(sql);
            }
        }
        rt_mutex_release(s_db_mutex);
    }

    return desc_id;
}

#include "varmanage.h"
void vVarManage_InsertNode(ExtData_t *data);
var_bool_t jsonFillExtDataInfo(ExtData_t *data, cJSON *pItem);
var_bool_t jsonParseExtDataInfo(cJSON *pItem, ExtData_t *data);
rt_bool_t storage_cfg_varext_loadlist( ExtDataList_t *pList )
{
    rt_bool_t ret = RT_FALSE;
    if (s_db_mutex && s_dbinit) {
        rt_mutex_take(s_db_mutex, RT_WAITING_FOREVER);
        {
            sqlite3_stmt *stmt = RT_NULL;
            if (SQLITE_OK == sqlite3_prepare_v2(s_db, "SELECT id,name,cfg FROM "TABLE_CFG_VAREXT" WHERE 1 ORDER BY id ASC", -1, &stmt, 0)) {
                while (SQLITE_ROW == sqlite3_step(stmt)) {
                    //int id = sqlite3_column_int(stmt, 0);
                    const char *name = sqlite3_column_text(stmt, 1);
                    const char *cfg = sqlite3_column_text(stmt, 2);
                    if (name && cfg) {
                        ExtData_t *data = VAR_MANAGE_CALLOC(1, sizeof(ExtData_t));
                        if(data) {
                            cJSON *pItem = cJSON_Parse(cfg);
                            if(pItem) {
                                if (jsonParseExtDataInfo(pItem, data)) {
                                    vVarManage_InsertNode(data);
                                } else {
                                    VAR_MANAGE_FREE(data);
                                }
                            } else {
                                VAR_MANAGE_FREE(data);
                            }
                            cJSON_Delete(pItem);
                        }
                        ret = RT_TRUE;
                    }
                }
            }

            if (stmt) {
                sqlite3_finalize(stmt);
            }
        }
        rt_mutex_release(s_db_mutex);
    }

    return ret;
}

rt_bool_t storage_cfg_varext_del(const char *name, int commit)
{
    rt_bool_t ret = RT_FALSE;
    if (s_db_mutex && s_dbinit) {
        rt_mutex_take(s_db_mutex, RT_WAITING_FOREVER);
        {
            char *sql = rt_calloc(1, 128);
            sprintf(sql, "DELETE FROM "TABLE_CFG_VAREXT" WHERE name='%s'", name);
            if (SQLITE_OK == sqlite3_exec(s_db, sql, NULL, NULL, NULL)) {
                ret = RT_TRUE;
            }
            rt_free(sql);
            if(commit) {
                bStorageCommit(500);
            }
        }
        rt_mutex_release(s_db_mutex);
    }
    return ret;
}

rt_bool_t storage_cfg_varext_del_all(int commit)
{
    rt_bool_t ret = RT_FALSE;
    if (s_db_mutex && s_dbinit) {
        rt_mutex_take(s_db_mutex, RT_WAITING_FOREVER);
        {
            if (SQLITE_OK == sqlite3_exec(s_db, "DELETE FROM "TABLE_CFG_VAREXT, NULL, NULL, NULL)) {
                ret = RT_TRUE;
            }
            if(commit) {
                bStorageCommit(500);
            }
        }
        rt_mutex_release(s_db_mutex);
    }
    return ret;
}

rt_bool_t storage_cfg_varext_update(const char *name, ExtData_t *data, int commit)
{
    rt_bool_t ret = RT_FALSE;
    if (s_db_mutex && s_dbinit) {
        rt_mutex_take(s_db_mutex, RT_WAITING_FOREVER);
        {
            cJSON *pItem = cJSON_CreateObject();
            if(pItem) {
                sqlite3_stmt *stmt = RT_NULL;
                char *sql = rt_calloc(1, 128);
                if(sql) {
                    char *szJSON = VAR_NULL;
                    sprintf(sql, "UPDATE "TABLE_CFG_VAREXT" SET name=?,cfg=? WHERE name='%s'", name);
                    if (SQLITE_OK == sqlite3_prepare_v2(s_db, sql, -1, &stmt, 0)) {
                        sqlite3_bind_text(stmt, 1, data->xName.szName, strlen(data->xName.szName), NULL);
                        if (jsonFillExtDataInfo(data, pItem) && (szJSON = cJSON_PrintUnformatted(pItem)) != RT_NULL) {
                            sqlite3_bind_text(stmt, 2, szJSON, strlen(szJSON), NULL);
                        }
                        if (SQLITE_DONE == sqlite3_step(stmt)) {
                            ret = RT_TRUE;
                        }
                    }
                    rt_free(szJSON);
                    rt_free(sql);
                }
                if (stmt) sqlite3_finalize(stmt);
            }
            cJSON_Delete(pItem);
            if(commit) {
                bStorageCommit(500);
            }
        }
        rt_mutex_release(s_db_mutex);
    }
    return ret;
}

rt_bool_t storage_cfg_varext_add(ExtData_t *data, int commit)
{
    rt_bool_t ret = RT_FALSE;
    if (s_db_mutex && s_dbinit) {
        rt_mutex_take(s_db_mutex, RT_WAITING_FOREVER);
        {
            cJSON *pItem = cJSON_CreateObject();
            if(pItem) {
                char *szJSON = VAR_NULL;
                sqlite3_stmt *stmt = RT_NULL;

                storage_cfg_varext_del((const char *)data->xName.szName, commit);
                if (SQLITE_OK == sqlite3_prepare_v2(s_db, "INSERT INTO "TABLE_CFG_VAREXT" values(NULL,?,?,?)", -1, &stmt, 0)) {
                    sqlite3_bind_int(stmt, 1, time(0));
                    sqlite3_bind_text(stmt, 2, (const char *)data->xName.szName, strlen(data->xName.szName), NULL);
                    if (jsonFillExtDataInfo(data, pItem) && (szJSON = cJSON_PrintUnformatted(pItem)) != RT_NULL) {
                        sqlite3_bind_text(stmt, 3, szJSON, strlen(szJSON), NULL);
                    }
                    if (SQLITE_DONE == sqlite3_step(stmt)) {
                        ret = RT_TRUE;
                    }
                }
                rt_free(szJSON);
                if (stmt) sqlite3_finalize(stmt);
            }
            cJSON_Delete(pItem);
            if(commit) {
                bStorageCommit(500);
            }
        }
        rt_mutex_release(s_db_mutex);
    }
    return ret;
}
extern struct netif *netif_list;
static rt_bool_t _s_in_upload = RT_FALSE;
rt_bool_t log_upload(int level, const char *tag, const char *log, int size, rt_time_t time)
{
    rt_bool_t ret = RT_FALSE;
    if(!_s_in_upload) {
        _s_in_upload = RT_TRUE;
        if(netif_list && (netif_list->flags & NETIF_FLAG_UP)) {
            hclient_session_t *session = hclient_create(512);
            if (session) {
                cJSON *json = cJSON_CreateObject();
                if (json) {
                    char strbuf[64] = {0};
                    cJSON_AddStringToObject(json, "product_model", PRODUCT_MODEL);
                    rt_memset(strbuf, 0, sizeof(strbuf));
                    rt_memcpy(strbuf, g_xDevInfoReg.xDevInfo.xSN.szSN, sizeof(DevSN_t));
                    cJSON_AddStringToObject(json, "sn", strbuf);
                    rt_sprintf(strbuf, "%d.%02d", SW_VER_MAJOR, SW_VER_MINOR);
                    cJSON_AddStringToObject(json, "software_no", strbuf);
                    rt_sprintf(strbuf, "%d.%02d", HW_VER_VERCODE/100, HW_VER_VERCODE%100);
                    cJSON_AddStringToObject(json, "device_no", strbuf);
                    cJSON_AddNumberToObject(json, "type", level);
                    cJSON_AddStringToObject(json, "code", _STR(tag));
                    if(log) {
                        char *log_buf = rt_calloc(1, size+1);
                        if(log_buf) {
                            rt_strncpy(log_buf, log, size);
                            log_buf[size] = '\0';
                            cJSON_AddStringToObject(json, "msg", log_buf);
                            rt_free(log_buf);
                        }
                    }
                    {
                        struct tm lt;
                        localtime_r(&time, &lt);
                        rt_sprintf(strbuf, "%04d-%02d-%02d% 02d:%02d:%02d",
                                   lt.tm_year + 1900,
                                   lt.tm_mon + 1,
                                   lt.tm_mday,
                                   lt.tm_hour,
                                   lt.tm_min,
                                   lt.tm_sec
                                  );
                        cJSON_AddStringToObject(json, "date_time", strbuf);
                    }
                    hclient_get_sign(json, strbuf, "sn", "device_no", "code", "date_time", NULL);
                    cJSON_AddStringToObject(json, "sign", strbuf);
                    {
                        char *szJson = cJSON_PrintUnformatted(json);
                        if (szJson) {
                            //rt_kprintf("log_upload:%s\n", szJson);
                            hclient_err_e err = hclient_post(session, HTTP_SERVER_HOST"/log_add.action", szJson, 5000);
                            rt_free(szJson);
                            if (HCLIENT_ERR_OK == err) {
                                ret = RT_TRUE;
                            }
                        }
                    }
                    cJSON_Delete(json);
                }
                hclient_destroy(session);
            }
        }
    }
    _s_in_upload = RT_FALSE;
    return ret;
}

