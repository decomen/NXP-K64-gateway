#ifndef __THREAD_DOG_H__
#define __THREAD_DOG_H__

typedef enum {
    THD_DOG_S_RUNNING,  //����״̬
    THD_DOG_S_SUSPEND,  //�̹߳���
    THD_DOG_S_DIE,      //�߳�����
    THD_DOG_S_EXIT,     //�߳̽���
} thddog_state_t;

typedef struct {
    char        name[12];       //thread_name
    long        use         :1; //�Ƿ���ʹ��
    long        is_over     :1; //�Ƿ����
    long        over_sec    :8; //��ʼ���������ʱ��(S)
    long        tick_reg;       //ע��tick
    long        tick_feed;      //���ι��tick
    const char  *func;          //�����
    int         line;           //����ļ���
    const char  *run_desc;      //�������
    thddog_state_t state;       //��ǰ״̬
    const void  *data;          //��������
} thddog_t;     // 44 byte

void threaddog_init(void);
thddog_t *threaddog_register(const char *thread_name, int sec, const void *data);
void threaddog_unregister(thddog_t *dog);
void threaddog_update(thddog_t *dog, const char *func, const int line, const char *desc, thddog_state_t state);

#define threaddog_feed(_dog, _desc) threaddog_update(_dog, __FUNCTION__, __LINE__, _desc, THD_DOG_S_RUNNING)
#define threaddog_suspend(_dog, _desc) threaddog_update(_dog, __FUNCTION__, __LINE__, _desc, THD_DOG_S_SUSPEND)
#define threaddog_resume(_dog, _desc) threaddog_update(_dog, __FUNCTION__, __LINE__, _desc, THD_DOG_S_RUNNING)
#define threaddog_exit(_dog, _desc) threaddog_update(_dog, __FUNCTION__, __LINE__, _desc, THD_DOG_S_EXIT)

const void *threaddog_check(void);

#endif

