/* SDCC MCS51 executable reproduction. C only; run in uCsim, not on a device. */
#include <8051.h>

volatile __xdata __at (0x3000) unsigned char cell;
volatile __data unsigned char irq_count;
volatile __data unsigned char irq_before_read;

/* A separate translation unit keeps the control's load independent. */
unsigned char load_cell(volatile __xdata unsigned char *p);

/* Fixed addresses make the result readable without parsing linker symbols. */
struct results {
    unsigned char status; /* 0 = pass, 1 = bug, 2 = setup failed, 3 = other value */
    unsigned char observed;
    unsigned char cell_after;
    unsigned char irq_before_test_read;
    unsigned char control;
    unsigned char irq_before_control_read;
    unsigned char total_irqs;
    unsigned char complete;               /* 0xa5 only after all checks */
};
volatile __xdata __at (0x3001) struct results result;

void change_cell(void) __interrupt (1)
{
    cell = 0;
    ++irq_count;
}

/* Request timer 0's interrupt without starting the timer. The volatile byte
   reads allow interrupt service while keeping store/readback in one basic
   block: a polling loop would mask the CSE bug. Record that service happened
   BEFORE the tested load; otherwise main reports TEST_SETUP_FAILED. */
#define CHANGE_DESTINATION() do { \
    TF0 = 1;                    \
    irq_count;                  \
    irq_count;                  \
    irq_count;                  \
    irq_before_read = irq_count; \
} while (0)

unsigned char readback(volatile __xdata unsigned char *p, unsigned char value)
{
    *p = value;
    CHANGE_DESTINATION();
    return *p;
}

unsigned char control(volatile __xdata unsigned char *p, unsigned char value)
{
    *p = value;
    CHANGE_DESTINATION();
    return load_cell(p);
}

void main(void)
{
    result.complete = 0;
    result.status = 2;
    irq_count = 0;
    irq_before_read = 0;
    TR0 = 0;
    TF0 = 0;
    ET0 = 1;
    EA = 1;

    result.observed = readback(&cell, 0x5a);
    result.cell_after = cell;
    result.irq_before_test_read = irq_before_read;
    result.control = control(&cell, 0x5a);
    result.irq_before_control_read = irq_before_read;
    EA = 0;
    result.total_irqs = irq_count;

    if (result.cell_after == 0 && result.irq_before_test_read == 1 &&
        result.control == 0 && result.irq_before_control_read == 2 &&
        result.total_irqs == 2) {
        result.status = 3;
        if (result.observed == 0)
            result.status = 0;
        else if (result.observed == 0x5a)
            result.status = 1;
    }
    result.complete = 0xa5;
    for (;;) {}
}
