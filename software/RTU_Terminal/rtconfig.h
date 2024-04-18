/* RT-Thread config file */
#ifndef __RTTHREAD_CFG_H__
#define __RTTHREAD_CFG_H__

#define RT_USING_LUA
#define LUA_USE_STDIO
#define RT_USING_SQLITE

/* RT_NAME_MAX*/
#define RT_NAME_MAX                 12

/* RT_ALIGN_SIZE*/
#define RT_ALIGN_SIZE               4

/* PRIORITY_MAX */
#define RT_THREAD_PRIORITY_MAX      32

/* Tick per Second */
#define RT_TICK_PER_SECOND          1000

/* SECTION: RT_DEBUG */
/* Thread Debug */
//#define RT_DEBUG
#define RT_THREAD_DEBUG
//#define RT_USING_COMPONENTS_INIT
//#define RT_DEBUG_INIT   1
//#define RT_USING_MODULE

//#define RT_USING_OVERFLOW_CHECK

/* Using Hook */
#define RT_USING_HOOK

#define IDLE_THREAD_STACK_SIZE      512

/* Using Software Timer */
//#define RT_USING_TIMER_SOFT 
#define RT_TIMER_THREAD_PRIO		4
#define RT_TIMER_THREAD_STACK_SIZE	512
#define RT_TIMER_TICK_PER_SECOND	10

/* SECTION: IPC */
/* Using Semaphore*/
#define RT_USING_SEMAPHORE

/* Using Mutex */
#define RT_USING_MUTEX

/* Using Event */
#define RT_USING_EVENT

/* Using MailBox */
#define RT_USING_MAILBOX

/* Using Message Queue */
#define RT_USING_MESSAGEQUEUE

/* SECTION: Memory Management */
/* Using Memory Pool Management*/
#define RT_USING_MEMPOOL

/* Using Dynamic Heap Management */
#define RT_USING_HEAP
/* Using Small MM */
#define RT_USING_SMALL_MEM

//#define RT_USING_MEMHEAP

//#define RT_USING_SLAB


/* "Using Device Driver Framework" default="true" */
#define RT_USING_DEVICE
/* Using IPC in Device Driver Framework" default="true" */
#define RT_USING_DEVICE_IPC
/* Using Serial Device Driver Framework" default="true" */
#define RT_USING_RTC
#define RT_USING_PIN
#define RT_USING_SERIAL
#define RT_USING_SPI
/* SECTION: Console options */
#define RT_USING_CONSOLE
/* the buffer size of console*/
#define RT_CONSOLEBUF_SIZE	384

/* SECTION: finsh, a C-Express shell */
#ifdef RT_USING_CONSOLE
#define RT_USING_FINSH
#endif

/* Using symbol table */
#define FINSH_USING_SYMTAB
#define FINSH_USING_DESCRIPTION
//#define FINSH_USING_MSH
//#define FINSH_USING_MSH_DEFAULT
#define FINSH_THREAD_STACK_SIZE   0x400

/* SECTION: device filesystem */
#define RT_USING_DFS 
//#define RT_USING_DFS_ROMFS
#define RT_USING_DFS_ELMFAT
#define RT_DFS_ELM_WORD_ACCESS
#define DFS_USING_WORKDIR
/* Reentrancy (thread safe) of the FatFs module.  */
#define RT_DFS_ELM_REENTRANT
/* Number of volumes (logical drives) to be used. */
#define RT_DFS_ELM_DRIVES           2
#define RT_DFS_ELM_USE_LFN          3 
#define RT_DFS_ELM_MAX_LFN			64
#define RT_DFS_ELM_CODE_PAGE        437  
/* Maximum sector size to be handled. */
#define RT_DFS_ELM_MAX_SECTOR_SIZE  4096

/* the max number of mounted filesystem */
#define DFS_FILESYSTEMS_MAX			2
/* the max number of opened files 		*/
#define DFS_USING_STDIO 
#define DFS_FD_MAX					32

//#define RT_USING_DFS_NFS
#define RT_NFS_HOST_EXPORT      "192.168.0.5:/"

/* SECTION: lwip, a lighwight TCP/IP protocol stack */
#define RT_USING_LWIP
/* LwIP uses RT-Thread Memory Management */
//#define RT_LWIP_USING_RT_MEM
/* Enable ICMP protocol*/
#define RT_LWIP_ICMP
/* Enable UDP protocol*/
#define RT_LWIP_UDP
/* Enable TCP protocol*/
#define RT_LWIP_TCP
/* Enable DNS */
#define RT_LWIP_DNS
#define RT_LWIP_DEBUG
#define RT_LWIP_DHCP
/* the number of simulatenously active TCP connections*/
#define RT_LWIP_TCP_PCB_NUM	            13
#define RT_LWIP_UDP_PCB_NUM             6

#define RT_LWIP_PBUF_NUM	            20
#define RT_LWIP_PBUF_POOL_BUFSIZE       (256 + 7)
//#define RT_LWIP_TCP_SEG_NUM	            12

/* tcp thread options */
#define RT_LWIP_TCPTHREAD_PRIORITY		5
#define RT_LWIP_TCPTHREAD_MBOX_SIZE		8
#define RT_LWIP_TCPTHREAD_STACKSIZE		(1048)

/* ethernet if thread options */
#define RT_LWIP_ETHTHREAD_PRIORITY		6
#define RT_LWIP_ETHTHREAD_MBOX_SIZE		8
#define RT_LWIP_ETHTHREAD_STACKSIZE		(1048)

/* TCP sender buffer space */
#define RT_LWIP_TCP_SND_BUF	            (4*1024)
/* TCP receive window. */
#define RT_LWIP_TCP_WND		            (2*1024)

#define BOARD_UART_MAX                  6
#define BOARD_ENET_TCPIP_NUM            4
#define BOARD_GPRS_TCPIP_NUM            4
#define BOARD_TCPIP_MAX                 (BOARD_ENET_TCPIP_NUM+BOARD_GPRS_TCPIP_NUM)
#define MB_TCP_ID_OFS                   BOARD_UART_MAX
#define MB_TCP_NUMS                     BOARD_TCPIP_MAX
#define MB_SLAVE_NUMS                   (BOARD_UART_MAX+MB_TCP_NUMS)
#define MB_MASTER_NUMS                  (BOARD_UART_MAX+MB_TCP_NUMS)
#define MB_IN_UART(_p)                  ((_p)<BOARD_UART_MAX)
#define MB_IN_ENET(_p)                  (((_p)>=MB_TCP_ID_OFS)&&((_p)<(MB_TCP_ID_OFS+BOARD_ENET_TCPIP_NUM)))
#define MB_IN_GPRS(_p)                  (((_p)>=(MB_TCP_ID_OFS+BOARD_ENET_TCPIP_NUM))&&((_p)<(MB_TCP_ID_OFS+BOARD_ENET_TCPIP_NUM+BOARD_GPRS_TCPIP_NUM)))

#if MB_TCP_ID_OFS != BOARD_UART_MAX
#error MB_TCP_ID_OFS != BOARD_UART_MAX
#endif

#endif
