#include "msp430g2553.h"
 
#define TXLED BIT0
#define RXLED BIT6
#define TXD BIT2
#define RXD BIT1


enum state{Wakeup, Header, Length, Reading, Done};
enum bool{true,false};
 
char string2[] = { "Hello World\r\n" };
char string3[] = {"Test 4 DSO!\r\n"};
char string0[] = { "YES Pressed\r\n" };
char string1[] = {"NOT Pressed\r\n"};
char string[14];

unsigned int i; //Counter
enum state state_machine(enum state actual_state, unsigned char buf[256], int s, unsigned char* rx_msg);   //FSM for the UART
enum bool flag_tx_finished=false;
int string_selector = 1,j;

int main(void)
{

   P1REN = 0x08;
   P1OUT |= 0x08;
   P1DIR &= ~BIT3;     // switch connected to bit3
   
    if((P1IN && BIT3 )== 0)
    {
        for(j=0;j<14;j++)
            string[j] = string0[j];
        P1OUT |= RXLED;
    }
    else
    {
        for(j=0;j<14;j++)
            string[j] = string1[j];
        P1OUT &= ~RXLED;
     }


   WDTCTL = WDTPW + WDTHOLD; // Stop WDT
   DCOCTL = 0; // Select lowest DCOx and MODx settings
   BCSCTL1 = CALBC1_1MHZ; // Set DCO
   DCOCTL = CALDCO_1MHZ;
   
   P2DIR |= 0xFF; // All P2.x outputs
   P2OUT &= 0x00; // All P2.x reset
   P1SEL |= RXD + TXD ; // P1.1 = RXD, P1.2=TXD
   P1SEL2 |= RXD + TXD ; // P1.1 = RXD, P1.2=TXD
   P1DIR |= RXLED + TXLED;

   TACTL |= TACLR;
   TACTL = TASSEL_2 + ID_0 + MC_0;      // SMCLK, /1 divider, stop the counter, interrupt enabled    
   TACCR0 = 0xffff;
   TACCTL0 = CM0 + CCIS_1 + SCS + CCIE;
   TACCTL0 &= ~CAP;
   TACTL |= MC_1;   

   UCA0CTL1 |= UCSSEL_2; // SMCLK
   UCA0BR0 = 0x34; // 1MHz 19200bps
   UCA0BR1 = 0x00; // 1MHz 19200bps
   UCA0MCTL = UCBRS2 + UCBRS0; // Modulation UCBRSx = 5
   UCA0CTL1 &= ~UCSWRST; // **Initialize USCI state machine**
   UC0IE |= UCA0RXIE + UCA0TXIE; // Enable USCI_A0 TX and RX interrupt

   UCA0STAT |= UCLISTEN;    // UART in loopback mode

   __enable_interrupt();
   //__bis_SR_register(CPUOFF + GIE); // Enter LPM0 w/ int until Byte RXed
   
   
   while (1)
   { 
      /*  if(i== sizeof string -1)
        {
            i=0;*/
            if(flag_tx_finished)
            {
//                if(string_selector==0)
/*                if((P1IN && BIT3 )== 0)
                {
                    for(j=0;j<14;j++)
                        string[j] = string0[j];
                    P1OUT |= RXLED;
                }
                else
                {
                    for(j=0;j<14;j++)
                        string[j] = string1[j];
                    P1OUT &= ~RXLED;
                 }*/
               // UC0IE |= UCA0TXIE;      //enable USCI_A0 TX interrupt
              /*  if(string_selector == 1)
                    __delay_cycles(7500);
                else*/
                    __delay_cycles(65000);
                i=0;            
                flag_tx_finished = false;
            }
        //}
    }
}
 
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
    P1OUT |= TXLED; 
    UCA0TXBUF = string[i++]; // TX next character 
    if (i == sizeof string - 1) // TX over? 
    {
       UC0IE &= ~UCA0TXIE; // Disable USCI_A0 TX interrupt 
       flag_tx_finished = true;
       string_selector ^= 1;
    }
    P1OUT &= ~TXLED; 
} 
  
#pragma vector=USCIAB0RX_VECTOR 
__interrupt void USCI0RX_ISR(void) 
{ 
   /*if(UCA0RXBUF == 's' || UCA0RXBUF == 'e')
       P1OUT |= RXLED; 
   else
       P1OUT &= ~RXLED;*/
   if (UCA0RXBUF == '\n') // LF received
   { 
       TACCTL0 |= CCIE;
       __delay_cycles(65000);
       i = 0; 
       UC0IE |= UCA0TXIE; // Enable USCI_A0 TX interrupt 
       UCA0TXBUF = string[i++]; 
   } 
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A0_ISR(void)
{
    P1OUT ^= BIT6;
    TACCTL0 &= ~CCIE;
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
