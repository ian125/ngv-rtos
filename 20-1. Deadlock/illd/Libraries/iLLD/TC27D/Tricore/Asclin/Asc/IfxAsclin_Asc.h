/**
 * \file IfxAsclin_Asc.h
 * \brief ASCLIN ASC details
 * \ingroup IfxLld_Asclin
 *
 * \version iLLD_1_0_1_12_0
 * \copyright Copyright (c) 2020 Infineon Technologies AG. All rights reserved.
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
 * \defgroup IfxLld_Asclin_Asc_Usage How to use the ASCLIN ASC Interface driver?
 * \ingroup IfxLld_Asclin
 *
 * The ASC interface driver provides a default ASCLIN configuration for asynchronous serial communication in 8bit mode, and a set of data transfer routines.
 *
 * Data transfers are buffered by the hardware based FIFOs, and in addition by software based FIFOs with a configurable size. Incoming and outgoing data is transfered in background from/to the ASCLIN peripheral by interrupt service handlers, which are part of this driver as well. This allows a nonblocking communication without stalling the thread(s) from where data is sent and received.
 *
 * In the following sections it will be described, how to integrate the driver into the application framework.
 *
 * \section IfxLld_Asclin_Asc_Preparation Preparation
 * \subsection IfxLld_Asclin_Asc_Include Include Files
 *
 * Include following header file into your C code:
 * \code
 * #include <Asclin/Asc/IfxAsclin_Asc.h>
 * \endcode
 *
 * \subsection IfxLld_Asclin_Asc_Variables Variables
 *
 * Declare the ASC handle and the FIFOs as global variables in your C code:
 *
 * \code
 * // used globally
 * static IfxAsclin_Asc asc;
 *
 * #define ASC_TX_BUFFER_SIZE 64
 * static uint8 ascTxBuffer[ASC_TX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8];
 *
 * #define ASC_RX_BUFFER_SIZE 64
 * static uint8 ascRxBuffer[ASC_RX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8];
 * \endcode
 *
 * As you can see above, the transfer buffers allocate not only memory for the data itself, but also for FIFO runtime variables. 8 bytes have to be added to ensure a proper circular buffer handling independent from the address to which the buffers have been located.
 *
 * \subsection IfxLld_Asclin_Asc_Interrupt Interrupt Handler Installation
 *
 * See also \ref IfxLld_Cpu_Irq_Usage
 *
 * Define priorities for the Interrrupt handlers. This is normally done in the Ifx_IntPrioDef.h file:
 * \code
 * // priorities are normally defined in Ifx_IntPrioDef.h
 * #define IFX_INTPRIO_ASCLIN0_TX  1
 * #define IFX_INTPRIO_ASCLIN0_RX  2
 * #define IFX_INTPRIO_ASCLIN0_ER  3
 * \endcode
 *
 * Add the interrupt service routines to your C code. They have to call the ASC interrupt handlers by passing the asc handle:
 * \code
 * IFX_INTERRUPT(asclin0TxISR, 0, IFX_INTPRIO_ASCLIN0_TX)
 * {
 *     IfxAsclin_Asc_isrTransmit(&asc);
 * }
 *
 * IFX_INTERRUPT(asclin0RxISR, 0, IFX_INTPRIO_ASCLIN0_RX)
 * {
 *     IfxAsclin_Asc_isrReceive(&asc);
 * }
 *
 * IFX_INTERRUPT(asclin0ErISR, 0, IFX_INTPRIO_ASCLIN0_ER)
 * {
 *     IfxAsclin_Asc_isrError(&asc);
 * }
 * \endcode
 *
 * Finally install the interrupt handlers in your initialisation function:
 * \code
 *     // install interrupt handlers
 *     IfxCpu_Irq_installInterruptHandler(&asclin0TxISR, IFX_INTPRIO_ASCLIN0_TX);
 *     IfxCpu_Irq_installInterruptHandler(&asclin0RxISR, IFX_INTPRIO_ASCLIN0_RX);
 *     IfxCpu_Irq_installInterruptHandler(&asclin0ErISR, IFX_INTPRIO_ASCLIN0_ER);
 *     IfxCpu_enableInterrupts();
 * \endcode
 *
 * \subsection IfxLld_Asclin_Asc_Init Module Initialisation
 *
 * The module initialisation can be done in the same function. Here an example:
 * \code
 *     // create module config
 *     IfxAsclin_Asc_Config ascConfig;
 *     IfxAsclin_Asc_initModuleConfig(&ascConfig, &MODULE_ASCLIN0);
 *
 *     // set the desired baudrate
 *     ascConfig.baudrate.prescaler = 1;
 *     ascConfig.baudrate.baudrate = 1000000; // FDR values will be calculated in initModule
 *
 *     // ISR priorities and interrupt target
 *     ascConfig.interrupt.txPriority = IFX_INTPRIO_ASCLIN0_TX;
 *     ascConfig.interrupt.rxPriority = IFX_INTPRIO_ASCLIN0_RX;
 *     ascConfig.interrupt.erPriority = IFX_INTPRIO_ASCLIN0_ER;
 *     ascConfig.interrupt.typeOfService =   IfxCpu_Irq_getTos(IfxCpu_getCoreIndex());
 *
 *     // FIFO configuration
 *     ascConfig.txBuffer = &ascTxBuffer;
 *     ascConfig.txBufferSize = ASC_TX_BUFFER_SIZE;
 *
 *     ascConfig.rxBuffer = &ascRxBuffer;
 *     ascConfig.rxBufferSize = ASC_RX_BUFFER_SIZE;
 *
 *     // pin configuration
 *     const IfxAsclin_Asc_Pins pins = {
 *         NULL,                           IfxPort_InputMode_pullUp,    // CTS pin not used
 *         &IfxAsclin0_RXA_P14_1_IN,   IfxPort_InputMode_pullUp,    // Rx pin
 *         NULL,                           IfxPort_OutputMode_pushPull, // RTS pin not used
 *         &IfxAsclin0_TX_P14_0_OUT,   IfxPort_OutputMode_pushPull, // Tx pin
 *         IfxPort_PadDriver_cmosAutomotiveSpeed1
 *     };
 *     ascConfig.pins = &pins;
 *
 *     // initialize module
 *     //IfxAsclin_Asc asc; // defined globally
 *     IfxAsclin_Asc_initModule(&asc, &ascConfig);
 * \endcode
 *
 * The ASC is ready for use now!
 *
 *
 * \section IfxLld_Asclin_Asc_DataTransfers Data Transfers
 *
 * \subsection IfxLld_Asclin_Asc_DataSimple Simple Transfers
 *
 * The ASC driver provides simple to use transfer functions, which are blocking.
 *
 * This means: you can send as much data as you want without taking care for the fill state of the FIFO. If the FIFO is full, the blocking function will wait until the next byte has been transfered to ASCLIN before putting the new byte into the FIFO:
 * \code
 *     // send 3 bytes
 *     IfxAsclin_Asc_blockingWrite(&asc, 0x01);
 *     IfxAsclin_Asc_blockingWrite(&asc, 0x02);
 *     IfxAsclin_Asc_blockingWrite(&asc, 0x03);
 * \endcode
 *
 * A simple to use receive function is available as well. If no data is in the receive FIFO, it will wait until the next byte has been received:
 * \code
 *     // receive a byte
 *     uint8 data = IfxAsclin_Asc_blockingRead(&asc);
 * \endcode
 *
 *
 * \subsection IfxLld_Asclin_Asc_DataStream Streamed Transfers
 *
 * Streamed transfers are handled faster by the ASC driver and therefore they are recommended whenever a large bulk of data should be sent. Here an example:
 * \code
 *     uint8     txData[9] = { 0x49, 0x6e, 0x66, 0x69, 0x6e, 0x65, 0x6f, 0x6e, 0x0a };
 *     {
 *         Ifx_SizeT count = 9;
 *         IfxAsclin_Asc_write(&asc, txData, &count, TIME_INFINITE);
 *     }
 * \endcode
 *
 *
 * Data can be received the following way:
 * \code
 *     uint8 rxData[5];
 *
 *     {
 *         // wait until 5 bytes have been received
 *         Ifx_SizeT count = 5;
 *         IfxAsclin_Asc_read(&asc, rxData, &count, TIME_INFINITE);
 *     }
 * \endcode
 *
 * Or alternatively with:
 * \code
 *     uint8 rxData[5];
 *
 *     {
 *         // how many bytes have been received?
 *         Ifx_SizeT count = IfxAsclin_Asc_getReadCount(&asc);
 *
 *         // limit to our buffer size
 *         count = count < 5 ? count : 5;
 *
 *         // transfer received data into buffer
 *         IfxAsclin_Asc_read(&asc, rxData, &count, TIME_INFINITE);
 *     }
 * \endcode
 *
 * \defgroup IfxLld_Asclin_Asc ASC
 * \ingroup IfxLld_Asclin
 * \defgroup IfxLld_Asclin_Asc_DataStructures Data Structures
 * \ingroup IfxLld_Asclin_Asc
 * \defgroup IfxLld_Asclin_Asc_InterruptFunctions Interrupt Functions
 * \ingroup IfxLld_Asclin_Asc
 * \defgroup IfxLld_Asclin_Asc_SimpleCom Simple Communication
 * \ingroup IfxLld_Asclin_Asc
 * \defgroup IfxLld_Asclin_Asc_StreamCom Stream based Communication (STDIO)
 * \ingroup IfxLld_Asclin_Asc
 * \defgroup IfxLld_Asclin_Asc_ModuleFunctions Module Functions
 * \ingroup IfxLld_Asclin_Asc
 */

