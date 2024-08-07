/********************************** (C) COPYRIGHT *******************************
* File Name          :USBDEV.C
* Author             : WCH
* Version            : V1.3
* Date               : 2016/06/24
* Description        : CH559模拟USB 37X设备
 USB device example for CH559, use CH372 tool TEST.exe to check it under Windows
*******************************************************************************/
#include "..\..\DEBUG.C"                                                       //调试信息打印
#include "..\..\DEBUG.H"
#include <string.h>

#define THIS_ENDP0_SIZE         DEFAULT_ENDP0_SIZE
#define ENDP1_IN_SIZE           64
#define ENDP2_IN_SIZE           64
#define ENDP2_OUT_SIZE          64

/*设备描述符*/
UINT8C  MyDevDescr[] = { 0x12, 0x01, 0x10, 0x01,
                         0xFF, 0x80, 0x55, THIS_ENDP0_SIZE,
                         0x48, 0x43, 0x37, 0x55,                              // 厂商ID和产品ID
                         0x00, 0x01, 0x01, 0x02,
                         0x00, 0x01
                       };
/*配置描述符*/
UINT8C  MyCfgDescr[] = { 0x09, 0x02, 0x27, 0x00, 0x01, 0x01, 0x00, 0x80, 0x32,
                         0x09, 0x04, 0x00, 0x00, 0x03, 0xFF, 0x80, 0x55, 0x00,
                         0x07, 0x05, 0x82, 0x02, ENDP2_IN_SIZE, 0x00, 0x00,
                         0x07, 0x05, 0x02, 0x02, ENDP2_OUT_SIZE, 0x00, 0x00,
                         0x07, 0x05, 0x81, 0x03, ENDP1_IN_SIZE, 0x00, 0x00
                       };
/*语言描述符*/
UINT8C  MyLangDescr[] = { 0x04, 0x03, 0x09, 0x04 };
/*厂家信息*/
UINT8C  MyManuInfo[] = { 0x0E, 0x03, 'w', 0, 'c', 0, 'h', 0, '.', 0, 'c', 0, 'n', 0 };
/*产品信息*/
UINT8C  MyProdInfo[] = { 0x0C, 0x03, 'C', 0, 'H', 0, '5', 0, '5', 0, '9', 0 };

UINT8   UsbConfig = 0;                                                         // USB配置标志
UINT8X  Ep0Buffer[MIN(64,THIS_ENDP0_SIZE+2)] _at_ 0x0000 ; // OUT&IN, must even address
UINT8X  Ep1Buffer[MIN(64,ENDP1_IN_SIZE+2)] _at_ MIN(64,THIS_ENDP0_SIZE+2); // IN, must even address
UINT8X  Ep2Buffer[MIN(64,ENDP2_IN_SIZE+2)+MIN(64,ENDP2_OUT_SIZE+2)] _at_ (MIN(64,THIS_ENDP0_SIZE+2)+MIN(64,ENDP1_IN_SIZE+2));// OUT+IN, must even address

#define UsbSetupBuf     ((PUSB_SETUP_REQ)Ep0Buffer)

/*运行状态指示*/
sbit LED_RUN  = P1^0;
sbit LED_CFG  = P1^1;
sbit LED_TMP  = P1^2;

#pragma NOAREGS

