/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 *
 */

/**
 * @addtogroup k64
 */
/*@{*/

#include <board.h>
#include <rtthread.h>
#include <time.h>
#include "mdtypedef.h"

#include "rtt_drv.h"
#include "radio.h"

#include "mxml.h"
#include "common.h"
#include "sntp.h"
#include "board_lua.h"

#ifdef RT_USING_LWIP
extern void lwip_sys_init(void);
extern struct netif *netif_list;
#endif

rt_thread_t g_AIReadThread;
rt_thread_t g_DIReadThread;
rt_thread_t g_DOWriteThread;
rt_thread_t g_ExtDataRefreshThread;

extern mdBOOL g_bIsTestMode;

static void __doSystemReset(void *arg)
{
    bStorageDoClose();
    NVIC_SystemReset();
}

void vDoSystemReset(void)
{
    static rt_timer_t reset_timer = RT_NULL;
    if (!reset_timer) {
        reset_timer = rt_timer_create(
                      "reset",
                      __doSystemReset,
                      RT_NULL,
                      2*RT_TICK_PER_SECOND,
                      RT_TIMER_FLAG_ONE_SHOT
                      );
    }
    if (reset_timer) {
        rt_timer_start(reset_timer);
    } else {
        bStorageDoClose();
        NVIC_SystemReset();
    }
}

void rt_thread_idle(void)
{
    static rt_tick_t regtick = 0;
    extern rt_tick_t g_ulLastBusyTick;
    rt_tick_t tickNow = rt_tick_get();

    cpu_usage();

    if (tickNow - g_ulLastBusyTick >= rt_tick_from_millisecond(1000)) {
        g_ulLastBusyTick = tickNow;
        rt_pin_write(BOARD_GPIO_NET_LED, PIN_HIGH);
    }

    if (tickNow - regtick >= rt_tick_from_millisecond(30000)) {
        // 此处不需要调用 reg_check, 用于释放空闲任务所占用的栈空间
        //g_isreg = reg_check(g_reg.key);
        if (!g_isreg) {
            reg_testdo(rt_millisecond_from_tick(tickNow - regtick) / 1000);
            if (reg_testover()) {
                rt_kprintf("test time over\n");
                if (
#ifdef USR_TEST_CODE
                    !g_bIsTestMode &&
#endif
                    !g_istestover_poweron) {
                    rt_kprintf("reset system\n");
                    NVIC_SystemReset();
                }
            }
        }
        regtick = tickNow;
    }
}

int compar(const void *a, const void *b)
{
    return *(var_float_t *)a > *(var_float_t *)b;
}

AIResult_t g_AIEngUnitResult;
AIResult_t g_AIMeasResult;
var_double_t g_dAIStorage_xx[ADC_CHANNEL_NUM] = { 0.0f };
int g_nAIStorageCnt[ADC_CHANNEL_NUM] = { 0 };
rt_tick_t g_dAI_last_storage_tick[ADC_CHANNEL_NUM] = { 0 };


var_double_t g_dAIHourStorage_xx[ADC_CHANNEL_NUM] = { 0.0f };
int g_nAIHourStorageCnt[ADC_CHANNEL_NUM] = { 0 };
rt_tick_t g_dAIHour_last_storage_tick[ADC_CHANNEL_NUM] = { 0 };