#ifndef IFXASCLIN_ASC_H
#define IFXASCLIN_ASC_H 1

/******************************************************************************/
/*----------------------------------Includes----------------------------------*/
/******************************************************************************/

#include "illd\Libraries\iLLD\TC27D\Tricore\Asclin\Std/IfxAsclin.h"
#include "illd\Libraries\iLLD\TC27D\Tricore\_Lib\DataHandling/Ifx_Fifo.h"
#include "illd\Libraries\iLLD\TC27D\Tricore\Stm\Std/IfxStm.h"
#include "illd\Libraries\Service\CpuGeneric\StdIf/IfxStdIf_DPipe.h"

/******************************************************************************/
/*-----------------------------Data Structures--------------------------------*/
/******************************************************************************/

/** \brief Structure for Error Flags
 */
typedef struct
{
    uint8 parityError : 1;         /**< \brief parity error */
    uint8 frameError : 1;          /**< \brief transmit complete/pending error */
    uint8 rxFifoOverflow : 1;      /**< \brief receive FIFO overflow error */
    uint8 rxFifoUnderflow : 1;     /**< \brief receive FIFO underflow error */
    uint8 txFifoOverflow : 1;      /**< \brief transmit FIFO overflow error */
} IfxAsclin_Asc_ErrorFlags;

