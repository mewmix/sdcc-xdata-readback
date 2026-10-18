# SDCC can skip a volatile memory read

A program writes a byte to memory, then reads it back to check what is there.
SDCC can return the byte it tried to write instead of reading memory again.
If a device changes that memory, the program can report the old value and
mistakenly treat the write as successful.

The destination is marked `volatile`, so the compiler must perform the read.
This reproduction demonstrates the problem on MCS51 XDATA. All test sources
are C, using SDCC's address-space and interrupt extensions. No physical
device is needed.

## Example

The complete function in `readback.c` is:

```c
unsigned char readback(volatile __xdata unsigned char *p, unsigned char value)
{
    *p = value;
    return *p;
}
```

With SDCC's MCS51 target and Make installed, run:

```sh
make assembly
```

Expected: a write to the destination, then a fresh read whose value is returned.
On SDCC 4.6.0 #16555 the relevant output is:

```asm
movx @dptr,a                 ; write memory
mov  dpl,_readback_PARM_2    ; return the input byte
ret                         ; no read from memory
```

## Observed affected builds

The small `readback.c` expression returns the stored input instead of the
destination's readback value on these tested, unmodified compiler builds:

| SDCC version and build | Observed behavior on MCS51 XDATA |
| --- | --- |
| 3.9.0 #11195 | Read emitted, but its result is discarded. |
| 4.0.0 #11528 | Read emitted, but its result is discarded. |
| 4.1.0 #12072 | Read emitted, but its result is discarded. |
| 4.2.0 #13081 | Read emitted, but its result is discarded. |
| 4.2.10 #13787 (locally built SVN r13787) | Read emitted, but its result is discarded. |
| 4.2.10 #13788 (locally built SVN r13788) | Destination read removed during local CSE. |
| 4.3.0 #14184 | Destination read absent. |
| 4.4.0 #14620 | Destination read absent. |
| 4.5.0 #15242 | Destination read absent. |
| 4.6.0 #16555 | Destination read absent. |
| 4.6.2 #16768 (binary snapshot) | Destination read absent. |
| 4.6.4 #16957 (locally built source snapshot, 2026-10-03) | Destination read absent. |

## Additional methods

With uCsim's `s51` also installed, run:

```sh
make
```

The executable test in `reproduce.c` does the following:

1. Writes `0x5a` through the volatile pointer.
2. Requests an interrupt. Its C handler changes the destination to `0x00`.
3. Records whether the interrupt ran, then reads through the pointer.
4. Repeats with an independent C read in `load.c` as a control.

The test confirms that the interrupt happened before each read, memory
changed to zero, and the control read zero. On SDCC 4.6.0 #16555 it reports:

```text
BUG_REPRODUCED: returned 0x5a; destination and C control read 0x00
```

The interrupt request and a few volatile byte accesses are additions to the
smallest example so the changed memory is observable. A polling loop would
mask this optimization bug.