/*******************************************************************************
* Function Name  : USB_DeviceInterrupt()
* Description    : CH559USB模拟设置中断处理函数
*******************************************************************************/
void    USB_DeviceInterrupt( void ) interrupt INT_NO_USB using 1               /* USB中断服务程序,使用寄存器组1 */
{
    UINT8   i, len;
    static  UINT8   SetupReqCode, SetupLen;
    static  PUINT8  pDescr;
    LED_TMP = 0;
	
    if ( UIF_TRANSFER )                                                        // USB传输完成
    {
        if ( U_IS_NAK )                                                        // not enable for this example
        {
//          switch ( USB_INT_ST & ( MASK_UIS_TOKEN | MASK_UIS_ENDP ) )         // 分析操作令牌和端点号
//          {  
//              case UIS_TOKEN_OUT | 2:                                        // endpoint 2# 批量端点下传
//                  break;
//              case UIS_TOKEN_IN | 2:                                         // endpoint 2# 批量端点上传
//                  break;
//              case UIS_TOKEN_IN | 1:                                         // endpoint 1# 中断端点上传
//                  break;
//              default:
//                  break;
//          }
            printf("NAK INT,PrepareData\n");
        }
        else
        {
            switch ( USB_INT_ST & ( MASK_UIS_TOKEN | MASK_UIS_ENDP ) )         // 分析操作令牌和端点号
            {
            case UIS_TOKEN_OUT | 2:                                            // endpoint 2# 批量端点下传
                if ( U_TOG_OK )                                                // 不同步的数据包将丢弃
                {
                    UEP2_CTRL ^= bUEP_R_TOG;                                   // 手动翻转
                    len = USB_RX_LEN;
                    for ( i = 0; i < len; i ++ )
                    {
                        Ep2Buffer[MAX_PACKET_SIZE+i] = Ep2Buffer[i] ^ 0xFF;    // OUT数据取反到IN由计算机验证
                    }
                    UEP2_T_LEN = len;
                    UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;  // 允许上传
                }
                break;
            case UIS_TOKEN_IN | 2:                                             // endpoint 2# 批量端点上传
                UEP2_CTRL ^= bUEP_T_TOG;                                       // 手动翻转
                UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;      // 暂停上传
                break;
            case UIS_TOKEN_IN | 1:                                             // endpoint 1# 中断端点上传
                UEP1_CTRL ^= bUEP_T_TOG;                                       // 手动翻转
                UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;      // 暂停上传
                break;
            case UIS_TOKEN_SETUP | 0:                                          // endpoint 0# SETUP
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
				len = USB_RX_LEN;
                if ( len == sizeof( USB_SETUP_REQ ) )                          // SETUP包长度
                {
                    SetupLen = UsbSetupBuf->wLengthL;
                    if ( UsbSetupBuf->wLengthH || SetupLen > 0x7F )
                    {
                        SetupLen = 0x7F;                                       // 限制总长度
                    }
                    len = 0;                                                   // 默认为成功并且上传0长度
                    if ( ( UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK ) != USB_REQ_TYP_STANDARD )/* 只支持标准请求 */
                    {
                        len = 0xFF;                                            // 操作失败
                        printf("ErrEp0ReqType=%02X\n",(UINT16)UsbSetupBuf->bRequestType);
                    }
                    else                                                       // 标准请求
                    {
                        SetupReqCode = UsbSetupBuf->bRequest;
                        switch( SetupReqCode )                                 // 请求码
                        {
                        case USB_GET_DESCRIPTOR:
                            switch( UsbSetupBuf->wValueH )
                            {
                            case 1:                                            // 设备描述符
                                pDescr = (PUINT8)( &MyDevDescr[0] );
                                len = sizeof( MyDevDescr );
                                break;
                            case 2:                                            // 配置描述符
                                pDescr = (PUINT8)( &MyCfgDescr[0] );
                                len = sizeof( MyCfgDescr );
                                break;
                            case 3:                                            // 字符串描述符
                                switch( UsbSetupBuf->wValueL )
                                {
                                case 1:
                                    pDescr = (PUINT8)( &MyManuInfo[0] );       
                                    len = sizeof( MyManuInfo );
                                    break;
                                case 2:
                                    pDescr = (PUINT8)( &MyProdInfo[0] );        
                                    len = sizeof( MyProdInfo );
                                    break;
                                case 0:
                                    pDescr = (PUINT8)( &MyLangDescr[0] );
                                    len = sizeof( MyLangDescr );
                                    break;
                                default:
                                    len = 0xFF;                                 // 不支持的字符串描述符
                                    break;
                                }
                                break;
                            default:
                                len = 0xFF;                                     // 不支持的描述符类型
                                break;
                            }
                            if ( SetupLen > len )
                            {
                                SetupLen = len;                                 // 限制总长度
                            }
                            len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen;  // 本次传输长度
                            memcpy( Ep0Buffer, pDescr, len );                   /* 加载上传数据 */
                            SetupLen -= len;
                            pDescr += len;
                            break;
                        case USB_SET_ADDRESS:
                            SetupLen = UsbSetupBuf->wValueL;                    // 暂存USB设备地址
                            break;
                        case USB_GET_CONFIGURATION:
                            Ep0Buffer[0] = UsbConfig;
                            if ( SetupLen >= 1 )
                            {
                                len = 1;
                            }
                            break;
                        case USB_SET_CONFIGURATION:
                            UsbConfig = UsbSetupBuf->wValueL;
                            if ( UsbConfig )
                            {
                                LED_CFG = 0;
                            }
                            else
                            {
                                LED_CFG = 1;
                            }
                            break;
                        case USB_CLEAR_FEATURE:
                            if ( ( UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP )// 端点
                            {
                                switch( UsbSetupBuf->wIndexL )
                                {
                                case 0x82:
                                    UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                    break;
                                case 0x02:
                                    UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
                                    break;
                                case 0x81:
                                    UEP1_CTRL = UEP1_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                    break;
                                case 0x01:
                                    UEP1_CTRL = UEP1_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
                                    break;
                                default:
                                    len = 0xFF;                                            // 不支持的端点
                                    break;
                                }
                            }
                            else
                            {
                                len = 0xFF;                                                // 不是端点不支持
                            }
                            break;
                        case USB_SET_FEATURE:                                              /* Set Feature */
                        if( ( UsbSetupBuf->bRequestType & 0x1F ) == 0x00 )                 /* 设置设备 */
                        {
                          if( ( ( ( UINT16 )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x01 )
                          {
                            if( MyCfgDescr[ 7 ] & 0x20 )
                            {
                                 /* 设置唤醒使能标志 */
                            }
                            else
                            {
                                len = 0xFF;                                                 /* 操作失败 */
                            }
                          }
                          else
                          {
                            len = 0xFF;                                                     /* 操作失败 */
                          }
                        }
                        else if( ( UsbSetupBuf->bRequestType & 0x1F ) == 0x02 )             /* 设置端点 */
                        {
                          if( ( ( ( UINT16 )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x00 )
                          {
                            switch( ( ( UINT16 )UsbSetupBuf->wIndexH << 8 ) | UsbSetupBuf->wIndexL )
                            {
                                case 0x82:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* 设置端点2 IN STALL */
                                    break;

                                case 0x02:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* 设置端点2 OUT Stall */
                                    break;

                                case 0x81:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* 设置端点1 IN STALL */
                                    break;

                                default:
                                    len = 0xFF;                                    /* 操作失败 */
                                    break;
                              }
                             }
                             else
                             {
                               len = 0xFF;                                          /* 操作失败 */
                             }
                           }
                           else
                            {
                             len = 0xFF;                                             /* 操作失败 */
                            }
                            break;													
                        case USB_GET_INTERFACE:
                            Ep0Buffer[0] = 0x00;
                            if ( SetupLen >= 1 )
                            {
                                len = 1;
                            }
                            break;
                        case USB_GET_STATUS:
                            Ep0Buffer[0] = 0x00;
                            Ep0Buffer[1] = 0x00;
                            if ( SetupLen >= 2 )
                            {
                                len = 2;
                            }
                            else
                            {
                                len = SetupLen;
                            }
                            break;
                        default:
                            len = 0xFF;                                             // 操作失败
                            printf("ErrEp0ReqCode=%02X\n",(UINT16)SetupReqCode);
                            break;
                        }
                    }
                }
                else
                {
                    len = 0xFF;                                                    // SETUP包长度错误
                    printf("ErrEp0ReqSize\n");
                }
                if ( len == 0xFF )                                                 // 操作失败
                {
                    SetupReqCode = 0xFF;
                    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;// STALL
                }
                else if ( len <= THIS_ENDP0_SIZE )                                 // 上传数据或者状态阶段返回0长度包
                {
                    UEP0_T_LEN = len;
                    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;// 默认数据包是DATA1
                }
                else                                                               // 下传数据或其它
                {
                    UEP0_T_LEN = 0;                                                // 虽然尚未到状态阶段，但是提前预置上传0长度数据包以防主机提前进入状态阶段
                    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;// 默认数据包是DATA1
                }
                break;
            case UIS_TOKEN_IN | 0:                                                 // endpoint 0# IN
                switch( SetupReqCode )
                {
                case USB_GET_DESCRIPTOR:
                    len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen; // 本次传输长度
                    memcpy( Ep0Buffer, pDescr, len );                               /* 加载上传数据 */
                    SetupLen -= len;
                    pDescr += len;
                    UEP0_T_LEN = len;
                    UEP0_CTRL ^= bUEP_T_TOG;                                        // 翻转
                    break;
                case USB_SET_ADDRESS:
                    USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
                    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                    break;
                default:
                    UEP0_T_LEN = 0;                                                 // 状态阶段完成中断或者是强制上传0长度数据包结束控制传输
                    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                    break;
                }
                break;
            case UIS_TOKEN_OUT | 0:                                                 // endpoint 0# OUT
                switch( SetupReqCode )
                {
//                      case download:
//                          if ( U_TOG_OK ) {                                       // 不同步的数据包将丢弃
//                              UEP0_CTRL ^= bUEP_R_TOG;                            // 翻转
//                              get_data;
//                              //UEP0_CTRL = UEP0_CTRL & bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;// 预置上传0长度数据包DATA1以防主机提前进入状态阶段
//                          }
//                          break;
                case USB_GET_DESCRIPTOR:
                default:
                    if ( U_TOG_OK )                                                 // 不同步的数据包将丢弃
                    {
//                              if ( USB_RX_LEN ) control_status_error;
//                              else control_ok;                                    // 收到0长度包表示控制读操作/上传OK
                    }
//                          else control_status_error;
                    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;                      // 准备下一控制传输
                    break;
                }
                break;
            default:
                printf("ErrEndp INT\n");
                break;
            }
        }
        UIF_TRANSFER = 0;                                                           // 清中断标志
    }
    else if ( UIF_BUS_RST )                                                         // USB总线复位
    {
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP1_CTRL = UEP_T_RES_NAK;
        UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        USB_DEV_AD = 0x00;
        LED_CFG = 1;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;
        UIF_BUS_RST = 0;                                                            // 清中断标志
    }
    else if ( UIF_SUSPEND )                                                         // USB总线挂起/唤醒完成
    {
        UIF_SUSPEND = 0;
        if ( USB_MIS_ST & bUMS_SUSPEND )                                            // 挂起
        {
            printf( "Zzz" );                                                        // 睡眠状态
            LED_RUN = 1;
            while ( XBUS_AUX & bUART0_TX );                                         // 等待发送完成
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO;                                 // USB或者RXD0有信号时可被唤醒
            PCON |= PD;                                                             // 睡眠
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = 0x00;
        }
        else                                                                        // 唤醒
        {
            LED_RUN = 0;
        }
    }
    else 
    {                                                                               // 意外的中断,不可能发生的情况
        printf("Unknown INT\n");
        USB_INT_FG = 0xFF;                                                          // 清中断标志
    }
    LED_TMP = 1;
}

void    InitUSB_Device( void )                                                      // 初始化USB设备
{
    IE_USB = 0;
    LED_CFG = 1;
    LED_RUN = 0;
    USB_CTRL = 0x00;                                                                // 先设定模式
    UEP4_1_MOD = bUEP1_TX_EN;                                                       // 端点1上传IN
    UEP2_3_MOD = bUEP2_RX_EN | bUEP2_TX_EN;                                         // 端点2下传OUT和上传IN
    UEP0_DMA = Ep0Buffer;
    UEP1_DMA = Ep1Buffer;
    UEP2_DMA = Ep2Buffer;
    USB_DEV_AD = 0x00;
    UDEV_CTRL = bUD_DP_PD_DIS | bUD_DM_PD_DIS;                                      // 禁止DP/DM下拉电阻
    USB_CTRL = bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;                           // 启动USB设备及DMA，在中断期间中断标志未清除前自动返回NAK
    UDEV_CTRL |= bUD_PORT_EN;                                                       // 允许USB端口
    USB_INT_FG = 0xFF;                                                              // 清中断标志
    USB_INT_EN = bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;
    IE_USB = 1;
}

main( )
{
    UINT8   i;
    mDelaymS(30);                                                                   //上电延时,等待内部晶振稳定,必加
    mInitSTDIO( );                                                                  /* 为了让计算机通过串口监控演示过程 */
    printf( "Start @ChipID=%02X\n", (UINT16)CHIP_ID );
    InitUSB_Device( );
    EA = 1;
    while ( 1 )
    {
        i = getkey( );
        printf( "%c", (UINT8)i );
        if ( i >= '0' && i <= 'z' )
        {
            while(( UEP1_CTRL & MASK_UEP_T_RES ) == UEP_T_RES_ACK);                 //等待上传完成，避免同时读写缓冲区					
            memcpy( Ep1Buffer, (PUINT8C)(i-'0'), MAX_PACKET_SIZE );                 /* 加载上传数据 */
            UEP1_T_LEN = i-'0' > 8 ? 8 : i-'0';
            UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;
        }
    }
}