/** \addtogroup IfxLld_Asclin_Asc_DataStructures
 * \{ */
/** \brief Structure for baudrate
 */
typedef struct
{
    float32                      baudrate;           /**< \brief value of the required baudrate */
    uint16                       prescaler;          /**< \brief BITCON.PRESCALER, the division ratio of the predevider */
    IfxAsclin_OversamplingFactor oversampling;       /**< \brief BITCON.OVERSAMPLING, division ratio of the baudrate post devider */
} IfxAsclin_Asc_BaudRate;

/** \brief Structure for bit timings
 */
typedef struct
{
    IfxAsclin_SamplesPerBit       medianFilter;              /**< \brief BITCON.SM, number of samples per bit (1 or 3), sample mode/median filter */
    IfxAsclin_SamplePointPosition samplePointPosition;       /**< \brief BITCON.SAMPLEPOINT, sample point position */
} IfxAsclin_Asc_BitTimingControl;

/** \brief Structure for FIFO control
 */
typedef struct
{
    IfxAsclin_TxFifoInletWidth     inWidth;                    /**< \brief TXFIFOCON.INW, transmit FIFO inlet width */
    IfxAsclin_RxFifoOutletWidth    outWidth;                   /**< \brief RXFIFOCON.OTW, receive FIFO oulet width */
    IfxAsclin_TxFifoInterruptLevel txFifoInterruptLevel;       /**< \brief TXFIFOCON.INTLEVEL, Tx FIFO interrupt level */
    IfxAsclin_RxFifoInterruptLevel rxFifoInterruptLevel;       /**< \brief RXFIFOCON.INTLEVEL, Rx FIFO interrupt level */
    IfxAsclin_ReceiveBufferMode    buffMode;                   /**< \brief RXFIFOCON.BUFF, receive buffer mode (Rx FIFO or Rx buffer) */
} IfxAsclin_Asc_FifoControl;

