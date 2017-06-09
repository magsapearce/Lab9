/* ************************************************************************** */
/** Header file for LEDs

  @Engineer
    Kelby Gan

  @Organization
    University of California, Santa Cruz

  @File Name
    Leds.h

 */
/* ************************************************************************** */

#ifndef LEDS_H    /* Guard against multiple inclusion */
#define LEDS_H

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <xc.h>
#include "BOARD.h"
/* This section lists the other files that are included in this file.
 */
#define LEDS_INIT() do { \
    TRISE &= ~(0xFF); \
    LATE &= ~(0xFF); \
} while(0)

#define LEDS_GET() (LATE)
#define LEDS_SET(x) (LATE = (x))

#endif /* LEDS_H */

/* *****************************************************************************
 End of File
 */

