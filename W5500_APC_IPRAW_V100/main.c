#include <stdio.h>
#include <string.h>
#include "xhw_types.h"
#include "xhw_memmap.h"
#include "xhw_spi.h"
#include "xsysctl.h"
#include "xgpio.h"
#include "xuart.h"
#include "xspi.h"
#include "xcore.h"


#include "socket.h"	// Just include one header for WIZCHIP


///////////////////////////////////
// STM32F103X SPI PIN Definition //
///////////////////////////////////
#define WIZCHIP_SPI_PERIPH    xSYSCTL_PERIPH_SPI1
#define WIZCHIP_SPI_BASE      xSPI1_BASE
#define WIZCHIP_SPI_CLK       SPI1CLK(3)
#define WIZCHIP_SPI_MOSI      SPI1MOSI(3)
#define WIZCHIP_SPI_MISO      SPI1MISO(1)
#define WIZCHIP_SPI_CLK_PIN   PA5
#define WIZCHIP_SPI_MOSI_PIN  PA7
#define WIZCHIP_SPI_MISO_PIN  PA6

/////////////////////////////////////////
// SOCKET NUMBER DEFINION for Examples //
/////////////////////////////////////////
#define SOCK_TCPS        0
#define SOCK_UDPS        1

#define SUCCESS		     3

////////////////////////////////////////////////
// Shared Buffer Definition for LOOPBACK TEST //
////////////////////////////////////////////////
#define DATA_BUF_SIZE   2048
uint8_t gDATABUF[DATA_BUF_SIZE];

///////////////////////////////////
// Default Network Configuration //
///////////////////////////////////
wiz_NetInfo gWIZNETINFO = { .mac = {0x00, 0x08, 0xdc,0x00, 0xab, 0xcd},
                            .ip = {192, 168, 0, 226},
                            .sn = {255,255,255,0},
                            .gw = {192, 168, 0, 1},
                            .dns = {0,0,0,0},
                            .dhcp = NETINFO_STATIC };


// initialize the dependent host peripheral
void platform_init(void);


//////////
// TODO //
//////////////////////////////////////////////////////////////////////////////////////////////
// Call back function for W5500 SPI - Theses used as parameter of reg_wizchip_xxx_cbfunc()  //
// Should be implemented by WIZCHIP users because host is dependent                         //
//////////////////////////////////////////////////////////////////////////////////////////////
void  wizchip_select(void);
void  wizchip_deselect(void);
void  wizchip_write(uint8_t wb);
uint8_t wizchip_read();
//////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////
// For example of ioLibrary_BSD //
//////////////////////////////////
void network_init(void);								// Initialize Network information and display it
int32_t loopback_tcps(uint8_t, uint8_t*, uint16_t);		// Loopback TCP server
int32_t loopback_udps(uint8_t, uint8_t*, uint16_t);		// Loopback UDP server
//////////////////////////////////


int main(void)
{
   uint8_t tmp=0;
   int32_t ret = 0;
   uint8_t memsize[2][8] = {{2,2,2,2,2,2,2,2},{2,2,2,2,2,2,2,2}};
   uint8_t txsize[8] = {2,2,2,2,2,2,2,2};
   uint8_t rxsize[8] = {2,2,2,2,2,2,2,2};
   uint8_t pDestaddr[4] = {192,168,0,200};
   int8_t num;

   uint8_t ping_reply_received = 0;

   ///////////////////////////////////////////
   // Host dependent peripheral initialized //
   ///////////////////////////////////////////
   platform_init();


   //////////
   // TODO //
   ////////////////////////////////////////////////////////////////////////////////////////////////////
   // First of all, Should register SPI callback functions implemented by user for accessing WIZCHIP //
   ////////////////////////////////////////////////////////////////////////////////////////////////////
   /* Critical section callback - No use in this example */
   //reg_wizchip_cris_cbfunc(0, 0);
   /* Chip selection call back */
#if   _WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_SPI_VDM_
    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
#elif _WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_SPI_FDM_
    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_select);  // CS must be tried with LOW.
#else
   #if (_WIZCHIP_IO_MODE_ & _WIZCHIP_IO_MODE_SIP_) != _WIZCHIP_IO_MODE_SIP_
      #error "Unknown _WIZCHIP_IO_MODE_"
   #else
      reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
   #endif
#endif
    /* SPI Read & Write callback function */
    reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);
    ////////////////////////////////////////////////////////////////////////

    /* WIZCHIP SOCKET Buffer initialize */

    wizchip_init(txsize,rxsize);

    /* Network initialization */
    network_init();


	/*******************************/
	/* WIZnet W5500 Code Examples  */
	/* TCPS/UDPS IPRAW test     */
	/*******************************/
    printf("------------PING_TEST_START-----------------------\r\n");
    tmp = ping_auto(0,pDestaddr);
    //tmp = ping_count(0,3,pDestaddr);
    if(tmp == SUCCESS)
    	printf("-----------PING TEST OK----------\r\n");
    else
        printf("----------ERROR  = %d----------\r\n",tmp);

} // end of main()


//////////
// TODO //
/////////////////////////////////////////////////////////////////
// SPI Callback function for accessing WIZCHIP                 //
// WIZCHIP user should implement with your host spi peripheral //
/////////////////////////////////////////////////////////////////
void  wizchip_select(void)
{
   xGPIOPinWrite( xGPIO_PORTA_BASE, xGPIO_PIN_4, 0);
}

