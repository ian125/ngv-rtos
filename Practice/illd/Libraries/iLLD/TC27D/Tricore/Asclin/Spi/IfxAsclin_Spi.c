/**
 * \file IfxAsclin_Spi.c
 * \brief ASCLIN SPI details
 *
 * \version iLLD_1_0_1_12_0
 * \copyright Copyright (c) 2019 Infineon Technologies AG. All rights reserved.
 *
 *
 *
 *                                 IMPORTANT NOTICE
 *
 * Use of this file is subject to the terms of use agreed between (i) you or
 * the company in which ordinary course of business you are acting and (ii)
 * Infineon Technologies AG or its licensees. If and as long as no such terms
 * of use are agreed, use of this file is subject to following:
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer, must
 * be included in all copies of the Software, in whole or in part, and all
 * derivative works of the Software, unless such copies or derivative works are
 * solely in the form of machine-executable object code generated by a source
 * language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 *
 */

/******************************************************************************/
/*----------------------------------Includes----------------------------------*/
/******************************************************************************/

#include "IfxAsclin_Spi.h"

/******************************************************************************/
/*-----------------------Private Function Prototypes--------------------------*/
/******************************************************************************/

/**
 * \param asclin module handle
 * \return status of the on going job
 */
IFX_STATIC IfxAsclin_Spi_Status IfxAsclin_Spi_lock(IfxAsclin_Spi *asclin);

/**
 * \param asclin module handle
 * \return None
 */
IFX_STATIC void IfxAsclin_Spi_unlock(IfxAsclin_Spi *asclin);

/******************************************************************************/
/*-------------------------Function Implementations---------------------------*/
/******************************************************************************/

void IfxAsclin_Spi_disableModule(IfxAsclin_Spi *asclin)
{
    Ifx_ASCLIN *asclinSFR = asclin->asclin;       /* getting the pointer to ASCLIN registers from module handler */
    uint16      psw       = IfxScuWdt_getCpuWatchdogPassword();
    IfxScuWdt_clearCpuEndinit(psw);               /* clearing the endinit protection*/
    IfxAsclin_setDisableModuleRequest(asclinSFR); /* disabling the module */
    IfxScuWdt_setCpuEndinit(psw);                 /* setting the endinit protection back on */
}


IfxAsclin_Spi_Status IfxAsclin_Spi_exchange(IfxAsclin_Spi *asclin, void *src, void *dest, uint32 count)
{
    IfxAsclin_Spi_Status status = IfxAsclin_Spi_lock(asclin);   /* lock the driver until the communication is done */

    if (status == IfxAsclin_Spi_Status_ok)
    {
        asclin->transferInProgress = 1;     /* setting transfer in progress status */
        asclin->txJob.data         = src;   /* data to be transmitted */
        asclin->txJob.pending      = count; /* count of Tx data */
        asclin->rxJob.data         = dest;  /* empty buffer to receive data */
        asclin->rxJob.pending      = count; /* count of Rx data */

        IfxAsclin_Spi_write(asclin);        /* write data into Tx fifo */
    }

    return status;
}


IfxAsclin_Spi_Status IfxAsclin_Spi_getStatus(IfxAsclin_Spi *asclin)
{
    IfxAsclin_Spi_Status status = IfxAsclin_Spi_Status_ok;

    if ((asclin->transferInProgress != 0) || (asclin->sending != 0))
    {
        status = IfxAsclin_Spi_Status_busy;
    }

    return status;
}


