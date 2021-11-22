/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#include	"project_config.h"


/*_____ D E C L A R A T I O N S ____________________________________________*/

/*_____ D E F I N I T I O N S ______________________________________________*/
volatile uint32_t BitFlag = 0;
volatile uint32_t counter_tick = 0;

//#define MONITOR_TEST_CNT  							(3)
//volatile uint8_t    g_u8MonRxData[(MONITOR_TEST_CNT + 2) * 9] = {0};

volatile uint8_t    g_u8MonRxData[5] = {0};
volatile uint8_t    g_u8MonDataCnt = 0;

volatile enum UI2C_SLAVE_EVENT s_Event;

typedef void (*UI2C_FUNC)(uint32_t u32Status);

volatile static UI2C_FUNC s_UI2C0HandlerFn = NULL;

/*_____ M A C R O S ________________________________________________________*/

#define MONITOR_I2C_PORT							(UI2C0)
#define MONITOR_I2C_SPEED							(100000)
#define MONITOR_I2C_ADDR_7BIT						(0x76)
#define MONITOR_I2C_ADDR_8BIT						(MONITOR_I2C_ADDR_7BIT << 1)

/*_____ F U N C T I O N S __________________________________________________*/

void tick_counter(void)
{
	counter_tick++;
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void compare_buffer(uint8_t *src, uint8_t *des, int nBytes)
{
    uint16_t i = 0;	
	
    for (i = 0; i < nBytes; i++)
    {
        if (src[i] != des[i])
        {
            printf("error idx : %4d : 0x%2X , 0x%2X\r\n", i , src[i],des[i]);
			set_flag(flag_error , ENABLE);
        }
    }

	if (!is_flag_set(flag_error))
	{
    	printf("%s finish \r\n" , __FUNCTION__);	
		set_flag(flag_error , DISABLE);
	}

}

void reset_buffer(void *dest, unsigned int val, unsigned int size)
{
    uint8_t *pu8Dest;
//    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;

	#if 1
	while (size-- > 0)
		*pu8Dest++ = val;
	#else
	memset(pu8Dest, val, size * (sizeof(pu8Dest[0]) ));
	#endif
	
}

void copy_buffer(void *dest, void *src, unsigned int size)
{
    uint8_t *pu8Src, *pu8Dest;
    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;
    pu8Src  = (uint8_t *)src;


	#if 0
	  while (size--)
	    *pu8Dest++ = *pu8Src++;
	#else
    for (i = 0; i < size; i++)
        pu8Dest[i] = pu8Src[i];
	#endif
}

void dump_buffer(uint8_t *pucBuff, int nBytes)
{
    uint16_t i = 0;
    
    printf("dump_buffer : %2d\r\n" , nBytes);    
    for (i = 0 ; i < nBytes ; i++)
    {
        printf("0x%2X," , pucBuff[i]);
        if ((i+1)%8 ==0)
        {
            printf("\r\n");
        }            
    }
    printf("\r\n\r\n");
}

void  dump_buffer_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0)
    {
        printf("0x%04X  ", nIdx);
        for (i = 0; i < 16; i++)
            printf("%02X ", pucBuff[nIdx + i]);
        printf("  ");
        for (i = 0; i < 16; i++)
        {
            if ((pucBuff[nIdx + i] >= 0x20) && (pucBuff[nIdx + i] < 127))
                printf("%c", pucBuff[nIdx + i]);
            else
                printf(".");
            nBytes--;
        }
        nIdx += 16;
        printf("\n");
    }
    printf("\n");
}

void delay(uint16_t dly)
{
/*
	delay(100) : 14.84 us
	delay(200) : 29.37 us
	delay(300) : 43.97 us
	delay(400) : 58.5 us	
	delay(500) : 73.13 us	
	
	delay(1500) : 0.218 ms (218 us)
	delay(2000) : 0.291 ms (291 us)	
*/

	while( dly--);
}


void delay_ms(uint16_t ms)
{
	TIMER_Delay(TIMER0, 1000*ms);
}

void UI2Cx_Monitor_display(void)
{
    uint32_t i;

	#if 1

	if (is_flag_set(flag_I2C_STOP))
	{
		set_flag(flag_I2C_STOP , DISABLE);

		printf("STOP :");
	    for(i = 0; i < (g_u8MonDataCnt); i++)
	    {
			printf("[0x%2X], ", g_u8MonRxData[i]);
	    }

		printf("\r\n");

		g_u8MonDataCnt = 0;
		reset_buffer((void*) g_u8MonRxData , 0x00 , 5);
	}
	else if (is_flag_set(flag_I2C_NACK))
	{
		set_flag(flag_I2C_NACK , DISABLE);

		printf("NACK :");
	    for(i = 0; i < (g_u8MonDataCnt); i++)
	    {
			printf("[0x%2X], ", g_u8MonRxData[i]);
	    }

		printf("\r\n");

		g_u8MonDataCnt = 0;
		reset_buffer((void*) g_u8MonRxData , 0x00 , 5);
	}

	#else
	
    while(g_u8MonDataCnt < (MONITOR_TEST_CNT * 9));

    printf("\r\nMonitor : SLV+W, ADDR_H, ADDR_L, Data, SLV+W, ADDR_H, ADDR_L, SLV+R, Data");
    for(i = 0; i < (MONITOR_TEST_CNT * 9); i++)
    {
        if((i%9) == 0)
            printf("\nUI2C0 Get Data : ");

        printf("[0x%X], ", g_u8MonRxData[i]);
    }
	#endif
}

