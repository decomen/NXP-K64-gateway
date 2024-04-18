
#include <board.h>

cpu_usage_t g_xCpuUsage;

void cpu_flash_refresh( void )
{
    struct statfs buffer;

    if( dfs_statfs( BOARD_SPIFLASH_FLASH_MOUNT, &buffer) != 0 ) {
        rt_kprintf("dfs_statfs failed.\n");
    } else {
        g_xCpuUsage.flash_size = (buffer.f_blocks * buffer.f_bsize) / 1024;
        g_xCpuUsage.flash_use = g_xCpuUsage.flash_size - (buffer.f_bsize * buffer.f_bfree / 1024);
    }

    if( rt_sd_in() ) {
        if( dfs_statfs( BOARD_SDCARD_MOUNT, &buffer) != 0 ) {
            rt_kprintf("dfs_statfs failed.\n");
        } else {
            g_xCpuUsage.sd_size = ((long long)buffer.f_blocks * buffer.f_bsize) / 1024;
            g_xCpuUsage.sd_use = g_xCpuUsage.sd_size - (buffer.f_bsize * (long long)buffer.f_bfree / 1024);
        }
    }
}

void cpu_usage( void )
{
#define CPU_USAGE_CALC_TICK     100
#define CPU_USAGE_LOOP          1000

    static rt_uint32_t total_count = 0;
    rt_tick_t tick;
    rt_uint32_t count;
    volatile rt_uint32_t loop;

    if (total_count == 0)
    {
        /* get total count */
        rt_enter_critical();
        tick = rt_tick_get();
        while(rt_tick_get() - tick < CPU_USAGE_CALC_TICK)
        {
            total_count ++;
            loop = 0;

            while (loop < CPU_USAGE_LOOP) loop ++;
        }
        rt_exit_critical();
    }

    count = 0;
    /* get CPU usage */
    tick = rt_tick_get();
    while (rt_tick_get() - tick < CPU_USAGE_CALC_TICK)
    {
        count ++;
        loop  = 0;
        while (loop < CPU_USAGE_LOOP) loop ++;
    }

    /* calculate major and minor */
    if (count < total_count)
    {
        count = total_count - count;
        //cpu_usage_major = (count * 100) / total_count;
        //cpu_usage_minor = ((count * 100) % total_count) * 100 / total_count;
        g_xCpuUsage.cpu = (count * 100) / total_count;
    }
    else
    {
        total_count = count;

        /* no CPU usage */
        //cpu_usage_major = 0;
        //cpu_usage_minor = 0;
        g_xCpuUsage.cpu = 0;
    }

    rt_memory_info( &g_xCpuUsage.mem_all, &g_xCpuUsage.mem_now_use, &g_xCpuUsage.mem_max_use );
}

DEF_CGI_HANDLER(getCpuUsage)
{
    char* szRetJSON = RT_NULL;
    cJSON *pItem = cJSON_CreateObject();
    if(pItem) {
        cJSON_AddNumberToObject( pItem, "ret", RT_EOK );
        
        cJSON_AddNumberToObject( pItem, "cpu", g_xCpuUsage.cpu );
        cJSON_AddNumberToObject( pItem, "ms", g_xCpuUsage.mem_all );
        cJSON_AddNumberToObject( pItem, "ma", g_xCpuUsage.mem_max_use );
        cJSON_AddNumberToObject( pItem, "mu", g_xCpuUsage.mem_now_use );
        cJSON_AddNumberToObject( pItem, "fs", g_xCpuUsage.flash_size );
        cJSON_AddNumberToObject( pItem, "fu", g_xCpuUsage.flash_use );
        cJSON_AddNumberToObject( pItem, "ss", g_xCpuUsage.sd_size );
        cJSON_AddNumberToObject( pItem, "su", g_xCpuUsage.sd_use );
        
        szRetJSON = cJSON_PrintUnformatted( pItem );
        if(szRetJSON) {
            WEBS_PRINTF( szRetJSON );
            rt_free( szRetJSON );
        }
    }
    cJSON_Delete( pItem );

	WEBS_DONE(200);
}