void vAIReadTask(void *parameter)
{
  //const rt_uint8_t chan_tbl[] = { 6, 7, 3, 2, 1, 0, 4, 5 };
    const rt_uint8_t chan_tbl[] = { 6, 7, 0, 1, 2, 3, 4, 5 };
    s_AdcValue_t xAdcVal;
    //static var_float_t fAI_xx[ADC_CHANNEL_NUM][10] = { 0.0f };
    //static var_float_t fAIEng_xx[ADC_CHANNEL_NUM][10] = { 0.0f };
    static analog_cfg_t s_analog_cfgs[ADC_CHANNEL_NUM];

    while (1) {
        memcpy(&s_analog_cfgs[0], &g_analog_cfgs[0], sizeof(g_analog_cfgs));
        //  for( int count = 0; count < 10; count++ ) {
        for (ADC_CHANNEL_E chan = ADC_CHANNEL_0; chan <= ADC_CHANNEL_7; chan++) {
            int index = chan_tbl[chan];
            analog_cfg_t *pCfg = &s_analog_cfgs[index];
            if (1/*pCfg->enable*/) {
                if (pCfg->unit!=Unit_Meter) pCfg->ext_corr.factor = 0;
				if(ADC_MODE_NORMAL == gAdcCfgPram.eMode){
               		 vGetAdcValue(chan, Range_0_20MA, pCfg->range, pCfg->ext_range_min, pCfg->ext_range_max, pCfg->ext_corr, &xAdcVal);
				}else if(ADC_MODE_TEST == gAdcCfgPram.eMode){
					vGetAdcValueTest(chan, Range_0_20MA, pCfg->range, pCfg->ext_range_min, pCfg->ext_range_max, pCfg->ext_corr, &xAdcVal);
				}else if(ADC_MODE_CALC == gAdcCfgPram.eMode){
					vGetAdcValueCal(chan, Range_0_20MA,  pCfg->range, pCfg->ext_range_min, pCfg->ext_range_max, pCfg->ext_corr, &xAdcVal);
				}
            } else {
                memset(&xAdcVal, 0, sizeof(s_AdcValue_t));
            }

			//rt_kprintf("chan = %d,adc = %d , I = %f\r\n", chan, xAdcVal.usEngUnit,xAdcVal.fMeasure);

            //	float vol = (float)xAdcVal.usEngUnit * 5.0 / 16777215.0f ;
            //	rt_kprintf("chan = %d, adc = %d, vol = %d.%d , meter = %d.%d\r\n", chan , xAdcVal.usEngUnit, (int)((int)(vol * 10000) / 10000),
            //	(int)((int)(vol*10000)%10000),(int)((int)(xAdcVal.fPercentUnit * 10000) / 10000), (int)((int)(xAdcVal.fPercentUnit*10000)%10000));

            // fAIEng_xx[chan][count] = (float)xAdcVal.usEngUnit;
            g_AIEngUnitResult.fAI_xx[index] = (float)xAdcVal.usEngUnit;
            g_AIMeasResult.fAI_xx[index] = xAdcVal.fMeasure;

            switch (pCfg->unit) {
            case Unit_Eng:
                //fAI_xx[chan][count] = (float)xAdcVal.usEngUnit;
                g_xAIResultReg.xAIResult.fAI_xx[index] = (float)xAdcVal.usEngUnit;
                break;
            case Unit_Binary:
                // fAI_xx[chan][count] = (float)xAdcVal.usBinaryUnit;
                g_xAIResultReg.xAIResult.fAI_xx[index] = (float)xAdcVal.usBinaryUnit;
                break;
            case Unit_Percent:
                //fAI_xx[chan][count] = xAdcVal.fPercentUnit;
                g_xAIResultReg.xAIResult.fAI_xx[index] = xAdcVal.fPercentUnit;
                break;
            case Unit_Meter:
                //fAI_xx[chan][count] = xAdcVal.fMeterUnit;
                g_xAIResultReg.xAIResult.fAI_xx[index] = xAdcVal.fMeterUnit;
                break;
            }

            g_dAIStorage_xx[index] += g_xAIResultReg.xAIResult.fAI_xx[index];
            g_nAIStorageCnt[index]++;
            g_dAIHourStorage_xx[index] += g_xAIResultReg.xAIResult.fAI_xx[index];
            g_nAIHourStorageCnt[index]++;

            if (g_storage_cfg.bEnable && pCfg->enable) {
                // min
                if (rt_tick_get() - g_dAI_last_storage_tick[index] >= rt_tick_from_millisecond(g_storage_cfg.ulStep * 60 * 1000)) {
                    char ident[10] = "";
                    if (g_nAIStorageCnt[index] > 0) {
                        var_double_t value = g_dAIStorage_xx[index] / g_nAIStorageCnt[index];
                        g_dAIStorage_xx[index] = 0;
                        g_nAIStorageCnt[index] = 0;
                        g_dAI_last_storage_tick[index] = rt_tick_get();
                        rt_sprintf(ident, "#AI_%d", index);
                        rt_thddog_suspend("bStorageAddData min");
                        bStorageAddData(ST_T_MINUTE_DATA, ident, value, "");
                        rt_thddog_resume();
                    }
                }

                // hour
                if (rt_tick_get() - g_dAIHour_last_storage_tick[index] >= rt_tick_from_millisecond(60 * 60 * 1000)) {
                    char ident[10] = "";
                    if (g_nAIHourStorageCnt[index] > 0) {
                        var_double_t value = g_dAIHourStorage_xx[index] / g_nAIHourStorageCnt[index];
                        g_dAIHourStorage_xx[index] = 0;
                        g_nAIHourStorageCnt[index] = 0;
                        g_dAIHour_last_storage_tick[index] = rt_tick_get();
                        rt_sprintf(ident, "#AI_%d", index);
                        rt_thddog_suspend("bStorageAddData hour");
                        bStorageAddData(ST_T_HOURLY_DATA, ident, value, "");
                        rt_thddog_resume();
                    }
                }
            }
            //}

        }

        rt_thread_delay(gAdcCfgPram.ChannelSleepTime);
        rt_thddog_feed("");

        /*
            for( ADC_CHANNEL_E chan = ADC_CHANNEL_0; chan <= ADC_CHANNEL_7; chan++ ) {
                var_float_t sum = 0, sum_eng = 0;
                qsort( fAI_xx[chan], sizeof(var_float_t), 10, compar );
                qsort( fAI_xx[chan], sizeof(var_float_t), 10, compar );
                for( int i = 3; i <= 6; i++ ) {
                    sum += fAI_xx[chan][i];
                    sum_eng += fAIEng_xx[chan][i];
                }
                sum /= 4; sum_eng /= 4;
                g_AIEngUnitResult.fAI_xx[chan_tbl[chan]] = sum_eng;
                g_xAIResultReg.xAIResult.fAI_xx[chan_tbl[chan]] = sum;
            }*/
    }
    g_AIReadThread = RT_NULL;
    rt_thddog_exit();
}

void vAIReadStop(void)
{
    if (g_AIReadThread) {
        rt_thddog_unregister(g_AIReadThread);
        if (RT_EOK == rt_thread_delete(g_AIReadThread)) {
            g_AIReadThread = RT_NULL;
        }
    }
}

rt_err_t xAIReadReStart(void)
{
    vAIReadStop();

    if (RT_NULL == g_AIReadThread) {
        g_AIReadThread = rt_thread_create("AI", vAIReadTask, RT_NULL, 0x300, 20, 20);

        if (g_AIReadThread) {
            rt_thddog_register(g_AIReadThread, 30);
            rt_thread_startup(g_AIReadThread);
            return RT_EOK;
        }
    }

    return RT_ERROR;
}