void USCI01_IRQHandler(void)
{
    uint32_t u32Status;

    //UI2C0 Interrupt
    u32Status = UI2C_GET_PROT_STATUS(MONITOR_I2C_PORT);

    if (s_UI2C0HandlerFn != NULL)
    {
        s_UI2C0HandlerFn(u32Status);
    }
}

void UI2Cx_SLV_Monitor(uint32_t u32Status)
{
    uint8_t u8Rxdata;

    if((u32Status & UI2C_PROTSTS_STARIF_Msk) == UI2C_PROTSTS_STARIF_Msk)
    {
        /* Clear START INT Flag */
        UI2C_CLR_PROT_INT_FLAG(MONITOR_I2C_PORT, UI2C_PROTSTS_STARIF_Msk);

        s_Event = SLAVE_ADDRESS_ACK;
        UI2C_SET_CONTROL_REG(MONITOR_I2C_PORT, (UI2C_CTL_PTRG | UI2C_CTL_AA));
    }
    else if((u32Status & UI2C_PROTSTS_ACKIF_Msk) == UI2C_PROTSTS_ACKIF_Msk)
    {
        /* Clear ACK INT Flag */
        UI2C_CLR_PROT_INT_FLAG(MONITOR_I2C_PORT, UI2C_PROTSTS_ACKIF_Msk);

        if(s_Event == SLAVE_ADDRESS_ACK)
        {
            if((MONITOR_I2C_PORT->PROTSTS & UI2C_PROTSTS_SLAREAD_Msk) == UI2C_PROTSTS_SLAREAD_Msk)
            {
                u8Rxdata = (uint8_t)UI2C_GET_DATA(MONITOR_I2C_PORT);
                g_u8MonRxData[g_u8MonDataCnt++] = u8Rxdata;
            }
            else
            {
                u8Rxdata = (uint8_t)UI2C_GET_DATA(MONITOR_I2C_PORT);;
                g_u8MonRxData[g_u8MonDataCnt++] = u8Rxdata;
            }

            s_Event = SLAVE_GET_DATA;
        }
        else if(s_Event == SLAVE_GET_DATA)
        {
            u8Rxdata = (uint8_t)UI2C_GET_DATA(MONITOR_I2C_PORT);
            g_u8MonRxData[g_u8MonDataCnt++] = u8Rxdata;
        }

        UI2C_SET_CONTROL_REG(MONITOR_I2C_PORT, (UI2C_CTL_PTRG | UI2C_CTL_AA));
    }
    else if((u32Status & UI2C_PROTSTS_NACKIF_Msk) == UI2C_PROTSTS_NACKIF_Msk)
    {
        /* Clear NACK INT Flag */
        UI2C_CLR_PROT_INT_FLAG(MONITOR_I2C_PORT, UI2C_PROTSTS_NACKIF_Msk);

        u8Rxdata = (uint8_t)UI2C_GET_DATA(MONITOR_I2C_PORT);
        g_u8MonRxData[g_u8MonDataCnt++] = u8Rxdata;

		set_flag(flag_I2C_NACK ,ENABLE);		
        UI2C_SET_CONTROL_REG(MONITOR_I2C_PORT, (UI2C_CTL_PTRG | UI2C_CTL_AA));
    }
    else if((u32Status & UI2C_PROTSTS_STORIF_Msk) == UI2C_PROTSTS_STORIF_Msk)
    {
        /* Clear STO INT Flag */
        UI2C_CLR_PROT_INT_FLAG(MONITOR_I2C_PORT, UI2C_PROTSTS_STORIF_Msk);

        s_Event = SLAVE_ADDRESS_ACK;
		set_flag(flag_I2C_STOP ,ENABLE);
        UI2C_SET_CONTROL_REG(MONITOR_I2C_PORT, (UI2C_CTL_PTRG | UI2C_CTL_AA));
    }
}

