/********************************** (C) COPYRIGHT *******************************
* File Name          : TouchKey.C
* Author             : WCH
* Version            : V1.2
* Date               : 2017/11/18
* Description        : Touchkey driver c file.
*******************************************************************************/
#include "CH554.H"                                                          
//#include "..\Public\Debug.H"
#include "stdio.H"
#include "TOUCHKEY.H"
/*******************************************************************************
Input channel as below:

 bTKC_CHAN2 |bTKC_CHAN1	|bTKC_CHAN0	|	CHANNEL				| PIN		
 -----------+-----------+-----------+-----------------------+------------
 	0		|	0		|	0		|	no channel			| ****		
 	0		|	0		|	1		|	TIN0				| P1.0		
 	0		|	1		|	0		|	TIN1				| P1.1		
 	0		|	1		|	1		|	TIN2				| P1.4		
 	1		|	0		|	0		|	TIN3				| P1.5		
 	1		|	0		|	1		|	TIN4				| P1.6		
 	1		|	1		|	0		|	TIN5				| P1.7		
	1		|	1		|	1		|	Enable touch core	| no channel
	
*******************************************************************************/
	
UINT8 	TK_CH[TOUCH_NUM] = {								/* Arrange the input channel */			
//	0x01, 0x02, 															  /* CH0 CH1 */
//	0x03, 0x04, 															  /* CH2 CH3 */
0x02,	0x05, 0x06																  /* CH1 CH4 CH5 */
};		

UINT16 	Key_FreeBuf[TOUCH_NUM];
UINT8V 	Touch_CH = 0;												  /* BIT6 & BIT7 reserved, other bit means touch state */

/*******************************************************************************
* Function Name  : TK_SelectChannel
* Description    : Select TK input channel.
* Input          : ch : input channel.
* Return         : SUCCESS/FAIL.
*******************************************************************************/
UINT8 TK_SelectChannel( UINT8 ch )
{
	if ( ch <= TOUCH_NUM )
	{
		TKEY_CTRL = ( TKEY_CTRL & 0XF8) | TK_CH[ch];
		return SUCCESS;
	}

	return	FAIL;
}

/*******************************************************************************
* Function Name  : TK_Init
* Description    : Init input channel. Float input, if it used for touchkey
* Input          : channel   : IO bits, configure the Hi-z mode for touch detection.
				   queryFreq : Scan frequence. 0: 1ms , 1: 2ms.
				   ie		 : Enable interrupt, 0:disable ; i: enable.
* Return         : Return FAIL, if channel's error.
*******************************************************************************/
UINT8 TK_Init( UINT8 channel , UINT8 queryFreq, UINT8 ie )
{

	UINT8 	i,j;
	UINT16 	sum;
	UINT16 	OverTime;
	if ( ( channel & (BIT2+BIT3) ) != 0 )							/* not include BIT2 & BIT3 */
	{
		return FAIL;
	}
	
	P1_DIR_PU &= ~channel;
	P1_MOD_OC &= ~channel;
	
	if( queryFreq != 0 ) 
	{
		TKEY_CTRL |= bTKC_2MS ;
	}
	
	/* Get Key_FreeBuf. Save the data in flash or macro define. */
	/* DO NOT get Key_FreeBuf in Mass Production. */
	for ( i = 0; i < TOUCH_NUM; i++ )
	{
		sum = 0;
		j = SAMPLE_TIMES;
		TK_SelectChannel( i );
//		TKEY_CTRL |= TK_Code[i];
		while( j-- )
		{
			OverTime = 0;
			while( ( TKEY_CTRL & bTKC_IF ) == 0 )
			{
				if( ++OverTime == 0 )
				{
					return FAIL;
				}
			}
			sum += TKEY_DAT;													/*  */
		}
		Key_FreeBuf[i] = sum / SAMPLE_TIMES;
		//printf( "Key_FreeBuf[%d]=%d\t", (UINT16)(i), (UINT16)Key_FreeBuf[i] );
	}
	//printf( "\n" );
	if( ie != 0 )																/* Enable interrupt ?  */		
	{
		IE_TKEY = 1;     												
	}		
	
	return SUCCESS;
}

/*******************************************************************************
* Function Name  : TK_Measure
* Description    : Measure touch input.
* Input          : None
* Return         : None
*******************************************************************************/
UINT8 TK_Measure( void )
{
	UINT8 	i;
	UINT16 	OverTime, KeyData;
	
	Touch_CH = 0;
	
	for( i = 0; i < TOUCH_NUM; i++ )
	{
		OverTime = 0;
		TK_SelectChannel( i );

		while( ( TKEY_CTRL & bTKC_IF ) == 0 )
		{
			if( ++OverTime == 0 )
			{
				return FAIL;
			}
		}

		KeyData = TKEY_DAT;

		if ( KeyData < ( Key_FreeBuf[i] - TH_VALUE ) )
		{
			Touch_CH |=  1 << ( TK_CH[i] - 1 );
		}
		
//		printf( "ch[%d]=%d\t", (UINT16)(TK_Code[i] - 1), (UINT16)KeyData );
	}
	
//	printf( "%02x\n", (UINT16)Touch_IN );
	
	return SUCCESS;
}




/* End of file. */