void vDIReadTask(void *parameter)
{
    eInOut_stat_t xStat;
    DIResult_t xDIResultBak = g_xDIResultReg.xDIResult;

    while (1) {
        for (eTTL_Input_Chanel_t chan = TTL_INPUT_1; chan <= TTL_INPUT_4; chan++) {
            vTTLInputputGet(chan, &xStat);
            g_xDIResultReg.xDIResult.usDI_xx[chan] = (xStat == PIN_RESET ? 0 : 1);
            if (g_di_cfgs[chan].enable && xDIResultBak.usDI_xx[chan] != g_xDIResultReg.xDIResult.usDI_xx[chan]) {
                if (g_storage_cfg.bEnable) {
                    char ident[10] = "";
                    rt_sprintf(ident, "#DI_%d", chan);
                    rt_thddog_suspend("bStorageAddData TLL");
                    bStorageAddData(ST_T_DIDO_DATA, ident, xDIResultBak.usDI_xx[chan] ? 1 : 0, RT_NULL);
                    rt_thddog_resume();
                }
                rt_thddog_suspend("lua_rtu_doexp TLL");
                lua_rtu_doexp(g_di_cfgs[chan].exp);
                rt_thddog_resume();
            }
        }

        for (eOnOff_Input_Chanel_t chan = ONOFF_INPUT_1; chan <= ONOFF_INPUT_4; chan++) {
            vOnOffInputGet(chan, &xStat);
            g_xDIResultReg.xDIResult.usDI_xx[4 + chan] = (xStat == PIN_RESET ? 0 : 1);
            if (g_di_cfgs[4 + chan].enable && xDIResultBak.usDI_xx[4 + chan] != g_xDIResultReg.xDIResult.usDI_xx[4 + chan]) {
                if (g_storage_cfg.bEnable) {
                    char ident[10] = "";
                    rt_sprintf(ident, "#DI_%d", 4 + chan);
                    rt_thddog_suspend("bStorageAddData OnOff");
                    bStorageAddData(ST_T_DIDO_DATA, ident, xDIResultBak.usDI_xx[4 + chan] ? 1 : 0, RT_NULL);
                    rt_thddog_resume();
                }
                rt_thddog_suspend("lua_rtu_doexp OnOff");
                lua_rtu_doexp(g_di_cfgs[4 + chan].exp);
                rt_thddog_resume();
            }
        }

        xDIResultBak = g_xDIResultReg.xDIResult;

        rt_thddog_feed("");
        rt_thread_delay(RT_TICK_PER_SECOND / 10);
    }
    rt_thddog_exit();
}

void vDIReadStop(void)
{
    if (g_DIReadThread) {
        rt_thddog_unregister(g_DIReadThread);
        if (RT_EOK == rt_thread_delete(g_DIReadThread)) {
            g_DIReadThread = RT_NULL;
        }
    }
}

rt_err_t xDIReadReStart(void)
{
    vDIReadStop();

    if (RT_NULL == g_DIReadThread) {
        g_DIReadThread = rt_thread_create("DI", vDIReadTask, RT_NULL, 0x200, 20, 20);

        if (g_DIReadThread) {
            rt_thddog_register(g_DIReadThread, 30);
            rt_thread_startup(g_DIReadThread);
            return RT_EOK;
        }
    }

    return RT_ERROR;
}

void vDOWriteTask(void *parameter)
{
    DOResult_t xDOResultBak = g_xDOResultReg.xDOResult;

    while (1) {

        for (eRelays_OutPut_Chanel_t chan = RELAYS_OUTPUT_1; chan <= RELAYS_OUTPUT_4; chan++) {
            if (g_xDOResultReg.xDOResult.usDO_xx[chan] != 0) {
                vRelaysOutputConfig(chan, PIN_SET);
            } else {
                vRelaysOutputConfig(chan, PIN_RESET);
            }

            if (xDOResultBak.usDO_xx[chan] != g_xDOResultReg.xDOResult.usDO_xx[chan]) {
                if (g_storage_cfg.bEnable) {
                    char ident[10] = "";
                    rt_sprintf(ident, "#DO_%d", chan);
                    rt_thddog_suspend("bStorageAddData Relays");
                    bStorageAddData(ST_T_DIDO_DATA, ident, xDOResultBak.usDO_xx[chan] ? 1 : 0, RT_NULL);
                    rt_thddog_resume();
                }
                rt_thddog_suspend("lua_rtu_doexp Relays");
                lua_rtu_doexp(g_do_cfgs[chan].exp);
                rt_thddog_resume();
            }
        }
        for (eTTL_Output_Chanel_t chan = TTL_OUTPUT_1; chan <= TTL_OUTPUT_2; chan++) {
            if (g_xDOResultReg.xDOResult.usDO_xx[4 + chan] != 0) {
                vTTLOutputConfig(chan, PIN_SET);
            } else {
                vTTLOutputConfig(chan, PIN_RESET);
            }

            if (xDOResultBak.usDO_xx[4 + chan] != g_xDOResultReg.xDOResult.usDO_xx[4 + chan]) {
                if (g_storage_cfg.bEnable) {
                    char ident[10] = "";
                    rt_sprintf(ident, "#DO_%d", 4 + chan);
                    rt_thddog_suspend("bStorageAddData TTL");
                    bStorageAddData(ST_T_DIDO_DATA, ident, xDOResultBak.usDO_xx[4 + chan] ? 1 : 0, RT_NULL);
                    rt_thddog_resume();
                }
                rt_thddog_suspend("lua_rtu_doexp TTL");
                lua_rtu_doexp(g_di_cfgs[4 + chan].exp);
                rt_thddog_resume();
            }
        }

        xDOResultBak = g_xDOResultReg.xDOResult;

        rt_thddog_feed("");
        rt_thread_delay(RT_TICK_PER_SECOND / 10);
    }
    rt_thddog_exit();
}

void vDOWriteStop(void)
{
    if (g_DOWriteThread) {
        rt_thddog_unregister(g_DOWriteThread);
        if (RT_EOK == rt_thread_delete(g_DOWriteThread)) {
            g_DOWriteThread = RT_NULL;
        }
    }
}

rt_err_t xDOWriteReStart(void)
{
    vDOWriteStop();

    if (RT_NULL == g_DOWriteThread) {
        g_DOWriteThread = rt_thread_create("DO", vDOWriteTask, RT_NULL, 0x200, 20, 20);

        if (g_DOWriteThread) {
            rt_thddog_register(g_DOWriteThread, 30);
            rt_thread_startup(g_DOWriteThread);
            return RT_EOK;
        }
    }

    return RT_ERROR;
}

