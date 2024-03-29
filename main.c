/*******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the XMC MCU: WDT FEED Example for
*              ModusToolbox.
*              The WDT needs to be fed every second for the proper serving of the
*              WDT. The SysTick timer is used to feed the WDT. When feeding
*              occurs, the User LED blinks. If there is no feeding, the device
*              goes into reset. After the reset, the MCU checks the reason for
*              the last reset. If it is due to a failure to feed the WDT,
*              the User LED blinks at a faster rate.
*
* Related Document: See README.md
*
********************************************************************************
*
* Copyright (c) 2015-2022, Infineon Technologies AG
* All rights reserved.
*
* Boost Software License - Version 1.0 - August 17th, 2003

* Permission is hereby granted, free of charge, to any person or organization
* obtaining a copy of the software and accompanying documentation covered by
* this license (the "Software") to use, reproduce, display, distribute,
* execute, and transmit the Software, and to prepare derivative works of the
* Software, and to permit third-parties to whom the Software is furnished to
* do so, all subject to the following:
*
* The copyright notices in the Software and this entire statement, including
* the above license grant, this restriction and the following disclaimer,
* must be included in all copies of the Software, in whole or in part, and
* all derivative works of the Software, unless such copies or derivative
* works are solely in the form of machine-executable object code generated by
* a source language processor.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
* SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
* FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*
********************************************************************************/
#include <stdio.h>
#include "cybsp.h"
#include "cy_utils.h"
#include "cy_retarget_io.h"

/*******************************************************************************
* Defines
*******************************************************************************/
#if (UC_SERIES == XMC11) || (UC_SERIES == XMC12) || (UC_SERIES == XMC13)
#define COUNTS_DELAY                        (200000U)
#endif

#if (UC_SERIES == XMC14)
#define COUNTS_DELAY                        (500000U)
#endif

#if (UC_SERIES == XMC48) || (UC_SERIES == XMC47) || (UC_SERIES == XMC45) || (UC_SERIES == XMC44) || (UC_SERIES == XMC43) || (UC_SERIES == XMC42)
#define COUNTS_DELAY                        (2000000U)
#endif

#define TICKS_PER_SECOND                    (1000U)
#define TICKS_WAIT                          (1000U)
#define MAX_NUM_FEEDS                       (10U)

/* Define macro to enable/disable printing of debug messages */
#define ENABLE_XMC_DEBUG_PRINT              (0)

/* Define macro to set the loop count before printing debug messages */
#if ENABLE_XMC_DEBUG_PRINT
#define DEBUG_LOOP_COUNT_MAX                (1U)
#endif

/*******************************************************************************
* Defines
*******************************************************************************/
static volatile bool interrupt_handler_flag = false;
/*******************************************************************************
* Function Name: SysTick Handler
********************************************************************************
* Summary:
* This is the interrupt handler function for the System Tick interrupt.
*
* Parameters:
*  none
*
* Return:
*  void
*
*******************************************************************************/
void SysTick_Handler(void)
{
    /* Ticks wait */
    static uint32_t ticks = 0;
    /* Number of feeds */
    static uint32_t feeds = 0;

    ticks++;
    /* Watchdog is fed 10 times inside system handler ISR */
    if (ticks == TICKS_WAIT && feeds < MAX_NUM_FEEDS)
    {
        interrupt_handler_flag = true;
        /* User LED blinks 5 times */
        XMC_GPIO_ToggleOutput(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN);
        /* Service watchdog when the count value of watchdog timer is between lower and upper window bounds */
        XMC_WDT_Service();
        ticks = 0;
        feeds++;
    }
}

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function. It starts WDT. It set up a timer to trigger a
* periodic interrupt.
*
* Parameters:
*  none
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    #if ENABLE_XMC_DEBUG_PRINT
    /* Assign false to disable printing of debug messages*/
    static volatile bool debug_printf = true;
    /* Initialize the current loop count to zero */
    static uint32_t debug_loop_count = 0;
    #endif

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initialize retarget-io to use the debug UART port */
    cy_retarget_io_init(CYBSP_DEBUG_UART_HW);

    #if ENABLE_XMC_DEBUG_PRINT
    printf("Initialization done\r\n");
    #endif

    /* Check the reason for reset */
    if ((XMC_SCU_RESET_GetDeviceResetReason() & XMC_SCU_RESET_REASON_WATCHDOG) != 0)
    {
        /* Clear system reset status */
        XMC_SCU_RESET_ClearDeviceResetReason();
        while(1)
        {
            /* Toggle User LED faster due to watchdog reset */
            XMC_GPIO_ToggleOutput(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN);
            for (int i= 0; i < COUNTS_DELAY; ++i)
            {
                __NOP();
            }

            #if ENABLE_XMC_DEBUG_PRINT
            debug_loop_count++;

            if (debug_printf && debug_loop_count == DEBUG_LOOP_COUNT_MAX)
            {
                debug_printf = false;
                /* Print message after the loop has run DEBUG_LOOP_COUNT_MAX times */
                printf("LED toggle due to watchdog reset\r\n");
            }
            #endif
        }
    }

    /* Clear system reset status */
    XMC_SCU_RESET_ClearDeviceResetReason();

    #if (UC_SERIES == XMC48) || (UC_SERIES == XMC47) || (UC_SERIES == XMC45) || (UC_SERIES == XMC44) || (UC_SERIES == XMC43) || (UC_SERIES == XMC42)
    /* Use standby clock as watchdog clock source */
    XMC_SCU_HIB_EnableHibernateDomain();
    XMC_SCU_CLOCK_SetWdtClockSource(XMC_SCU_CLOCK_WDTCLKSRC_STDBY);
    XMC_SCU_CLOCK_EnableClock(XMC_SCU_CLOCK_WDT);
    #endif

    /* Start the watchdog timer */
    XMC_WDT_Start();

    /* Feed the watchdog periodically every 1s */
    SysTick_Config(SystemCoreClock / TICKS_PER_SECOND);

    while (1)
    {
        /* Infinite loop */

        #if ENABLE_XMC_DEBUG_PRINT
        if(interrupt_handler_flag && debug_printf)
        {
            debug_printf = false;
            interrupt_handler_flag = false;
            /* Print message after the loop has run DEBUG_LOOP_COUNT_MAX times */
            printf("LED toggle when the count value of watchdog timer is between lower and upper window bounds\r\n");
        }
        #endif
    }
}

/* [] END OF FILE */
