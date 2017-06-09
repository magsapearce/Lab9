
#include "Agent.h"
#include "Protocol.h"
#include "Field.h"
#include "Oled.h"
#include <GenericTypeDefs.h>

/**
 * The Init() function for an Agent sets up everything necessary for an agent before the game
 * starts. This can include things like initialization of the field, placement of the boats,
 * etc. The agent can assume that stdlib's rand() function has been seeded properly in order to
 * use it safely within.
 */
typedef enum {
    Mine,
    Theirs,
    None
} Turn;

struct AgentData {
    NegotiationData AgentNegData; //Agent's Negotiation Data
    NegotiationData OppNegData; //Opposition's Negotiation Data
    GuessData AgentGuessData; //Agent's Guess Data
    GuessData OppGuessData; //Opposition's Guess Data
    AgentState CurrAgentState; //Agent Run States
    ProtocolParserStatus ParserStatus; //Current Parser State Machine States
    Turn GameTurn; //CurrentTurn
};

static struct AgentData AgentData;
static Field fieldUse;
static Field enemyField;
void ParsingFailed();

void AgentInit(void) {
    //Initialize the field

    FieldPosition p = FIELD_POSITION_EMPTY;
    FieldInit(&fieldUse, p);
    //place the boats randomly
    //uint8_t FieldAddBoat(Field *f, uint8_t row, uint8_t col, BoatDirection dir, BoatType type)
    BoatDirection randDirection;
    uint8_t randRow; // random between 0 and 6
    uint8_t randCol;
    //small boat
    uint8_t pass = 0;
    for (; pass == FALSE;) {
        randDirection = rand() % 4; //to get 0-3, North, east, south,west,respectively
        randRow = rand() % FIELD_ROWS; // random between 0 and 6
        randCol = rand() % FIELD_COLS;
        pass = FieldAddBoat(&fieldUse, randRow, randCol, randDirection, FIELD_BOAT_SMALL);
    }
    //medium boat
    pass = FALSE;
    for (; pass == FALSE;) {
        randDirection = rand() % 4; //to get 0-3, North, east, south,west,respectively
        randRow = rand() % FIELD_ROWS; // random between 0 and 6
        randCol = rand() % FIELD_COLS;
        pass = FieldAddBoat(&fieldUse, randRow, randCol, randDirection, FIELD_BOAT_MEDIUM);
    }
    //large boat
    pass = FALSE;
    for (; pass == FALSE;) {
        randDirection = rand() % 4; //to get 0-3, North, east, south,west,respectively
        randRow = rand() % FIELD_ROWS; // random between 0 and 6
        randCol = rand() % FIELD_COLS;
        pass = FieldAddBoat(&fieldUse, randRow, randCol, randDirection, FIELD_BOAT_LARGE);
    }
    //huge boat
    pass = FALSE;
    for (; pass == FALSE;) {
        randDirection = rand() % 4; //to get 0-3, North, east, south,west,respectively
        randRow = rand() % FIELD_ROWS; // random between 0 and 6
        randCol = rand() % FIELD_COLS;
        pass = FieldAddBoat(&fieldUse, randRow, randCol, randDirection, FIELD_BOAT_HUGE);
    }
    AgentData.CurrAgentState = AGENT_STATE_GENERATE_NEG_DATA;
}

/**
 * The Run() function for an Agent takes in a single character. It then waits until enough
 * data is read that it can decode it as a full sentence via the Protocol interface. This data
 * is processed with any output returned via 'outBuffer', which is guaranteed to be 255
 * characters in length to allow for any valid NMEA0183 messages. The return value should be
 * the number of characters stored into 'outBuffer': so a 0 is both a perfectly valid output and
 * means a successful run.
 * @param in The next character in the incoming message stream.
 * @param outBuffer A string that should be transmit to the other agent. NULL if there is no
 *                  data.
 * @return The length of the string pointed to by outBuffer (excludes \0 character).
 */