void vExtDataRefreshStop(void)
{
    if (g_ExtDataRefreshThread) {
        rt_thddog_unregister(g_ExtDataRefreshThread);
        if (RT_EOK == rt_thread_delete(g_ExtDataRefreshThread)) {
            g_ExtDataRefreshThread = RT_NULL;
        }
    }
}

rt_err_t xExtDataRefreshReStart(void)
{
    vExtDataRefreshStop();

    if (RT_NULL == g_ExtDataRefreshThread) {
        g_ExtDataRefreshThread = \
            rt_thread_create(
                                 "ExtData",
                                 vVarManage_ExtDataRefreshTask,
                                 RT_NULL, 0x600, 10, 10
                                 );

        if (g_ExtDataRefreshThread) {
            rt_thddog_register(g_ExtDataRefreshThread, 30);
            rt_thread_startup(g_ExtDataRefreshThread);
            return RT_EOK;
        }
    }

    return RT_ERROR;
}

void rt_init_gprs_thread_entry(void *parameter);
void rt_init_zigbee_thread_entry(void *parameter);

void rt_init_thread_entry(void *parameter)
{
    rt_hw_sd_init();
//FS
#ifdef RT_USING_SPI
    rt_hw_spi_init(BOARD_SPIFLASH_INSTANCE, BOARD_SPIFLASH_CSN);
#ifdef RT_USING_DFS
    dfs_init();
#ifdef RT_USING_DFS_ELMFAT
    elm_init();
#endif
#ifdef RT_USING_DFS_ROMFS
    dfs_romfs_init();
#endif

    list_mem();

    WDOG_FEED();


#ifdef USR_TEST_CODE

    rt_pin_mode(BOARD_GPIO_ZIGBEE_SLEEP, PIN_MODE_INPUT_PULLUP);

	if(rt_pin_read(BOARD_GPIO_ZIGBEE_SLEEP) == PIN_LOW){
		g_bIsTestMode = mdTRUE;
		g_istestover_poweron = mdFALSE;
    	xTestTaskStart();
	}
	
#endif
    int flash_cnt = 0;
    char dev_name[16];
    rt_bool_t mount_flag = RT_FALSE;
    rt_snprintf(dev_name, sizeof(dev_name), "%s%d", BOARD_SPI_DEV_NAME(BOARD_SPIFLASH_INSTANCE), BOARD_SPIFLASH_CSN);
    w25qxx_init(BOARD_SPIFLASH_FLASH_NAME, dev_name);
    //dfs_mkfs("elm", BOARD_SPIFLASH_FLASH_NAME );
    while (!mount_flag) {
        if (dfs_mount(BOARD_SPIFLASH_FLASH_NAME, BOARD_SPIFLASH_FLASH_MOUNT, "elm", 0, 0) == 0) {
            mount_flag = RT_TRUE;
        } else {
            rt_kprintf(BOARD_SPIFLASH_FLASH_NAME " mount failed, fatmat and try again!\n");
            rt_thread_delay(RT_TICK_PER_SECOND / 10);
            dfs_mkfs("elm", BOARD_SPIFLASH_FLASH_NAME);
            if (g_bIsTestMode) {
                if (flash_cnt++ > 5) {
                    flash_cnt = 0;
                    break;
                }
            }
        }
    }
    rt_kprintf(BOARD_SPIFLASH_FLASH_NAME " mount ok!\n");

/*
    int err = 0;
    do{
        err =  dfs_mkfs("elm", BOARD_SPIFLASH_FLASH_NAME );
         rt_thread_delay( RT_TICK_PER_SECOND / 10 );
    }while(err == -1);*/

    set_timestamp(time(NULL));

    mkdir("/www", 0);
    mkdir("/cfg", 0);
    mkdir("/log", 0);
    mkdir("/data", 0);
    if (rt_sd_in()) {
        rt_kprintf(BOARD_SDCARD_NAME " sdcard in ...!\n");
        int retry = 0;
        mkdir(BOARD_SDCARD_MOUNT, 0);

        mount_flag = RT_FALSE;
        while (!mount_flag && retry++ < 1) {
            if (dfs_mount(BOARD_SDCARD_NAME, BOARD_SDCARD_MOUNT, "elm", 0, 0) == 0) {
                mount_flag = RT_TRUE;
            } else {
                rt_kprintf(BOARD_SDCARD_NAME " mount failed, fatmat and try again!\n");
                rt_thread_delay(RT_TICK_PER_SECOND / 10);
                dfs_mkfs("elm", BOARD_SDCARD_NAME);
            }
        }
        rt_kprintf(BOARD_SDCARD_NAME " mount ok!\n");
        mkdir(BOARD_SDCARD_MOUNT"/data", 0);
    } else {
        unlink(BOARD_SDCARD_MOUNT);
    }

    cpu_flash_refresh();

    rt_thddog_suspend("bDoUpdate");
    extern rt_bool_t bDoUpdate(void);
    if (bDoUpdate()) {
        rt_kprintf("update finish!\n");
    } else {
        rt_kprintf("no update packet!\n");
    }
    rt_thddog_resume();
    WDOG_FEED();
    
    board_cfg_init();
    // 必须先 storage_cfg_init
    storage_cfg_init();
    vStorageInit();

    vDevCfgInit();      // 放在最前面
    vAdcCalCfgInit();
    host_cfg_init();     // 放在第二
    reg_init();
    auth_cfg_init();
    net_cfg_init();
    tcpip_cfg_init();
    uart_cfg_init();
    analog_cfgs_init();
    gprs_cfg_init();
    zigbee_cfg_init();
    di_cfgs_init();
    do_cfgs_init();
    xfer_uart_cfg_init();
    vVarManage_ExtDataInit();
    rt_thddog_feed("");
	
	if (!g_bIsTestMode) {
	    if (!g_host_cfg.bDebug) {
	        rt_kprintf("rt_console_close_device!\n");
	        rt_console_close_device();
	    } else {
#ifdef RT_USING_FINSH
	        
	            finsh_system_init();
	        
#endif
	    }
	}

#ifdef RT_USING_DFS_ROMFS
    extern const struct romfs_dirent romfs_root;
    if (dfs_mount(RT_NULL, "/www", "rom", 0, &romfs_root) == 0) {
        rt_kprintf("ROM File System initialized!\n");
    } else
        rt_kprintf("ROM File System initialzation failed!\n");
#endif
    

#endif /* RT_USING_DFS */
#endif

/* LwIP Initialization */
#ifdef RT_USING_LWIP
    {
        extern struct netif *netif_list;
        eth_system_device_init();
        lwip_sys_init();
        rt_hw_enet_phy_init();
        rt_thddog_feed("");

        extern char *_hostname;
        netif_set_hostname(netif_list, g_host_cfg.szHostName);
        if (!g_net_cfg.dhcp) {
            struct netif *netif = netif_list;
            struct ip_addr *ip;
            struct ip_addr addr;

            dhcp_stop(netif);
            netif_set_down(netif);
            if (ipaddr_aton(g_net_cfg.ipaddr, &addr)) {
                netif_set_ipaddr(netif, (struct ip_addr *)&addr);
            }

            /* set gateway address */
            if (ipaddr_aton(g_net_cfg.gw, &addr)) {
                netif_set_gw(netif, (struct ip_addr *)&addr);
            }

            /* set netmask address */
            if (ipaddr_aton(g_net_cfg.netmask, &addr)) {
                netif_set_netmask(netif, (struct ip_addr *)&addr);
            }
            netif_set_up(netif);
        }
        rt_kprintf("TCP/IP initialized!\n");
    }
#endif

    rt_thread_idle_sethook(rt_thread_idle);
    if (!g_istestover_poweron) {
        xAIReadReStart();
        xDIReadReStart();
        xDOWriteReStart();
    }

    if (!g_istestover_poweron) {
        for (int port = 0; port < BOARD_UART_MAX; port++) {
            if ((UART_TYPE_232 == g_uart_cfgs[port].uart_type ||
                 UART_TYPE_485 == g_uart_cfgs[port].uart_type /*||
               UART_TYPE_ZIGBEE == g_uart_cfgs[port].uart_type*/) &&
                (PROTO_MODBUS_RTU == g_uart_cfgs[port].proto_type ||
                 PROTO_MODBUS_ASCII == g_uart_cfgs[port].proto_type)
#ifdef RT_USING_CONSOLE
                && (!g_host_cfg.bDebug || BOARD_CONSOLE_USART != port)
#endif
               ) {

                if (!g_xfer_net_dst_uart_occ[port]) {
                    if (PROTO_SLAVE == g_uart_cfgs[port].proto_ms) {
                        xMBRTU_ASCIISlavePollReStart(port, (PROTO_MODBUS_RTU == g_uart_cfgs[port].proto_type) ? (MB_RTU) : (MB_ASCII));
                    } else if (PROTO_MASTER == g_uart_cfgs[port].proto_ms) {
                        xMBRTU_ASCIIMasterPollReStart(port, (PROTO_MODBUS_RTU == g_uart_cfgs[port].proto_type) ? (MB_RTU) : (MB_ASCII));
                    }
                }
            }
        }
        sdccp_serial_openall();
    }

    
    if (!g_istestover_poweron) {
        xExtDataRefreshReStart();

        {
            rt_thread_t init_zgb_thread = rt_thread_create("initzgb", rt_init_zigbee_thread_entry, RT_NULL, 512, 20, 20);

            if (init_zgb_thread != RT_NULL) {
                rt_thddog_register(init_zgb_thread, 30);
                rt_thread_startup(init_zgb_thread);
            }
        }

        {
            rt_thread_t init_gprs_thread = rt_thread_create("initgprs", rt_init_gprs_thread_entry, RT_NULL, 1024, 20, 20);

            if (init_gprs_thread != RT_NULL) {
                rt_thddog_register(init_gprs_thread, 30);
                rt_thread_startup(init_gprs_thread);
            }
        }
    }

    extern void lua_rtu_init(void);
    lua_rtu_init();

    // 先初始化串口
    xfer_helper_serial_init();

//GUI

#if defined(RT_USING_LWIP)
    while (netif_list && !(netif_list->flags & NETIF_FLAG_UP)) {
        rt_thread_delay(RT_TICK_PER_SECOND);
        rt_thddog_feed("wait net connect");
    }
    ftpd_start();
    sntp_start();
#ifdef RT_USING_FINSH
    list_if();
#endif
    webnet_init();

    if (!g_istestover_poweron) {
        xfer_helper_enet_init();

        for (int n = 0; n < BOARD_ENET_TCPIP_NUM; n++) {
            if (g_tcpip_cfgs[n].enable && 
                TCP_IP_M_NORMAL == g_tcpip_cfgs[n].mode &&
                (TCP_IP_TCP == g_tcpip_cfgs[n].tcpip_type ||
                 TCP_IP_UDP == g_tcpip_cfgs[n].tcpip_type)) {

                if(PROTO_MODBUS_TCP == g_tcpip_cfgs[n].cfg.normal.proto_type) {
                    if (PROTO_SLAVE == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                        xMBTCPSlavePollReStart(n);
                    } else if (PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                        xMBTCPMasterPollReStart(n);
                    }
                } else if(PROTO_MODBUS_RTU_OVER_TCP == g_tcpip_cfgs[n].cfg.normal.proto_type) {
                    if (PROTO_SLAVE == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                        xMBRTU_OverTCPSlavePollReStart(n);
                    } else if (PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                        xMBRTU_OverTCPMasterPollReStart(n);
                    }
                }
            }
        }

        for (int n = 0; n < BOARD_ENET_TCPIP_NUM; n++) {
            if (g_tcpip_cfgs[n].enable &&
                TCP_IP_M_NORMAL == g_tcpip_cfgs[n].mode &&
                TCP_IP_TCP == g_tcpip_cfgs[n].tcpip_type &&
                TCPIP_CLIENT == g_tcpip_cfgs[n].tcpip_cs &&
                (
                    PROTO_CC_BJDC == g_tcpip_cfgs[n].cfg.normal.proto_type ||
                    PROTO_HJT212 == g_tcpip_cfgs[n].cfg.normal.proto_type
                )) {
                cc_net_open(n);
            }
        }
    }

    board_udp_init();
    while (!reg_hclient_query()) {
        rt_thddog_feed("redo reg_hclient_query");
        rt_thread_delay(rt_tick_from_millisecond(2000));
    }
    while (!update_hclient_query()) {
        rt_thddog_feed("redo reg_hclient_query");
        rt_thread_delay(rt_tick_from_millisecond(2000));
    }

    //elog_v("test", "this is a test log!");
    //update_hclient_query();
#endif

    /*for(int i = 0; i < 100; i++) {
        elog_e("test", "testaaaaaaaaaaaaaa----%d", i);
        rt_thddog_feed("elog_e test");
        rt_thread_delay(10);
    }*/

    rt_thddog_unreg_inthd();
}

