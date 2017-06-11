#include "Protocol.h"
#include "BOARD.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    DECODE_WAITING,
    DECODE_RECORDING,
    DECODE_FIRST_CHECKSUM_HALF,
    DECODE_SECOND_CHECKSUM_HALF,
    DECODE_NEWLINE
} DecodeStateMachineStates;

struct ProtocolDecodeStruct {
    DecodeStateMachineStates DecodeSM;
    char Sentence[PROTOCOL_MAX_MESSAGE_LEN];
    uint32_t sentenceIndex;
    uint8_t checksum;
};

//User & Helper Functions
static uint8_t CheckSumFunction(char *dataArray);
static uint8_t ASCIIConvert(char ASCIIValue);
static char *SplitCommas(char *data);

//Static Variables
static struct ProtocolDecodeStruct ProtocolDecodeInfo;

int ProtocolEncodeCooMessage(char *message, const GuessData *data) {
    char dataArray[PROTOCOL_MAX_MESSAGE_LEN];
    sprintf(dataArray, PAYLOAD_TEMPLATE_COO, data->row, data->col);
    //need to do checksum
    uint8_t checkSumOutput = CheckSumFunction(dataArray);
    int length = sprintf(message, MESSAGE_TEMPLATE, dataArray, checkSumOutput);
    return length;
}

int ProtocolEncodeHitMessage(char *message, const GuessData *data) {
    char dataArray[PROTOCOL_MAX_MESSAGE_LEN];
    sprintf(dataArray, PAYLOAD_TEMPLATE_HIT, data->row, data->col, data->hit);
    //need to do checksum
    uint8_t checkSumOutput = CheckSumFunction(dataArray);
    int length = sprintf(message, MESSAGE_TEMPLATE, dataArray, checkSumOutput);
    return length;
}

int ProtocolEncodeChaMessage(char *message, const NegotiationData *data) {
    char dataArray[PROTOCOL_MAX_MESSAGE_LEN];
    sprintf(dataArray, PAYLOAD_TEMPLATE_CHA,
            data->encryptedGuess, data->hash);
    //need to do checksum
    uint8_t checkSumOutput = CheckSumFunction(dataArray);
    int length = sprintf(message, MESSAGE_TEMPLATE, dataArray, checkSumOutput);
    return length;
}

int ProtocolEncodeDetMessage(char *message, const NegotiationData *data) {
    char dataArray[PROTOCOL_MAX_MESSAGE_LEN];
    sprintf(dataArray, PAYLOAD_TEMPLATE_DET,
            data->guess, data->encryptionKey);
    //need to do checksum
    uint8_t checkSumOutput = CheckSumFunction(dataArray);
    int length = sprintf(message, MESSAGE_TEMPLATE, dataArray, checkSumOutput);
    return length;
}