/** \brief Structure for frame control
 */
typedef struct
{
    IfxAsclin_IdleDelay      idleDelay;        /**< \brief FRAMECON.IDLE, idle delay */
    IfxAsclin_StopBit        stopBit;          /**< \brief FRAMECON.STOP, number of stop bits */
    IfxAsclin_FrameMode      frameMode;        /**< \brief FRAMECON.MODE, mode of operation of the module */
    IfxAsclin_ShiftDirection shiftDir;         /**< \brief FRAMECON.MSB, shift direction */
    IfxAsclin_ParityType     parityType;       /**< \brief FRAMECON.ODD, parity type (even or odd) */
    IfxAsclin_DataLength     dataLength;       /**< \brief DATCON.DATALENGTH, data length, number of bits per transfer */
    boolean                  parityBit;        /**< \brief FRAMECON.PEN, parity enable */
} IfxAsclin_Asc_FrameControl;

/** \brief Structure for interrupt configuration
 */
typedef struct
{
    uint16     txPriority;          /**< \brief transmit interrupt priority */
    uint16     rxPriority;          /**< \brief receive interrupt priority */
    uint16     erPriority;          /**< \brief error interrupt priority */
    IfxSrc_Tos typeOfService;       /**< \brief type of interrupt service */
} IfxAsclin_Asc_InterruptConfig;

/** \brief Structure for ASC pin configuration
 */
typedef struct
{
    IFX_CONST IfxAsclin_Cts_In  *cts;             /**< \brief ASC clear to send (CTS) pin */
    IfxPort_InputMode            ctsMode;         /**< \brief Cts pin as input */
    IFX_CONST IfxAsclin_Rx_In   *rx;              /**< \brief ASC Rx pin */
    IfxPort_InputMode            rxMode;          /**< \brief Rx pin as input */
    IFX_CONST IfxAsclin_Rts_Out *rts;             /**< \brief ASC (request to send) RTS pin */
    IfxPort_OutputMode           rtsMode;         /**< \brief Rts as output */
    IFX_CONST IfxAsclin_Tx_Out  *tx;              /**< \brief ASC Tx pin */
    IfxPort_OutputMode           txMode;          /**< \brief Tx as output */
    IfxPort_PadDriver            pinDriver;       /**< \brief pad driver */
} IfxAsclin_Asc_Pins;

/** \} */

/** \brief This union contains the error flags. In addition it allows to write and read to/from all flags as once via the ALL member.
 */
typedef union
{
    uint8                    ALL;
    IfxAsclin_Asc_ErrorFlags flags;
} IfxAsclin_Asc_ErrorFlagsUnion;

/** \addtogroup IfxLld_Asclin_Asc_DataStructures
 * \{ */
/** \brief Module Handle
 */
typedef struct
{
    Ifx_ASCLIN                   *asclin;                 /**< \brief pointer to ASCLIN registers */
    Ifx_Fifo                     *tx;                     /**< \brief Transmit FIFO buffer */
    Ifx_Fifo                     *rx;                     /**< \brief Receive FIFO buffer */
    volatile boolean              txInProgress;           /**< \brief Ongoing transfer. Will be set by IfxAsclin_Asc_initiateTransmission, and cleared by IfxAsclin_Asc_isrTransmit */
    volatile boolean              rxSwFifoOverflow;       /**< \brief Will be set by IfxAsclin_Asc_isrReceive if the SW Fifo overflowed */
    IfxAsclin_Asc_ErrorFlagsUnion errorFlags;             /**< \brief error reported by ASCLIN during runtime (written by IfxAsclin_Asc_isrError) */
    Ifx_DataBufferMode            dataBufferMode;         /**< \brief Rx buffer mode */
    volatile uint32               sendCount;              /**< \brief Number of byte that are send out, this value is reset with the function Asc_If_resetSendCount() */
    volatile Ifx_TickTime         txTimestamp;            /**< \brief Time stamp of the latest send byte */
} IfxAsclin_Asc;