rt_bool_t g_zigbee_init = RT_FALSE;
void rt_init_zigbee_thread_entry(void *parameter)
{
    /*zigbee 初始化*/
    if (!bZigbeeInit(BOARD_ZIGBEE_UART, RT_FALSE)) {
        rt_kprintf("zigbee init err!\n");
        while (1) rt_thread_delay(1 * RT_TICK_PER_SECOND);
    }

    {
        int retry = 3;
        ZIGBEE_DEV_INFO_T xInfo;
        while (ZIGBEE_ERR_OK != eZigbeeGetDevInfo(&xInfo, &g_zigbee_cfg.ucState, &g_zigbee_cfg.usType, &g_zigbee_cfg.usVer) ) {
            //bZigbeeInit( HW_UART3, RT_TRUE );
            vZigbeeHWReset();
            rt_thread_delay(1 * RT_TICK_PER_SECOND);
            rt_thddog_feed("zgb re_init");
            if(--retry <= 0) {
                g_zigbee_init = RT_FALSE;
                elog_w("init_zgb", "zigbee init err 3 times");
                goto _END;
            }
        }

        if (xInfo.WorkMode == ZIGBEE_WORK_END_DEVICE &&
            g_zigbee_cfg.xInfo.WorkMode == ZIGBEE_WORK_COORDINATOR) {
            xInfo.WorkMode = ZIGBEE_WORK_COORDINATOR;
        }
        g_zigbee_cfg.xInfo = xInfo;

        if (!g_xfer_net_dst_uart_occ[BOARD_ZIGBEE_UART]) {
            if (ZIGBEE_WORK_END_DEVICE == g_zigbee_cfg.xInfo.WorkMode ||
                ZIGBEE_WORK_ROUTER == g_zigbee_cfg.xInfo.WorkMode) {
                xMBRTU_ASCIISlavePollReStart(BOARD_ZIGBEE_UART, MB_RTU);
            } else if (ZIGBEE_WORK_COORDINATOR == g_zigbee_cfg.xInfo.WorkMode) {
                xMBRTU_ASCIIMasterPollReStart(BOARD_ZIGBEE_UART, MB_RTU);
            }
        }
    }

    g_zigbee_init = RT_TRUE;
    rt_kprintf("zigbee init ok! ver = %d\n", g_zigbee_cfg.usVer);

_END:
    rt_thddog_unreg_inthd();
}

