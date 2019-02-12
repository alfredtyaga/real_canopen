// vi: sw=2 ts=2 sts=2
//
// TODO:
//   * watchdog timer and associated beckhoff functionality

/******************************************************************************
Project description:
Test project for a SparkFun AST-CAN485 using the AT90CAN128 microcontroller

Short description:
  PORTA:        Inputs (Keys, low active)
  PORTB:        Outputs (CAN_STBY on PB4)
  PORTD:        Used for I2C (PD0, PD1)
  PORTE:        8 outputs: RXI, TXO, D4, D5, D6, D7, D8, D9
  PORTF:        4 inputs; lower bits map to A0, A1, A2, A3 on the AST-CAN485
******************************************************************************/

#include "hardware.h"
#include "canfestival.h"
#include "can_AVR.h"
#include "real_objdict.h"
#include "uart.h"
#include "pwm.h"
#include <stdio.h>
#include <util/delay.h>


unsigned char timer_interrupt = 0;              // Set if timer interrupt eclapsed
unsigned char inputs;

unsigned char nodeID;
unsigned char digital_input[1] = {0};
unsigned char digital_output[1] = {0};

#ifdef DEBUG_MESSAGES
#undef DEBUG_MESSAGES
#endif

UNS16 pwm_on_time[REAL_PWM_NUM] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
};

UNS16 pwm_off_time[REAL_PWM_NUM] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
};

static Message m = Message_Initializer;         // contain a CAN message

void sys_init();

#define sys_timer                timer_interrupt
#define reset_sys_timer()        timer_interrupt = 0
#define CYCLE_TIME               1000           // Sample Timebase [us]
#define DEFAULT_NODE_ID          1
#define DEBUG_MESSAGES           0

unsigned char digital_input_handler(CO_Data* d, unsigned char *newInput, unsigned char size)
{
  /*
  unsigned char loops, i, input, transmission = 0;

  loops = (sizeof(Read_Inputs_8_Bit) <= size) ? sizeof(Read_Inputs_8_Bit) : size;

  for (i=0; i < loops; i++)
  {
    input = *newInput ^ Polarity_Input_8_Bit[i];
    if (Read_Inputs_8_Bit[i] != input)
    {
      if (Global_Interrupt_Enable_Digital)
      {
        if ((Interrupt_Mask_Any_Change_8_Bit[i] & (Read_Inputs_8_Bit[i] ^ input))
         || (Interrupt_Mask_Low_to_High_8_Bit[i] & ~Read_Inputs_8_Bit[i] & input)
         || (Interrupt_Mask_High_to_Low_8_Bit[i] & Read_Inputs_8_Bit[i] & ~input))
           transmission = 1;
      }
      // update the object dictionary here:
      Read_Inputs_8_Bit[i] = input;
    }
    newInput++;
  }

  if (transmission)
  {
    // force emission of PDO by artificially changing last emitted
    d->PDO_status[0].last_message.cob_id = 0;
    sendPDOevent(d);
  }
  */
  return 1;
}

unsigned char digital_output_handler(CO_Data* d, unsigned char *newOutput, unsigned char size)
{ unsigned char loops, i, error, type;
  UNS32 varsize = 1;

  loops = (sizeof(Write_Outputs_8_Bit) <= size) ? sizeof(Write_Outputs_8_Bit) : size;

  for (i=0; i < loops; i++)
  {
    getODentry(d, 0x1001, 0x0, &error, &varsize, &type, RO);
    if ((getState(d) == Stopped) || (error != 0))       // node stopped or error
    {
      Write_Outputs_8_Bit[i] &= (~Error_Mode_Outputs_8_Bit[i] | Error_Value_Outputs_8_Bit[i]);
      Write_Outputs_8_Bit[i] |= (Error_Mode_Outputs_8_Bit[i] & Error_Value_Outputs_8_Bit[i]);
    }
    *newOutput = Write_Outputs_8_Bit[i] ^ Change_Polarity_Outputs_8_Bit[i];
    newOutput++;
  }
  return 1;
}

unsigned char analog_input_handler(CO_Data* d, unsigned int *newInput, unsigned char size)
{
  return 0;
}

unsigned char analog_output_handler(CO_Data* d, unsigned int *newOutput, unsigned char size)
{
  return 0;
}

unsigned char pwm_output_handler(CO_Data* d, UNS16 *on_time, UNS16 *off_time)
{
  unsigned char i, error, type;
  UNS32 varsize = 1;
  unsigned char outputs_disabled;
  UNS16 new_on, new_off;

  // Get the error status
  getODentry(d, 0x1001, 0x0, &error, &varsize, &type, RO);

  outputs_disabled = (getState(d) == Stopped) || (error != 0);

  if (outputs_disabled) {
    printf("disabled: %d ", outputs_disabled);
    if (getState(d) == Stopped) {
      printf("state=stopped\n");
    }
    if (error != 0) {
      printf("error %d\n", error);
    }
  }

  for (i=0; i < REAL_PWM_NUM; i++)
  {
    if (outputs_disabled)
    {
        new_on = 0;
        new_off = 0;
    } else {
        new_on = 0;
        new_off = (Write_Outputs_16_Bit[i] & 0x0FFF);
    }

    if ((new_on != *on_time) || (new_off != *off_time)) {
        *on_time = new_on;
        *off_time = new_off;
/* #if DEBUG_MESSAGES */
        printf("PWM %d %d %d\n", i, new_on, new_off);
/* #endif */
        // TODO addresses
        if (i < 16) {
            set_pwm_output(0x40, i, new_on, new_off);
        } else {
            // set_pwm_output(0x41, i - 16, new_on, new_off);
        }
    }

    on_time++;
    off_time++;
  }
  return 1;
}

