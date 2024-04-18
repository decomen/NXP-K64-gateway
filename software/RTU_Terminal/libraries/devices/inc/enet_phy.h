/**
  ******************************************************************************
  * @file    enet_phy.h
  * @author  YANDLD
  * @version V2.5
  * @date    2015.02.11
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  ******************************************************************************
  */
#ifndef __ENEY_PHY_H__
#define __ENEY_PHY_H__

#include <stdbool.h>




//!< API
int enet_phy_init(void);
bool enet_phy_is_linked(void);
bool enet_phy_is_full_dpx(void);
bool enet_phy_is_10m_speed(void);


#endif