/** \brief Configuration structure of the module
 */
typedef struct
{
    Ifx_ASCLIN                    *asclin;               /**< \brief pointer to ASCLIN registers */
    IfxAsclin_Asc_BaudRate         baudrate;             /**< \brief structure for baudrate */
    IfxAsclin_Asc_BitTimingControl bitTiming;            /**< \brief structure for bit timings */
    IfxAsclin_Asc_FrameControl     frame;                /**< \brief structure for frame control */
    IfxAsclin_Asc_FifoControl      fifo;                 /**< \brief structure for FIFO control */
    IfxAsclin_Asc_InterruptConfig  interrupt;            /**< \brief structure for interrupt configuration */
    IFX_CONST IfxAsclin_Asc_Pins  *pins;                 /**< \brief structure for ASC pins */
    IfxAsclin_ClockSource          clockSource;          /**< \brief CSR.CLKSEL, clock source selection */
    IfxAsclin_Asc_ErrorFlagsUnion  errorFlags;           /**< \brief structure for error flags */
    Ifx_SizeT                      txBufferSize;         /**< \brief Size of the tx buffer */
    void                          *txBuffer;             /**< \brief The buffer parameter must point on a free memory location where the buffer object will be Initialised.
                                                          *
                                                          * The Size of this area must be at least equals to "txBufferSize + sizeof(Ifx_Fifo) + 8". Not tacking this in account may result in unpredictable behavior.
                                                          *
                                                          * If set to NULL_PTR, the buffer will  be allocated dynamically according to txBufferSize */
    Ifx_SizeT rxBufferSize;                              /**< \brief Size of the rx buffer */
    void     *rxBuffer;                                  /**< \brief The buffer parameter must point on a free memory location where the buffer object will be Initialised.
                                                          *
                                                          * The Size of this area must be at least equals to "rxBufferSize + sizeof(Ifx_Fifo) + 8". Not tacking this in account may result in unpredictable behavior.
                                                          *
                                                          * If set to NULL, the buffer will be allocated dynamically according to rxBufferSize */
    boolean            loopBack;                         /**< \brief IOCR.LB, loop back mode selection, 0 for disable, 1 for enable */
    Ifx_DataBufferMode dataBufferMode;                   /**< \brief Rx buffer mode */
} IfxAsclin_Asc_Config;

/** \} */

/** \addtogroup IfxLld_Asclin_Asc_InterruptFunctions
 * \{ */

/******************************************************************************/
/*-------------------------Global Function Prototypes-------------------------*/
/******************************************************************************/

/** \brief ISR error routine.
 * \see IfxSdtIf_DPipe_OnError
 *
 * Currently it only stores error flags in the handle (asclin->errorFlags) whenever an error happened.
 * The user software could react on these flags, e.g. it could re-initialize the module.
 * \param asclin module handler
 * \return None
 */
IFX_EXTERN void IfxAsclin_Asc_isrError(IfxAsclin_Asc *asclin);

/** \brief ISR receive routine
 * \see IfxSdtIf_DPipe_OnReceive
 * \param asclin module handler
 * \return None
 */
IFX_EXTERN void IfxAsclin_Asc_isrReceive(IfxAsclin_Asc *asclin);

/** \brief ISR transmit routine
 * \see IfxSdtIf_DPipe_OnTransmit
 * \param asclin module handler
 * \return None
 */
IFX_EXTERN void IfxAsclin_Asc_isrTransmit(IfxAsclin_Asc *asclin);

/** \} */