void  wizchip_deselect(void)
{
   xGPIOPinWrite( xGPIO_PORTA_BASE, xGPIO_PIN_4, 1);
}

void  wizchip_write(uint8_t wb)
{
   xSPISingleDataReadWrite(WIZCHIP_SPI_BASE,wb);
}

uint8_t wizchip_read()
{
   return xSPISingleDataReadWrite(WIZCHIP_SPI_BASE,0xFF);
}
//////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////
// Intialize the network information to be used in WIZCHIP //
/////////////////////////////////////////////////////////////
void network_init(void)
{
   uint8_t tmpstr[6];
	ctlnetwork(CN_SET_NETINFO, (void*)&gWIZNETINFO);
	ctlnetwork(CN_GET_NETINFO, (void*)&gWIZNETINFO);

	// Display Network Information
	ctlwizchip(CW_GET_ID,(void*)tmpstr);
	printf("\r\n=== %s NET CONF ===\r\n",(char*)tmpstr);
	printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",gWIZNETINFO.mac[0],gWIZNETINFO.mac[1],gWIZNETINFO.mac[2],
		  gWIZNETINFO.mac[3],gWIZNETINFO.mac[4],gWIZNETINFO.mac[5]);
	printf("SIP: %d.%d.%d.%d\r\n", gWIZNETINFO.ip[0],gWIZNETINFO.ip[1],gWIZNETINFO.ip[2],gWIZNETINFO.ip[3]);
	printf("GAR: %d.%d.%d.%d\r\n", gWIZNETINFO.gw[0],gWIZNETINFO.gw[1],gWIZNETINFO.gw[2],gWIZNETINFO.gw[3]);
	printf("SUB: %d.%d.%d.%d\r\n", gWIZNETINFO.sn[0],gWIZNETINFO.sn[1],gWIZNETINFO.sn[2],gWIZNETINFO.sn[3]);
	printf("DNS: %d.%d.%d.%d\r\n", gWIZNETINFO.dns[0],gWIZNETINFO.dns[1],gWIZNETINFO.dns[2],gWIZNETINFO.dns[3]);
	printf("======================\r\n");
}
/////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Platform (STM32F103X) initialization for peripherals as GPIO, SPI, UARTs //
//////////////////////////////////////////////////////////////////////////////
void platform_init(void)
{
    xSysCtlClockSet(72000000,  xSYSCTL_OSC_MAIN | xSYSCTL_XTAL_8MHZ);

    xSysCtlDelay((xSysCtlClockGet()/1000)*50); // wait 50ms


    /**************************/
    /* GPIO_A For SPI CS PIN  */// It must be first to enable GPIO peripheral than other peripheral. (Otherwise, UART do not run in STM32F103X)
    /**************************/
    xSysCtlPeripheralEnable( xSYSCTL_PERIPH_GPIOA );
    xGPIODirModeSet(xGPIO_PORTA_BASE, xGPIO_PIN_4, xGPIO_DIR_MODE_OUT);


    /************/
    /* For UART */
    /************/
    xSysCtlPeripheralReset(xSYSCTL_PERIPH_UART1);
    xSysCtlPeripheralEnable(xSYSCTL_PERIPH_UART1);
    xSPinTypeUART(UART1TX,PA9);
    xSPinTypeUART(UART1RX,PA10);
    xUARTConfigSet(xUART1_BASE, 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    xUARTEnable(xUART1_BASE, (UART_BLOCK_UART | UART_BLOCK_TX | UART_BLOCK_RX));


    /***********/
    /* For SPI */
    /***********/
    xSysCtlPeripheralReset(WIZCHIP_SPI_PERIPH);
    xSysCtlPeripheralEnable(WIZCHIP_SPI_PERIPH);
    xSPinTypeSPI(WIZCHIP_SPI_CLK, WIZCHIP_SPI_CLK_PIN);  // xGPIODirModeSet(xGPIO_PORTA, xGPIO_PIN_5, GPIO_TYPE_AFOUT_STD, GPIO_OUT_SPEED_50M);
    xSPinTypeSPI(WIZCHIP_SPI_MOSI,WIZCHIP_SPI_MOSI_PIN);  // xGPIODirModeSet(xGPIO_PORTA, xGPIO_PIN_7, GPIO_TYPE_AFOUT_STD, GPIO_OUT_SPEED_50M);
    xSPinTypeSPI(WIZCHIP_SPI_MISO,WIZCHIP_SPI_MISO_PIN);  // xGPIODirModeSet(xGPIO_PORTA, xGPIO_PIN_6, GPIO_TYPE_IN_FLOATING, GPIO_IN_SPEED_FIXED);
    xSPIConfigSet(WIZCHIP_SPI_BASE, xSysCtlClockGet()/2, xSPI_MOTO_FORMAT_MODE_0 | xSPI_MODE_MASTER | xSPI_MSB_FIRST | xSPI_DATA_WIDTH8);

    xSPISSSet(WIZCHIP_SPI_BASE, SPI_SS_SOFTWARE, xSPI_SS_NONE);
    xSPIEnable(WIZCHIP_SPI_BASE);

    printf("HCLK = %dMHz\r\n", (unsigned int)(xSysCtlClockGet()/1000000));
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