unsigned char read_inputs()
{
  // Only read 4 bits from PORTF, mapping to A0, A1, A2, A3
  unsigned char reading = (PINF & 0x0F);
#if DEBUG_MESSAGES
  if (reading != 0) {
	printf("PINF=%x\n", reading);
  }
#endif
  return reading;
}

void write_outputs(unsigned char value)
{
  // PORTE output maps to RXI, TXO, D4, D5, D6, D7, D8, D9 on the board
  PORTE = value;
}

int main(void)
{
  sys_init();                            // Initialize system
  uart_init();
  pwm_init();

  set_pwm_frequency(0x40, 1000.0f);

  for (int i=0; i < 16; i++) {
    set_pwm_output(0x40, i, i * 100, 2000);
  }

  stdout = &uart_output;
  stdin  = &uart_input;

  canInit(CAN_BAUDRATE);                 // Initialize the CANopen bus
  initTimer();                           // Start timer for the CANopen stack
  nodeID = DEFAULT_NODE_ID;
  setNodeId(&ObjDict_Data, nodeID);
  setState(&ObjDict_Data, Initialisation);  // Init the state

  printf("Online with node ID: %d\n", nodeID);

  for(;;)                                // forever loop
  {
    if (sys_timer)                       // Cycle timer, invoke action on every time slice
    {
      reset_sys_timer();                 // Reset timer
      // digital_input[0] = read_inputs();
      // digital_input_handler(&ObjDict_Data, digital_input, sizeof(digital_input));
      // digital_output_handler(&ObjDict_Data, digital_output, sizeof(digital_output));
      pwm_output_handler(&ObjDict_Data, (UNS16*)&pwm_on_time, (UNS16*)&pwm_off_time);
      // write_outputs(digital_output[0]);
    }

    // a message was received pass it to the CANstack
    if (canReceive(&m)) {               // a message received
#if DEBUG_MESSAGES
      printf("<-%d/%d/", m.cob_id, m.rtr);
      for (int i=0; i< m.len; i++){
          printf("%d ", m.data[i]);
      }
      printf("\n");
#endif
      canDispatch(&ObjDict_Data, &m);         // process it
    } else {
      // Enter sleep mode
#ifdef WD_SLEEP           // Watchdog and Sleep
      wdt_reset();
      sleep_enable();
      sleep_cpu();
#endif                            // Watchdog and Sleep
    }
  }
}

void sys_init()
/******************************************************************************
Initialize the relays, the main states and the modbus protocol stack.
INPUT   LOCK_STATES *lock_states
OUTPUT  void
******************************************************************************/
{
  OSCCAL = 0x43;            // adjust the RC oscillator

  PORTA = 0xFF;             // Inputs (Keys, low active) with pullup
  DDRA  = 0x00;             //

  PORTB = 0x00;             // All outputs -
  DDRB  = 0xFF;             //  Important output PB4=CAN_STBY set to 0 for normal mode

  PORTC = 0xFF;             // 1 BCD switch with pullup
  DDRC  = 0x00;             //

  // PORTD0 / SCL
  // PORTD1 / SDA
  // initialized in pwm_init

  PORTE = 0x00;             // Output
  DDRE  = 0xFF;             // - 2x not used, 2x not used

  PORTF = 0x00;             // Inputs
  DDRF  = 0x00;             // - Lower 4 bits are inputs

  PORTG = 0x00;             // Not used
  DDRG  = 0x1F;             // - Output for debug (only 5 pins)

  // Set timer 0 for main schedule time
  TCCR0A |= 1 << WGM01 | 1 << CS01 | 1 << CS00;// Timer 0 CTC , Timer 0 mit CK/64 starten
  TIMSK0 = 1 << OCIE0A;                 // Timer Interrupts: Timer 0 Compare
  OCR0A = (unsigned char)(F_CPU / 64 * CYCLE_TIME/1000000 - 1); // Reloadvalue for timer 0
#ifdef WD_SLEEP             // Watchdog and Sleep
  wdt_reset();
  wdt_enable(WDTO_15MS);    // Watchdogtimer start with 16 ms timeout
#endif                      // Watchdog and Sleep
  sei();                    // Enable Interrupts
}


#ifdef  __IAR_SYSTEMS_ICC__
#pragma type_attribute = __interrupt
#pragma vector=TIMER0_COMP_vect
void TIMER0_COMP_interrupt(void)
#else   // GCC
ISR(TIMER0_COMP_vect)
#endif  // GCC
/******************************************************************************
Interrupt service routine: Timer 2 Compare A for the main cycle
******************************************************************************/

{
  timer_interrupt = 1;  // Set flag
}