ProtocolParserStatus ProtocolDecode(char in, NegotiationData *nData, GuessData *gData) {
    switch (ProtocolDecodeInfo.DecodeSM) {
        case DECODE_WAITING:
            if (in == '$') {
                ProtocolDecodeInfo.sentenceIndex = 0;
                ProtocolDecodeInfo.DecodeSM = DECODE_RECORDING;
                return PROTOCOL_PARSING_GOOD;
            }
            return PROTOCOL_WAITING;
        case DECODE_RECORDING:
            if (in == '*') {
                ProtocolDecodeInfo.DecodeSM = DECODE_FIRST_CHECKSUM_HALF;
                return PROTOCOL_PARSING_GOOD;
            } else if (in != NULL) {
                ProtocolDecodeInfo.Sentence[ProtocolDecodeInfo.sentenceIndex] = in;
                ProtocolDecodeInfo.sentenceIndex++;
            }
            return PROTOCOL_PARSING_GOOD;
        case DECODE_FIRST_CHECKSUM_HALF:
            if (ASCIIConvert(in) > 0 && ASCIIConvert(in) <= 15) { //ASCII for hex representation
                ProtocolDecodeInfo.checksum &= 0x00;
                uint8_t shiftLeft = ASCIIConvert(in) << 4;
                ProtocolDecodeInfo.checksum |= shiftLeft;
                ProtocolDecodeInfo.DecodeSM = DECODE_SECOND_CHECKSUM_HALF;
                return PROTOCOL_PARSING_GOOD;
            }
            return 0;
        case DECODE_SECOND_CHECKSUM_HALF:
            if (ASCIIConvert(in) > 0 && ASCIIConvert(in) <= 15) { //ASCII for hex representation
                ProtocolDecodeInfo.checksum &= 0xF0;
                ProtocolDecodeInfo.checksum |= ASCIIConvert(in);
                ProtocolDecodeInfo.Sentence[ProtocolDecodeInfo.sentenceIndex] = '\0';
                if (ProtocolDecodeInfo.checksum ==
                        CheckSumFunction(ProtocolDecodeInfo.Sentence)) {
                    ProtocolDecodeInfo.DecodeSM = DECODE_NEWLINE;
                    return PROTOCOL_PARSING_GOOD;
                } else {
                    ProtocolDecodeInfo.DecodeSM = DECODE_WAITING;
                    return PROTOCOL_PARSING_FAILURE;
                }
            }
            return 0;
        case DECODE_NEWLINE:
            if (in == 0x0A) {
                if (!strncmp(PAYLOAD_TEMPLATE_CHA, ProtocolDecodeInfo.Sentence, 3)) {
                    char *tokenized = SplitCommas(ProtocolDecodeInfo.Sentence);
                    tokenized = SplitCommas(ProtocolDecodeInfo.Sentence);
                    nData->encryptedGuess = atof(tokenized);
                    tokenized = SplitCommas(ProtocolDecodeInfo.Sentence);
                    nData->hash = atof(tokenized);
                    ProtocolDecodeInfo.DecodeSM = DECODE_WAITING;
                    return PROTOCOL_PARSED_CHA_MESSAGE;
                } else if (!strncmp(PAYLOAD_TEMPLATE_COO, ProtocolDecodeInfo.Sentence, 3)) {
                    char *tokenized = SplitCommas(ProtocolDecodeInfo.Sentence);
                    tokenized = SplitCommas(ProtocolDecodeInfo.Sentence);
                    gData->row = atof(tokenized);
                    tokenized = SplitCommas(ProtocolDecodeInfo.Sentence);
                    gData->col = atof(tokenized);
                    ProtocolDecodeInfo.DecodeSM = DECODE_WAITING;
                    return PROTOCOL_PARSED_COO_MESSAGE;
                } else if (!strncmp(PAYLOAD_TEMPLATE_DET, ProtocolDecodeInfo.Sentence, 3)) {
                    char *tokenized = SplitCommas(ProtocolDecodeInfo.Sentence);
                    tokenized = SplitCommas(ProtocolDecodeInfo.Sentence);
                    nData->guess = atof(tokenized);
                    tokenized = SplitCommas(ProtocolDecodeInfo.Sentence);
                    nData->encryptionKey = atof(tokenized);
                    ProtocolDecodeInfo.DecodeSM = DECODE_WAITING;
                    return PROTOCOL_PARSED_DET_MESSAGE;
                } else if (!strncmp(PAYLOAD_TEMPLATE_HIT, ProtocolDecodeInfo.Sentence, 3)) {
                    char *tokenized = SplitCommas(ProtocolDecodeInfo.Sentence);
                    tokenized = SplitCommas(ProtocolDecodeInfo.Sentence);
                    gData->row = atof(tokenized);
                    tokenized = SplitCommas(ProtocolDecodeInfo.Sentence);
                    gData->col = atof(tokenized);
                    tokenized = SplitCommas(ProtocolDecodeInfo.Sentence);
                    gData->hit = atof(tokenized);
                    ProtocolDecodeInfo.DecodeSM = DECODE_WAITING;
                    return PROTOCOL_PARSED_HIT_MESSAGE;
                } else {
                    ProtocolDecodeInfo.DecodeSM = DECODE_WAITING;
                    return PROTOCOL_PARSING_FAILURE;
                }
            } else {
                return 0;
            }
    }
}

void ProtocolGenerateNegotiationData(NegotiationData * data) {
    data->guess = rand() & 0xFFFF;
    data->encryptionKey = rand() & 0xFFFF;
    data->encryptedGuess = data->guess ^ data->encryptionKey;
    uint8_t outHash = 0;
    outHash ^= (data->guess & 0xFF00) >> 8;
    outHash ^= (data->encryptionKey & 0xFF00) >> 8;
    outHash ^= data->guess & 0x00FF;
    outHash ^= data->encryptionKey & 0x00FF;
    data->hash = outHash;
}

uint8_t ProtocolValidateNegotiationData(const NegotiationData * data) {
    uint32_t tester = 0;
    tester ^= (data->guess & 0xFF00) >> 8;
    tester ^= (data->encryptionKey & 0xFF00) >> 8;
    tester ^= data->guess & 0x00FF;
    tester ^= data->encryptionKey & 0x00FF;
    if (tester == data->hash)
        return TRUE;
    else
        return FALSE;
}

TurnOrder ProtocolGetTurnOrder(const NegotiationData *myData, const NegotiationData * oppData) {
    uint8_t XORKeys = myData->encryptionKey ^ oppData->encryptionKey;
    XORKeys &= 0x01;
    if (XORKeys) { //If equals 1, greatest first.
        if (myData->guess > oppData->guess)
            return TURN_ORDER_START;
        else if (myData->guess < oppData->guess)
            return TURN_ORDER_DEFER;
        else
            return TURN_ORDER_TIE;
    } else { //If equals 0, lowest first
        if (myData->guess > oppData->guess)
            return TURN_ORDER_DEFER;
        else if (myData->guess < oppData->guess)
            return TURN_ORDER_START;
        else
            return TURN_ORDER_TIE;
    }
}

//CHECKSUM <- a two-digit ASCII checksum that's the XOR of all of the bytes between the $ and *

uint8_t CheckSumFunction(char *dataArray) {
    uint8_t checkSumOutput = 0;
    int index = 0;
    while (dataArray[index] != '\0') {
        checkSumOutput ^= dataArray[index];
        index++;
    }
    return checkSumOutput;
}

uint8_t ASCIIConvert(char ASCIIValue) {
    if (ASCIIValue >= 'a' && ASCIIValue <= 'f')
        return ASCIIValue - 87;
    else if (ASCIIValue >= 'A' && ASCIIValue <= 'F')
        return ASCIIValue - 55;
    else if (ASCIIValue >= '0' && ASCIIValue <= '9')
        return ASCIIValue - 48;
    else
        return STANDARD_ERROR;
}

char *SplitCommas(char *data) {
    if (data[3] == NULL)
        return strtok(NULL, ",");
    return strtok(data, ",");
}