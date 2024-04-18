
#include <board.h>
#include <sntp.h>

#ifndef bzero
#define bzero(a, b) memset(a, 0, b)
#endif

static int __GetNTPTime( SNTP_Header *H_SNTP, int num )
{
    int sockfd=0;
    struct sockaddr_in server;
    fd_set set;
    struct timeval timeout;

    bzero(H_SNTP,sizeof(SNTP_Header));
    bzero(&server,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(NTP_PORT);

    if( num <= 1 && g_host_cfg.szNTP[num][0] != '\0' ) {
        if( inet_aton( g_host_cfg.szNTP[num], &server.sin_addr ) <= 0 ) {
            struct hostent* host = lwip_gethostbyname( g_host_cfg.szNTP[num] );
            if( host && host->h_addr_list && host->h_addr ) {
                memcpy( &server.sin_addr, host->h_addr, sizeof(struct in_addr) );
            } else {
                return -1;
            }
        }
    } else {
        return -1;
    }

    sockfd = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd<0) {
        rt_kprintf( "sntp: sockfd error!\n" );
        return -1;
    }

    H_SNTP->LiVnMode = 0x1b;
    
    if( lwip_sendto(sockfd, (void*)H_SNTP, sizeof(SNTP_Header), 0, (struct sockaddr*)&server, sizeof(server)) < 0 ) {
        rt_kprintf( "sntp: sendto error!\n" );
        lwip_close(sockfd);
        return -1;
    }

    FD_ZERO(&set);
    FD_SET(sockfd, &set);
    timeout.tv_sec = 10; timeout.tv_usec = 0;
    
    if( lwip_select(sockfd+1, &set, NULL, NULL, &timeout) <= 0 ) {
        rt_kprintf("sntp: select wait timeout!\n");
        lwip_close(sockfd);
        return -1;
    }
    
    if( lwip_recv(sockfd, (void*)H_SNTP, sizeof(SNTP_Header), 0) < 0 ) {
        rt_kprintf("sntp: recv error!\n"); 
        lwip_close(sockfd);
        return -1;
    }

    H_SNTP->TranTimeInt = ntohl(H_SNTP->TranTimeInt);
    H_SNTP->TranTimeFraction = ntohl(H_SNTP->TranTimeFraction);

    lwip_close(sockfd);
    return 0;
}

rt_tick_t ulNextSyncTick = 0;

static void _sntp_thread(void* parameter)
{
    extern struct netif *netif_list;
    SNTP_Header HeaderSNTP = {0};
    time_t utc_t;
    int num = 0;
    
    while(!netif_list || !(netif_list->flags & NETIF_FLAG_UP) ) {
		rt_thread_delay( 1 * RT_TICK_PER_SECOND );
	}
	while(1) {
        while(g_host_cfg.bSyncTime) {
            if( 0 == __GetNTPTime(&HeaderSNTP,num) ) {
                time_t now = time(0);
                utc_t = HeaderSNTP.TranTimeInt - JAN_1970;
                rt_kprintf("now:%s(%u)\n", ctime(&now), now);
                rt_kprintf("utc:%s(%u)\n", ctime(&utc_t), utc_t);
                if( now != utc_t ) {
                    set_timestamp( utc_t );
                }
                ulNextSyncTick = rt_tick_get() + rt_tick_from_millisecond(30*60*1000);//30*60*1000
            } else {
                num++; 
                if( num > 1 ) num = 0;
                rt_thread_delay( 5 * RT_TICK_PER_SECOND );
            }
            rt_thread_delay( 5 * RT_TICK_PER_SECOND );
            while(!netif_list || !(netif_list->flags & NETIF_FLAG_UP) || rt_tick_get() < ulNextSyncTick ) {
        		rt_thread_delay( 1 * RT_TICK_PER_SECOND );
        	}
        }
        rt_thread_delay( 1 * RT_TICK_PER_SECOND );
    }
}

void sntp_start( void )
{
    rt_thread_t sntpthread = rt_thread_create( "sntp", _sntp_thread, RT_NULL, 0x300, 20, 100 );
    
    if( sntpthread ) {
        //rt_thddog_register(sntpthread, 30);
        rt_thread_startup( sntpthread );
    } else {
        rt_kprintf("Sntp: rt_thread_create error!\n");
    }
}