mdBOOL bIsGprsInitOk;

void rt_init_gprs_thread_entry(void *parameter)
{
    rt_bool_t bAllInitOk = RT_FALSE;
    rt_bool_t bHasGPRSFlag = RT_FALSE;
    int retry = 3;
    bIsGprsInitOk = mdFALSE;
    /*gprs 初始化*/

    xGPRS_Init(HW_UART2);

    while (!bAllInitOk) {
        rt_thddog_feed("");
        vGPRS_PowerDown();
        rt_thread_delay(3 * RT_TICK_PER_SECOND);
        if (g_gprs_work_cfg.eWMode != GPRS_WM_SHUTDOWN) {

            rt_thddog_feed("");
            vGPRS_PowerUp();
            rt_thread_delay(RT_TICK_PER_SECOND / 10);

            // DelayMs(300);
            vGPRS_TermUp();
            if(bGPRS_ATTest()) {
                bHasGPRSFlag = RT_TRUE;
                bGPRS_TcpOt();
                rt_kprintf("wait 5 sec before getcsq\n");
                rt_thread_delay(5 * RT_TICK_PER_SECOND);
                bGPRS_GetCSQ(RT_TRUE);
                rt_kprintf("CSQ = %d\n", g_nGPRS_CSQ);

                rt_uint8_t btCREG = 0xFF;
                bGPRS_GetCREG(RT_TRUE);
                rt_kprintf("CREG = %d\n", g_nGPRS_CREG);

                bAllInitOk = bGPRS_NetInit(g_gprs_net_cfg.szAPN, g_gprs_net_cfg.szAPNNo, g_gprs_net_cfg.szUser, g_gprs_net_cfg.szPsk);
                rt_kprintf("gprs net init = %d\n", bAllInitOk);

                if(bAllInitOk) {
                    bGPRS_SetCSCA(g_gprs_net_cfg.szMsgNo);
                    break;
                }
            }
        } else {
            rt_kprintf("gprs cfg shutdown!\n");
            break;
        }
        
        if(!bHasGPRSFlag && --retry <= 0) {
            // 初始化三次失败, 关掉 GPRS
            g_gprs_work_cfg.eWMode = GPRS_WM_SHUTDOWN;
            bIsGprsInitOk = RT_FALSE;
            elog_w("init_gprs", "GPRS init err 3 times");
            goto _END;
        }
    }
    if (g_gprs_work_cfg.eWMode != GPRS_WM_SHUTDOWN) {
        bIsGprsInitOk = mdTRUE;

        xfer_helper_gprs_init();

        for (int n = BOARD_ENET_TCPIP_NUM; n < BOARD_TCPIP_MAX; n++) {
            if (g_tcpip_cfgs[n].enable &&
                TCP_IP_M_NORMAL == g_tcpip_cfgs[n].mode &&
                (TCP_IP_TCP == g_tcpip_cfgs[n].tcpip_type ||
                 TCP_IP_UDP == g_tcpip_cfgs[n].tcpip_type)) {

                if(PROTO_MODBUS_TCP == g_tcpip_cfgs[n].cfg.normal.proto_type) {
                    if (PROTO_SLAVE == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                        xMBTCPSlavePollReStart(n);
                    } else if (PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                        xMBTCPMasterPollReStart(n);
                    }
                } else if(PROTO_MODBUS_RTU_OVER_TCP == g_tcpip_cfgs[n].cfg.normal.proto_type) {
                    if (PROTO_SLAVE == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                        xMBRTU_OverTCPSlavePollReStart(n);
                    } else if (PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                        xMBRTU_OverTCPMasterPollReStart(n);
                    }
                }
            }

            if (g_tcpip_cfgs[n].enable &&
                TCP_IP_M_NORMAL == g_tcpip_cfgs[n].mode &&
                TCP_IP_TCP == g_tcpip_cfgs[n].tcpip_type &&
                TCPIP_CLIENT == g_tcpip_cfgs[n].tcpip_cs &&
                (
                    PROTO_CC_BJDC == g_tcpip_cfgs[n].cfg.normal.proto_type ||
                    PROTO_HJT212 == g_tcpip_cfgs[n].cfg.normal.proto_type
                )) {
                cc_net_open(n);
            }
        }

        //bGPRS_GetCOPS();
        bGPRS_GetSMOND(RT_TRUE);

        for (int i = 0; i < 30; i++) {
            rt_thddog_feed("");
            bGPRS_GetCREG(RT_TRUE);
            rt_thread_delay(1 * RT_TICK_PER_SECOND);
            if (1 == g_nGPRS_CREG || 5 == g_nGPRS_CREG) {
                bGPRS_GetCSQ(RT_TRUE);
                rt_kprintf("CSQ = %d\n", g_nGPRS_CSQ);
                rt_kprintf("CREG = %d\n", g_nGPRS_CREG);
                break;
            }
        }
    }
_END:
    rt_thddog_unreg_inthd();
}

