/*****************************************************************************
Test.c
******************************************************************************
 MICROCONTROLLORE TEXAS MSP430G2433
******************************************************************************

+++++++++++++++++ STATO DI AVANZAMENTO DEI LAVORI +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Test per comunicazione tra sensore optex e microcontrollore

*/


#include "msp430g2452.h"
//#include "USI_I2CMaster.h"
//#include "uart.h"

#define TI_USI_EXIT_LPM            1
#define TI_USI_STAY_LPM            0

//------------------------------------------------------------------------------
// Hardware-related definitions
//------------------------------------------------------------------------------
#define UART_TXD   0x02                     // TXD on P1.1 (Timer0_A.OUT0)
#define UART_RXD   0x04                     // RXD on P1.2 (Timer0_A.CCI1A)
#define TXD BIT2
#define RXD BIT1
#define USIDIV_7            (0xE0)    /* USI  Clock Divider: 7 */
#define USICKPL             (0x02)    /* USI  Clock Polarity 0:Inactive=Low / 1:Inactive=High */
#define USISSEL_2           (0x08)    /* USI  Clock Source: 2 */

// global variables 

enum state{Wakeup, Header, Length, Reading, Done};
unsigned char timerA_UART_mode = 0;
unsigned int txData; 
int flag = 0;

const char string[] = { "Hello World\r\n" };
unsigned int i; //Counter 

// function prototypes

//void TI_USI_I2C_MasterInit(unsigned char ClockConfig, int(*StatusCallback)(unsigned char) );

void TimerA_UART_init(void);
void TimerA_UART_shutdown(void);
void TimerA_UART_tx(unsigned char byte);

int StatusCallback(unsigned char c);  
void init();    // initialization
//enum state state_machine(enum state actual_state, unsigned char buf[256], int s, unsigned char* rx_msg);   //FSM for the i2c


int main()
{
    WDTCTL = WDTPW + WDTHOLD;           // Stop watchdog timer
//    long unsigned int i=0,j=0;
//    init();
    P1SEL = 0;
    P1SEL2 = 0;
    P1DIR = 0xff;
    P1REN = 0x08;
    P1OUT = 0x08;
    P1OUT &= ~BIT0;
    P1OUT &= ~BIT6;
    P1DIR &= ~BIT3;

    DCOCTL = 0x00;                          // Set DCOCLK to 1MHz
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;
    BCSCTL2 &= ~DIVS_3;  
                       
    TACTL |= TACLR;
    TACTL = TASSEL_2 + ID_0 + MC_0 + TAIE;      // SMCLK, /8 divider, stop the counter, interrupt enabled    
    TACCR0 = 0xffff;
    TACCTL0 = CM0 + CCIS_1 + SCS + CCIE;
    TACCTL0 &= ~CAP;
    TACTL |= MC_1;                       // start the counter in up mode 

    P1IE = BIT3;
    P1IES = BIT3;
    P1IFG &= ~BIT3;
    __bis_SR_register(GIE);             // enable interrupts    

    while(1)
    {          
        if((P1IN & BIT3) == 0)     // switch pressed
            P1OUT |= BIT0;           // turn on LED1
        else
            P1OUT &= ~BIT0;          // turn off LED1
        
    }
  

 
    /*while(1)
    {
            TimerA_UART_tx(0xAA);    
            __delay_cycles(65535);
    }*/
    return 0;
}


#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A0_ISR(void)
{
    P1OUT ^= BIT6;
    __delay_cycles(65535);
}

#pragma vector = PORT1_VECTOR
__interrupt void Port1_ISR(void)
{
    int i,j;
    P1OUT ^= BIT0;        
    P1IFG &= ~BIT3;    
    for(i=0;i<30000;i++)
        for(j=0;j<5;j++)
            __no_operation();
}

void init()
{
    // Initialize USI module, clock ~ SMCLK/128 
    //TI_USI_I2C_MasterInit(USIDIV_7+USISSEL_2+USICKPL, StatusCallback);   // initialize the USI interface as i2c Master. The ClockConfig is (111)(010)(1)(0) = Clock/128, SMCLK clock source, Inactive clock state is High

    // Initialize UART interface (communication with Optex sensor)
    TimerA_UART_init();   
}

