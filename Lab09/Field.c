#include "Field.h"
#include <GenericTypeDefs.h>


void FieldInit(Field *f, FieldPosition p)
{//set all f to p, and f points to a 10x6 array
    //do two for loops, one for rows then for columns
    int i, j;
    for (i = 0; i < FIELD_ROWS; i++) {
        for (j = 0; j < FIELD_COLS; j++) {
            f->field[i][j] = p;
        }
    }
    f->hugeBoatLives = FIELD_BOAT_LIVES_HUGE;
    f->largeBoatLives = FIELD_BOAT_LIVES_LARGE;
    f->mediumBoatLives = FIELD_BOAT_LIVES_MEDIUM;
    f->smallBoatLives = FIELD_BOAT_LIVES_SMALL;
}


FieldPosition FieldAt(const Field *f, uint8_t row, uint8_t col)
{
    FieldPosition spot;
    spot = f->field[row][col];
    return spot;
}


FieldPosition FieldSetLocation(Field *f, uint8_t row, uint8_t col, FieldPosition p)
{
    FieldPosition oldValue;
    oldValue = f->field[row][col];
    f->field[row][col] = p; //set this now equal to the new value
    return oldValue;
}


uint8_t FieldAddBoat(Field *f, uint8_t row, uint8_t col, BoatDirection dir, BoatType type)
{
    int i;
    if (f->field[row][col] != FIELD_POSITION_EMPTY) {
        return FALSE;
    }
    if((dir==FIELD_BOAT_DIRECTION_EAST)&&((col+type+3)>FIELD_COLS)){
        // if it is pointing east, then col+type+3<FIELD_COLS
        return FALSE;
    }
    if((dir==FIELD_BOAT_DIRECTION_WEST)&&((col-type-3)<0)){
        // if it is pointing west
        return FALSE;
    }
    if((dir==FIELD_BOAT_DIRECTION_NORTH)&&((row+type+3)>FIELD_ROWS)){
        // if it is pointing north
        return FALSE;
    }
    if((dir==FIELD_BOAT_DIRECTION_SOUTH)&&((row-type-3)>FIELD_ROWS)){
        // if it is pointing south
        return FALSE;
    }
    if (dir == FIELD_BOAT_DIRECTION_NORTH) {//if pointed north
        for (i = 0; i < (type + 3); i++) {//length of boat
            if ((row + i) > FIELD_ROWS) {
                return FALSE;
            }
            if (f->field[row + i][col] != FIELD_POSITION_EMPTY) {
                return FALSE;
            }
            if (type == FIELD_BOAT_SMALL) {
                f->field[row + i][col] = FIELD_POSITION_SMALL_BOAT;
            } else if (type == FIELD_BOAT_MEDIUM) {
                f->field[row + i][col] = FIELD_POSITION_MEDIUM_BOAT;
            } else if (type == FIELD_BOAT_LARGE) {
                f->field[row + i][col] = FIELD_POSITION_LARGE_BOAT;
            } else if (type == FIELD_BOAT_HUGE) {
                f->field[row + i][col] = FIELD_POSITION_HUGE_BOAT;
            } else {
                return FALSE;
            }

        }
        return TRUE;
    } else if (dir == FIELD_BOAT_DIRECTION_SOUTH) {//pointed South
        for (i = 0; i < (type + 3); i++) {//length of boat
            if ((row - i) < 0) {
                return FALSE;
            }
            if (f->field[row - i][col] != FIELD_POSITION_EMPTY) {
                return FALSE;
            }
            if (type == FIELD_BOAT_SMALL) {
                f->field[row - i][col] = FIELD_POSITION_SMALL_BOAT;
            } else if (type == FIELD_BOAT_MEDIUM) {
                f->field[row - i][col] = FIELD_POSITION_MEDIUM_BOAT;
            } else if (type == FIELD_BOAT_LARGE) {
                f->field[row - i][col] = FIELD_POSITION_LARGE_BOAT;
            } else if (type == FIELD_BOAT_HUGE) {
                f->field[row - i][col] = FIELD_POSITION_HUGE_BOAT;
            } else {
                return FALSE;
            }

        }
        return TRUE;
    } else if (dir == FIELD_BOAT_DIRECTION_EAST) {//east
        for (i = 0; i < (type + 3); i++) {//length of boat
            if ((col + i) > FIELD_COLS) {
                return FALSE;
            }
            if (f->field[row][col + i] != FIELD_POSITION_EMPTY) {
                return FALSE;
            }
            if (type == FIELD_BOAT_SMALL) {
                f->field[row][col + i] = FIELD_POSITION_SMALL_BOAT;
            } else if (type == FIELD_BOAT_MEDIUM) {
                f->field[row][col + i] = FIELD_POSITION_MEDIUM_BOAT;
            } else if (type == FIELD_BOAT_LARGE) {
                f->field[row][col + i] = FIELD_POSITION_LARGE_BOAT;
            } else if (type == FIELD_BOAT_HUGE) {
                f->field[row][col + i] = FIELD_POSITION_HUGE_BOAT;
            } else {
                return FALSE;
            }

        }
        return TRUE;
    } else if (dir == FIELD_BOAT_DIRECTION_WEST) {//west
        for (i = 0; i < (type + 3); i++) {//length of boat
            if ((col - i) < 0) {
                return FALSE;
            }
            if (f->field[row][col - i] != FIELD_POSITION_EMPTY) {
                return FALSE;
            }
            if (type == FIELD_BOAT_SMALL) {
                f->field[row][col - i] = FIELD_POSITION_SMALL_BOAT;
            } else if (type == FIELD_BOAT_MEDIUM) {
                f->field[row][col - i] = FIELD_POSITION_MEDIUM_BOAT;
            } else if (type == FIELD_BOAT_LARGE) {
                f->field[row][col - i] = FIELD_POSITION_LARGE_BOAT;
            } else if (type == FIELD_BOAT_HUGE) {
                f->field[row][col - i] = FIELD_POSITION_HUGE_BOAT;
            } else {
                return FALSE;
            }

        }
        return TRUE;
    } else {
        //none of these
        return FALSE;
    }
}



