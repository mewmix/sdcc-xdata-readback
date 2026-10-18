/* SDCC MCS51 volatile XDATA readback reproducer. Compiler-only; no hardware. */
unsigned char readback(volatile __xdata unsigned char *p, unsigned char value)
{
    *p = value;
    return *p;
}
