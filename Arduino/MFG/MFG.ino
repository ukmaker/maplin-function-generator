#include <WInterrupts.h>
/**
 * 
 * Pins are:
 * 
 * PORTB - Data
 * PD0   - F+    - PIN0
 * PD1   - F-    - PIN1
 * PD2   - MODE  - PIN2
 * PD3   - sin   - PIN3
 * PD4   - sq    - PIN4
 * PD5   - tri   - PIN5
 * PD6   - saw   - PIN6
 * 
 * PA2   - RESET
 * PA1   - filtering cap enable
 * PA0   - interrupt flag
 *
 * Phase reg is 0x000000wx xxxxxxxx ******** ********
 * One loop is 23 cycles @ 8MHz = 2.875us
 * One complete cycle is generated every 0x04 00 00 00 = 67,108,864
 * Time for one complete cycle is 192.94s
 * So a 1Hz increment is approx 193
 * 
 **/
#define f1Hz 193
#define f10Hz 1930
#define f100Hz 19300
#define f1kHz 193000
#define f10kHz 1930000

byte wavetable[512] PROGMEM = {
    255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,254,
    254,254,254,254,254,254,254,254,
    254,254,253,253,253,253,253,253,
    253,252,252,252,252,252,252,251,
    251,251,251,251,250,250,250,250,
    250,249,249,249,249,248,248,248,
    248,247,247,247,246,246,246,246,
    245,245,245,244,244,244,243,243,
    243,242,242,242,241,241,241,240,
    240,240,239,239,239,238,238,237,
    237,237,236,236,235,235,234,234,
    234,233,233,232,232,231,231,230,
    230,230,229,229,228,228,227,227,
    226,226,225,225,224,224,223,223,
    222,222,221,221,220,219,219,218,
    218,217,217,216,216,215,214,214,
    213,213,212,212,211,210,210,209,
    209,208,207,207,206,206,205,204,
    204,203,202,202,201,200,200,199,
    199,198,197,197,196,195,195,194,
    193,193,192,191,191,190,189,189,
    188,187,186,186,185,184,184,183,
    182,182,181,180,179,179,178,177,
    177,176,175,174,174,173,172,172,
    171,170,169,169,168,167,166,166,
    165,164,163,163,162,161,160,160,
    159,158,157,157,156,155,154,154,
    153,152,151,150,150,149,148,147,
    147,146,145,144,144,143,142,141,
    140,140,139,138,137,137,136,135,
    134,133,133,132,131,130,130,129,
    128,127,126,126,125,124,123,123,
    122,121,120,119,119,118,117,116,
    116,115,114,113,112,112,111,110,
    109,109,108,107,106,106,105,104,
    103,102,102,101,100,99,99,98,
    97,96,96,95,94,93,93,92,
    91,90,90,89,88,87,87,86,
    85,84,84,83,82,82,81,80,
    79,79,78,77,77,76,75,74,
    74,73,72,72,71,70,70,69,
    68,67,67,66,65,65,64,63,
    63,62,61,61,60,59,59,58,
    57,57,56,56,55,54,54,53,
    52,52,51,50,50,49,49,48,
    47,47,46,46,45,44,44,43,
    43,42,42,41,40,40,39,39,
    38,38,37,37,36,35,35,34,
    34,33,33,32,32,31,31,30,
    30,29,29,28,28,27,27,26,
    26,26,25,25,24,24,23,23,
    22,22,22,21,21,20,20,19,
    19,19,18,18,17,17,17,16,
    16,16,15,15,15,14,14,14,
    13,13,13,12,12,12,11,11,
    11,10,10,10,10,9,9,9,
    8,8,8,8,7,7,7,7,
    6,6,6,6,6,5,5,5,
    5,5,4,4,4,4,4,4,
    3,3,3,3,3,3,3,2,
    2,2,2,2,2,2,2,2,
    2,2,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1

};

#define SINE 8
#define SQUARE 16
#define TRIANGLE 32
#define SAWTOOTH 64

// define combinations as well to save code space
// The leds we want off when incrementing the frequency
#define INCREMENTING 104
#define DECREMENTING 88
#define ALL 127

// We use RA0 as the interrupt signal
#define INT 1

// RA1 is taken low or tristate to enable the filtering capacitor
#define FILTER 2

volatile uint32_t phaseAccumulator = 0;
volatile uint32_t phaseIncrement = f100Hz;

//volatile byte state = RUNNING;

#define DEBOUNCE_DELAY 1000
#define MEDIUM_PRESS_DELAY 10000
#define LONG_PRESS_DELAY 30000
// one tick is 25us
// we want 5 clicks per second for a medium press
// so .2/25us = 
#define MEDIUM_TICKS 2000
// 20/s fr a long press
#define FAST_TICKS 500

