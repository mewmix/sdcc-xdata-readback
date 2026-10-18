SDCC ?= sdcc
S51 ?= s51
SDCCFLAGS ?= -mmcs51 --std-c99
EXPECT ?= bug
OUTPUT ?= build

.PHONY: reproduce assembly

# Always rebuild so changing SDCC or SDCCFLAGS cannot reuse an old binary.
reproduce:
	@case "$(EXPECT)" in bug|pass) ;; *) echo 'EXPECT must be bug or pass' >&2; exit 2;; esac
	mkdir -p "$(OUTPUT)"
	$(SDCC) --version > "$(OUTPUT)/compiler-version.txt"
	$(SDCC) $(SDCCFLAGS) -c load.c -o "$(OUTPUT)/load.rel"
	$(SDCC) $(SDCCFLAGS) reproduce.c "$(OUTPUT)/load.rel" -o "$(OUTPUT)/reproduce.ihx"
	$(S51) -t 51 -q -c - "$(OUTPUT)/reproduce.ihx" < run.sim > "$(OUTPUT)/simulator.log" 2>&1
	@cat "$(OUTPUT)/simulator.log"
	@if grep -Eiq '^0x3001[[:space:]]+01 5a 00 01 00 02 02 a5([[:space:]]|$$)' "$(OUTPUT)/simulator.log"; then \
	    echo 'BUG_REPRODUCED: returned 0x5a; destination and C control read 0x00'; \
	    test "$(EXPECT)" = bug; \
	elif grep -Eiq '^0x3001[[:space:]]+00 00 00 01 00 02 02 a5([[:space:]]|$$)' "$(OUTPUT)/simulator.log"; then \
	    echo 'PASS: returned the changed destination value 0x00'; \
	    test "$(EXPECT)" = pass; \
	elif grep -Eiq '^0x3001[[:space:]]+03 [[:xdigit:]]{2} 00 01 00 02 02 a5([[:space:]]|$$)' "$(OUTPUT)/simulator.log"; then \
	    echo 'UNEXPECTED_VALUE: setup checks passed, but the returned byte was neither 0x00 nor 0x5a' >&2; \
	    exit 1; \
	else \
	    echo 'TEST_SETUP_FAILED: interrupt, control, or completion check failed; no compiler verdict' >&2; \
	    exit 2; \
	fi

assembly:
	mkdir -p "$(OUTPUT)"
	$(SDCC) --version
	$(SDCC) $(SDCCFLAGS) -S readback.c -o "$(OUTPUT)/readback.asm"
	sed -n '/^_readback:/,/^[[:space:]]*ret/p' "$(OUTPUT)/readback.asm"