/** \addtogroup IfxLld_Asclin_Asc_SimpleCom
 * \{ */

/******************************************************************************/
/*-------------------------Global Function Prototypes-------------------------*/
/******************************************************************************/

/** \brief Reads data from the Rx FIFO
 * \param asclin module handle
 * \return number of received data words
 *
 * A coding example can be found in \ref IfxLld_Asclin_Asc_Usage
 *
 */
IFX_EXTERN uint8 IfxAsclin_Asc_blockingRead(IfxAsclin_Asc *asclin);

/** \brief Writes data into the Tx FIFO
 * \param asclin module handle
 * \param data the data byte which should be sent
 * \return Returns TRUE if data could be written
 *
 * A coding example can be found in \ref IfxLld_Asclin_Asc_Usage
 *
 */
IFX_EXTERN boolean IfxAsclin_Asc_blockingWrite(IfxAsclin_Asc *asclin, uint8 data);

/** \} */

/** \addtogroup IfxLld_Asclin_Asc_StreamCom
 * \{ */

/******************************************************************************/
/*-------------------------Global Function Prototypes-------------------------*/
/******************************************************************************/

/** \brief \see IfxStdIf_DPipe_CanReadCount
 * \param asclin module handle
 * \param count Count of data which should be checked (in bytes)
 * \param timeout in system timer ticks
 * \return Returns TRUE if at least count bytes are available for read in the rx buffer, if not the Event is armed to be set when the buffer count is bigger or equal to the requested count.
 */
IFX_EXTERN boolean IfxAsclin_Asc_canReadCount(IfxAsclin_Asc *asclin, Ifx_SizeT count, Ifx_TickTime timeout);

/** \brief \see IfxStdIf_DPipe_CanWriteCount
 * \param asclin module handle
 * \param count Count of data which should be checked (in bytes)
 * \param timeout in system timer ticks
 * \return Returns TRUE if at least count bytes can be written to the tx buffer, if not the Event is armed to be set when the buffer free count is bigger or equal to the requested count
 */
IFX_EXTERN boolean IfxAsclin_Asc_canWriteCount(IfxAsclin_Asc *asclin, Ifx_SizeT count, Ifx_TickTime timeout);

/** \brief \see IfxStdIf_DPipe_ClearRx
 * \param asclin module handle
 * \return None
 */
IFX_EXTERN void IfxAsclin_Asc_clearRx(IfxAsclin_Asc *asclin);

/** \brief \see IfxStdIf_DPipe_ClearTx
 * \param asclin module handle
 * \return None
 */
IFX_EXTERN void IfxAsclin_Asc_clearTx(IfxAsclin_Asc *asclin);

/** \brief \see IfxStdIf_DPipe_FlushTx
 * \param asclin module handle
 * \param timeout in system timer ticks
 * \return Returns TRUE if the FIFO is empty
 */
IFX_EXTERN boolean IfxAsclin_Asc_flushTx(IfxAsclin_Asc *asclin, Ifx_TickTime timeout);

/** \brief \see IfxStdIf_DPipe_GetReadCount
 * \param asclin module handle
 * \return The number of bytes in the rx buffer
 */
IFX_EXTERN sint32 IfxAsclin_Asc_getReadCount(IfxAsclin_Asc *asclin);

/** \brief \see IIfxStdIf_DPipe_GetReadEvent
 * \param asclin module handle
 * \return Read event object
 */
IFX_EXTERN IfxStdIf_DPipe_ReadEvent IfxAsclin_Asc_getReadEvent(IfxAsclin_Asc *asclin);

/** \brief \see IfxStdIf_DPipe_GetSendCount
 * \param asclin module handle
 * \return number of bytes send
 */
IFX_EXTERN uint32 IfxAsclin_Asc_getSendCount(IfxAsclin_Asc *asclin);

/** \brief \see IfxStdIf_DPipe_GetTxTimeStamp
 * \param asclin module handle
 * \return Time in ticks
 */
IFX_EXTERN Ifx_TickTime IfxAsclin_Asc_getTxTimeStamp(IfxAsclin_Asc *asclin);