#define FINC_BUTTON 1
#define FDEC_BUTTON 2
#define FUNC_BUTTON 4

// Max frequency we want for a sine wave is 50kHz = 193*50000 = 9,650,000 = 0x933f50 
#define MAX_INC 0x933f50
#define SLOW_INC 10
#define FAST_INC 50

unsigned long previousTicks = 0;
unsigned long ticks = 0;
uint8_t lastButtonState = 0x07;
uint8_t buttonState = 0x07;
uint8_t buttonsChanged = 0;

uint8_t mode = SINE;

int timeout = 0;

void setup() {

    DDRD  = B11111000;
    DDRA  = B00000001; // all inputs to start
    PORTD = B00001111; // pull-ups on the buttons, sine LED lit
    PORTA = B00000000; // we're going to use PA0 as a flag to break out of the genloop

    DDRB = B11111111;  // All outputs to the R/2R ladder

    MCUCR = 2;         // Falling edge on INT0 (PD2) generates an interrupt
    GIMSK = 0x40;      // Enable INT0
    TIMSK = 0;         // turn off the timer

}

ISR(INT0_vect) {
    PORTA |= 1;
}

void generate() {
    switch(mode) {
    case SINE:
        sineLoop();
        break;
    case SQUARE:
        squareLoop();
        break;
    case TRIANGLE:
        triangleLoop();
        break;
    case SAWTOOTH:
        sawtoothLoop();
        break;
    }
}

void loop() {

    if(timeout == 0) {
        PORTA &= ~1;
        generate();
        // Return happens when a button generates an interrupt
    }

    if(update()) {

        timeout = 5000;
    }

    timeout--;
}

void led(uint8_t led, bool on) {
    if(on) {
        PORTD |= led;
    } 
    else {
        PORTD &= ~led;
    }
}

void toggle(uint8_t t) {
    bool on = PIND & t;
    led(t, !on);
}

bool update() {

    ticks++; 

    uint8_t newState = PIND & 0x07;

    buttonsChanged = 0;

    if(newState != buttonState) {
        if(delta() >= DEBOUNCE_DELAY) {
            previousTicks = ticks;
            buttonsChanged = (newState & ~buttonState) | (~newState & buttonState);
            buttonState = newState;
        } 
    }

    if(risingEdge(FINC_BUTTON)) { 

        incPhase();

    } 
    else if(mediumPress(FINC_BUTTON)) { 

        deltaClick(false, MEDIUM_TICKS);

    } 
    else if(longPress(FINC_BUTTON)) { 

        deltaClick(false, FAST_TICKS);

    } 
    else if(risingEdge(FDEC_BUTTON)) { 

        decPhase();

    } 
    else if(mediumPress(FDEC_BUTTON)) {

        deltaClick(true, MEDIUM_TICKS);

    } 
    else if(longPress(FDEC_BUTTON)) {

        deltaClick(true, FAST_TICKS);

    } 
    else if(risingEdge(FUNC_BUTTON)) {
        mode = mode << 1;
        if(mode > SAWTOOTH) mode = SINE;
    } 
    else {
        return false;
    }
    return true;
}

void incPhase() {
    phaseIncrement += getInc();
    if(phaseIncrement >= MAX_INC) {
        phaseIncrement = MAX_INC;
        led(ALL, false);
        led(SINE, true);
    } 
    else {
        led(INCREMENTING, false);
        toggle(SQUARE);
    }
}

void decPhase() {
    unsigned long decrement = getInc();
    if(decrement > phaseIncrement) {
        led(SAWTOOTH, true);
        led(ALL, false);
    } 
    else {
        phaseIncrement -= decrement;
        led(DECREMENTING, false);
        toggle(TRIANGLE);
    }
}

void deltaClick(bool down, unsigned long everyTicks) {
    if(ticks % everyTicks == 0) {
        if(down) {
            decPhase();
        } 
        else {
            incPhase();
        }
    }
}

// Get the amount to use for an increment based on the current frequency
// <100Hz : 1Hz
// <1kHz : 10Hz etc
unsigned long getInc() {
    // crufty hack for now
    // 1Hz = 192
    unsigned long freq = phaseIncrement / f1Hz; 
    if(freq < 100)     return f1Hz;
    if(freq < 1000)    return f10Hz;
    if(freq < 10000)   return f100Hz;
    //return f100Hz;
    // if(freq < 100000)  
    return f1kHz;
}


// one tick is approx 200us
unsigned long delta() {
    return ticks - previousTicks;
}

bool mediumPress(uint8_t button) {
    unsigned long d = delta();
    return pressed(button) && (d > MEDIUM_PRESS_DELAY) && (d < LONG_PRESS_DELAY);
}

