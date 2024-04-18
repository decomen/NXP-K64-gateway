#include <board.h>
#include <in_out.h>

void vInOutInit(void)
{
	rt_pin_mode( BOARD_GPIO_RELAYS_OUT1, PIN_MODE_OUTPUT );
	rt_pin_mode( BOARD_GPIO_RELAYS_OUT2, PIN_MODE_OUTPUT );
	rt_pin_mode( BOARD_GPIO_RELAYS_OUT3, PIN_MODE_OUTPUT );
	
	rt_pin_mode( BOARD_GPIO_RELAYS_OUT4, PIN_MODE_OUTPUT );
	PORTA->PCR[19] &= ~PORT_PCR_MUX_MASK; //PTA19需要设置复用功能才能当做GPIO口用
	PORTA->PCR[19] |= PORT_PCR_MUX(0x01);

	rt_pin_mode( BOARD_GPIO_TTL_OUT1, PIN_MODE_OUTPUT );
	rt_pin_mode( BOARD_GPIO_TTL_OUT2, PIN_MODE_OUTPUT );

	rt_pin_mode( BOARD_GPIO_TTL_INPUT1, PIN_MODE_INPUT );
	rt_pin_mode( BOARD_GPIO_TTL_INPUT2, PIN_MODE_INPUT );
	rt_pin_mode( BOARD_GPIO_TTL_INPUT3, PIN_MODE_INPUT );
	rt_pin_mode( BOARD_GPIO_TTL_INPUT4, PIN_MODE_INPUT );

	rt_pin_mode( BOARD_GPIO_ONOFF_INPUT1, PIN_MODE_INPUT );
	rt_pin_mode( BOARD_GPIO_ONOFF_INPUT2, PIN_MODE_INPUT );
	rt_pin_mode( BOARD_GPIO_ONOFF_INPUT3, PIN_MODE_INPUT );
	rt_pin_mode( BOARD_GPIO_ONOFF_INPUT4, PIN_MODE_INPUT );
	
	
}

void vRelaysOutputConfig(eRelays_OutPut_Chanel_t chan, eInOut_stat_t sta)
{
	UCHAR ucSta = 0;
	if(sta == PIN_RESET) {
		ucSta = PIN_LOW;
	}else {
		ucSta = PIN_HIGH;
	}
	
	switch(chan){
		case RELAYS_OUTPUT_1:{
			rt_pin_write( BOARD_GPIO_RELAYS_OUT1, ucSta);
			break;
		}
		case RELAYS_OUTPUT_2:{
			rt_pin_write( BOARD_GPIO_RELAYS_OUT2, ucSta);
			break;
		}
		case RELAYS_OUTPUT_3:{
			rt_pin_write( BOARD_GPIO_RELAYS_OUT3, ucSta);
			break;
		}
		case RELAYS_OUTPUT_4:{
			rt_pin_write( BOARD_GPIO_RELAYS_OUT4, ucSta);
			break;
		}
	}
}

void vRelaysOutputGet(eRelays_OutPut_Chanel_t chan, eInOut_stat_t *sta)
{
	UCHAR ucSta = 0;
	
	switch(chan){
		case RELAYS_OUTPUT_1:{
			if(rt_pin_read( BOARD_GPIO_RELAYS_OUT1) == PIN_LOW){
				ucSta = PIN_RESET;
			}else {
				ucSta = PIN_SET;
			}
			break;
		}
		case RELAYS_OUTPUT_2:{
			if(rt_pin_read( BOARD_GPIO_RELAYS_OUT2) == PIN_LOW){
				ucSta = PIN_RESET;
			}else {
				ucSta = PIN_SET;
			}
			break;
		}
		case RELAYS_OUTPUT_3:{
			if(rt_pin_read( BOARD_GPIO_RELAYS_OUT3) == PIN_LOW){
				ucSta = PIN_RESET;
			}else {
				ucSta = PIN_SET;
			}
			break;
		}
		case RELAYS_OUTPUT_4:{
			if(rt_pin_read( BOARD_GPIO_RELAYS_OUT4) == PIN_LOW){
				ucSta = PIN_RESET;
			}else {
				ucSta = PIN_SET;
			}
			break;
		}
	}
	*sta = ucSta;
}