IfxAsclin_Status IfxAsclin_Spi_initModule(IfxAsclin_Spi *asclin, const IfxAsclin_Spi_Config *config)
{
    Ifx_ASCLIN      *asclinSFR = config->asclin;                                                                                  /* pointer to ASCLIN registers */
    IfxAsclin_Status status    = IfxAsclin_Status_noError;

    asclin->asclin = asclinSFR;                                                                                                   /* adding register pointer to module handler*/
    IfxAsclin_enableModule(asclinSFR);                                                                                            /* enabling the module */

    /* mode initialisation */
    IfxAsclin_setClockSource(asclinSFR, IfxAsclin_ClockSource_noClock);                                                           /* disabling the clock */
    IfxAsclin_setFrameMode(asclinSFR, IfxAsclin_FrameMode_initialise);                                                            /* setting the module in Initialise mode */
    IfxAsclin_setClockSource(asclinSFR, config->clockSource);                                                                     /* setting the clock source */

    /* spi mode initialisation */
    IfxAsclin_setClockSource(asclinSFR, IfxAsclin_ClockSource_noClock);                                                           /* disabling the clock */
    IfxAsclin_setFrameMode(asclinSFR, config->frameMode);                                                                         /* setting the module in Spi mode */
    IfxAsclin_setPrescaler(asclinSFR, config->baudrate.prescaler);                                                                /* setting the prescaler*/
    IfxAsclin_setClockSource(asclinSFR, config->clockSource);                                                                     /* setting the clock source */

    IfxAsclin_SamplePointPosition samplePointPosition = (IfxAsclin_SamplePointPosition)((config->baudrate.oversampling + 1) / 2); /* sample point position at half of oversampling factor */

    /* baudrate generation */
    status = (IfxAsclin_Status)IfxAsclin_setBitTiming(asclinSFR, config->baudrate.baudrate,
        config->baudrate.oversampling,
        samplePointPosition,
        config->bitSampling.medianFilter);                                      /* setting the baudrate bit fields to generate the required baudrate */

    IfxAsclin_setClockSource(asclinSFR, IfxAsclin_ClockSource_noClock);         /* disabling the clock again */

    /* input output control initialisation */
    IfxAsclin_setRxInput(asclinSFR, config->inputOutput.alti);                  /* selecting the Rx(alternate) input pin */
    IfxAsclin_setClockPolarity(asclinSFR, config->inputOutput.cpol);            /* setting the clock polarity */
    IfxAsclin_setSlavePolarity(asclinSFR, config->inputOutput.spol);            /* setting the slave polarity */
    IfxAsclin_enableLoopBackMode(asclinSFR, config->inputOutput.loopBack);      /* selecting loopbak (enable/disable) */

    /* frame control initialisation */
    IfxAsclin_setIdleDelay(asclinSFR, config->frame.idleDelay);                 /* setting idle delay */
    IfxAsclin_setLeadDelay(asclinSFR, config->frame.leadDelay);                 /* setting lead delay */
    IfxAsclin_setStopBit(asclinSFR, config->frame.stopBit);                     /* setting the stop bit (trail delay) */
    IfxAsclin_setShiftDirection(asclinSFR, config->frame.shiftDir);             /* setting the shift direction */

    /* data control initialisation */
    IfxAsclin_setDataLength(asclinSFR, config->dataLength);                     /* setting the number of bytes to be transfered */

    /* fifo control initialisation */
    /* set the FIFO widths based on the supplied datalength */
    if (config->dataLength <= 8)
    {
        IfxAsclin_setTxFifoInletWidth(asclinSFR, IfxAsclin_TxFifoInletWidth_1);   /* setting Tx FIFO inlet width to 1 byte */
        IfxAsclin_setRxFifoOutletWidth(asclinSFR, IfxAsclin_RxFifoOutletWidth_1); /* setting Rx FIFO outlet width to 1 byte */
        asclin->dataWidth = 1;                                                    /* echo the data width to module handle*/
    }

    else
    {
        IfxAsclin_setTxFifoInletWidth(asclinSFR, IfxAsclin_TxFifoInletWidth_2);      /* setting Tx FIFO inlet width to 2 bytes */
        IfxAsclin_setRxFifoOutletWidth(asclinSFR, IfxAsclin_RxFifoOutletWidth_2);    /* setting Rx FIFO outlet width to 2 bytes */
        asclin->dataWidth = 2;                                                       /* echo the data width to module handle*/
    }

    IfxAsclin_setRxBufferMode(asclinSFR, config->fifo.buffMode);                     /* setting Rx FIFO mode */
    IfxAsclin_setTxFifoInterruptLevel(asclinSFR, config->fifo.txFifoInterruptLevel); /* setting Tx FIFO level at which a Tx interrupt will be triggered */
    IfxAsclin_setRxFifoInterruptLevel(asclinSFR, config->fifo.rxFifoInterruptLevel); /* setting Rx FIFO interrupt level at which a Rx interrupt will be triggered */
    IfxAsclin_flushRxFifo(asclinSFR);                                                /* flushing Rx FIFO */
    IfxAsclin_flushTxFifo(asclinSFR);                                                /* flushing Tx FIFO */

    IfxAsclin_disableAllFlags(asclinSFR);                                            /* disable all flags */
    IfxAsclin_clearAllFlags(asclinSFR);                                              /* clear all flags */

    /* initialising the interrupts */
    IfxSrc_Tos tos = config->interrupt.typeOfService;

    if ((config->interrupt.rxPriority > 0) || (tos == IfxSrc_Tos_dma))
    {
        volatile Ifx_SRC_SRCR *src;
        src = IfxAsclin_getSrcPointerRx(asclinSFR);
        IfxSrc_init(src, tos, config->interrupt.rxPriority);
        IfxAsclin_enableRxFifoFillLevelFlag(asclinSFR, TRUE);
        IfxSrc_enable(src);
    }

    if ((config->interrupt.txPriority > 0) || (tos == IfxSrc_Tos_dma))
    {
        volatile Ifx_SRC_SRCR *src;
        src = IfxAsclin_getSrcPointerTx(asclinSFR);
        IfxSrc_init(src, tos, config->interrupt.txPriority);
        IfxAsclin_enableTxFifoFillLevelFlag(asclinSFR, TRUE);
        IfxSrc_enable(src);
    }

    if (config->interrupt.erPriority > 0) /*These interrupts are not serviced by dma*/
    {
        volatile Ifx_SRC_SRCR *src;
        src = IfxAsclin_getSrcPointerEr(asclinSFR);
        IfxSrc_init(src, tos, config->interrupt.erPriority);
        IfxAsclin_enableFrameErrorFlag(asclinSFR, TRUE);
        IfxSrc_enable(src);
    }

    /* Pin mapping */
    const IfxAsclin_Spi_Pins *pins = config->pins;

    if (pins != NULL_PTR)
    {
        IfxAsclin_Sclk_Out *sclk = pins->sclk;

        if (sclk != NULL_PTR)
        {
            IfxAsclin_initSclkPin(sclk, pins->sclkMode, pins->pinDriver);
        }

        IfxAsclin_Rx_In *rx = pins->rx;

        if (rx != NULL_PTR)
        {
            IfxAsclin_initRxPin(rx, pins->rxMode, pins->pinDriver);
        }

        IfxAsclin_Tx_Out *tx = pins->tx;

        if (tx != NULL_PTR)
        {
            IfxAsclin_initTxPin(tx, pins->txMode, pins->pinDriver);
        }

        IfxAsclin_Slso_Out *slso = pins->slso;

        if (slso != NULL_PTR)
        {
            IfxAsclin_initSlsoPin(slso, pins->slsoMode, pins->pinDriver);
        }
    }

    IfxAsclin_enableParity(asclinSFR, FALSE);                       /* no parity */

    IfxAsclin_setClockSource(asclinSFR, config->clockSource);       /* setting the clock source*/

    asclin->sending = 0;
    IfxAsclin_enableTxFifoOutlet(asclinSFR, TRUE);                  /* disabling Rx FIFO for recieving */
    IfxAsclin_enableRxFifoInlet(asclinSFR, TRUE);                   /* disabling Tx FIFO for transmitting */

    return status;
}