bool longPress(uint8_t button) {
    return pressed(button) && (delta() >= LONG_PRESS_DELAY);
}

bool pressed(uint8_t button) {
    return (buttonState & button) == 0;
}

bool changed(uint8_t button) {
    return (buttonsChanged & button) != 0;
}

bool risingEdge(uint8_t button) {
    return changed(button) && !pressed(button);
}

void sineLoop() {

    // register assignments will be as follows:
    // tableStart in Z
    // phaseAccumulator in r17,r18,r19, ZL
    // phaseIncrement in wherever the compiler sticks it
    // Remember the compiler reserves r0 for temporary storage and r1 for zero
    // The button interrupt handler will set the T flag so we can break out of the loop

        PGM_P start;
    start = (PGM_P)&wavetable;

    // Enable the filtering cap
    DDRA  |= 2;

    // Switch on the sine LED
    PORTD = B00001111;

    __asm__ __volatile__ (

    "8:"
        "ldi r19,2" "\r\n"  

        "9:" 	// 23 cycles per loop
       "nop" "\r\n" // nop to make 23 cycles 
    "movw %[wavptr],%[wavbase]" "\r\n"  // load wave table base address (1)
    "add %A[phase], %A[delta]" "\r\n" // (1)
    "adc %B[phase], %B[delta]" "\r\n" // (1)
    "adc %C[phase], %C[delta]" "\r\n"
        "adc %D[phase], %D[delta]" "\r\n" // add delta to phase
    "mov r18,%D[phase]" "\r\n"        // get 9-bit offset into the wavetable
    "andi r18,0x01"     "\r\n"        
        // Now, if bit 10 is set, the address in the wavetable should be 512 - offset
    "sbrc %D[phase], 1" "\r\n"  // 1 cycle if not skipped, 2 if skipped
    "rjmp 10f" "\r\n" // so go to the decrementing code 2 cycles

    // +2 cycles
    // 2 nops to balance
    "nop" "\r\n"
        "nop" "\r\n"
        "nop" "\r\n"
        "nop" "\r\n"
        "add %A[wavptr], %C[phase]" "\r\n"
        "adc %B[wavptr], r18" "\r\n" // add top 8 bits of phase to base address
    "lpm r0, Z" "\r\n" // Read a byte from the wavetable (3)
    "out %[portb], r0" "\r\n" // Read a byte from the wavetable (1)

    "sbis %[pind],0" "\r\n" //(1)
    "rjmp 9b" "\r\n"  // If the pins haven't changed, carry on (2)

    "rjmp 11f" "\r\n"

        "10:"   // +3 cycles     
    "add %B[wavptr], r19" "\r\n"
        // +4 cycles
    "sub %A[wavptr], %C[phase]" "\r\n"
        "sbc %B[wavptr], r18" "\r\n" // add top 8 bits of phase to base address
    "sbiw %A[wavptr], 1" "\r\n"
        "lpm r0, Z" "\r\n" // Read a byte from the wavetable
    "out %[portb], r0" "\r\n" // Read a byte from the wavetable

    "11:"
        "sbis %[pind],0" "\r\n"
        "rjmp 9b" "\r\n"  // If the pins haven't changed, carry on

: 

    [phase] "+w" (phaseAccumulator),
    [wavbase] "+y" (start),
    [wavptr] "+z" (start)

: 
    [delta] "r" (phaseIncrement),
    [portb] "I" (_SFR_IO_ADDR(PORTB)),
    [pind] "I" (_SFR_IO_ADDR(PINA))

: 
    "cc", "r18", "r19"
        );

}

void squareLoop() {

    // Disable the filtering cap
    DDRA  &= ~2;
    // Switch on the square LED
    PORTD = B00010111;

    __asm__ __volatile__ (

        "ldi r19,0xff" "\r\n"   // so we can output a high level later
    "13:" 	
    "sbic %[pind],0" "\r\n" // Seen an interrupt?
    "rjmp 151f" "\r\n"       // If the pins haven't changed, carry on
        "14:"
        "add %A[phase], %A[delta]" "\r\n"
        "adc %B[phase], %B[delta]" "\r\n"
        "adc %C[phase], %C[delta]" "\r\n"
        "adc %D[phase], %D[delta]" "\r\n" // add delta to phase
    "nop" "\r\n" // nops so the loop is the same length as sine
    "nop" "\r\n" // and we can use the same code for freq calculation
    "nop" "\r\n"
        "nop" "\r\n"
        "nop" "\r\n"
        "nop" "\r\n"
        "nop" "\r\n"
        "nop" "\r\n"
        "nop" "\r\n"
        "nop" "\r\n"
        "nop" "\r\n"
        // Now, if bit 10 is set, the address in the wavetable should be 512 - offset
    "sbrc %D[phase], 1" "\r\n"  // 1 cycle if not skipped, 2 if skipped
    "rjmp 15f" "\r\n" // so go to the decrementing code 2 cycles

    // +2 cycles
    // 2 nops to balance
    "nop" "\r\n"
        "out %[portb], r19" "\r\n" // high level
    "rjmp 13b" "\r\n"  

        "15:" // low level
    "out %[portb], r1" "\r\n" // r1 is always 0
    "rjmp 13b" "\r\n"  // If the pins haven't changed, carry on
    "151:"

: 

    [phase] "+w" (phaseAccumulator)

: 
    [delta] "r" (phaseIncrement),
    [portb] "I" (_SFR_IO_ADDR(PORTB)),
    [pind] "I" (_SFR_IO_ADDR(PINA))

: 
    "cc", "r19"
        );
}