int AgentRun(char in, char *outBuffer) {
    switch (AgentData.CurrAgentState) {
        case AGENT_STATE_GENERATE_NEG_DATA:
            ProtocolGenerateNegotiationData(&AgentData.AgentNegData);
            AgentData.CurrAgentState = AGENT_STATE_SEND_CHALLENGE_DATA;
            return ProtocolEncodeChaMessage(outBuffer, &AgentData.AgentNegData);
        case AGENT_STATE_SEND_CHALLENGE_DATA:
            AgentData.ParserStatus = ProtocolDecode(in, &AgentData.OppNegData,
                    &AgentData.OppGuessData);
            if (AgentData.ParserStatus == PROTOCOL_PARSED_CHA_MESSAGE) {
                AgentData.CurrAgentState = AGENT_STATE_DETERMINE_TURN_ORDER;
                return ProtocolEncodeDetMessage(outBuffer, &AgentData.AgentNegData);
            } else if (AgentData.ParserStatus == PROTOCOL_PARSING_FAILURE) {
                ParsingFailed(); //Invalid if parsing failed
                return 0;
            } else
                return 0;
        case AGENT_STATE_DETERMINE_TURN_ORDER:
            AgentData.ParserStatus = ProtocolDecode(in, &AgentData.OppNegData,
                    &AgentData.OppGuessData);
            if (AgentData.ParserStatus == PROTOCOL_PARSED_DET_MESSAGE) {
                if (ProtocolValidateNegotiationData(&AgentData.OppNegData)) {
                    //If Validate Fails go to else (== FALSE/0)
                    TurnOrder Order;
                    Order = ProtocolGetTurnOrder(&AgentData.AgentNegData,
                            &AgentData.OppNegData); //Get Order
                    switch (Order) {
                        case TURN_ORDER_TIE:
                            AgentData.CurrAgentState = AGENT_STATE_INVALID;
                            OledClear(0);
                            OledDrawString(AGENT_ERROR_STRING_ORDERING);
                            return 0;
                        case TURN_ORDER_START:
                            AgentData.GameTurn = Mine;
                            AgentData.CurrAgentState = AGENT_STATE_SEND_GUESS;
                            OledUpdate();
                            return 0;
                        case TURN_ORDER_DEFER:
                            AgentData.GameTurn = Theirs;
                            AgentData.CurrAgentState = AGENT_STATE_WAIT_FOR_GUESS;
                            OledUpdate();
                            return 0;
                        default:
                            AgentData.CurrAgentState = AGENT_STATE_INVALID;
                            OledClear(0);
                            OledDrawString(AGENT_ERROR_STRING_ORDERING);
                            return 0;
                    }
                } else {
                    AgentData.CurrAgentState = AGENT_STATE_INVALID;
                    OledClear(0);
                    OledDrawString(AGENT_ERROR_STRING_NEG_DATA);
                    return 0;
                }
            } else if (AgentData.ParserStatus == PROTOCOL_PARSING_FAILURE) {
                ParsingFailed();
                return 0;
            } else
                return 0;
        case AGENT_STATE_SEND_GUESS:
            AgentData.AgentGuessData.row = rand() % FIELD_ROWS;
            AgentData.AgentGuessData.col = rand() % FIELD_COLS;
            ProtocolEncodeCooMessage(AgentData.AgentGuessData);
            return 0;
        case AGENT_STATE_WAIT_FOR_GUESS:
            AgentData.ParserStatus = ProtocolDecode(in, &AgentData.OppNegData,
                    &AgentData.OppGuessData);
            if (AgentData.ParserStatus == PROTOCOL_PARSED_COO_MESSAGE) {
                break;
            } else if (AgentData.ParserStatus == PROTOCOL_PARSING_FAILURE) {
                ParsingFailed();
                return 0;
            } else
                return 0;
            //            if (COOmessage && (AgentGetStatus() != 0)) {
            //                AgentData.GameTurn == Mine;
            //                FieldRegisterEnemyAttack(&fieldUse, &gData);
            //                OledUpdate();
            //                return ProtocolEncodeHitMessage(outBuffer, AgentData.OppGuessData)
            //            } else if (COOmessage && (AgentGetStatus() == 0)) {
            //                //set turn to none
            //                OledUpdate();
            //                //send HIT message
            //            }
        case AGENT_STATE_WAIT_FOR_HIT:
            AgentData.ParserStatus = ProtocolDecode(in, &AgentData.OppNegData,
                    &AgentData.OppGuessData);
            if (AgentData.ParserStatus == PROTOCOL_PARSED_HIT_MESSAGE) {
                switch (AgentData.OppGuessData.hit) {
                    case FIELD_POSITION_EMPTY:
                        //Note FIELD POS UNKNOWN
                        break;
                    case FIELD_POSITION_SMALL_BOAT:
                        //Note Boat type here, cont
                        break;
                    case FIELD_POSITION_MEDIUM_BOAT:
                        //Note boat
                        break;
                    case FIELD_POSITION_LARGE_BOAT:
                        break;
                    case FIELD_POSITION_HUGE_BOAT:
                        break;
                    default:
                        break;
                }
            } //CHECK GAME WIN HERE WITH IF ELSE NEST
//            AgentData.GameTurn = Theirs;
//            OledUpdate();
//            return 0; 
            else if (AgentData.ParserStatus == PROTOCOL_PARSING_FAILURE) {
                ParsingFailed();
                return 0;
            } else return 0;
        case AGENT_STATE_WON:
            break;
            //            //this is your turn
            //            if (HITmessage && (AgentGetEnemyStatus() == 0)) {//HIT message&&no enemy boats left
            //                //set turn to none
            //                OledUpdate();
            //            } else if (HITmessage && (AgentGetEnemyStatus != 0)) {
            //                FieldUpdateKnowledge(&enemyField, &gData);
            //                //set turn to theirs
            //                OledUpdate();
            //            }
        default:
            break;
    }

    //to go to invslid state, message parsing failed: then so OledClear(OLED_COLOR_BLACK); 
    //OledDrawString(ERROR<STRING_PARSING)<- maybe?
    //
    //anytime to update screen, do update OLED
    //from dot to Invalid branch, OledClear and then put in the error
    //TO CHECK FRIENDLY BOATS LEFT:::: call AgentGetStatus()=variable if variable==0, you have no more boats
    //TO CHECK ENEMY BOATS:::: call AgentGetEnemyStatus()=variable if==0, they have no more boats
    //TO register hit on you : FieldRegisterEnemyAttack(Field *field, GuessData *gData );
    //TO register hit on them: FieldUpdateKnowledge(Field *f, const GuessData *gData);


}

