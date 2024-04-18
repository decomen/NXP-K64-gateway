
#include "threaddog.h"
#include "rtthread.h"
#include <stdio.h>
#include <string.h>

// Ϊ��ֹ���ڴ����,���þ�̬����
// ��Ҫ���ݵ�ǰ����߳�������
#define THREAD_DOG_NUM      (50)

static thddog_t thddogs_list[THREAD_DOG_NUM];
static int thddogs_cnt = 0;

#define threaddog_printf(_fmt,...)   rt_kprintf( "[######threaddog######]->" _fmt, ##__VA_ARGS__ )

// 0 ����ӡ
// 1 ��ӡ������Ϣ
// 2 ��ӡ��Ҫ��Ϣ
// 3 ��ӡԽ����Ϣ
#define THD_DOG_DEBUG_LVL       2

long threaddog_sec2tick(long sec)
{
    return sec * RT_TICK_PER_SECOND;
}

long threaddog_tick(void)
{
    return rt_tick_get();
}

void threaddog_init(void)
{
    thddogs_cnt = 0;
    memset(thddogs_list, 0, sizeof(thddogs_list));
}

thddog_t *threaddog_register(const char *thread_name, int over_sec, const void *data)
{
    for(int i = 0; i < THREAD_DOG_NUM; i++) {
        if(!thddogs_list[i].use) {
            memset(&thddogs_list[i], 0, sizeof(thddog_t));
            strncpy(thddogs_list[i].name, thread_name, 12);
            thddogs_list[i].is_over = 0;
            thddogs_list[i].over_sec = over_sec & 0xFF;
            thddogs_list[i].tick_reg = threaddog_tick();
            thddogs_list[i].tick_feed = threaddog_tick();
            thddogs_list[i].func = NULL;
            thddogs_list[i].line = -1;
            thddogs_list[i].run_desc = NULL;
            thddogs_list[i].state = THD_DOG_S_RUNNING;
            thddogs_list[i].data = data;
            thddogs_list[i].use = 1;
            thddogs_cnt++;
#if THD_DOG_DEBUG_LVL && THD_DOG_DEBUG_LVL < 3
            threaddog_printf("register:%s,%d SEC\n", thread_name, over_sec);
#endif
            return &thddogs_list[i];
        }
    }

    return NULL;
}

void threaddog_unregister(thddog_t *dog)
{
    if(dog && dog->use) {
        dog->use = 0;
        thddogs_cnt--;
#if THD_DOG_DEBUG_LVL && THD_DOG_DEBUG_LVL < 3
        threaddog_printf("unregister:%s\n", dog->name);
#endif
    }
}

void threaddog_update(thddog_t *dog, const char *func, const int line, const char *desc, thddog_state_t state)
{
    if(dog && dog->use && !dog->is_over) {
        dog->tick_feed = threaddog_tick();
        dog->is_over = 0;
        dog->func = func;
        dog->line = line;
        if(desc) dog->run_desc = desc;
#if THD_DOG_DEBUG_LVL && THD_DOG_DEBUG_LVL < 2
        threaddog_printf(
            "update:%s, %s,%d, %s, %s\n", 
            dog->name, func?func:"", line, desc?desc:"", 
            state==THD_DOG_S_SUSPEND? \
            "suspend": \
            (dog->state==THD_DOG_S_SUSPEND?"resume":(dog->state==THD_DOG_S_EXIT?"exit":"feed"))
        );
#endif
        dog->state = state;
    }
}

const void *threaddog_check(void)
{
    long tick = threaddog_tick();
    for(int i = 0; i < THREAD_DOG_NUM; i++) {
        thddog_t *dog = &thddogs_list[i];
        if(dog->use && !dog->is_over && dog->state != THD_DOG_S_SUSPEND && dog->state != THD_DOG_S_EXIT) {
            long over_tick = threaddog_sec2tick(dog->over_sec);
            // tickԽ�紦��
            if(tick < dog->tick_feed) dog->tick_feed = 0;
            // dog over ����
            if(tick >= over_tick && tick - dog->tick_feed > over_tick) {
                dog->is_over = 1;
                dog->state = THD_DOG_S_DIE;
#if THD_DOG_DEBUG_LVL
                threaddog_printf(
                    "over:%s, %s,%d,%s\n", 
                    dog->name, dog->func?dog->func:"", 
                    dog->line, dog->run_desc?dog->run_desc:""
                );
#endif
                return dog->data;
            }
        }
    }

    return NULL;
}

#include "finsh.h"
long list_thddog(void)
{
    rt_kprintf("------------------------tick:%d-------------------\n", threaddog_tick());
    rt_kprintf(" n  thread  use  status  sec  tick_reg  tick_feed  function,line,desc\n");
    rt_kprintf("--- -------- --- ------- --- ---------- ---------- -----------------------------------------------\n");
    for(int i = 0; i < thddogs_cnt; i++) {
        thddog_t *dog = &thddogs_list[i];
        rt_kprintf("%03d %-8.*s %01d  ", i, RT_NAME_MAX, dog->name, dog->use);

        if (dog->state == THD_DOG_S_RUNNING)        rt_kprintf(" run    ");
        else if (dog->state == THD_DOG_S_SUSPEND)   rt_kprintf(" suspend");
        else if (dog->state == THD_DOG_S_DIE)       rt_kprintf(" die    ");
        else if (dog->state == THD_DOG_S_EXIT)      rt_kprintf(" exit   ");

        rt_kprintf(" %3d  %8d   %8d  %s,%d,%s\n", dog->over_sec, dog->tick_reg, dog->tick_feed, dog->func?dog->func:"", dog->line, dog->run_desc?dog->run_desc:"");
    }
	
    return 0;
}
FINSH_FUNCTION_EXPORT(list_thddog, list thread dog);

