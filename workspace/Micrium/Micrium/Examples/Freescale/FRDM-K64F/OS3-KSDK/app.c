/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can find our product's user manual, API reference, release notes and
*               more information at https://doc.micrium.com.
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            EXAMPLE CODE
*
*                                        Freescale Kinetis K64
*                                               on the
*
*                                         Freescale FRDM-K64F
*                                          Evaluation Board
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : FF
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <math.h>
#include  <lib_math.h>
#include  <cpu_core.h>

#include  <app_cfg.h>
#include  <os.h>

#include  <fsl_os_abstraction.h>
#include  <system_MK64F12.h>
#include  <board.h>

#include  <bsp_ser.h>

//Using interrupts
#include "fsl_interrupt_manager.h"
#include "fsl_gpio_common.h"

//Accelerometer
#include "KDS/APP/FXOS8700CQ.h"
#include "KDS/APP/I2C.h"
#include "../../platform/CMSIS/Include/device/MK64F12/MK64F12.h"

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  APP_TASK_EQ_0_ITERATION_NBR              16u
#define  APP_TASK_EQ_1_ITERATION_NBR              18u


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  OS_TCB       AppTaskStartTCB;
static  CPU_STK      AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];

                                                                /* --------------- SEMAPHORE TASK TEST --------------- */
static  OS_TCB       App_TaskObj0TCB;
static  CPU_STK      App_TaskObj0Stk[APP_CFG_TASK_OBJ_STK_SIZE];

static  OS_TCB       App_TaskObj1TCB;
static  CPU_STK      App_TaskObj1Stk[APP_CFG_TASK_OBJ_STK_SIZE];
                                                                /* ------------ FLOATING POINT TEST TASK -------------- */
static  OS_TCB       App_TaskEq0FpTCB;
static  CPU_STK      App_TaskEq0FpStk[APP_CFG_TASK_EQ_STK_SIZE];

static  OS_TCB       App_TaskEq1FpTCB;
static  CPU_STK      App_TaskEq1FpStk[APP_CFG_TASK_EQ_STK_SIZE];

/*
*********************************************************************************************************
*                                       TASK VARIABLES
*********************************************************************************************************
*/

// for Anemometer with software implementation (the task is disabled)
static  OS_TCB 		 Anemometer_TCB;
static  CPU_STK      Anemometer_Stk[APP_CFG_TASK_EQ_STK_SIZE];

//for Anemometer with FTM0
static  OS_TCB 		 TaskFtm0TCB;
static  CPU_STK      TaskFtm0Stk[APP_CFG_TASK_EQ_STK_SIZE];

static  OS_TCB 		 Anemometer_TCB;
static  CPU_STK      Anemometer_Stk[APP_CFG_TASK_EQ_STK_SIZE];

static  OS_TCB 		 Rain_sensor_TCB;
static  CPU_STK      Rain_sensor_Stk[APP_CFG_TASK_EQ_STK_SIZE];

static  OS_TCB 		 Thermometer_TCB;
static  CPU_STK      Thermometer_Stk[APP_CFG_TASK_EQ_STK_SIZE];

static  OS_TCB 		 Serial_print_values_TCB;
static  CPU_STK      Serial_print_values_Stk[APP_CFG_TASK_EQ_STK_SIZE];

static  OS_TCB 		 Zero_Hz_TCB;
static  CPU_STK      Zero_Hz_Stk[APP_CFG_TASK_EQ_STK_SIZE];

static  OS_TCB 		 Accelerometer_TCB;
static  CPU_STK      Accelerometer_Stk[APP_CFG_TASK_EQ_STK_SIZE];

#if (OS_CFG_SEM_EN > 0u)
static  OS_SEM       App_TraceSem;
#endif

#if (OS_CFG_SEM_EN > 0u)
static  OS_SEM       App_TaskObjSem;
#endif

#if (OS_CFG_MUTEX_EN > 0u)
static  OS_MUTEX     App_TaskObjMutex;
#endif

#if (OS_CFG_Q_EN > 0u)
static  OS_Q         App_TaskObjQ;
#endif

#if (OS_CFG_FLAG_EN > 0u)
static  OS_FLAG_GRP  App_TaskObjFlag;
#endif


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  AppObjCreate (void);
static  void  AppTaskCreate(void);
static  void  AppTaskStart (void  *p_arg);

static  void  App_TaskEq0Fp(void  *p_arg);                      /* Floating Point Equation 0 task.                      */
static  void  App_TaskEq1Fp(void  *p_arg);                      /* Floating Point Equation 1 task.                      */
static  void  App_TaskObj0 (void  *p_arg);
static  void  App_TaskObj1 (void  *p_arg);


/*
*********************************************************************************************************
*                                       LOCAL FUNCTION AND VARIABLES PROJECT
*********************************************************************************************************
*/


//Requirement #1: Anemometer with software implementation (the task is disabled)
static OS_SEM PTA1sem;
int timestamp = 0;

static void PTA1_int_hdlr( void );


//Requirement #1: Anemometer with FTM0
volatile uint32_t ftm0_pulse;
static OS_SEM ftm0sem;
static OS_SEM flag_sem;
int flag = 0;

static void Anemometer( void *p_arg );
static void Zero_Hz( void *p_arg );
static void ftm0_setup( void );
void ftm0_int_hdlr( void );
static void TaskFtm0( void *p_arg ); //Anemometer task