void IfxAsclin_Spi_initModuleConfig(IfxAsclin_Spi_Config *config, Ifx_ASCLIN *asclin)
{
    const IfxAsclin_Spi_Config defaultConfig = {
        .asclin      = NULL_PTR,                          /* will be initialized below */

        .frameMode   = IfxAsclin_FrameMode_spi,           /* SPI mode */
        .clockSource = IfxAsclin_ClockSource_kernelClock, /* kernel clock, fclc*/

        /* Default values for input output control */
        .inputOutput              = {
            .alti     = IfxAsclin_RxInputSelect_0,               /* alternate input 0; */
            .cpol     = IfxAsclin_ClockPolarity_idleLow,         /* CPOL  active low */
            .spol     = IfxAsclin_SlavePolarity_idlehigh,        /* SPOL  active high */
            .loopBack = FALSE,                                   /* no loop back */
        },

        /* Default values for baudrate */
        .baudrate                 = {
            .baudrate     = 100000.0,                       /* default baudrate (the fractional dividier setup will be calculated in initModule) */
            .prescaler    = 2,                              /* default prescaler */
            .oversampling = IfxAsclin_OversamplingFactor_8, /* default oversampling factor */
        },

        /* Default Values for Bit sampling */
        .bitSampling              = {
            .medianFilter         = IfxAsclin_SamplesPerBit_one, /* one sample per bit */
        },

        /* Default Values for Frame Control */
        .frame                    = {
            .idleDelay = IfxAsclin_IdleDelay_0,                /* no idle delay */
            .leadDelay = IfxAsclin_LeadDelay_1,                /* one lead bit */
            .stopBit   = IfxAsclin_StopBit_1,                  /* one stop bit (trail delay) */
            .shiftDir  = IfxAsclin_ShiftDirection_lsbFirst,    /* shift direction LSB first */
        },

        /* Default Values for Data Control*/
        .dataLength               = IfxAsclin_DataLength_8,          /* number of bits per transfer 8*/

        /* Default Values for fifo Control */
        .fifo                     = {
            .outWidth             = IfxAsclin_RxFifoOutletWidth_1,      /* 8-bit wide read */
            .inWidth              = IfxAsclin_TxFifoInletWidth_1,       /*8-bit wide write */
            .buffMode             = IfxAsclin_ReceiveBufferMode_rxFifo, /* RxFIFO */
            .txFifoInterruptLevel = IfxAsclin_TxFifoInterruptLevel_15,  /* Tx FIFO interrupt level */
            .rxFifoInterruptLevel = IfxAsclin_RxFifoInterruptLevel_1,   /* Rx FIFO interrupt level */
        },

        /* Default Values for Interrupt Config */
        .interrupt                = {
            .rxPriority    = 0,                            /* receive interrupt priority 0 */
            .txPriority    = 0,                            /* transmit interrupt priority 0 */
            .erPriority    = 0,                            /* error interrupt priority 0 */
            .typeOfService = IfxSrc_Tos_cpu0,              /* type of service CPU0 */
        },

        .pins                     = NULL_PTR,              /* pins to null pointer */
    };

    /* Default Configuration */
    *config        = defaultConfig;
    /* take over module pointer */
    config->asclin = asclin;
}


