#include "bsw.h"
extern App_AsclinAsc g_AsclinAsc;



ISR(asclin0TxISR)
{
    IfxAsclin_Asc_isrTransmit(&g_AsclinAsc.drivers.asc);
}

ISR2(TimerISR)
{
    osEE_tc_stm_set_sr0_next_match( 1000000U );
    static long c = 0;
    printfSerial("\n%4ld: ", c++);
}


TASK(Task1)
{
    printfSerial("Hello World\n");

    mdelay(3000);

    printfSerial("Goodbye World\n");
    TerminateTask();
}