FieldPosition FieldRegisterEnemyAttack(Field *f, GuessData *gData)
{
    FieldPosition hit = f->field[gData->row][gData->col];
    if (hit == FIELD_POSITION_EMPTY) {
        gData->hit = HIT_MISS;
        return hit;
    } else if (hit == FIELD_POSITION_SMALL_BOAT) {
        f->smallBoatLives = (f->smallBoatLives) - 1;;
        if (f->smallBoatLives == 0) {
            gData->hit = HIT_SUNK_SMALL_BOAT;
        } else {
            gData->hit = HIT_HIT;
        }
        return hit;
    } else if (hit == FIELD_POSITION_MEDIUM_BOAT) {
         f->mediumBoatLives = (f->mediumBoatLives) - 1;
        if (f->mediumBoatLives == 0) {
            gData->hit = HIT_SUNK_MEDIUM_BOAT;
        } else {
            gData->hit = HIT_HIT;
        }
        return hit;
    } else if (hit == FIELD_POSITION_LARGE_BOAT) {
         f->largeBoatLives = (f->largeBoatLives) - 1;
        if (f->largeBoatLives == 0) {
            gData->hit = HIT_SUNK_LARGE_BOAT;
        } else {
            gData->hit = HIT_HIT;
        }
        return hit;
    } else if (hit == FIELD_POSITION_HUGE_BOAT) {
        f->hugeBoatLives = (f->hugeBoatLives) - 1;
        if (f->hugeBoatLives == 0) {
            gData->hit = HIT_SUNK_HUGE_BOAT;
        } else {
            gData->hit = HIT_HIT;
        }
        return hit;
    }
}


FieldPosition FieldUpdateKnowledge(Field *f, const GuessData *gData)
{
    FieldPosition coordinate = f->field[gData->row][gData->col];
    if (gData->hit == HIT_HIT) {
        f->field[gData->row][gData->col] = gData->hit;
        if (f->hugeBoatLives == 0) {
            f->field[gData->row][gData->col] = HIT_SUNK_HUGE_BOAT;
            f->field[gData->row][gData->col] = FIELD_POSITION_HIT;
            return coordinate;
        } else if (f->largeBoatLives == 0) {
            f->field[gData->row][gData->col] = HIT_SUNK_LARGE_BOAT;
            f->field[gData->row][gData->col] = FIELD_POSITION_HIT;
            return coordinate;
        } else if (f->mediumBoatLives == 0) {
            f->field[gData->row][gData->col] = HIT_SUNK_MEDIUM_BOAT;
            f->field[gData->row][gData->col] = FIELD_POSITION_HIT;
            return coordinate;
        } else if (f->smallBoatLives == 0) {
            f->field[gData->row][gData->col] = HIT_SUNK_SMALL_BOAT;
            f->field[gData->row][gData->col] = FIELD_POSITION_HIT;
            return coordinate;
        }
        return coordinate;
    } else if (gData->hit == HIT_MISS) {
        f->field[gData->row][gData->col] = FIELD_POSITION_MISS;
        return coordinate;
    }
}


uint8_t FieldGetBoatStates(const Field *f)
{
    uint8_t boats, huge, large, medium, small;
    if (f->hugeBoatLives == 0) {
        huge = 0;
    } else {
        huge = FIELD_BOAT_STATUS_HUGE;
    }
    if (f->largeBoatLives == 0) {
        large = 0;
    } else {
        large = FIELD_BOAT_STATUS_LARGE;
    }
    if (f->mediumBoatLives == 0) {
        medium = 0;
    } else {
        medium = FIELD_BOAT_STATUS_MEDIUM;
    }
    if (f->smallBoatLives == 0) {
        small = 0;
    } else {
        small = FIELD_BOAT_STATUS_SMALL;
    }
    boats = small | medium | large | huge;
    return boats;
}
