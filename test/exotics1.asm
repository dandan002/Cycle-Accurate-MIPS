# exotics1.asm
# This MIPS assembly code tests Icache miss, Dcache miss, and load-stall trigger

.data
array: .word 0x12345678, 0x9abcdef0, 0x0fedcba9, 0x87654321

.text
main:
    # Initialize base address for array
    la $t0, array

    # Load word from array to cause Dcache miss
    lw $t1, 0($t0)       # Load first element (Dcache miss)
    lw $t2, 4($t0)       # Load second element (Dcache miss)
    lw $t3, 8($t0)       # Load third element (Dcache miss)
    lw $t4, 12($t0)      # Load fourth element (Dcache miss)

    # Perform some operations to cause load-stall
    add $t5, $t1, $t2    # Add first and second elements
    sub $t6, $t3, $t4    # Subtract fourth element from third

    # Branch to a label to cause Icache miss
    j label              # Jump to label (Icache miss)

label:
    nop                  # No operation (to fill delay slot)
    add $t7, $t5, $t6    # Add results of previous operations

    # End of program
    .word 0xfeedfeed