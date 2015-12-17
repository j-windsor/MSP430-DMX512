#include <msp430.h> 
#define RX_PORT BIT1 //Port1
#define R_PORT BIT4 //Port2
#define G_PORT BIT5 //Port2
#define B_PORT BIT3 //Port2

/*
 * main.c - LED DMX
 * Jay Windsor - jhw3qh
 * This code takes in a DMX signal, a DMX channel number via dip switch, and color mixes an RGB LED with three DMX channels
 */
void Init();
unsigned char rgb[3] = {50,50,50}; // Holds current brigtness of each LED
unsigned int count512 = 0; // DMX channel counter
unsigned char count255 = 0; // PWM Period Counter
unsigned char dmx_addr = 0; // Countents of P2 - DMX starting address
unsigned char rx_data = 0; // Recent data received
unsigned char byte_received = 0; // Indicator variable - 1 if byte just received

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
	
    Init();
    _BIS_SR(GIE); // Global Interrupt enable

    while(1){
    	//UCA0TXBUF = 0;
    	//get dmx values and load into rgb
    	if (UCA0STAT & UCBRK){ // In between loads of data or idle
    		dmx_addr = P2IN; // get most recent DIP switch channel value
    		count512 = 0; // Reset channel counter (since we are starting over)
    	}
    	if (byte_received) {
    		byte_received = 0; // reset indicator variable
        	count512++; // increment count
        	if(count512 == dmx_addr+2) { // if address channel
        		rgb[0] = rx_data; // load current data into red index
        	}
        	else if(count512 == dmx_addr+3) { // if address+1
        		rgb[1] = rx_data; // load current data into green index
        	}
        	else if(count512 == dmx_addr+4) { // if address +2
        		rgb[2] = rx_data; // load current data into blue index
        	}
        }

    	// PWM CODE
    	// This is the inverse of what you would expect, because the signal is fed to the cathode of the LED
    	if (count255 >= rgb[0]) P1OUT |= R_PORT;
    	else P1OUT &= ~R_PORT;
    	if (count255 >= rgb[1]) P1OUT |= G_PORT;
    	else P1OUT &= ~G_PORT;
    	if (count255 >= rgb[2]) P1OUT |= B_PORT;
    	else P1OUT &= ~B_PORT;
    	count255++;
    }
	return 0;
}

void InitPorts() {
	P1SEL = RX_PORT; // Select UART functionality
	P1SEL2 = RX_PORT;
	P1DIR &= ~RX_PORT; // Set as INPUT
	//P1SEL = BIT2;
	//P1SEL2 = BIT2;
	//P1DIR |= BIT2;

	P1OUT &= ~R_PORT; // SET LED pins to off initially
	P1OUT &= ~G_PORT;
	P1OUT &= ~B_PORT;
	P1DIR = R_PORT | G_PORT | B_PORT; // Set the LED pins as outputs

	P2DIR = ~0xFF; // SET all P2 pins as inputs for DIP Switch
	P2REN = 0xFF; // Turn on pull-down resistors
	P2OUT = 0x00;

}

void InitTimer() {
	DCOCTL = CALDCO_8MHZ;  			// |Set clock speed to 8 MHz|
	BCSCTL1 = CALBC1_8MHZ;  		// |                        |

}

void InitUSCI() {
	UCA0CTL1 |= UCSWRST; // Pause state machine during init
	UCA0CTL1 |= UCSSEL_2; // Use Small Clock, no error detection - dmx does not include parity bits
 	UCA0CTL1 |= UCRXEIE;
	UCA0CTL0 = UCSPB | UCMODE_1; // DMX Property - two low stop bits
	UCA0BR0 = 32; // DMX operates at 250000 bits/sec soo... 8E6/250000 = 32
	UCA0BR1 = 0; // |||||||||||||||||||||||||||||
	UCA0CTL1 &= ~UCSWRST; // Restart State Machine after init
	IE2 |= UCA0RXIE; // Interrupt on receive

}

void Init() {
	InitPorts();
	InitTimer();
	InitUSCI();
}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void) {
	rx_data = UCA0RXBUF;
	byte_received = 1; // Set indicator variable
	IFG2 &= ~UCA0RXIFG; // Clear flag
}

