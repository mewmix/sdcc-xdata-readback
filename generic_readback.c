/* Fixed cross-target check; review each target's generated assembly. */
unsigned char readback(volatile unsigned char *p, unsigned char value)
{
    *p = value;
    return *p;
}

unsigned char read_only(volatile unsigned char *p)
{
    return *p;
}