/*enum state state_machine(enum state actual_state, unsigned char buf[256], int s, unsigned char* rx_msg)
{
    enum state return_state = Wakeup;
    
    switch(actual_state)
    {
        case Wakeup:if(buf[pos] == 0x00)
                    {
                        count = 0;
                        pos++;
                        return_state = Header;
                    }
                    else                                           
                        return_state = Done;
                    break;
        case Header:           
                    if(pos < s)
                    {                    
                        if(buf[pos] == 0xaa)
                        {
                            pos++;
                            count++;                        
                            return_state = Length;  
                        }
                        else
                                return_state = Done;                                         
                    }
                    else
                    {    
                        pos = 0;
                        return_state = Header;
                    }
                    break;
        case Length:if(pos < s)                        
                    {
                        length = buf[pos];
                        pos++;
                        count++;
                        return_state = Reading;
                    }
                    else
                    {
                            pos = 0;
                            return_state = Length;
                    }
                    break;
        case Reading:if(count < length)
                     {                                           
                         while(pos < s && count < length)
                         {                                 
                             *rx_msg = buf[pos];
                             rx_msg++;                                             
                             count++;    
                             pos++;                            
                         }
                         pos = 0;
                         if(count < length)
                             return_state = Reading;
                         else
                         {
                             *rx_msg = '\0';                  
                             return_state = Done;
                             flag_answer = 0;
                         }
                     }
                     else
                     {
                         return_state = Done;
                         *rx_msg = '\0';        
                         flag_answer = 0;          
                     }                     
                     break;
        case Done:   return_state = Wakeup; 
                     pos = 0;
                     usleep(1000);        //sleep for 1ms: time beween rx and tx can be 0-100ms                
                     break;
    }
    return return_state;
}*/

int StatusCallback(unsigned char c)
{
 return TI_USI_EXIT_LPM;                       // exit active for next transfer
}

//------------------------------------------------------------------------------
// Function configures Timer_A for full-duplex UART operation
//------------------------------------------------------------------------------
void TimerA_UART_init(void)
{
  DCOCTL = 0x00;                          // Set DCOCLK to 1MHz
  BCSCTL1 = CALBC1_1MHZ;
  DCOCTL = CALDCO_1MHZ;
  BCSCTL2 &= ~DIVS_3;                     // SMCLK = 1MHz  

//  UCA0BR0_ = 52;                           // no oversampling, 19200bps
  
  P1SEL |= UART_TXD + UART_RXD;            // Timer function for TXD/RXD pins
//  P1SEL |= UART_TXD ;
  P1DIR |= UART_TXD;                        // TXD 
  P1DIR &= ~UART_RXD;
  
  TACCTL0 = OUT;                          // Set TXD Idle as Mark = '1'
  // TACCTL1 = SCS + CM1 + CAP + CCIE;       // Sync, Neg Edge, Capture, Int
  TACTL |= TACLR;                           // SMCLK, start in continuous mode  
  TACTL = TASSEL_2 + MC_2;                // SMCLK, start in continuous mode
  timerA_UART_mode = 1;
}
//------------------------------------------------------------------------------
// Function unconfigures Timer_A for full-duplex UART operation
//------------------------------------------------------------------------------
void TimerA_UART_shutdown(void)
{
  timerA_UART_mode = 0;
  P1SEL &= ~(UART_TXD + UART_RXD);            // Timer function for TXD/RXD pins
//  P1SEL &= ~(UART_TXD );            // Timer function for TXD/RXD pins  
  TACCTL1 &= ~CCIE;                           // Sync, Neg Edge, Capture, Int
  TACTL &= ~MC_3;                             // Clear TA modes --> Stop Timer Module
  P1OUT &= ~UART_TXD;
}
//------------------------------------------------------------------------------
// Outputs one byte using the Timer_A UART
//------------------------------------------------------------------------------
void TimerA_UART_tx(unsigned char byte)
{
    while (TACCTL0 & CCIE);                 // Ensure last char got TX'd
    TACCR0 = TAR;                           // Current state of TA counter
    TACCR0 += UART_TBIT;                    // One bit time till first bit
    txData = byte;                          // Load global variable
    txData |= 0x100;                        // Add mark stop bit to TXData
    txData <<= 1;                           // Add space start bit
    TACCTL0 = OUTMOD0 + CCIE;               // Set TXD on EQU2 (idle), Int
    __bis_SR_register( LPM0_bits + GIE);
}
//------------------------------------------------------------------------------
// Prints a string over using the Timer_A UART
//------------------------------------------------------------------------------
void TimerA_UART_print(char *string)
{
    while (*string) {
        TimerA_UART_tx(*string++);
    }
}
//------------------------------------------------------------------------------
// Timer_A UART - Transmit Interrupt Handler
//------------------------------------------------------------------------------
/*#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A0_ISR(void)
{
    static unsigned char txBitCnt = 10;
    if (!timerA_UART_mode)
      __bic_SR_register_on_exit(LPM3_bits+GIE); 
    else
    {
      TACCR0 += UART_TBIT;                    // Add Offset to CCRx
      if (--txBitCnt == 0)                    // All bits TXed?
      {                    
          TACCTL0 &= ~CCIE;                   // All bits TXed, disable interrupt
          txBitCnt = 10;
          __bic_SR_register_on_exit(LPM0_bits+GIE);
      }
      else {
          if (txData & 0x01) {
            TACCTL0 &= ~OUTMOD2;              // TX Mark '1'
          }
          else {
            TACCTL0 |= OUTMOD2;               // TX Space '0'
          }
          txData >>= 1;
          
      }
    }
}*/
   
