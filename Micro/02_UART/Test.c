#include "msp430g2553.h"
 
#define TXLED BIT0
#define RXLED BIT6
#define TXD BIT2
#define RXD BIT1

void riconoscimento_sensore_connesso(void);     // riconosce il sensore esterno e il tipo di connessione con cui si interfaccia
void sensore_tx(unsigned char string[11]);

enum state{Wakeup, Header, Length, Reading};
enum bool{true,false};
 
char string2[] = { "Hello World\r\n" };
char string3[] = {"Test 4 DSO!\r\n"};
char string0[] = { "YES Pressed\r\n" };
char string1[] = {"NOT Pressed\r\n"};
char string[14];

unsigned int i; //Counter
enum bool flag_tx_finished=false;
int string_selector = 1,j;
unsigned char rx_mex[11], tx_mex[]={"ABCDE\0"};
unsigned int tipo_connessione_sensore;
unsigned char tx_string[11], rx_string[1+9];   // stringhe di rx e tx per comunicazione UART
unsigned int tx_index=1, rx_index=0;               // indici per comunicazione UART 
enum state actual_state = Wakeup;
int flag_answer=1;
unsigned char test[11];//="dtrefgstk o";

int main(void)
{

 /*  P1REN = 0x08;
   P1OUT |= 0x08;
   P1DIR &= ~BIT3;     // switch connected to bit3


    if((P1IN && BIT3 )== 0)
    {
        P1OUT &= ~TXLED;
        P1OUT |= RXLED;
    }
    else
    {
        P1OUT |= TXLED;
        P1OUT &= ~RXLED;
     }
*/

   WDTCTL = WDTPW + WDTHOLD; // Stop WDT
   DCOCTL = 0; // Select lowest DCOx and MODx settings
   BCSCTL1 = CALBC1_1MHZ; // Set DCO
   DCOCTL = CALDCO_1MHZ;
   
   P1DIR |= RXLED + TXLED;
   P1OUT &= ~RXLED & ~TXLED;

   rx_mex[0] = 'A';

   riconoscimento_sensore_connesso();
   _EINT();
   __enable_interrupt();
   while(1)
   {
        if(flag_answer == 0)
        {
            flag_answer = 1;
            sensore_tx(rx_mex);
        }
        __delay_cycles(65000);
        //sensore_rx(rx_mex);               
        UC0IE |= UCA0TXIE;
   }
 
}
 

void riconoscimento_sensore_connesso(void)
{
        tipo_connessione_sensore = 1;   
        P1SEL |= BIT1 + BIT2;
        P1SEL2 |= BIT1 + BIT2;
        UCA0CTL1 |= UCSSEL_2; // SMCLK
        UCA0BR0 = 0x34; // 8MHz 19200bps : UCABR0 = 416d = 1a0h
        UCA0BR1 = 0x00; // 8MHz 19200bps
        UCA0MCTL = UCBRS2 + UCBRS0; // UCBRSx = 6, UCBRFx = 0
        UCA0CTL1 &= ~UCSWRST; // **Inizializza USCI state machine**
        UC0IE |= UCA0RXIE + UCA0TXIE; // Abilita USCI_A0 TX and RX interrupt
   
}

void sensore_tx(unsigned char string[11])
{
        int i;
        switch(tipo_connessione_sensore)
        {
            case 0:         // connesione contatto
                    break;
            case 1:         // connessione seriale UART
                    for(i = 0; i < sizeof tx_string; i++)  // copia la stringa da trasmettere nella stringa gestita dall'interrupt della UART
                        tx_string[i] = string[i];       
                    UC0IE |= UCA0TXIE;              // abilita interrupt per la trasmissione dei caratteri su UART  
                    break;                   
            default:
                    ;
        }        
}

// Funzioni per la gestione delle isr della UART in tx e rx
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
    UCA0TXBUF = tx_string[tx_index++]; // TX next character 
    if (tx_index == sizeof tx_string - 1 ) // TX over? 
    {        
       UC0IE &= ~UCA0TXIE; // Disable USCI_A0 TX interrupt              
       tx_index = 0;       
    }
} 
  
#pragma vector=USCIAB0RX_VECTOR 
__interrupt void USCI0RX_ISR(void) 
{ 
    static int length=0, pos=0;
    switch(actual_state)
    {
                    case Wakeup:if(UCA0RXBUF == 0x00)
                                {
                                    pos = 0;
                                    actual_state = Header;                                    
                                }
                                else                                           
                                    actual_state = Wakeup;
                                break;
                    case Header:if(UCA0RXBUF == 0xaa)
                                {                                                     
                                    actual_state = Length;                                     
                                }
                                else
                                        actual_state = Wakeup;                                         
                                break;
                    case Length:length = UCA0RXBUF;                                
                                actual_state = Reading;
                                break;
                    case Reading:if(pos+3 <= length)            // pos è l'indicde del contenuto del pacchetto, disallineato di 3 Bytes rispetto all'intero pacchetto rx (0x00,0xAA,length)
                                 {                                           
                                     rx_mex[pos] = UCA0RXBUF;                                             
                                     pos++;                            
                                     actual_state = Reading;
                                 }
                                 else
                                 {
                                     actual_state = Wakeup;
                                     flag_answer = 0;   
                                 }                    
                             break;
            }         
}
