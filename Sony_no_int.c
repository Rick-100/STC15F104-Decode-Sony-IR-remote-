/*
decode Sony Ir remote commands
uses the hello_num_1.c program I previously posted as a base
receives the Sony commands through a ir receiver hooked to P3.2
doesn't use any interrupts but does use timer 0 in 16 bit mode
uses the timer like a stop watch to measure the low pulses output by the ir receiver / each count is 1 micro second
the output of the receiver is low when a carrier signal is present
the program is very basic and just waits for a start pulse between 2200 and 2400 micro seconds
then it gets the next 12 low pulses and sets the bits of the bitPattern variable
if the pulse is > LOGIC1 then the bit is set to 1 otherwise it is left at 0

it prints the data out the serial port


uses the utility functions for printing numbers to P3.1 at 2400 baud with 12 MHz clock

*/

#include "my_stc_.h"	// converted header file

/*
Windows bat file for compiling / put in same directory as my_stc_.h and this source file

sdcc -mmcs51  --iram-size 128 --xram-size 0 --code-size 4088 --verbose hello_num_1.c -o hello_num_1.ihx
packihx hello_num_1.ihx > hello_num_1.hex
pause

 */

#define led  P3_3
#define txLine  P3_1
#define irRec P3_2


#define TX_HIGH txLine = 1
#define TX_LOW txLine = 0

#define STARTMIN 2200	//start pulse shortest pulse length in micro seconds / 2400 is spec
#define STARTMAX 2600	//start pulse longest pulse length 
#define LOGIC1 900	//logic 1 pulse length in micro seconds / 1200 is spec 

//function protypes
void sendChar(unsigned char c);
void sendIntAsDec(unsigned int number);
void sendCRLF(void);
unsigned char getAddress(unsigned int sonyVal);
unsigned char getCommand(unsigned int sonyVal);



unsigned int getLowPulseLength(void)
{
	
	while(!irRec){}	//if line is already low wait until it's high
	while(irRec){}	//wait for line to go low / start of low pulse
	
	TH0 = 0;	// Reset timer
	TL0 = 0;
	TF0 = 0;	//reset overflow / probably unnecessary
	TR0 = 1;	//start timer
	
	while(!irRec){}	//wait for line to go high / end of low pulse
	
	TR0 = 0;	//stop timer

	return TL0 + (TH0 << 8);
	
}


unsigned int getIrCode(void)
{
	unsigned int pulseLength;
	unsigned int bitPattern = 0;
	unsigned char state;
	
	do
	{
		pulseLength = getLowPulseLength();
	}while(!((pulseLength > STARTMIN) && (pulseLength < STARTMAX)));	//wait for start bit

	for(state = 0;state < 12;state++)
	{
		pulseLength = getLowPulseLength();
		if(pulseLength > LOGIC1)
		{
			bitPattern |= 1 << state  ;
		}

	}
	return bitPattern;
}



void sendComAdd(unsigned int sonyCode)	// extract 5 bit address and send as decimal / then send 7 bit command as decimal
{

	sendChar('A');
	sendChar('=');
	sendIntAsDec((unsigned char)getAddress(sonyCode));
	sendCRLF();
	sendChar('C');
	sendChar('=');
	sendIntAsDec((unsigned char)getCommand(sonyCode));
	sendCRLF();
	sendCRLF();

	
}
unsigned char getAddress(unsigned int sonyVal)
{
	return (unsigned char) (sonyVal >> 7);	//shift out the 7 command bits leaving the 5 address bits *********** brackets  are important
}

unsigned char getCommand(unsigned int sonyVal)
{
	return (unsigned char)(sonyVal & 0x7f);	//just return the 7 address bits
}



void setPortMode(unsigned char port,unsigned char bitNum, unsigned char mode)	// only works on ports 0, 3 for now
{
	unsigned char tmp1 = 0;
	unsigned char tmp2 = 0;
	unsigned char tmp3 = 0;
	unsigned char tmp4 = 0;
	
		if(mode & 0x01){
			tmp1 = 1 << bitNum;
		}
		if(mode & 0x02){
			tmp2 = 1 << bitNum;
		}

	switch(port){
		case 1 :
			tmp3 = (P1M0 & ~tmp1) | tmp1;
			tmp4 = (P1M1 & ~tmp1) | tmp2;

			P1M0 = tmp3;
			P1M1 = tmp4;
			break;
		case 3 :
			tmp3 = (P3M0 & ~tmp1) | tmp1;
			tmp4 = (P3M1 & ~tmp1) | tmp2;

			P3M0 = tmp3;
			P3M1 = tmp4;
			break;
	}	
	
}



