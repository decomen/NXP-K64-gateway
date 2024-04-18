/**
  ******************************************************************************
  * @file    enet_phy.c
  * @author  YANDLD
  * @version V2.5
  * @date    2015.02.11
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  ******************************************************************************
  */

#include "rtthread.h"
#include "rtconfig.h"
#include "enet_phy.h"
#include "enet.h"
#include "common.h"

/* MII寄存器地址 */
#define PHY_BMCR                    (0x00) /* Basic Control */
#define PHY_BMSR                    (0x01) /* Basic Status */
#define PHY_PHYIDR1                 (0x02) /* PHY Identifer 1 */
#define PHY_PHYIDR2                 (0x03) /* PHY Identifer 2 */
#define PHY_ANAR                    (0x04) /* Auto-Negotiation Advertisement */
#define PHY_ANLPAR                  (0x05) /* Auto-Negotiation Link Partner Ability */
#define PHY_ANER                    (0x06) /* Auto-Negotiation Expansion */
#define PHY_LPNPA                   (0x07) /* Link Partner Next Page Ability */
#define PHY_RXERC                   (0x15) /* RXER Counter */
#define PHY_ICS                     (0x1B) /* Interrupt Control/Status */
#define PHY_PHYC1                   (0x1E) /* PHY Control 1 */
#define PHY_PHYC2                   (0x1F) /* PHY Control 2 */     
     
     
/* PHY_BMCR寄存器位定义 */
#define PHY_BMCR_RESET              (0x8000)
#define PHY_BMCR_LOOP               (0x4000)
#define PHY_BMCR_SPEED              (0x2000)
#define PHY_BMCR_AN_ENABLE          (0x1000)
#define PHY_BMCR_POWERDOWN          (0x0800)
#define PHY_BMCR_ISOLATE            (0x0400)
#define PHY_BMCR_AN_RESTART         (0x0200)
#define PHY_BMCR_FDX                (0x0100)
#define PHY_BMCR_COL_TEST           (0x0080)

/* PHY_BMSR寄存器位定义 */
#define PHY_BMSR_100BT4             (0x8000)
#define PHY_BMSR_100BTX_FDX         (0x4000)
#define PHY_BMSR_100BTX             (0x2000)
#define PHY_BMSR_10BT_FDX           (0x1000)
#define PHY_BMSR_10BT               (0x0800)
#define PHY_BMSR_NO_PREAMBLE        (0x0040)
#define PHY_BMSR_AN_COMPLETE        (0x0020)
#define PHY_BMSR_REMOTE_FAULT       (0x0010)
#define PHY_BMSR_AN_ABILITY         (0x0008)
#define PHY_BMSR_LINK               (0x0004)
#define PHY_BMSR_JABBER             (0x0002)
#define PHY_BMSR_EXTENDED           (0x0001)

/* PHY_ANAR寄存器位定义 */
#define PHY_ANAR_NEXT_PAGE          (0x8001)
#define PHY_ANAR_REM_FAULT          (0x2001)
#define PHY_ANAR_PAUSE              (0x0401)
#define PHY_ANAR_100BT4             (0x0201)
#define PHY_ANAR_100BTX_FDX         (0x0101)
#define PHY_ANAR_100BTX             (0x0081)
#define PHY_ANAR_10BT_FDX           (0x0041)
#define PHY_ANAR_10BT               (0x0021)
#define PHY_ANAR_802_3              (0x0001)

/* PHY_ANLPAR寄存器位定义 */
#define PHY_ANLPAR_NEXT_PAGE        (0x8000)
#define PHY_ANLPAR_ACK              (0x4000)
#define PHY_ANLPAR_REM_FAULT        (0x2000)
#define PHY_ANLPAR_PAUSE            (0x0400)
#define PHY_ANLPAR_100BT4           (0x0200)
#define PHY_ANLPAR_100BTX_FDX       (0x0100)
#define PHY_ANLPAR_100BTX           (0x0080)
#define PHY_ANLPAR_10BTX_FDX        (0x0040)
#define PHY_ANLPAR_10BT             (0x0020)


/* PHY硬件特性 对应 PHY_PHYC2 */
#define PHY_DUPLEX_STATUS           ( 4<<2 )
#define PHY_SPEED_STATUS            ( 1<<2 )


#define ENET_PHY_DEBUG		1
#if ( ENET_PHY_DEBUG == 1 )
#define ENET_PHY_TRACE	rt_kprintf
#else
#define ENET_PHY_TRACE(...)
#endif

#define ENET_DELAY_MS(_ms)          rt_thread_delay( RT_TICK_PER_SECOND * (_ms) / 1000 )

static int gChipAddr;


 bool _AutoDiscover(uint8_t * Addr)
{
    uint8_t i;
    uint16_t data;

    for(i = 0; i< 0xFF; i++)
    {
        data = 0;
        ENET_MII_Read(i, PHY_PHYIDR1, &data);
        if((data !=0) && (data != 0xFFFF))
        {
            ENET_PHY_TRACE("Addr:%d Val:0x%X found!\r\n", i, data);
            *Addr = i;
            return true;
        }
    }
    return false;
}

