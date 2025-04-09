#include "bsw.h"

void ErrorHook(StatusType error)
{
    printfSerial("[ErrorHook: error = %d, service = %d, TaskID = %d]", error, OSErrorGetServiceId(), OSError_GetTaskState_TaskID());
}

ISR2(TimerISR)
{
    static long c = -4;
    osEE_tc_stm_set_sr0_next_match(1000000U);
    //conf.oil; TASK SensorTask; AUTOSTART = TRUE;
    // if(c == 0)
    //     ActivateTask(SensorTask);
    IncrementCounter(mycounter);
    printfSerial("\n%4ld: ", c++);
}

ISR2(ButtonISR)
{
    unsigned int a0;
    DisableAllInterrupts();
    osEE_tc_delay(5000);
    a0 = readADCValue(3);
    if (a0 < 500) {
        printfSerial("<BUTTON:T>");
        SetEvent(SensorTask, EventFront);
    } else if (a0 < 1200) {
        printfSerial("<BUTTON:D>");
        ShutdownOS(1);
    } else if (a0 < 1600) {
        printfSerial("<BUTTON:L>");
        SetEvent(SensorTask, EventLeft);
    } else if (a0 < 2200) {
        printfSerial("<BUTTON:R>");
        SetEvent(SensorTask, EventRight);
    } else {
        printfSerial("<BUTTON:?>");
    }
    osEE_tc_delay(3000);
    EnableAllInterrupts();
}

TASK(SensorTask) //Sensor
{
    EventMaskType mask;
    printfSerial("SensorTask Begins...");
    while(1){
        printfSerial("SensorTask Waits...");
        WaitEvent(EventFront | EventLeft | EventRight);
        printfSerial("SensorTask Wakes Up...");
        GetEvent(SensorTask, &mask);
        if (mask & EventFront) {
            ActivateTask(AvoidFrontTask);
            ClearEvent(EventFront);
        }
        if (mask & EventLeft) {
            ActivateTask(AvoidLeftTask);
            ClearEvent(EventLeft);
        }
        if (mask & EventRight) {
            ActivateTask(AvoidRightTask);
            ClearEvent(EventRight);
        }
        printfSerial("SensorTask Finishes...");
    }
    TerminateTask();
}

TASK(AvoidFrontTask)
{
    printfSerial("[AvoidFrontTask]");
    TerminateTask();
}

TASK(AvoidLeftTask)
{
    printfSerial("[AvoidLeftTask]");
    TerminateTask();
}

TASK(AvoidRightTask)
{
    printfSerial("[AvoidRightTask]");
    TerminateTask();
}