void Delay2400(){	// 1 bit time for 2400 baud at 12 MHz
	__asm
		push 0x30
		push 0x31
		mov 0x30,#4
		mov 0x31,#220
NEXT:
		djnz 0x31,NEXT
		djnz 0x30,NEXT
		pop 0x31
		pop 0x30
	__endasm;
}


void sendChar(unsigned char c)	//send an ASCII character
{
	unsigned char mask = 1;	//bit mask
	unsigned char i;
         
	Delay2400();// wait 2 Stop bits before sending the char to give a stop bit if routine is called again before a stop bit time period has passed
	Delay2400();

	TX_LOW;              // low the line for start bit

	Delay2400();	//wait 1 bit time for start bit

	for (i=0; i<8 ;i++){
		if (c & mask){
			TX_HIGH;
		}
		else{
			TX_LOW;
		}
		mask <<= 1;
		Delay2400();
	}
	TX_HIGH;            //Return TXDATA pin to "1".
}



void sendByteAsHex(unsigned char b)	// send byte as 2 digit hex number
{
	unsigned char digitToDisplay;
	unsigned char byteToDisplay;

	byteToDisplay = b;
	digitToDisplay = byteToDisplay >> 4;
	if(digitToDisplay < 10){
		digitToDisplay = digitToDisplay + 48;
		sendChar(digitToDisplay);
	}
	else{
		digitToDisplay = digitToDisplay + 55;
		sendChar(digitToDisplay);			
	}

	digitToDisplay = byteToDisplay & 0xf;
	if(digitToDisplay < 10){
		digitToDisplay = digitToDisplay + 48;
		sendChar(digitToDisplay);
	}
	else{
		digitToDisplay = digitToDisplay + 55;
		sendChar(digitToDisplay);			
	}	

}

void sendIntAsDec(unsigned int number)	//print unsigned int as decimal number
{
	unsigned int divider = 10000;
	unsigned char zero = 0;
	
	unsigned int n;
	if(number == 0)
	{
		sendChar('0');
		return;		// bust out of function
	}
	for(n = 0;n < 5; n++)
	{
		if(number >= divider)
			{
			zero = 1;
			sendChar((number / divider) + 48);
			number %= divider;	//get remainder

			}
		else if(zero)
			{
			sendChar('0');
			}
	divider /= 10;
	}
}


void sendCRLF(void)
{
	sendChar(13);
	sendChar(10);
}

void sendIntAsHex(unsigned int out_data)	//send an 
{

    sendByteAsHex((unsigned char)(out_data >> 8));
    sendByteAsHex((unsigned char)(out_data));
}
void sendByteAsBin(unsigned char number)
{
	unsigned char bitNum;
	unsigned char mask = 128;
	
	for(bitNum = 8;bitNum > 0;bitNum--)
	{
		if(number & mask) 
		{
			sendChar('1');
		}
		else{
			sendChar('0');
		}
		mask >>= 1;
	}
}

void sendIntAsBin(unsigned int intVal)	// sents 16 bit integer as binary with space between bytes
{
	unsigned char temp;
	
	temp = (unsigned char)(intVal >> 8);
	sendByteAsBin(temp);
	sendChar(' ');
	temp = (unsigned char)(intVal & 0xff);
	sendByteAsBin(temp);

	
}

void main()
{

	//unsigned char temp;
	unsigned int irValue;
	unsigned char comVal;
	unsigned char addVal;
	
	
	// if line is commented out the port will be in default mode 0
	//setPortMode(3,3,1);		//this should make bit 3 on P3 a push pull output
	//setPortMode(3,4,1);		//this should make bit 4 on P3 a push pull output
	//setPortMode(3,5,1);		//this should make bit 5 on P3 a push pull output
	led = 0;

	sendChar('H');
	sendChar('I');
	sendCRLF();
	
/* 	
	
	sendByteAsHex(temp);
	sendIntAsDec(miscCtr);
	sendIntAsHex(0x00ff);
 */
	
    while(1)
    {
		irValue = getIrCode();
		//led = !led;	//toggle led
		sendIntAsDec(irValue);
		sendCRLF();
		sendIntAsBin(irValue);
		sendCRLF();
		sendComAdd(irValue);
		
		comVal = getCommand(irValue);
		addVal = getAddress(irValue);
		
		if((comVal == 0) && (addVal == 1 ))	//key 1
		{
			led = 1;	//off
		}
		if((comVal == 9) && (addVal == 1 ))	//key 0
		{
			led = 0;	//on
		}		
		//temp = P3;
		//sendByteAsBin(P3);	// print the port pin states as binary
		//sendCRLF();
    }		
}	  