int enet_phy_init(void)
{
    uint16_t usData;
	uint16_t timeout = 0;
    
    /* init MII interface */
    ENET_MII_Init();
    
    uint8_t addr;
    _AutoDiscover(&addr);
    gChipAddr = addr;
    
    /* software reset PHY */
    ENET_MII_Write(gChipAddr, PHY_BMCR, PHY_BMCR_RESET);
    ENET_DELAY_MS(10);
    
    /* waitting for reset OK */
    do
    {
        ENET_DELAY_MS(10);
        timeout++;
        usData = 0xFFFF;
        ENET_MII_Read(gChipAddr, PHY_PHYIDR1, &usData );
        if((usData != 0xFFFF) && (usData != 0x0000))
        {
            ENET_PHY_TRACE("PHY_PHYIDR1:0x%X\r\n",usData);
            break;
        }
    }while(timeout < 10);
    
    
    ENET_MII_Read(gChipAddr, PHY_BMCR, &usData );
    ENET_PHY_TRACE("PHY_BMCR:0x%X\r\n",usData);
    ENET_MII_Read(gChipAddr, PHY_BMSR, &usData );
    ENET_PHY_TRACE("PHY_BMSR:0x%X\r\n",usData);
    ENET_MII_Read(gChipAddr, PHY_PHYIDR1, &usData );
    ENET_PHY_TRACE("PHY_PHYIDR1:0x%X\r\n",usData);
    ENET_MII_Read(gChipAddr, PHY_PHYIDR2, &usData );
    ENET_PHY_TRACE("PHY_PHYIDR2:0x%X\r\n",usData);
    ENET_MII_Read(gChipAddr, PHY_ANAR, &usData );
    ENET_PHY_TRACE("PHY_ANAR:0x%X\r\n",usData);
    ENET_MII_Read(gChipAddr, PHY_ANLPAR, &usData );
    ENET_PHY_TRACE("PHY_ANLPAR:0x%X\r\n",usData);
    ENET_MII_Read(gChipAddr, PHY_ANER, &usData );
    ENET_PHY_TRACE("PHY_ANER:0x%X\r\n",usData);
    ENET_MII_Read(gChipAddr, PHY_LPNPA, &usData );
    ENET_PHY_TRACE("PHY_LPNPA:0x%X\r\n",usData);
    ENET_MII_Read(gChipAddr, PHY_RXERC, &usData );
    ENET_PHY_TRACE("PHY_RXERC:0x%X\r\n",usData);
    ENET_MII_Read(gChipAddr, PHY_ICS, &usData );
    ENET_PHY_TRACE("PHY_ICS:0x%X\r\n",usData);
    ENET_MII_Read(gChipAddr, PHY_PHYC1, &usData );
    ENET_PHY_TRACE("PHY_PHYC1:0x%X\r\n",usData);
    ENET_MII_Read(gChipAddr, PHY_PHYC2, &usData );
    ENET_PHY_TRACE("PHY_PHYC2:0x%X\r\n",usData);

    /* start auto-negotiation */
    ENET_MII_Write(gChipAddr, PHY_BMCR, (PHY_BMCR_AN_RESTART | PHY_BMCR_AN_ENABLE ));
    ENET_MII_Read(gChipAddr, PHY_BMCR, &usData );
    ENET_PHY_TRACE("PHY_BMCR=0x%X\r\n",usData);
    /* waitting for auto-negotiation completed */
    do
    {
        ENET_DELAY_MS(50);
		timeout++;
		if(timeout > 60)
        {
            ENET_PHY_TRACE("enet Auto-Negotiation failed\r\n");
            break;
        }
        ENET_MII_Read(gChipAddr, PHY_BMSR, &usData );
    } while( !( usData & PHY_BMSR_AN_COMPLETE ) );
    
    if(!enet_phy_is_linked())
    {
        ENET_PHY_TRACE("enet_phy - no wire connection!\r\n");
    }
    
    return 0;
}

bool enet_phy_is_full_dpx(void)
{
    uint16_t usData;
    ENET_MII_Read(gChipAddr, PHY_PHYC2, &usData );
    if( usData & PHY_DUPLEX_STATUS )
    {
        ENET_PHY_TRACE("enet_phy-full duplex\r\n");
        return true;
    }
    ENET_PHY_TRACE("enet_phy-falf duplex\r\n");
    return false;
}

bool enet_phy_is_10m_speed(void)
{
    uint16_t usData;
    ENET_MII_Read(gChipAddr, PHY_PHYC2, &usData );
    if( usData & PHY_SPEED_STATUS )
    {
        ENET_PHY_TRACE("enet_phy-10M speed\r\n");
        return true;
    }
    ENET_PHY_TRACE("enet_phy-100M speed\r\n");
    return false;
}


/**
 * @brief  网线是否连接
 * @retval 0:连接 1:未连接
 */
bool enet_phy_is_linked(void)
{
	uint16_t reg = 0;
	ENET_MII_Read(gChipAddr, PHY_BMSR, &reg);
	if(reg & 0x0004)
	{
		return true;
	}
	else
	{
		return false;
	}
}