void vRelaysOutputToggle(eRelays_OutPut_Chanel_t chan)
{
	eInOut_stat_t xStat;
	
	switch(chan){
		case RELAYS_OUTPUT_1:{
			vRelaysOutputGet( RELAYS_OUTPUT_1 , &xStat) ;
			if(xStat == PIN_SET){
				vRelaysOutputConfig(RELAYS_OUTPUT_1, PIN_RESET);
			}else {
				vRelaysOutputConfig(RELAYS_OUTPUT_1, PIN_SET);
			}
			break;
		}
		case RELAYS_OUTPUT_2:{
			vRelaysOutputGet( RELAYS_OUTPUT_2 , &xStat) ;
			if(xStat == PIN_SET){
				vRelaysOutputConfig(RELAYS_OUTPUT_2, PIN_RESET);
			}else {
				vRelaysOutputConfig(RELAYS_OUTPUT_2, PIN_SET);
			}
			break;
		}
		case RELAYS_OUTPUT_3:{
			vRelaysOutputGet( RELAYS_OUTPUT_3 , &xStat) ;
			if(xStat == PIN_SET){
				vRelaysOutputConfig(RELAYS_OUTPUT_3, PIN_RESET);
			}else {
				vRelaysOutputConfig(RELAYS_OUTPUT_3, PIN_SET);
			}
			break;
		}
		case RELAYS_OUTPUT_4:{
			vRelaysOutputGet( RELAYS_OUTPUT_4 , &xStat) ;
			if(xStat == PIN_SET){
				vRelaysOutputConfig(RELAYS_OUTPUT_4, PIN_RESET);
			}else {
				vRelaysOutputConfig(RELAYS_OUTPUT_4, PIN_SET);
			}
			break;
		}
	}
}

void vTTLOutputConfig(eTTL_Output_Chanel_t chan, eInOut_stat_t sta)
{
	UCHAR ucSta = 0;
	if(sta == PIN_RESET){
		ucSta = PIN_LOW;
	}else {
		ucSta = PIN_HIGH;
	}
	
	switch(chan){
		case TTL_OUTPUT_1:{
			rt_pin_write( BOARD_GPIO_TTL_OUT1, ucSta);
			break;
		}
		case TTL_OUTPUT_2:{
			rt_pin_write( BOARD_GPIO_TTL_OUT2, ucSta);
			break;
		}
	}
}

void vTTLOutputGet(eTTL_Output_Chanel_t chan, eInOut_stat_t *sta)
{
	UCHAR ucSta = 0;
	
	switch(chan){
		case TTL_OUTPUT_1:{
			if(rt_pin_read( BOARD_GPIO_TTL_OUT1) == PIN_LOW){
				ucSta = PIN_RESET;
			}else {
				ucSta = PIN_SET;
			}
			break;
		}
		case TTL_OUTPUT_2:{
			if(rt_pin_read( BOARD_GPIO_TTL_OUT2) == PIN_LOW){
				ucSta = PIN_RESET;
			}else {
				ucSta = PIN_SET;
			}
			break;
		}
	}
	
	*sta = ucSta;
}

void vTTLOutputToggle(eTTL_Output_Chanel_t chan)
{
	eInOut_stat_t xStat;
	
	switch(chan){
		case TTL_OUTPUT_1:{
			vTTLOutputGet( TTL_OUTPUT_1 , &xStat) ;
			if(xStat == PIN_SET){
				vTTLOutputConfig(TTL_OUTPUT_1, PIN_RESET);
			}else {
				vTTLOutputConfig(TTL_OUTPUT_1, PIN_SET);
			}
			break;
		}
		case TTL_OUTPUT_2:{
			vTTLOutputGet( TTL_OUTPUT_2 , &xStat) ;
			if(xStat == PIN_SET){
				vTTLOutputConfig(TTL_OUTPUT_2, PIN_RESET);
			}else {
				vTTLOutputConfig(TTL_OUTPUT_2, PIN_SET);
			}
			break;
		}
		
	}
}

void vTTLInputputGet(eTTL_Input_Chanel_t chan, eInOut_stat_t *sta)
{
	UCHAR ucSta = 0;
	switch(chan){
		case TTL_INPUT_1:{
			if(rt_pin_read( BOARD_GPIO_TTL_INPUT1) == PIN_LOW){
				ucSta = PIN_RESET;
			}else {
				ucSta = PIN_SET;
			}
			break;
		}
		case TTL_INPUT_2:{
			if(rt_pin_read( BOARD_GPIO_TTL_INPUT2) == PIN_LOW){
				ucSta = PIN_RESET;
			}else {
				ucSta = PIN_SET;
			}
			break;
		}
		case TTL_INPUT_3:{
			if(rt_pin_read( BOARD_GPIO_TTL_INPUT3) == PIN_LOW){
				ucSta = PIN_RESET;
			}else {
				ucSta = PIN_SET;
			}
			break;
		}
		case TTL_INPUT_4:{
			if(rt_pin_read( BOARD_GPIO_TTL_INPUT4) == PIN_LOW){
				ucSta = PIN_RESET;
			}else {
				ucSta = PIN_SET;
			}
			break;
		}
	}
	*sta = ucSta;
}