//Requirement #2: Rain sensor with ADC0
static OS_SEM adc0sem;

static void setup_adc0( void );
static void adc0_int_hdlr( void );
volatile uint16_t ptb2value_adc0_int_hdlr;
static void Rain_sensor( void *p_arg );


//Requirement #3: Temperature sensor with ADC1
static OS_SEM adc1sem;

static void setup_adc1( void );
static void adc1_int_hdlr( void );
volatile uint16_t ptb10value_adc1_int_hdlr; //store the converted value are read by the interrupt handler
static void TaskIrqAdc0( void *p_arg );
static void Thermometer( void *p_arg );


//Requirement #5: Print Values
float windfvalue=0;
float rainfvalue=0;
float tempfvalue=0;

static OS_SEM windfvalue_sem;
static OS_SEM rainfvalue_sem;
static OS_SEM tempfvalue_sem;

static void Serial_print_values( void *p_arg );

//Requirement #8: Accelerometer
static void Accelerometer( void *p_arg );

static OS_SEM X_sem;
static OS_SEM Y_sem;
static OS_SEM Z_sem;

unsigned char AccelMagData[12];
short Xout_Accel_14_bit, Yout_Accel_14_bit, Zout_Accel_14_bit;
float Xout_g=0, Yout_g=0, Zout_g=0;

void MCU_Init(void);
void FXOS8700CQ_Init (void);
void FXOS8700CQ_Accel_Calibration (void);

/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : This the main standard entry point.
*
* Note(s)     : none.
*********************************************************************************************************
*/


int  main (void)
{
    OS_ERR   err;

#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_ERR  cpu_err;
#endif

    hardware_init();

    /*
    //Anemometer with software implementation (the task is disabled)
    OSSemCreate(&PTA1sem, "PTA1 Sempahore", 0u, &err);

    INT_SYS_InstallHandler(PORTA_IRQn, PTA1_int_hdlr);*/

    /*
    *********************************************************************************************************
    *                                       SEMAPHORE CREATION
    *********************************************************************************************************
    */


    OSSemCreate(&adc1sem, "ADC0 Sempahore", 0u, &err);

    OSSemCreate(&adc0sem, "ADC0 Sempahore", 0u, &err);

	OSSemCreate(&ftm0sem, "ftm0 Sempahore", 0u, &err);
	INT_SYS_InstallHandler(PORTC_IRQn, ftm0_int_hdlr);
	OSSemCreate(&flag_sem, "flag Sempahore", 1u, &err);

    OSSemCreate(&windfvalue_sem, "Wind value Sempahore", 0u, &err);
    OSSemCreate(&rainfvalue_sem, "Rain value Sempahore", 0u, &err);
    OSSemCreate(&tempfvalue_sem, "Temp Sempahore", 0u, &err);

	OSSemCreate(&X_sem, "X Sempahore", 1u, &err);
	OSSemCreate(&Y_sem, "Y Sempahore", 1u, &err);
	OSSemCreate(&Z_sem, "Z Sempahore", 1u, &err);

#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_NameSet((CPU_CHAR *)"MK64FN1M0VMD12",
                (CPU_ERR  *)&cpu_err);
#endif

    OSA_Init();                                                 /* Init uC/OS-III.                                      */

    OSTaskCreate(&AppTaskStartTCB,                              /* Create the start task                                */
                 "App Task Start",
                  AppTaskStart,
                  0u,
                  APP_CFG_TASK_START_PRIO,
                 &AppTaskStartStk[0u],
                 (APP_CFG_TASK_START_STK_SIZE / 10u),
                  APP_CFG_TASK_START_STK_SIZE,
                  0u,
                  0u,
                  0u,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
                 &err);

    OSA_Start();                                                /* Start multitasking (i.e. give control to uC/OS-III). */

    while (DEF_ON) {                                            /* Should Never Get Here                                */
        ;
    }
}


/*
*********************************************************************************************************
*                                          STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskStart()' by 'OSTaskCreate()'.
*
* Return(s)   : none.
*
* Caller(s)   : This is a task.
*
* Notes       : (1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                   used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

static  void  AppTaskStart (void *p_arg)
{
    OS_ERR    os_err;

    (void)p_arg;                                                /* See Note #1                                          */


    CPU_Init();                                                 /* Initialize the uC/CPU Services.                      */
    Mem_Init();                                                 /* Initialize the Memory Management Module              */
    Math_Init();                                                /* Initialize the Mathematical Module                   */

