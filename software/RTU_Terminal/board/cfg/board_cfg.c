
#include <board.h>
#include <stdio.h>

static struct rt_mutex cfg_mutex;
cfg_info_t g_cfg_info;

const rt_uint8_t _empty_buffer[128] = { 0 };

#define BOARD_CFG_PATH          "/cfg/board_v0.cfg"

void board_cfg_init(void)
{
    if (rt_mutex_init(&cfg_mutex, "cfg_mutex", RT_IPC_FLAG_PRIO) != RT_EOK) {
        rt_kprintf("init cfg_mutex failed\n");
        while (1);
        return;
    }

    if (!board_cfg_read(CFG_INFO_OFS, &g_cfg_info, sizeof(g_cfg_info))) {
        g_cfg_info.usVer = CFG_VER;
        board_cfg_write(CFG_INFO_OFS, &g_cfg_info, sizeof(g_cfg_info));
    }

    cfg_recover_try();
}

void board_cfg_del(void)
{
    rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);

    unlink(BOARD_CFG_PATH);
    board_cfg_write(CFG_INFO_OFS, &g_cfg_info, sizeof(g_cfg_info));
    board_cfg_write(REG_CFG_OFS, &g_reg, sizeof(g_reg));

    rt_mutex_release(&cfg_mutex);
}

// 读配置文件
int board_cfg_justread(int ofs, void *data, int len)
{
    int readlen = 0;
    int fd = -1;

    rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
    fd = open(BOARD_CFG_PATH, O_RDONLY, 0666);
    if (fd >= 0) {
        if (lseek(fd, ofs, SEEK_SET) == ofs) {
            readlen = read(fd, data, len);
        }
        close(fd);
    }

    rt_mutex_release(&cfg_mutex);


    return readlen;
}

// 带校验读
rt_bool_t board_cfg_read(int ofs, void *pcfg, int len)
{
    rt_bool_t ret = RT_FALSE;
    rt_uint32_t ulMagic = 0;
    int fd = -1;

    rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
    fd = open(BOARD_CFG_PATH, O_RDONLY, 0666);
    if (fd >= 0) {
        if (lseek(fd, ofs, SEEK_SET) == ofs) {
            read(fd, &ulMagic, 4);
            if (CFG_MAGIC == ulMagic) {
                if (len == read(fd, pcfg, len)) {
                    rt_uint16_t usCheck = 0;
                    if (2 == read(fd, &usCheck, 2)) {
                        ret = (usCheck == usMBCRC16(pcfg, len));
                    }
                }
            }
        }
        close(fd);
    }

    rt_mutex_release(&cfg_mutex);

    return ret;
}

int board_cfg_justwrite(int ofs, void *data, int len)
{
    int writelen = 0;
    int fd = -1;

    rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
    fd = open(BOARD_CFG_PATH, O_CREAT | O_RDWR, 0666);
    if (fd >= 0) {
        int pos = lseek(fd, ofs, SEEK_SET);
        if (pos < ofs) {
            int remain = ofs - pos;
            while (remain > 0) {
                int wlen = (remain > 128 ? 128 : remain);
                wlen = write(fd, _empty_buffer, wlen);
                remain -= wlen;
            }
        }
        if (lseek(fd, ofs, SEEK_SET) == ofs) {
            writelen = write(fd, data, len);
        }
        close(fd);
    }

    rt_mutex_release(&cfg_mutex);

    return writelen;
}

// 带校验写
rt_bool_t board_cfg_write(int ofs, void const *pcfg, int len)
{
    rt_bool_t ret = RT_FALSE;
    rt_uint32_t ulMagic = CFG_MAGIC;
    int fd = -1;

    rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
    fd = open(BOARD_CFG_PATH, O_CREAT | O_RDWR, 0666);

    if (fd >= 0) {
        int pos = lseek(fd, ofs, SEEK_SET);
        if (pos < ofs) {
            int remain = ofs - pos;
            while (remain > 0) {
                int wlen = (remain > 128 ? 128 : remain);
                wlen = write(fd, _empty_buffer, wlen);
                remain -= wlen;
            }
        }
        if (lseek(fd, ofs, SEEK_SET) == ofs) {
            if (4 == write(fd, &ulMagic, 4)) {
                if (len == write(fd, pcfg, len)) {
                    rt_uint16_t usCheck = usMBCRC16(pcfg, len);
                    ret = (2 == write(fd, &usCheck, 2));
                }
            }
        }
        close(fd);
    }

    rt_mutex_release(&cfg_mutex);

    return ret;
}

#include "cfg_recover.c"