/** \brief \see IfxStdIf_DPipe_GetWriteCount
 * \param asclin module handle
 * \return The number of free bytes in the tx buffer
 */
IFX_EXTERN sint32 IfxAsclin_Asc_getWriteCount(IfxAsclin_Asc *asclin);

/** \brief \see IIfxStdIf_DPipe_GetWriteEvent
 * \param asclin module handle
 * \return Write event object
 */
IFX_EXTERN IfxStdIf_DPipe_WriteEvent IfxAsclin_Asc_getWriteEvent(IfxAsclin_Asc *asclin);

/** \brief \see  IfxStdIf_DPipe_Read
 * \param asclin module handle
 * \param data Pointer to the start of data
 * \param count Pointer to the count of data (in bytes).
 * \param timeout in system timer ticks
 * \return Returns TRUE if all items could be read\n
 * Returns FALSE if not all the items could be read
 *
 * A coding example can be found in \ref IfxLld_Asclin_Asc_Usage
 *
 */
IFX_EXTERN boolean IfxAsclin_Asc_read(IfxAsclin_Asc *asclin, void *data, Ifx_SizeT *count, Ifx_TickTime timeout);

/** \brief \see IfxStdIf_DPipe_ResetSendCount
 * \param asclin module handle
 * \return None
 */
IFX_EXTERN void IfxAsclin_Asc_resetSendCount(IfxAsclin_Asc *asclin);

/** \brief \see IfxStdIf_DPipe_Write
 * \param asclin module handle
 * \param data Pointer to the start of data
 * \param count Pointer to the count of data (in bytes).
 * \param timeout in system timer ticks
 * \return Returns TRUE if all items could be written\n
 * Returns FALSE if not all the items could be written
 *
 * A coding example can be found in \ref IfxLld_Asclin_Asc_Usage
 *
 */
IFX_EXTERN boolean IfxAsclin_Asc_write(IfxAsclin_Asc *asclin, const void *data, Ifx_SizeT *count, Ifx_TickTime timeout);

/** \} */

/** \addtogroup IfxLld_Asclin_Asc_ModuleFunctions
 * \{ */

/******************************************************************************/
/*-------------------------Global Function Prototypes-------------------------*/
/******************************************************************************/

/** \brief Disables the module
 * \param asclin module handle
 * \return None
 */
IFX_EXTERN void IfxAsclin_Asc_disableModule(IfxAsclin_Asc *asclin);

/** \brief Initialises the module
 * \param asclin module handle
 * \param config predefined configuration structure of the module
 * \return Status
 *
 * A coding example can be found in \ref IfxLld_Asclin_Asc_Usage
 *
 */
IFX_EXTERN IfxAsclin_Status IfxAsclin_Asc_initModule(IfxAsclin_Asc *asclin, const IfxAsclin_Asc_Config *config);

/** \brief Fills the config structure with default values
 * \param config configuration structure of the module
 * \param asclin pointer to ASCLIN registers
 * \return None
 *
 * A coding example can be found in \ref IfxLld_Asclin_Asc_Usage
 *
 */
IFX_EXTERN void IfxAsclin_Asc_initModuleConfig(IfxAsclin_Asc_Config *config, Ifx_ASCLIN *asclin);

/** \} */

/******************************************************************************/
/*-------------------------Global Function Prototypes-------------------------*/
/******************************************************************************/

/** \brief Initiate the data transmission
 * \param asclin module handle
 * \return None
 *
 * A coding example can be found in \ref IfxLld_Asclin_Asc_Usage
 *
 */
IFX_EXTERN void IfxAsclin_Asc_initiateTransmission(IfxAsclin_Asc *asclin);

/** \brief Initialize the standard interface to the device driver
 * \param stdif standard interface object, will be initialized by the function
 * \param asclin device driver object used by the standard interface. must be initialised separately
 * \return TRUE on success, else FALSE
 */
IFX_EXTERN boolean IfxAsclin_Asc_stdIfDPipeInit(IfxStdIf_DPipe *stdif, IfxAsclin_Asc *asclin);

#endif /* IFXASCLIN_ASC_H */
