// **** Include libraries here ****
// Standard libraries

//CMPE13 Support Library
#include "BOARD.h"

// Microchip libraries
#include <xc.h>
#include <plib.h>

// User libraries
#include "Buttons.h"
#include "Oled.h"

#include "Protocol.h"

// **** Set any macros or preprocessor directives here ****

// **** Declare any data types here ****

// **** Define any module-level, global, or external variables here ****
static uint32_t counter;
static uint8_t buttonEvents;

// **** Declare any function prototypes here ****

int main() {
    BOARD_Init();

    // Configure Timer 2 using PBCLK as input. We configure it using a 1:16 prescalar, so each timer
    // tick is actually at F_PB / 16 Hz, so setting PR2 to F_PB / 16 / 100 yields a 10ms timer.
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_16, BOARD_GetPBClock() / 16 / 100);

    // Set up the timer interrupt with a medium priority of 4.
    INTClearFlag(INT_T2);
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
    INTSetVectorSubPriority(INT_TIMER_2_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
    INTEnable(INT_T2, INT_ENABLED);

    // Disable buffering on stdout
    setbuf(stdout, NULL);

    ButtonsInit();

    //OledInit();

    //// Prompt the user to start the game and block until the first character press.
    //OledDrawString("Press BTN4 to start.");
    //OledUpdate();
    //while ((buttonEvents & BUTTON_EVENT_4UP) == 0);


    /******************************************************************************
     * Your code goes in between this comment and the following one with asterisks.
     *****************************************************************************/
    NegotiationData Data; //Actual Data
    Data.guess = 1;
    Data.encryptionKey = 2;
    Data.encryptedGuess = 1;
    Data.hash = 0x46;
    NegotiationData nData;
    GuessData gData;
    int length1;
    int length2;
    char *outMessage1;
    char *outMessage2;
    outMessage1 = malloc(PROTOCOL_MAX_MESSAGE_LEN);
    outMessage2 = malloc(PROTOCOL_MAX_MESSAGE_LEN);
    length1 = ProtocolEncodeChaMessage(outMessage1, &Data);
    length2 = ProtocolEncodeDetMessage(outMessage2, &Data);
    int index = 0;
    while (outMessage1[index] != '\0') {
        ProtocolDecode(outMessage1[index], &nData, &gData);
        index++;
    }
    index = 0;
    while (outMessage2[index] != '\0') {
        ProtocolDecode(outMessage2[index], &nData, &gData);
        index++;
    }
    printf("%s length: %d\n", outMessage1, length1);
    printf("%s length: %d\n", outMessage2, length2);
    /******************************************************************************
     * Your code goes in between this comment and the preceeding one with asterisks
     *****************************************************************************/
    while (1);
}

/**
 * This is the interrupt for the Timer2 peripheral. It just keeps incrementing a counter used to
 * track the time until the first user input.
 */
void __ISR(_TIMER_2_VECTOR, IPL4AUTO) TimerInterrupt100Hz(void) {
    // Clear the interrupt flag.
    IFS0CLR = 1 << 8;

    // Increment a counter to see the srand() function.
    counter++;

    // Also check for any button events
    buttonEvents = ButtonsCheckEvents();
}