void triangleLoop() {
    // Switch on the triangle LED
    PORTD = B00100111;
    __asm__ __volatile__ (

    "16:" "\r\n"	// 23 cycles per loop (@ 8MHz = 2.875us)
    "add %A[phase], %A[delta]" "\r\n" // (1)
    "adc %B[phase], %B[delta]" "\r\n" // (1)
    "adc %C[phase], %C[delta]" "\r\n"
        "adc %D[phase], %D[delta]" "\r\n" // add delta to phase
    // Now, if bit 10 is set, we need a descending wave
    "mov r18, %C[phase]" "\r\n" 
        //"lsr r18" "\r\n"
        "lsr r18" "\r\n"
        "sbrc %D[phase], 0" "\r\n"  // 1 cycle if not skipped, 2 if skipped
        "sbr r18,0x80" "\r\n"

       // "17:"

       // "sbrc %D[phase], 1" "\r\n"  // 1 cycle if not skipped, 2 if skipped
       // "sbr r18,0x80" "\r\n"

       // "18:"
       "nop" "\r\n"
       "nop" "\r\n"
         "nop" "\r\n"
        "nop" "\r\n"
       "nop" "\r\n"
      "nop" "\r\n"
       "nop" "\r\n"
       "nop" "\r\n"

        "sbrc %D[phase], 1" "\r\n"  // 1 cycle if not skipped, 2 if skipped
    "rjmp 19f" "\r\n" // so go to the decrementing code 2 cycles
    "com r18" "\r\n"
        "19:"
        "out %[portb], r18" "\r\n" // Read a byte from the wavetable (1)
    "sbis %[pind],0" "\r\n"
        "rjmp 16b" "\r\n"  // If the pins haven't changed, carry on

: 

    [phase] "+w" (phaseAccumulator)

: 
    [delta] "r" (phaseIncrement),
    [portb] "I" (_SFR_IO_ADDR(PORTB)),
    [pind] "I" (_SFR_IO_ADDR(PINA))

: 
    "cc", "r18"
        );

}

void sawtoothLoop() {
    // Switch on the sawtooth LED
    PORTD = B01000111;
    __asm__ __volatile__ (


    "20:" 	// 23 cycles per loop (@ 8MHz = 2.875us)
    "add %A[phase], %A[delta]" "\r\n" // (1)
    "adc %B[phase], %B[delta]" "\r\n" // (1)
    "adc %C[phase], %C[delta]" "\r\n"
    "adc %D[phase], %D[delta]" "\r\n" // add delta to phase
    "mov r18, %C[phase]" "\r\n" 
        "lsr r18" "\r\n"
        "lsr r18" "\r\n"
        "sbrc %D[phase], 0" "\r\n"  // 1 cycle if not skipped, 2 if skipped
        "sbr r18,0x40" "\r\n"
 
       "sbrc %D[phase], 1" "\r\n"  // 1 cycle if not skipped, 2 if skipped
       "sbr r18,0x80" "\r\n"

     "out %[portb], r18" "\r\n" // Read a byte from the wavetable (1)
       "nop" "\r\n"
       "nop" "\r\n"
      "nop" "\r\n"
         "nop" "\r\n"
       "nop" "\r\n"
       "nop" "\r\n"
       "nop" "\r\n"
       "nop" "\r\n"
       
    "sbis %[pind],0" "\r\n"
        "rjmp 20b" "\r\n"  // If the pins haven't changed, carry on

: 

    [phase] "+w" (phaseAccumulator)

: 
    [delta] "r" (phaseIncrement),
    [portb] "I" (_SFR_IO_ADDR(PORTB)),
    [pind] "I" (_SFR_IO_ADDR(PINA))

: 
    "cc", "r18"
        );
}