/**
 * StateCheck() returns a 4-bit number indicating the status of that agent's ships. The smallest
 * ship, the 3-length one, is indicated by the 0th bit, the medium-length ship (4 tiles) is the
 * 1st bit, etc. until the 3rd bit is the biggest (6-tile) ship. This function is used within
 * main() to update the LEDs displaying each agents' ship status. This function is similar to
 * Field::FieldGetBoatStates().
 * @return A bitfield indicating the sunk/unsunk status of each ship under this agent's control.
 *
 * @see Field.h:FieldGetBoatStates()
 * @see Field.h:BoatStatus
 */

uint8_t AgentGetStatus(void) {
    uint8_t boats, huge, large, medium, small;
    if (fieldUse.hugeBoatLives == 0) {
        huge = 0;
    } else {
        huge = FIELD_BOAT_STATUS_HUGE;
    }
    if (fieldUse.largeBoatLives == 0) {
        large = 0;
    } else {
        large = FIELD_BOAT_STATUS_LARGE;
    }
    if (fieldUse.mediumBoatLives == 0) {
        medium = 0;
    } else {
        medium = FIELD_BOAT_STATUS_MEDIUM;
    }
    if (fieldUse.smallBoatLives == 0) {
        small = 0;
    } else {
        small = FIELD_BOAT_STATUS_SMALL;
    }
    boats = small | medium | large | huge;
    return boats;
}

/**
 * This function returns the same data as `AgentCheckState()`, but for the enemy agent.
 * @return A bitfield indicating the sunk/unsunk status of each ship under the enemy agent's
 *         control.
 *
 * @see Field.h:FieldGetBoatStates()
 * @see Field.h:BoatStatus
 */
uint8_t AgentGetEnemyStatus(void) {
    uint8_t boats, huge, large, medium, small;
    if (enemyField.hugeBoatLives == 0) {
        huge = 0;
    } else {
        huge = FIELD_BOAT_STATUS_HUGE;
    }
    if (enemyField.largeBoatLives == 0) {
        large = 0;
    } else {
        large = FIELD_BOAT_STATUS_LARGE;
    }
    if (enemyField.mediumBoatLives == 0) {
        medium = 0;
    } else {
        medium = FIELD_BOAT_STATUS_MEDIUM;
    }
    if (enemyField.smallBoatLives == 0) {
        small = 0;
    } else {
        small = FIELD_BOAT_STATUS_SMALL;
    }
    boats = small | medium | large | huge;
    return boats;
}

void ParsingFailed() {
    AgentData.CurrAgentState = AGENT_STATE_INVALID;
    OledClear(0);
    OledDrawString(AGENT_ERROR_STRING_ORDERING);
}
