# SDCC can skip a volatile memory read

SDCC can return a byte it tried to write instead of reading memory again.
If a device changes that memory, the program can report the old value and
mistakenly treat the write as successful.

The destination is marked `volatile`, so the compiler must perform the read.
The problem has been observed on MCS51/DS390, STM8, Z80, and MOS6502.
This repository includes the MCS51 XDATA example and a generic-pointer C case
for the other targets. The MCS51 executable test uses SDCC's address-space
and interrupt extensions.

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

## Observed in builds

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

## Other observed targets

Reviewed assembly for `generic_readback.c` shows the same wrong return value
on these target/version combinations. Its pointer is `volatile unsigned char *`,
without the MCS51-specific `__xdata` qualifier:

| Target | 4.2.0 #13081 | 4.6.0 #16555 | 4.6.2 #16768 snapshot | 4.6.4 #16957 source snapshot |
| --- | --- | --- | --- | --- |
| STM8 | Read emitted, result discarded | Read absent | Read absent | Read absent |
| Z80 | Read emitted, result discarded | Read absent | Read absent | Read absent |
| MOS6502 | Read emitted, result discarded | Read absent | Read absent | Read absent |
| MCS51 generic pointer | Read emitted, result discarded | Read absent | Read absent | Read absent |

The DS390 small-memory profile also returns the wrong value on the ten
release/snapshot builds in the table above. The two 4.2.10 SVN builds were
checked separately: r13787 retains the
load through local CSE, while r13788 removes it for MCS51 XDATA, MCS51 generic
pointers, STM8, Z80, and MOS6502.

To generate the generic case's assembly with an SDCC build supporting the
chosen target, run:

```sh
make generic-assembly TARGET=stm8
make generic-assembly TARGET=z80
make generic-assembly TARGET=mos6502
make generic-assembly TARGET=mcs51
```

The output is `build/<target>/generic_readback.asm`. Compare `readback`, which
stores then reads, with `read_only`, which independently reads the destination.
On the affected newer builds, `readback` returns the input without a destination
read; `read_only` emits the destination read.

For DS390, compile the XDATA example with:

```sh
make assembly SDCCFLAGS='-mds390 --model-small --std-c99'
```

## Additional methods

With uCsim's `s51` also installed, run:

```sh
make
```

`make` succeeds when the known bug is reproduced. To require correct behavior
when testing a compiler fix, use `make EXPECT=pass`.

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