void IfxAsclin_Spi_isrError(IfxAsclin_Spi *asclin)
{
    Ifx_ASCLIN *asclinSFR = asclin->asclin; /* getting the pointer to ASCLIN registers from module handler */

    /* store all the flags in the variable */
    if (IfxAsclin_getFrameErrorFlagStatus(asclinSFR))
    {
        IfxAsclin_clearFrameErrorFlag(asclinSFR);
        asclin->errorFlags.frameError = 1;
    }

    if (IfxAsclin_getRxFifoOverflowFlagStatus(asclinSFR))
    {
        IfxAsclin_clearRxFifoOverflowFlag(asclinSFR);
        asclin->errorFlags.rxFifoOverflow = 1;
    }

    if (IfxAsclin_getRxFifoUnderflowFlagStatus(asclinSFR))
    {
        IfxAsclin_clearRxFifoUnderflowFlag(asclinSFR);
        asclin->errorFlags.rxFifoUnderflow = 1;
    }

    if (IfxAsclin_getTxFifoOverflowFlagStatus(asclinSFR))
    {
        IfxAsclin_clearTxFifoOverflowFlag(asclinSFR);
        asclin->errorFlags.txFifoOverflow = 1;
    }
}


void IfxAsclin_Spi_isrReceive(IfxAsclin_Spi *asclin)
{
    Ifx_ASCLIN *asclinSFR = asclin->asclin;        /* getting the pointer to ASCLIN registers from module handler*/
    IfxAsclin_clearRxFifoFillLevelFlag(asclinSFR); /* clear the interrupt flag */
    IfxAsclin_Spi_read(asclin);                    /* read the remaining data */
}


void IfxAsclin_Spi_isrTransmit(IfxAsclin_Spi *asclin)
{
    Ifx_ASCLIN *asclinSFR = asclin->asclin;        /* getting the pointer to ASCLIN registers from module handler */
    IfxAsclin_clearTxFifoFillLevelFlag(asclinSFR); /* clear the interrupt flag */
    IfxAsclin_Spi_write(asclin);                   /* write the remaining data */
}