void UI2Cx_Init(uint32_t u32ClkSpeed)
{
    /* Open USCI_I2C0 and set clock to 100k */
    UI2C_Open(MONITOR_I2C_PORT, u32ClkSpeed);

    /* Get USCI_I2C0 Bus Clock */
    printf("UI2Cx clock %d Hz\r\n", UI2C_GetBusClockFreq(MONITOR_I2C_PORT));

    /* Set USCI_I2C0 Slave Addresses */
//    UI2C_SetSlaveAddr(MONITOR_I2C_PORT, 0, 0x16, UI2C_GCMODE_DISABLE);   /* Slave Address : 0x16 */
    UI2C_SetSlaveAddr(MONITOR_I2C_PORT, 1, MONITOR_I2C_ADDR_7BIT, UI2C_GCMODE_DISABLE);   /* Slave Address : 0x36 */

    /* Set USCI_I2C0 Slave Addresses Mask */
    UI2C_SetSlaveAddrMask(MONITOR_I2C_PORT, 0, 0x04);                    /* Slave Address : 0x4 */
//    UI2C_SetSlaveAddrMask(MONITOR_I2C_PORT, 1, 0x02);                    /* Slave Address : 0x2 */

    /* Enable UI2C0 protocol interrupt */
    UI2C_ENABLE_PROT_INT(MONITOR_I2C_PORT, (UI2C_PROTIEN_ACKIEN_Msk | UI2C_PROTIEN_NACKIEN_Msk | UI2C_PROTIEN_STORIEN_Msk | UI2C_PROTIEN_STARIEN_Msk));
    NVIC_EnableIRQ(USCI01_IRQn);

}

// UI2C0_SDA(PA.10), UI2C0_SCL(PA.11)
void UI2Cx_Monitor_Init(void)
{

	set_flag(flag_I2C_STOP ,DISABLE);

    /* Init USCI_I2C0 */
    UI2Cx_Init(MONITOR_I2C_SPEED);

    s_Event = SLAVE_ADDRESS_ACK;

    MONITOR_I2C_PORT->PROTCTL |= (UI2C_PROTCTL_MONEN_Msk | UI2C_PROTCTL_SCLOUTEN_Msk);

    UI2C_SET_CONTROL_REG(MONITOR_I2C_PORT, UI2C_CTL_AA);

    /* UI2C function to Slave receive/transmit data */
    s_UI2C0HandlerFn = UI2Cx_SLV_Monitor;

    printf("UI2Cx Monitor Mode is Running.\n\n");
}


void GPIO_Init (void)
{
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB14MFP_Msk)) | (SYS_GPB_MFPH_PB14MFP_GPIO);
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB15MFP_Msk)) | (SYS_GPB_MFPH_PB15MFP_GPIO);
	
    GPIO_SetMode(PB, BIT14, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PB, BIT15, GPIO_MODE_OUTPUT);	
}


void TMR1_IRQHandler(void)
{
//	static uint32_t LOG = 0;

	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{
//        	printf("%s : %4d\r\n",__FUNCTION__,LOG++);
			PB14 ^= 1;
		}

		if ((get_tick() % 50) == 0)
		{

		}	
    }
}


void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void UARTx_Process(void)
{
	uint8_t res = 0;
	res = UART_READ(UART0);

	if (res == 'x' || res == 'X')
	{
		NVIC_SystemReset();
	}

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
		switch(res)
		{
			case '1':
				break;

			case 'X':
			case 'x':
			case 'Z':
			case 'z':
				NVIC_SystemReset();		
				break;
		}
	}
}

void UART02_IRQHandler(void)
{

    if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
            UARTx_Process();
        }
    }

    if(UART0->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
    {
        UART_ClearIntFlag(UART0, (UART_INTSTS_RLSINT_Msk| UART_INTSTS_BUFERRINT_Msk));
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);
    UART_EnableInt(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_RXTOIEN_Msk);
    NVIC_EnableIRQ(UART02_IRQn);
	
	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());	
	#endif	

}

void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);	

//    CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);	

    /* Select HCLK clock source as HIRC and HCLK source divider as 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    CLK_EnableModuleClock(TMR1_MODULE);
  	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);

    /* Enable UI2C0 clock */
    CLK_EnableModuleClock(USCI0_MODULE);

    /* Set PB multi-function pins for UART0 RXD=PB.12 and TXD=PB.13 */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk)) |
                    (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);


    SYS->GPA_MFPH = (SYS->GPA_MFPH & ~(SYS_GPA_MFPH_PA11MFP_Msk | SYS_GPA_MFPH_PA10MFP_Msk)) |
                    (SYS_GPA_MFPH_PA11MFP_USCI0_CLK | SYS_GPA_MFPH_PA10MFP_USCI0_DAT0);

   /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();
}

/*
 * This is a template project for M031 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{
    SYS_Init();

	UART0_Init();
	GPIO_Init();
	TIMER1_Init();

    UI2Cx_Monitor_Init();

    /* Got no where to go, just loop forever */
    while(1)
    {
		UI2Cx_Monitor_display();

    }
}

/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
