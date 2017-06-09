
#include "Agent.h"
#include "Protocol.h"
#include "Field.h"
#include "Oled.h"
#include "FieldOled.h"
#include <GenericTypeDefs.h>

/**
 * The Init() function for an Agent sets up everything necessary for an agent before the game
 * starts. This can include things like initialization of the field, placement of the boats,
 * etc. The agent can assume that stdlib's rand() function has been seeded properly in order to
 * use it safely within.
 */

struct AgentData {
    NegotiationData AgentNegData; //Agent's Negotiation Data
    NegotiationData OppNegData; //Opposition's Negotiation Data
    GuessData AgentGuessData; //Guess Data of Both Sides
    AgentState CurrAgentState; //Agent Run States
    ProtocolParserStatus ParserStatus; //Current Parser State Machine States
    FieldOledTurn GameTurn; //CurrentTurn
};

struct FieldData {
    Field AgentField;
    Field OppField;
};

static struct AgentData AgentData;
static struct FieldData FieldData;
void ParsingFailed();

void AgentInit(void)
{
    //Initialize the field

    FieldPosition p = FIELD_POSITION_EMPTY;
    FieldInit(&FieldData.AgentField, p);
    //place the boats randomly
    //uint8_t FieldAddBoat(Field *f, uint8_t row, uint8_t col, BoatDirection dir, BoatType type)
    BoatDirection randDirection;
    uint8_t randRow; // random between 0 and 6
    uint8_t randCol;
    //small boat
    uint8_t pass = 0;
    while (pass == FALSE) {
        randDirection = rand() % 4; //to get 0-3, North, east, south,west,respectively
        randRow = rand() % FIELD_ROWS; // random between 0 and 6
        randCol = rand() % FIELD_COLS;
        pass = FieldAddBoat(&FieldData.AgentField, randRow, randCol, randDirection, FIELD_BOAT_SMALL);
    }
    //medium boat
    pass = FALSE;
    while (pass == FALSE) {
        randDirection = rand() % 4; //to get 0-3, North, east, south,west,respectively
        randRow = rand() % FIELD_ROWS; // random between 0 and 6
        randCol = rand() % FIELD_COLS;
        pass = FieldAddBoat(&FieldData.AgentField, randRow, randCol, randDirection, FIELD_BOAT_MEDIUM);
    }
    //large boat
    pass = FALSE;
    while (pass == FALSE) {
        randDirection = rand() % 4; //to get 0-3, North, east, south,west,respectively
        randRow = rand() % FIELD_ROWS; // random between 0 and 6
        randCol = rand() % FIELD_COLS;
        pass = FieldAddBoat(&FieldData.AgentField, randRow, randCol, randDirection, FIELD_BOAT_LARGE);
    }
    //huge boat
    pass = FALSE;
    while (pass == FALSE) {
        randDirection = rand() % 4; //to get 0-3, North, east, south,west,respectively
        randRow = rand() % FIELD_ROWS; // random between 0 and 6
        randCol = rand() % FIELD_COLS;
        pass = FieldAddBoat(&FieldData.AgentField, randRow, randCol, randDirection, FIELD_BOAT_HUGE);
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
int AgentRun(char in, char *outBuffer)
{
    switch (AgentData.CurrAgentState) {
    case AGENT_STATE_GENERATE_NEG_DATA:
        ProtocolGenerateNegotiationData(&AgentData.AgentNegData);
        AgentData.CurrAgentState = AGENT_STATE_SEND_CHALLENGE_DATA;
        return ProtocolEncodeChaMessage(outBuffer, &AgentData.AgentNegData);
    case AGENT_STATE_SEND_CHALLENGE_DATA:
        AgentData.ParserStatus = ProtocolDecode(in, &AgentData.OppNegData,
                &AgentData.AgentGuessData);
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
                &AgentData.AgentGuessData);
        if (AgentData.ParserStatus == PROTOCOL_PARSED_DET_MESSAGE) {
            if (ProtocolValidateNegotiationData(&AgentData.OppNegData)) {
                TurnOrder Order;
                Order = ProtocolGetTurnOrder(&AgentData.AgentNegData,
                        &AgentData.OppNegData); //Get Order
                switch (Order) {
                case TURN_ORDER_TIE:
                    AgentData.CurrAgentState = AGENT_STATE_INVALID;
                    OledClear(0);
                    OledDrawString(AGENT_ERROR_STRING_ORDERING);
                    OledUpdate();
                    return 0;
                case TURN_ORDER_START:
                    AgentData.GameTurn = FIELD_OLED_TURN_MINE;
                    AgentData.CurrAgentState = AGENT_STATE_SEND_GUESS;
                    FieldOledDrawScreen(&FieldData.AgentField, &FieldData.OppField
                            , AgentData.GameTurn);
                    OledUpdate();
                    return 0;
                case TURN_ORDER_DEFER:
                    AgentData.GameTurn = FIELD_OLED_TURN_THEIRS;
                    AgentData.CurrAgentState = AGENT_STATE_WAIT_FOR_GUESS;
                    FieldOledDrawScreen(&FieldData.AgentField, &FieldData.OppField,
                            AgentData.GameTurn);
                    OledUpdate();
                    return 0;
                default:
                    AgentData.CurrAgentState = AGENT_STATE_INVALID;
                    OledClear(0);
                    OledDrawString(AGENT_ERROR_STRING_ORDERING);
                    OledUpdate();
                    return 0;
                }
            } else { //If Validate Fails
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
        while (1) {
            AgentData.AgentGuessData.row = rand() % FIELD_ROWS;
            AgentData.AgentGuessData.col = rand() % FIELD_COLS;
            if (FieldAt(&FieldData.OppField, AgentData.AgentGuessData.row,
                    AgentData.AgentGuessData.col) == FIELD_POSITION_UNKNOWN)
                break; //Break if random guess hasn't been chosen before
        }
        AgentData.CurrAgentState = AGENT_STATE_WAIT_FOR_HIT;
        return ProtocolEncodeCooMessage(outBuffer, &AgentData.AgentGuessData);
    case AGENT_STATE_WAIT_FOR_GUESS:
        AgentData.ParserStatus = ProtocolDecode(in, &AgentData.OppNegData,
                &AgentData.AgentGuessData);
        if (AgentData.ParserStatus == PROTOCOL_PARSED_COO_MESSAGE) {
            if (FieldAt(&FieldData.AgentField, AgentData.AgentGuessData.row,
                    AgentData.AgentGuessData.col) == FIELD_POSITION_EMPTY ||
                    FIELD_POSITION_MISS) {
                AgentData.AgentGuessData.hit = HIT_MISS;
            } else {

                FieldRegisterEnemyAttack(&FieldData.AgentField, &AgentData.AgentGuessData);
                // Register Hits on yourself
                if (AgentData.AgentGuessData.hit == HIT_SUNK_SMALL_BOAT) {
                    //If ship was sunk, AgentData.OppGuessData.hit = X sunk. 
                    AgentData.OppGuessData.hit = HIT_SUNK_SMALL_BOAT;
                } else if (AgentData.AgentGuessData.hit == HIT_SUNK_MEDIUM_BOAT) {
                    AgentData.OppGuessData.hit = HIT_SUNK_MEDIUM_BOAT;
                } else if (AgentData.AgentGuessData.hit == HIT_SUNK_LARGE_BOAT) {
                    AgentData.OppGuessData.hit = HIT_SUNK_LARGE_BOAT;
                } else if (AgentData.AgentGuessData.hit == HIT_SUNK_HUGE_BOAT) {
                    AgentData.OppGuessData.hit = HIT_SUNK_HUGE_BOAT;
                } else {
                    AgentData.AgentGuessData.hit = HIT_HIT;
                }
                //If ship was only hit, AgentData.OppGuessData.hit = HIT_HIT
                FieldOledDrawScreen(&FieldData.AgentField, &FieldData.OppField,
                        AgentData.GameTurn);
                OledUpdate();
                //Register hit on map here
            }
            if (AgentGetStatus()) { //If Not Zero
                AgentData.GameTurn = FIELD_OLED_TURN_MINE;
                FieldOledDrawScreen(&FieldData.AgentField, &FieldData.OppField,
                        AgentData.GameTurn);
                OledUpdate();
                return ProtocolEncodeHitMessage(outBuffer, &AgentData.AgentGuessData);
            } else { //Lost Game
                AgentData.GameTurn = FIELD_OLED_TURN_NONE;
                //Update Field Knowledge
                FieldOledDrawScreen(&FieldData.AgentField, &FieldData.OppField,
                        AgentData.GameTurn);
                OledUpdate();
                AgentData.CurrAgentState = AGENT_STATE_LOST;
                return ProtocolEncodeHitMessage(outBuffer, &AgentData.AgentGuessData);
            }
        } else if (AgentData.ParserStatus == PROTOCOL_PARSING_FAILURE) {
            ParsingFailed();
            return 0;
        } else
            return 0;
    case AGENT_STATE_WAIT_FOR_HIT:
        AgentData.ParserStatus = ProtocolDecode(in, &AgentData.OppNegData,
                &AgentData.AgentGuessData);
        if (AgentData.ParserStatus == PROTOCOL_PARSED_HIT_MESSAGE) {
            switch (AgentData.AgentGuessData.hit) {
            case HIT_MISS:
                //Note FIELD POS AS MISS
                break;
            case HIT_HIT:
                //Note FIELD POS AS HIT
                break;
            case HIT_SUNK_SMALL_BOAT:
                //Note BOATS...
                break;
            case HIT_SUNK_MEDIUM_BOAT:
                break;
            case HIT_SUNK_LARGE_BOAT:
                break;
            case HIT_SUNK_HUGE_BOAT:
                break;
            }
            if (AgentGetEnemyStatus()) { //If Not Zero
                AgentData.GameTurn = FIELD_OLED_TURN_MINE;
                FieldOledDrawScreen(&FieldData.AgentField, &FieldData.OppField,
                        AgentData.GameTurn);
                OledUpdate();
                return ProtocolEncodeHitMessage(outBuffer, &AgentData.AgentGuessData);
            } else { //Won Game
                AgentData.GameTurn = FIELD_OLED_TURN_NONE;
                FieldOledDrawScreen(&FieldData.AgentField, &FieldData.OppField,
                        AgentData.GameTurn);
                OledUpdate();
                AgentData.CurrAgentState = AGENT_STATE_WON;
                return ProtocolEncodeHitMessage(outBuffer, &AgentData.AgentGuessData);
            }
        } else if (AgentData.ParserStatus == PROTOCOL_PARSING_FAILURE) {
            ParsingFailed();
            return 0;
        } else
            return 0;
    case AGENT_STATE_WON:
        break;
    case AGENT_STATE_LOST:
        break;
    case AGENT_STATE_INVALID:
        break;
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

uint8_t AgentGetStatus(void)
{
    uint8_t boats, huge, large, medium, small;
    if (FieldData.AgentField.hugeBoatLives == 0) {
        huge = 0;
    } else {
        huge = FIELD_BOAT_STATUS_HUGE;
    }
    if (FieldData.AgentField.largeBoatLives == 0) {
        large = 0;
    } else {
        large = FIELD_BOAT_STATUS_LARGE;
    }
    if (FieldData.AgentField.mediumBoatLives == 0) {
        medium = 0;
    } else {
        medium = FIELD_BOAT_STATUS_MEDIUM;
    }
    if (FieldData.AgentField.smallBoatLives == 0) {
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
uint8_t AgentGetEnemyStatus(void)
{
    uint8_t boats, huge, large, medium, small;
    if (FieldData.OppField.hugeBoatLives == 0) {
        huge = 0;
    } else {
        huge = FIELD_BOAT_STATUS_HUGE;
    }
    if (FieldData.OppField.largeBoatLives == 0) {
        large = 0;
    } else {
        large = FIELD_BOAT_STATUS_LARGE;
    }
    if (FieldData.OppField.mediumBoatLives == 0) {
        medium = 0;
    } else {
        medium = FIELD_BOAT_STATUS_MEDIUM;
    }
    if (FieldData.OppField.smallBoatLives == 0) {
        small = 0;
    } else {
        small = FIELD_BOAT_STATUS_SMALL;
    }
    boats = small | medium | large | huge;
    return boats;
}

void ParsingFailed()
{
    AgentData.CurrAgentState = AGENT_STATE_INVALID;
    OledClear(0);
    OledDrawString(AGENT_ERROR_STRING_ORDERING);
    OledUpdate();
}