#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&os_err);                            /* Compute CPU capacity with no task running            */
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

    BSP_Ser_Init(115200u);

    APP_TRACE_DBG(("Creating Application Objects...\n\r"));
    AppObjCreate();                                             /* Create Applicaiton Kernel Objects                    */

    APP_TRACE_DBG(("Creating Application Tasks...\n\r"));
    AppTaskCreate();  /* Create Application Tasks                             */

    BSP_Ser_Printf( "Welcome!\n\r");

    /*
    *********************************************************************************************************
    *                                       TASKS
    *********************************************************************************************************
    */

    //#Request 8: Accelerometer

    OSTaskCreate(&Accelerometer_TCB,                              // Create the start task
    							  "Accelerometer",
    							  Accelerometer,
    							   0u,
    							   APP_CFG_TASK_START_PRIO,
    							  &Accelerometer_Stk[0u],
    							  (APP_CFG_TASK_START_STK_SIZE / 10u),
    							   APP_CFG_TASK_START_STK_SIZE,
    							   0u,
    							   0u,
    							   0u,
    							  (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
    							  &os_err);



    //#Request 3: Thermometer

      INT_SYS_InstallHandler(ADC1_IRQn, adc1_int_hdlr);
      //enable the ADC1 interrupt
      INT_SYS_EnableIRQ(ADC1_IRQn);

      OSTaskCreate(&Thermometer_TCB,                              // Create the start task
							  "Thermometer",
							  Thermometer,
							   0u,
							   APP_CFG_TASK_START_PRIO,
							  & Thermometer_Stk[0u],
							  (APP_CFG_TASK_START_STK_SIZE / 10u),
							 APP_CFG_TASK_START_STK_SIZE,
							   0u,
							   0u,
							   0u,
							  (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
							  &os_err);



    //#Request 2: rain sensor

      //associate the previously defined interrupt handler to the ADC0 interrupt
      INT_SYS_InstallHandler(ADC0_IRQn, adc0_int_hdlr);
      //enable the ADC0 interrupt
      INT_SYS_EnableIRQ(ADC0_IRQn);

      OSTaskCreate(&Rain_sensor_TCB,                              // Create the start task
							  "Rain_sensor",
							  Rain_sensor,
							   0u,
							   APP_CFG_TASK_START_PRIO,
							  & Rain_sensor_Stk[0u],
							  (APP_CFG_TASK_START_STK_SIZE / 10u),
							   APP_CFG_TASK_START_STK_SIZE,
							   0u,
							   0u,
							   0u,
							  (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
							  &os_err);



      //#Request 1: anemometer

      OSTaskCreate(&TaskFtm0TCB,                              // Create the start task
							  "TaskFtm0",
							  TaskFtm0,
							   0u,
							   APP_CFG_TASK_START_PRIO,
							  &TaskFtm0Stk[0u],
							  (APP_CFG_TASK_START_STK_SIZE / 10u),
							   APP_CFG_TASK_START_STK_SIZE,
							   0u,
							   0u,
							   0u,
							  (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
							  &os_err);



		//#Request 5: Print value in a serial port
		OSTaskCreate(&Zero_Hz_TCB,                              // Create the start task
							  "Zero_Hz",
							  Zero_Hz,
							   0u,
							   APP_CFG_TASK_START_PRIO,
							  &Zero_Hz_Stk[0u],
							  (APP_CFG_TASK_START_STK_SIZE / 10u),
							   APP_CFG_TASK_START_STK_SIZE,
							   0u,
							   0u,
							   0u,
							  (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
							  &os_err);

		OSTaskCreate(&Serial_print_values_TCB,                              // Create the start task
							  "Serial_print_values",
							  Serial_print_values,
							   0u,
							   APP_CFG_TASK_START_PRIO,
							  &Serial_print_values_Stk[0u],
							  (APP_CFG_TASK_START_STK_SIZE / 10u),
							   APP_CFG_TASK_START_STK_SIZE,
							   0u,
							   0u,
							   0u,
							  (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
							  &os_err);



		/*
			   //Requirement #1: Anemometer with software implementation (the task is disabled)
				OSTaskCreate(&Anemometer_TCB,                              // Create the start task
									  "Anemometer",
									  Anemometer,
									   0u,
									   APP_CFG_TASK_START_PRIO,
									  &Anemometer_Stk[0u],
									  (APP_CFG_TASK_START_STK_SIZE / 10u),
									   APP_CFG_TASK_START_STK_SIZE,
									   0u,
									   0u,
									   0u,
									  (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
									  &os_err);
		*/


    while (DEF_TRUE) {                                          // Task body, always written as an infinite loop.
//        GPIO_DRV_TogglePinOutput(BOARD_GPIO_LED_RED);
//        OSTimeDlyHMSM(0u, 0u, 0u, 100u,
//                      OS_OPT_TIME_HMSM_STRICT,
//                      &os_err);
    }
}


/*********************************************************************************************************************************************** */
/*****!!!!ASSIGNMENT!!!!************************************************************************************************************************ */
/*********************************************************************************************************************************************** */

//Accelerometer
static void Accelerometer( void *p_arg )
{
	OS_ERR os_err;

	MCU_Init();
	FXOS8700CQ_Init();
	FXOS8700CQ_Accel_Calibration();

	while(DEF_TRUE)
	{


			I2C_ReadMultiRegisters(FXOS8700CQ_I2C_ADDRESS, OUT_X_MSB_REG, 12, AccelMagData);		// Read data output registers 0x01-0x06 and 0x33 - 0x38

			// 14-bit accelerometer data
			Xout_Accel_14_bit = ((short) (AccelMagData[0]<<8 | AccelMagData[1])) >> 2;		// Compute 14-bit X-axis acceleration output value
			Yout_Accel_14_bit = ((short) (AccelMagData[2]<<8 | AccelMagData[3])) >> 2;		// Compute 14-bit Y-axis acceleration output value
			Zout_Accel_14_bit = ((short) (AccelMagData[4]<<8 | AccelMagData[5])) >> 2;		// Compute 14-bit Z-axis acceleration output value

			OSSemPend(&X_sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
			OSSemPend(&Y_sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
			OSSemPend(&Z_sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
			// Accelerometer data converted to g's
			Xout_g = ((float) Xout_Accel_14_bit) / SENSITIVITY_2G;		// Compute X-axis output value in g's
			Yout_g = ((float) Yout_Accel_14_bit) / SENSITIVITY_2G;		// Compute Y-axis output value in g's
			Zout_g = ((float) Zout_Accel_14_bit) / SENSITIVITY_2G;		// Compute Z-axis output value in g's

			OSSemPost( &X_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );
			OSSemPost( &Y_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );
			OSSemPost( &Z_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );

			OSTimeDlyHMSM(0u, 0u, 1u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err);
	}
}

void MCU_Init(void)
{
	//I2C0 module initialization
	SIM_SCGC4 |= SIM_SCGC4_I2C0_MASK;		// Turn on clock to I2C0 module

	I2C0_F  |= I2C_F_ICR(0x14); 			// SDA hold time = 2.125us, SCL start hold time = 4.25us, SCL stop hold time = 5.125us
	I2C0_C1 |= I2C_C1_IICEN_MASK;    		// Enable I2C0 module

}

/******************************************************************************
* FXOS8700CQ initialization function
******************************************************************************/

void FXOS8700CQ_Init (void)
{
	I2C_WriteRegister(FXOS8700CQ_I2C_ADDRESS, CTRL_REG2, 0x40);		// Reset all registers to POR values

	I2C_WriteRegister(FXOS8700CQ_I2C_ADDRESS, XYZ_DATA_CFG_REG, 0x00);		// +/-2g range with 0.244mg/LSB

	I2C_WriteRegister(FXOS8700CQ_I2C_ADDRESS, M_CTRL_REG1, 0x1F);		// Hybrid mode (accelerometer + magnetometer), max OSR
	I2C_WriteRegister(FXOS8700CQ_I2C_ADDRESS, M_CTRL_REG2, 0x20);		// M_OUT_X_MSB register 0x33 follows the OUT_Z_LSB register 0x06 (used for burst read)

	I2C_WriteRegister(FXOS8700CQ_I2C_ADDRESS, CTRL_REG2, 0x02);     // High Resolution mode
	I2C_WriteRegister(FXOS8700CQ_I2C_ADDRESS, CTRL_REG3, 0x00);		// Push-pull, active low interrupt
	I2C_WriteRegister(FXOS8700CQ_I2C_ADDRESS, CTRL_REG4, 0x01);		// Enable DRDY interrupt
	I2C_WriteRegister(FXOS8700CQ_I2C_ADDRESS, CTRL_REG5, 0x01);		// DRDY interrupt routed to INT1 - PTD4
	I2C_WriteRegister(FXOS8700CQ_I2C_ADDRESS, CTRL_REG1, 0x35);		// ODR = 3.125Hz, Reduced noise, Active mode
}

/******************************************************************************
* Simple accelerometer offset calibration
******************************************************************************/

void FXOS8700CQ_Accel_Calibration (void)
{
	char X_Accel_offset, Y_Accel_offset, Z_Accel_offset;

	I2C_WriteRegister(FXOS8700CQ_I2C_ADDRESS, CTRL_REG1, 0x00);		// Standby mode

	I2C_ReadMultiRegisters(FXOS8700CQ_I2C_ADDRESS, OUT_X_MSB_REG, 6, AccelMagData);		// Read data output registers 0x01-0x06

	Xout_Accel_14_bit = ((short) (AccelMagData[0]<<8 | AccelMagData[1])) >> 2;		// Compute 14-bit X-axis acceleration output value
	Yout_Accel_14_bit = ((short) (AccelMagData[2]<<8 | AccelMagData[3])) >> 2;		// Compute 14-bit Y-axis acceleration output value
	Zout_Accel_14_bit = ((short) (AccelMagData[4]<<8 | AccelMagData[5])) >> 2;		// Compute 14-bit Z-axis acceleration output value

	X_Accel_offset = Xout_Accel_14_bit / 8 * (-1);		// Compute X-axis offset correction value
	Y_Accel_offset = Yout_Accel_14_bit / 8 * (-1);		// Compute Y-axis offset correction value
	Z_Accel_offset = (Zout_Accel_14_bit - SENSITIVITY_2G) / 8 * (-1);		// Compute Z-axis offset correction value

	I2C_WriteRegister(FXOS8700CQ_I2C_ADDRESS, OFF_X_REG, X_Accel_offset);
	I2C_WriteRegister(FXOS8700CQ_I2C_ADDRESS, OFF_Y_REG, Y_Accel_offset);
	I2C_WriteRegister(FXOS8700CQ_I2C_ADDRESS, OFF_Z_REG, Z_Accel_offset);

	I2C_WriteRegister(FXOS8700CQ_I2C_ADDRESS, CTRL_REG1, 0x35);		// Active mode again
}



//Print of values every second
static void Zero_Hz( void *p_arg )
{
	OS_ERR os_err;

	while (DEF_TRUE) {
		OSTimeDlyHMSM(0u, 0u, 2u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err);
		if(flag==1){
			OSSemPend(&flag_sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
		    flag = 0;
		    OSSemPost( &flag_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );
		}
		else{
			OSSemPend(&windfvalue_sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
			windfvalue = 0;
			OSSemPost( &windfvalue_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );
		}
	}

}

static void Serial_print_values( void *p_arg )
{
	OS_ERR os_err;

	(void)p_arg;

	while (DEF_TRUE) {
		OSSemPend(&rainfvalue_sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
		OSSemPend(&tempfvalue_sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
		OSSemPend(&windfvalue_sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
		OSSemPend(&X_sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
		OSSemPend(&Y_sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
		OSSemPend(&Z_sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);

		BSP_Ser_Printf( "<%d> <%.2f [m/s]> <%.0f [mm]> <%.1f [C]> <X= %.2f [g]> <Y=%.2f [g]> <Z=%.2f [g]>\n\r",timestamp++,
																		 windfvalue, rainfvalue, tempfvalue, Xout_g , Yout_g ,Zout_g);

		OSSemPost( &X_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );
		OSSemPost( &Y_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );
		OSSemPost( &Z_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );
		OSSemPost( &windfvalue_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );
		OSSemPost( &tempfvalue_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );
		OSSemPost( &rainfvalue_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );

		OSTimeDlyHMSM(0u, 0u, 1u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err);
	}

}



// Thermometer
static void Thermometer( void *p_arg )
{
	OS_ERR os_err;
	float ptb10fvalue;

	(void)p_arg;

	setup_adc1();
	OSSemPost( &tempfvalue_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );

	while (DEF_TRUE) {
		ADC1_SC1A = (14 & ADC_SC1_ADCH_MASK) | ADC_SC1_AIEN_MASK;
		//wait on the semaphore until the ADC0 interrupt handler provides a new data
		OSSemPend(&adc1sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);

		ptb10fvalue = (3.3*ptb10value_adc1_int_hdlr)/(65536);

		OSSemPend(&tempfvalue_sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
		tempfvalue = 80.0*ptb10fvalue/3.3 - 20.0;
		OSSemPost( &tempfvalue_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );

		OSTimeDlyHMSM(0u, 1u, 0u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err);
	}

}

static void setup_adc1( void )
{
	SIM_SCGC3 |= SIM_SCGC3_ADC1_MASK;
	ADC1_CFG1 |= ADC_CFG1_MODE(3); // 16bits ADC
	ADC1_SC1A |= ADC_SC1_ADCH(31); // Disable the module, ADCH = 1111
}

static void adc1_int_hdlr( void )
{
	OS_ERR os_err;

	CPU_CRITICAL_ENTER();
	OSIntEnter();

	ptb10value_adc1_int_hdlr = ADC1_RA;

	OSSemPost( &adc1sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );

	CPU_CRITICAL_EXIT();
	OSIntExit();
}



//Rain sensor
static void Rain_sensor( void *p_arg )
{
	OS_ERR os_err;
	uint16_t ptb2value;
	CPU_FP32 ptb2fvalue;

	(void)p_arg;

	setup_adc0();

	OSSemPost( &rainfvalue_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );

	while (DEF_TRUE) {
		ADC0_SC1A = (12 & ADC_SC1_ADCH_MASK) | ADC_SC1_AIEN_MASK;
		//wait on the semaphore until the ADC0 interrupt handler provides a new data
		OSSemPend(&adc0sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
		ptb2fvalue = (3.3*ptb2value_adc0_int_hdlr)/(65536);

		OSSemPend(&rainfvalue_sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
		rainfvalue = 20.0*ptb2fvalue/3.3;
		OSSemPost( &rainfvalue_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );

		OSTimeDlyHMSM(0u, 1u, 0u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err);
	}
}

static void setup_adc0( void )
{
	SIM_SCGC6 |= SIM_SCGC6_ADC0_MASK;
	ADC0_CFG1 |= ADC_CFG1_MODE(3); // 16bits ADC
	ADC0_SC1A |= ADC_SC1_ADCH(31); // Disable the module, ADCH = 1111
}

static void adc0_int_hdlr( void )
{
	OS_ERR os_err;

	CPU_CRITICAL_ENTER();
	OSIntEnter();

	ptb2value_adc0_int_hdlr = ADC0_RA;

	OSSemPost( &adc0sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );

	CPU_CRITICAL_EXIT();
	OSIntExit();
}



//Anemometer
static void ftm0_setup( void )
{
	INT_SYS_EnableIRQ(FTM0_IRQn);
	INT_SYS_InstallHandler(FTM0_IRQn, ftm0_int_hdlr);


	SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK; //Turn on FTM0 timer

	FTM0_CONF = 0xC0; //Set the timer in Debug mode, with BDM mode = 0xC0

	FTM0_FMS = 0x0; //Enable modifications to the FTM0 configuration
	FTM0_MODE |= (FTM_MODE_WPDIS_MASK|FTM_MODE_FTMEN_MASK); //Enable writing

	FTM0_CNTIN = FTM_CNTIN_INIT(0); //Initial value of 16 bit counter
	FTM0_MOD = FTM_MOD_MOD(0xFFFF); //Modulo of the count over 16 bits

	FTM0_SC = (FTM_SC_PS(0)| //ENABLE THE FTM0 with prescaler
				FTM_SC_CLKS(0x1)| //set to FTM_C_PS(0)=1
				FTM_SC_TOIE_MASK); //CLOCK set to 60Mhz
									//interrupt enabled


	//Dual-edge capturing for FTM0_CH2 (PTC3) and FTM0_CH3

	FTM0_COMBINE = FTM_COMBINE_DECAPEN1_MASK; //Set channel 2 as input for dual capture
	FTM0_C2SC = FTM_CnSC_ELSA_MASK; //Capture Rising Edge only on channel 2
	FTM0_C3SC = (FTM_CnSC_ELSB_MASK|FTM_CnSC_CHIE_MASK); //Capture Falling Edge
														//on channel 3 and
														//enable the interrupt

}

uint32_t overflow = 0;

void ftm0_int_hdlr( void )
{
	OS_ERR err;
	CPU_ERR cpu_err;


	CPU_CRITICAL_ENTER();
	OSIntEnter();
	FTM0_SC &= 0x7F; //clear the Timer Overflow (TOF) bit


	if(FTM0_STATUS & 0x8) //Check if the FTM0_CH3 captured the event
	{


		FTM0_STATUS &= 0x0; //Clear the flag
		//FTM0_C3V holds the timer value when falling edge has occurred
		//FTM0_C2V holds the timer value when the rising edge has occurred
		if( FTM0_C2V > FTM0_C3V)
			ftm0_pulse = 0xFFFF+FTM0_C3V-FTM0_C2V + (overflow - 1)*65536;
		else
			ftm0_pulse = FTM0_C3V-FTM0_C2V + (overflow)*65536;

			overflow=0;
			if(flag==0){
					OSSemPend(&flag_sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &err);
					flag = 1;
					OSSemPost( &flag_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &err );
				}
			OSSemPost( &ftm0sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &err );


	}else if(FTM0_STATUS & 0x4){
		overflow++;
		if(flag==0){
			OSSemPend(&flag_sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &err);
			flag = 1;
			OSSemPost( &flag_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &err );
		}
	}


	CPU_CRITICAL_EXIT();
	OSIntExit();
}

//Perform the measurement
static void TaskFtm0( void *p_arg )
{
	OS_ERR os_err;


	(void)p_arg;

	ftm0_setup();
	OSSemPost( &windfvalue_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );

	while (DEF_TRUE) {
		//Setting DECAP1 bit to launch the dual capture process
		FTM0_COMBINE |= FTM_COMBINE_DECAP1_MASK;

		OSSemPend(&ftm0sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);

		OSSemPend(&windfvalue_sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
		windfvalue = 0.75*(30000000/(1.0*ftm0_pulse));
		OSSemPost( &windfvalue_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );

		OSTimeDlyHMSM(0u, 0u, 1u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err);
	}
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// Anemometer software implementation
static void Anemometer( void *p_arg ){

	OS_ERR os_err;
		(void)p_arg;
		float wind;
		CPU_ERR          err;

		OSSemPost( &windfvalue_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );

		while (DEF_TRUE) {
			OSSemSet(&PTA1sem, 0, &os_err);
			//wait first rising edge
			OSSemPend(&PTA1sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
			wind = CPU_TS_Get64 ();

			//wait second rising and computer the correct timestamp
			OSSemPend(&PTA1sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
			wind = (CPU_TS_Get64 () - wind);


			//Compute and print the value
			OSSemPend(&windfvalue_sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err);
			windfvalue = 0.75*(1/wind)*CPU_TS_TmrFreqGet(&err);
			OSSemPost( &windfvalue_sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );

			OSTimeDlyHMSM(0u, 0u, 1u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err);

		}

}


static void PTA1_int_hdlr( void ){

	uint32_t ifsr;
	//get the port number from the GPIO identifier
	uint32_t portBaseAddr = g_portBaseAddr[GPIO_EXTRACT_PORT(kGpioPTA1)];
	OS_ERR os_err;

	//disable interrupts
	CPU_CRITICAL_ENTER();
	//inform the OS we are inside an interrupt service routine
	OSIntEnter();

	//read the ifsr for the port
	ifsr = PORT_HAL_GetPortIntFlag(portBaseAddr);

	if( (ifsr & (1 << GPIO_EXTRACT_PIN(kGpioPTA1))) ){
		OSSemPost( &PTA1sem, OS_OPT_POST_1 | OS_OPT_POST_NO_SCHED, &os_err );
		GPIO_DRV_ClearPinIntFlag( kGpioPTA1 );
	}

	//enable interrupts
	CPU_CRITICAL_EXIT();
	//signal the OS we are exiting from an interrupt service routine
	OSIntExit();
}



/*
*********************************************************************************************************
*                                            AppObjCreate()
*
* Description:  Creates the application kernel objects.
*
* Argument(s) :  none.
*
* Return(s)   :  none.
*
* Caller(s)   :  App_TaskStart().
*
* Note(s)     :  none.
*********************************************************************************************************
*/

static  void  AppObjCreate (void)
{
    OS_ERR  os_err;


#if (OS_CFG_SEM_EN > 0u)
    OSSemCreate(&App_TaskObjSem,
                "Sem Test",
                 0u,
                 &os_err);

    OSSemCreate(&App_TraceSem,
                "Trace Lock",
                1u,
                &os_err);
#endif

#if (OS_CFG_MUTEX_EN > 0u)
    OSMutexCreate(&App_TaskObjMutex,
                  "Mutex Test",
                  &os_err);
#endif

#if (OS_CFG_Q_EN > 0u)
    OSQCreate(&App_TaskObjQ,
              "Queue Test",
              1u,
              &os_err);
#endif

#if (OS_CFG_FLAG_EN > 0u)
    OSFlagCreate(&App_TaskObjFlag,
                 "Flag Test",
                  DEF_BIT_NONE,
                 &os_err);
#endif
}


/*
*********************************************************************************************************
*                                           AppTaskCreate()
*
* Description :  This function creates the application tasks.
*
* Argument(s) :  none.
*
* Return(s)   :  none.
*
* Caller(s)   :  App_TaskStart().
*
* Note(s)     :  none.
*********************************************************************************************************
*/

static  void  AppTaskCreate (void)
{
    OS_ERR  os_err;

                                                                /* ---------- CREATE KERNEL OBJECTS TEST TASK --------- */
    OSTaskCreate(&App_TaskObj0TCB,
                 "Kernel Objects Task 0",
                  App_TaskObj0,
                  0u,
                  APP_CFG_TASK_OBJ_PRIO,
                 &App_TaskObj0Stk[0u],
                  APP_CFG_TASK_OBJ_STK_SIZE_LIMIT,
                  APP_CFG_TASK_OBJ_STK_SIZE,
                  0u,
                  0u,
                  0u,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 &os_err);

    OSTaskCreate(&App_TaskObj1TCB,
                 "Kernel Objects Task 0",
                  App_TaskObj1,
                  0u,
                  APP_CFG_TASK_OBJ_PRIO,
                 &App_TaskObj1Stk[0u],
                  APP_CFG_TASK_OBJ_STK_SIZE_LIMIT,
                 (APP_CFG_TASK_OBJ_STK_SIZE - 1u),
                  0u,
                  0u,
                  0u,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 &os_err);

                                                                /* ------------- CREATE FLOATING POINT TASK ----------- */
    OSTaskCreate(&App_TaskEq0FpTCB,
                 "FP  Equation 1",
                  App_TaskEq0Fp,
                  0u,
                  APP_CFG_TASK_EQ_PRIO,
                 &App_TaskEq0FpStk[0u],
                  APP_CFG_TASK_EQ_STK_SIZE_LIMIT,
                 (APP_CFG_TASK_EQ_STK_SIZE - 1u),
                  0u,
                  0u,
                  0u,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
                 &os_err);

    OSTaskCreate(&App_TaskEq1FpTCB,
                 "FP  Equation 1",
                  App_TaskEq1Fp,
                  0u,
                  APP_CFG_TASK_EQ_PRIO,
                 &App_TaskEq1FpStk[0],
                  APP_CFG_TASK_EQ_STK_SIZE_LIMIT,
                 (APP_CFG_TASK_EQ_STK_SIZE - 1u),
                  0u,
                  0u,
                  0u,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
                 &os_err);
}


/*
*********************************************************************************************************
*                                            App_TaskObj0()
*
* Description : Test uC/OS-III objects.
*
* Argument(s) : p_arg is the argument passed to 'App_TaskObj0' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Caller(s)   : This is a task
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_TaskObj0 (void  *p_arg)
{
    OS_ERR  os_err;

    (void)p_arg;


    while (DEF_TRUE) {
#if (OS_CFG_SEM_EN > 0u)
        OSSemPost(&App_TaskObjSem,
                   OS_OPT_POST_1,
                  &os_err);
#endif

#if (OS_CFG_MUTEX_EN > 0u)
        OSMutexPend(&App_TaskObjMutex,
                     0u,
                     OS_OPT_PEND_BLOCKING,
                     0u,
                    &os_err);

        OSTimeDlyHMSM(0u, 0u, 0u, 100u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &os_err);

        OSMutexPost(&App_TaskObjMutex,
                     OS_OPT_POST_NONE,
                    &os_err);
#endif

#if (OS_CFG_Q_EN > 0u)
        OSQPost(        &App_TaskObjQ,
                (void *) 1u,
                         1u,
                         OS_OPT_POST_FIFO,
                        &os_err);

#endif

#if (OS_CFG_FLAG_EN > 0u)
        OSFlagPost(&App_TaskObjFlag,
                    DEF_BIT_00,
                    OS_OPT_POST_FLAG_SET,
                   &os_err);
#endif
        OSTimeDlyHMSM(0u, 0u, 0u, 10u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &os_err);
        APP_TRACE_INFO(("Object test task 0 running ....\n\r"));
    }
}


/*
*********************************************************************************************************
*                                            App_TaskObj1()
*
* Description : Test uC/OS-III objects.
*
* Argument(s) : p_arg is the argument passed to 'App_TaskObj1' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Caller(s)   : This is a task
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_TaskObj1 (void  *p_arg)
{
    OS_ERR       os_err;
    OS_MSG_SIZE  msg_size;


    (void)p_arg;

    while (DEF_TRUE) {

#if (OS_CFG_SEM_EN > 0u)
        OSSemPend(&App_TaskObjSem,
                   0u,
                   OS_OPT_PEND_BLOCKING,
                   0u,
                  &os_err);
#endif

#if (OS_CFG_MUTEX_EN > 0u)
        OSMutexPend(&App_TaskObjMutex,
                     0u,
                     OS_OPT_PEND_BLOCKING,
                     0u,
                    &os_err);

        OSMutexPost(&App_TaskObjMutex,
                     OS_OPT_POST_NONE,
                     &os_err);

#endif

#if (OS_CFG_Q_EN > 0u)
        OSQPend(&App_TaskObjQ,
                 0u,
                 OS_OPT_PEND_BLOCKING,
                &msg_size,
                 0u,
                &os_err);
#endif

#if (OS_CFG_FLAG_EN > 0u)
        OSFlagPend(&App_TaskObjFlag,
                    DEF_BIT_00,
                    0u,
                   (OS_OPT_PEND_FLAG_SET_ALL + OS_OPT_PEND_FLAG_CONSUME + OS_OPT_PEND_BLOCKING),
                    0u,
                   &os_err);
#endif
        OSTimeDlyHMSM(0u, 0u, 0u, 10u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &os_err);
        APP_TRACE_INFO(("Object test task 1 running ....\n\r"));
    }
}


/*
*********************************************************************************************************
*                                           App_TaskEq0Fp()
*
* Description : This task finds the root of the following equation.
*               f(x) =  e^-x(3.2 sin(x) - 0.5 cos(x)) using the bisection mehtod
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskEq0Fp' by 'OSTaskCreate()'.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  App_TaskEq0Fp (void  *p_arg)
{
    CPU_FP32    a;
    CPU_FP32    b;
    CPU_FP32    c;
    CPU_FP32    eps;
    CPU_FP32    f_a;
    CPU_FP32    f_c;
    CPU_FP32    delta;
    CPU_INT08U  iteration;
    RAND_NBR    wait_cycles;


    while (DEF_TRUE) {
        eps       = 0.00001;
        a         = 3.0;
        b         = 4.0;
        delta     = a - b;
        iteration = 0u;
        if (delta < 0) {
            delta = delta * -1.0;
        }

        while (((2.00 * eps) < delta) ||
               (iteration    > 20u  )) {
            c   = (a + b) / 2.00;
            f_a = (exp((-1.0) * a) * (3.2 * sin(a) - 0.5 * cos(a)));
            f_c = (exp((-1.0) * c) * (3.2 * sin(c) - 0.5 * cos(c)));

            if (((f_a > 0.0) && (f_c < 0.0)) ||
                ((f_a < 0.0) && (f_c > 0.0))) {
                b = c;
            } else if (((f_a > 0.0) && (f_c > 0.0)) ||
                       ((f_a < 0.0) && (f_c < 0.0))) {
                a = c;
            } else {
                break;
            }

            delta = a - b;
            if (delta < 0) {
               delta = delta * -1.0;
            }
            iteration++;

            wait_cycles = Math_Rand();
            wait_cycles = wait_cycles % 1000;

            while (wait_cycles > 0u) {
                wait_cycles--;
            }

            if (iteration > APP_TASK_EQ_0_ITERATION_NBR) {
                APP_TRACE_INFO(("App_TaskEq0Fp() max # iteration reached\n\r"));
                break;
            }
        }

        APP_TRACE_INFO(("Eq0 Task Running ....\n\r"));

        if (iteration == APP_TASK_EQ_0_ITERATION_NBR) {
            APP_TRACE_INFO(("Root = %f; f(c) = %f; #iterations : %d \n\r", c, f_c, iteration));
        }
    }
}



/*
*********************************************************************************************************
*                                           App_TaskEq1Fp()
*
* Description : This task finds the root of the following equation.
*               f(x) = x^2 -3 using the bisection mehtod
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskEq()' by 'OSTaskCreate()'.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  App_TaskEq1Fp (void  *p_arg)
{
    CPU_FP32    a;
    CPU_FP32    b;
    CPU_FP32    c;
    CPU_FP32    eps;
    CPU_FP32    f_a;
    CPU_FP32    f_c;
    CPU_FP32    delta;
    CPU_INT08U  iteration;
    RAND_NBR    wait_cycles;


    while (DEF_TRUE) {
        eps       = 0.00001;
        a         = 1.0;
        b         = 4.0;
        delta     = a - b;
        iteration = 0u;

        if (delta < 0) {
            delta = delta * -1.0;
        }

        while ((2.00 * eps) < delta) {
            c   = (a + b) / 2.0;
            f_a = a * a - 3.0;
            f_c = c * c - 3.0;

            if (((f_a > 0.0) && (f_c < 0.0)) ||
                ((f_a < 0.0) && (f_c > 0.0))) {
                b = c;
            } else if (((f_a > 0.0) && (f_c > 0.0)) ||
                       ((f_a < 0.0) && (f_c < 0.0))) {
                a = c;
            } else {
                break;
            }

            delta = a - b;
            if (delta < 0) {
               delta = delta * -1.0;
            }
            iteration++;

            wait_cycles = Math_Rand();
            wait_cycles = wait_cycles % 1000;

            while (wait_cycles > 0u) {
                wait_cycles--;
            }

            if (iteration > APP_TASK_EQ_1_ITERATION_NBR) {
                APP_TRACE_INFO(("App_TaskEq1Fp() max # iteration reached\n\r"));
                break;
            }
        }

        APP_TRACE_INFO(("Eq1 Task Running ....\n\r"));

        if (iteration == APP_TASK_EQ_1_ITERATION_NBR) {
            APP_TRACE_INFO(("Root = %f; f(c) = %f; #iterations : %d \n\r", c, f_c, iteration));
        }
    }
}