void rt_wdog_thread_entry(void *parameter)
{
    rt_pin_mode(BOARD_GPIO_ZIGBEE_SLEEP, PIN_MODE_INPUT_PULLUP);  //按键检测上拉，长按5S恢复出厂设置
    mdUINT32 key_cnt = 0;
#define RESET_SEC  (5)  //多少秒恢复出厂设置

#if   SX1278_TEST

    rt_pin_mode(BOARD_GPIO_NET_LED, PIN_MODE_OUTPUT);
    rt_pin_write(BOARD_GPIO_NET_LED, PIN_HIGH);

#define SX1278_RX
#define BUFFER_SIZE     30                          // Define the payload size here
    static uint16_t BufferSize = BUFFER_SIZE;           // RF buffer size
    static uint8_t  Buffer[BUFFER_SIZE] = { 0 };              // RF buffer
    static uint8_t MY_TEST_Msg[29] = "RTU_SX1278_TEST";

    tRadioDriver *Radio = RadioDriverInit();
    Radio->Init();

#if defined (SX1278_RX)
    Radio->StartRx();   //RFLR_STATE_RX_INIT
#elif defined (SX1278_TX)
    Radio->SetTxPacket(MY_TEST_Msg, sizeof(MY_TEST_Msg));
#endif
    int i = 0;
    int index = 0;
    while (1) {
#if defined (SX1278_RX)
        if (Radio->Process() == RF_RX_DONE) {
            Radio->GetRxPacket(Buffer, (uint16_t *)&BufferSize);
            rt_kprintf("size = %d: %s\n ", BufferSize, Buffer);
            memset(Buffer, 0, BufferSize);
            Radio->StartRx();
            rt_pin_toggle(BOARD_GPIO_NET_LED);
        }
        rt_thread_delay(RT_TICK_PER_SECOND / 100);
#elif defined (SX1278_TX)
        if (Radio->Process() == RF_TX_DONE) {
            rt_kprintf("RF_LoRa_TX_OK! %d \n", index);
            rt_thread_delay(RT_TICK_PER_SECOND / 2);
            rt_pin_toggle(BOARD_GPIO_NET_LED);
            sprintf(MY_TEST_Msg, "RTU_SX1278_TEST_%d", index++);
            Radio->SetTxPacket(MY_TEST_Msg, sizeof(MY_TEST_Msg));   //RFLR_STATE_TX_INIT
        }


#endif

    }
#endif

    while (1) {
        WDOG_FEED();
        rt_thread_delay(RT_TICK_PER_SECOND / 20);

        if (rt_pin_read(BOARD_GPIO_ZIGBEE_SLEEP) == PIN_LOW) {
            key_cnt++;
        } else {
            if (key_cnt >= 15 * RESET_SEC && !g_bIsTestMode) {
                //此处增加恢复出厂设置的函数。
                rt_kprintf("press key 15s , reset factory... \n");
                board_cfg_del();
                key_cnt = 0;
                NVIC_SystemReset();
            }
            key_cnt = 0;
        }
        if(!g_bIsTestMode) {
            rt_bool_t thd_dog_reset = RT_FALSE;
            rt_thread_t dog_thread = (rt_thread_t)threaddog_check();
            while(dog_thread) {
                thddog_t *dog = (thddog_t *)dog_thread->user_data;
                if(dog) {
                    thd_dog_reset = RT_TRUE;
                    elog_e(
                        "thddog", 
                        "over:%s,%s,%d,%s", 
                        dog->name, dog->func?dog->func:"", 
                        dog->line, dog->run_desc?dog->run_desc:""
                    );
                }
                dog_thread = (rt_thread_t)threaddog_check();
            }
            if(thd_dog_reset) {
                vDoSystemReset();
            }
            extern void uart_int_check(void);
            uart_int_check();
        }
    }
}