void vOnOffInputGet(eOnOff_Input_Chanel_t chan, eInOut_stat_t *sta)
{
	UCHAR ucSta = 0;
	switch(chan){
		case ONOFF_INPUT_1:{
			if(rt_pin_read( BOARD_GPIO_ONOFF_INPUT4) == PIN_LOW){
				ucSta = PIN_RESET;
			}else {
				ucSta = PIN_SET;
			}
			break;
		}
		case ONOFF_INPUT_2:{
			if(rt_pin_read( BOARD_GPIO_ONOFF_INPUT3) == PIN_LOW){
				ucSta = PIN_RESET;
			}else {
				ucSta = PIN_SET;
			}
			break;
		}
		case ONOFF_INPUT_3:{
			if(rt_pin_read( BOARD_GPIO_ONOFF_INPUT2) == PIN_LOW){
				ucSta = PIN_RESET;
			}else {
				ucSta = PIN_SET;
			}
			break;
		}
		case ONOFF_INPUT_4:{
			if(rt_pin_read( BOARD_GPIO_ONOFF_INPUT1) == PIN_LOW){
				ucSta = PIN_RESET;
			}else {
				ucSta = PIN_SET;
			}
			break;
		}
	}
	*sta = ucSta;
}

DEF_CGI_HANDLER(getDiValue)
{
    rt_bool_t first = RT_TRUE;

    first = RT_TRUE;
    WEBS_PRINTF( "{\"ret\":0,\"di\":[" );
    for( int i = 0; i < 8; i++ ) {
        cJSON *pItem = cJSON_CreateObject();
        if(pItem) {
            if( !first ) WEBS_PRINTF( "," );
            first = RT_FALSE;
            cJSON_AddNumberToObject( pItem, "va", (g_xDIResultReg.xDIResult.usDI_xx[i] == (PIN_RESET ? 0 : 1)) );
            char *szRetJSON = cJSON_PrintUnformatted( pItem );
            if(szRetJSON) {
                WEBS_PRINTF( szRetJSON );
                rt_free( szRetJSON );
            }
        }
        cJSON_Delete( pItem );
    }
    WEBS_PRINTF( "]}" );

	WEBS_DONE(200);
}

DEF_CGI_HANDLER(setDoValue)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse( szJSON ) : RT_NULL;
    if( pCfg ) {
        cJSON *pAry = cJSON_GetObjectItem( pCfg, "do" );
        if( pAry ) {
            int n = cJSON_GetArraySize( pAry );
            for( int i = 0; i < n; i++ ) {
                cJSON *pDO = cJSON_GetArrayItem( pAry, i );
                if(pDO) {
                    int n = cJSON_GetInt( pDO, "n", -1 );
                    int va = cJSON_GetInt( pDO, "va", -1 );
                    if( n >= 0 && n < DO_CHAN_NUM && va >= 0 ) {
                        g_xDOResultReg.xDOResult.usDO_xx[n] = (va!=0?PIN_SET:PIN_RESET);
                    }
                }
            }
        }
    }

    cJSON_Delete(pCfg);
    WEBS_PRINTF( "{\"ret\":0}" );
	WEBS_DONE(200);
}

DEF_CGI_HANDLER(getDoValue)
{
    rt_bool_t first = RT_TRUE;

    first = RT_TRUE;
    WEBS_PRINTF( "{\"ret\":0,\"do\":[" );
    for( int i = 0; i < DO_CHAN_NUM; i++ ) {
        cJSON *pItem = cJSON_CreateObject();
        if(pItem) {
            if( !first ) WEBS_PRINTF( "," );
            first = RT_FALSE;
            cJSON_AddNumberToObject( pItem, "va", (g_xDOResultReg.xDOResult.usDO_xx[i] == (PIN_RESET ? 0 : 1)) );
            char *szRetJSON = cJSON_PrintUnformatted( pItem );
            if(szRetJSON) {
                WEBS_PRINTF( szRetJSON );
                rt_free( szRetJSON );
            }
        }
        cJSON_Delete( pItem );
    }
    WEBS_PRINTF( "]}" );

	WEBS_DONE(200);
}

