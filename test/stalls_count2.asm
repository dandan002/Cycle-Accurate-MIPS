main:
    la   $t0, Arr         # Load immediate into $t0
    lw   $t1, 0($t0)
    # li   $t2, 1
    lw   $t2, 0($t0)
    # add $t2, $t2, $t2
    lw   $t1, 0($t0)
    lw   $t2, 0($t0)
    beq $t1, $t2, skip       
    .word 0xfeedfeed     # End of program

skip:
    add $t3, $t2, $t2     # Subtract $t0 from $t1
    .word 0xfeedfeed       # End of program

Arr: .word 0x1, 0x1         # Array with two identical values