IFX_STATIC IfxAsclin_Spi_Status IfxAsclin_Spi_lock(IfxAsclin_Spi *asclin)
{
    sint32 sending = __swap((void *)&asclin->sending, 1UL);
    return (sending == 0) ? IfxAsclin_Spi_Status_ok : IfxAsclin_Spi_Status_busy;
}


void IfxAsclin_Spi_read(IfxAsclin_Spi *asclin)
{
    Ifx_ASCLIN        *asclinSFR = asclin->asclin;                                  /* getting the pointer to ASCLIN registers from module handler */
    IfxAsclin_Spi_Job *job       = &asclin->rxJob;                                  /* getting the rxJob structure from module handler */

    uint32             count     = (uint32)IfxAsclin_getRxFifoFillLevel(asclinSFR); /* get the readable count of Rx fifo */
    count = __min(job->pending, count);                                             /* check for the end of the data */

    if (job->data == NULL_PTR)                                                      /* incase of no data to be received (only transmit case) */
    {
        uint32 i;
        job->pending = job->pending - count;                                        /* discount the current reading count from job pending */

        for (i = 0; i < count; i++)
        {
            IfxAsclin_readRxData(asclinSFR);    /* do dummy reads */
        }
    }
    else if (job->pending > 0)
    {
        job->pending = job->pending - count;    /* discount the current reading count from job pending */

        /* read data up to the count based on the out width */
        switch (asclin->dataWidth)
        {
        case 1:                                            /* in case of 8 bit wide */
            IfxAsclin_read8(asclinSFR, job->data, count);  /* reading from Rx FIFO */
            job->data = &(((uint8 *)job->data)[count]);    /* pointing to the remaining data */
            break;

        case 2:                                            /* in case of 16 bit wide*/
            IfxAsclin_read16(asclinSFR, job->data, count); /* reading from Rx FIFO */
            job->data = &(((uint16 *)job->data)[count]);   /* pointing to the remaining data */
            break;
        }
    }

    if (job->pending == 0)
    {
        asclin->transferInProgress = 0; /* clearing the transfer in progress status */
        IfxAsclin_Spi_unlock(asclin);   /* unlock the driver  */
    }
}


IFX_STATIC void IfxAsclin_Spi_unlock(IfxAsclin_Spi *asclin)
{
    asclin->sending = 0UL;
}


void IfxAsclin_Spi_write(IfxAsclin_Spi *asclin)
{
    Ifx_ASCLIN        *asclinSFR = asclin->asclin;    /* getting the pointer to ASCLIN registers from module handler */
    IfxAsclin_Spi_Job *job       = &asclin->txJob;    /* getting the txJob structure from module handler */

    if (job->pending)
    {
        boolean interruptState = IfxCpu_disableInterrupts();

        uint32  count          = (uint32)(16 - IfxAsclin_getTxFifoFillLevel(asclinSFR)); /* getting the fillable count of the Tx Fifo */
        count = __min(job->pending, count);                                              /* checking for the end of the data */

        if (job->data == NULL_PTR)                                                       /* incase of no data to be transmitted (only receive case) */
        {
            uint32 i;
            job->pending = job->pending - count;                                         /* discount the current filling count from job pending */

            for (i = 0; i < count; i++)
            {
                IfxAsclin_writeTxData(asclinSFR, ~0); /* write all 1's */
            }
        }
        else
        {
            job->pending = job->pending - count;                      /* discount the current filling count from job pending */

            /* write data up to the count based on the in width */
            switch (asclin->dataWidth)
            {
            case 1:                                             /* in case of 8 bit wide */
                IfxAsclin_write8(asclinSFR, job->data, count);  /* writing to Tx FIFO */
                job->data = &(((uint8 *)job->data)[count]);     /* pointing to the remaining data */
                break;

            case 2:                                             /* in case of 16 bit wide*/
                IfxAsclin_write16(asclinSFR, job->data, count); /* writing to Tx FIFO */
                job->data = &(((uint16 *)job->data)[count]);    /* pointing to the remaining data */
                break;
            }
        }

        IfxCpu_restoreInterrupts(interruptState);
    }
}