void board_assert(const char *ex_string, const char *func, rt_size_t line)
{
    volatile char dummy = 0;

    bStorageLogError("(%s) at (%s,%d)", ex_string, func, line);

    while (dummy == 0);
}

extern int encode_getid(rt_uint32_t in[4], rt_uint32_t code, rt_uint8_t id[64]);

void rt_elog_init(void);

ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t _wdog_stack[0x600];

int rt_application_init()
{
    //rt_assert_set_hook(board_assert);
    rt_elog_init();

    vMBInitState();
    vMBMasterInitState();

    static struct rt_thread wdog_thread;
    rt_thread_init(&wdog_thread, 
                    "wdog", 
                    rt_wdog_thread_entry, 
                    RT_NULL, 
                    &_wdog_stack[0], 
                    sizeof(_wdog_stack),
                    0, 100);
    rt_thread_startup(&wdog_thread);

    {
        rt_thread_t init_thread = rt_thread_create("init", rt_init_thread_entry, RT_NULL, 4096, RT_THREAD_PRIORITY_MAX - 1, 20);

        if (init_thread != RT_NULL) {
            rt_thddog_register(init_thread, 60);
            rt_thread_startup(init_thread);
        }
    }

    return 0;
}


/*void HardFault_Handler(void)
{
    list_mem();
    list_thread();
    while (1);
}*/


static void rtt_user_assert_hook(const char* ex, const char* func, rt_size_t line);
static void elog_user_assert_hook(const char* ex, const char* func, size_t line);
static rt_err_t exception_hook(void *context);

void rt_elog_init(void)
{
    /* initialize EasyFlash and EasyLogger */
    if ((elog_init() == ELOG_NO_ERR)) {
        /* set enabled format */
        elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL & ~ELOG_FMT_P_INFO);
        elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
        elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
        elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
        elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_P_INFO));
        elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_P_INFO));
        /* set EasyLogger assert hook */
        elog_assert_set_hook(elog_user_assert_hook);
        /* start EasyLogger */
        elog_start();
        /* set hardware exception hook */
        rt_hw_exception_install(exception_hook);
        /* set RT-Thread assert hook */
#ifdef RT_DEBUG
        rt_assert_set_hook(rtt_user_assert_hook);
#endif
        /* initialize OK and switch to running status */
        
    } else {
        /* initialize fail and switch to fault status */
    }
}


static void elog_user_assert_hook(const char* ex, const char* func, size_t line)
{

#ifdef ELOG_ASYNC_OUTPUT_ENABLE
    /* disable async output */
    elog_async_enabled(false);
#endif

    /* disable logger output lock */
    elog_output_lock_enabled(false);
    /* output logger assert information */
    elog_a("elog", "(%s) has assert failed at %s:%ld.", ex, func, line);
    while(1);
}

static void rtt_user_assert_hook(const char* ex, const char* func, rt_size_t line)
{

#ifdef ELOG_ASYNC_OUTPUT_ENABLE
    /* disable async output */
    elog_async_enabled(false);
#endif

    /* disable logger output lock */
    elog_output_lock_enabled(false);
    /* output rtt assert information */
    elog_a("rtt", "(%s) has assert failed at %s:%ld.", ex, func, line);
    // wait 30 sec
    {
        rt_tick_t now = rt_tick_get();
        while(rt_tick_get() - now < 30 * RT_TICK_PER_SECOND);
    }
    rt_hw_interrupt_disable();
    while(1);
}

static rt_err_t exception_hook(void *context)
{
    struct exception_stack_frame {
        rt_uint32_t r0;
        rt_uint32_t r1;
        rt_uint32_t r2;
        rt_uint32_t r3;
        rt_uint32_t r12;
        rt_uint32_t lr;
        rt_uint32_t pc;
        rt_uint32_t psr;
    };
    struct exception_stack_frame *exception_stack = (struct exception_stack_frame *) context;

    /* disable logger output lock */
    elog_output_lock_enabled(false);
    
    elog_e("hw_fault", "psr: 0x%08x", exception_stack->psr);
    elog_e("hw_fault", " pc: 0x%08x", exception_stack->pc);
    elog_e("hw_fault", " lr: 0x%08x", exception_stack->lr);
    elog_e("hw_fault", "r12: 0x%08x", exception_stack->r12);
    elog_e("hw_fault", "r03: 0x%08x", exception_stack->r3);
    elog_e("hw_fault", "r02: 0x%08x", exception_stack->r2);
    elog_e("hw_fault", "r01: 0x%08x", exception_stack->r1);
    elog_e("hw_fault", "r00: 0x%08x", exception_stack->r0);
    elog_e("hw_fault", "hard fault on thread: %s", rt_thread_self()?rt_thread_self()->name:"");

    // wait 30 sec
    {
        rt_tick_t now = rt_tick_get();
        while(rt_tick_get() - now < 30 * RT_TICK_PER_SECOND);
    }
    rt_hw_interrupt_disable();
    while(1);

    return RT_EOK;
}

/*@}*/
