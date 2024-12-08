# branch.asm
# This script tests branch delay slots in MIPS assembly

# Initialize registers
main:
    li $t0, 10       # Load immediate value 10 into $t0
    li $t1, 20       # Load immediate value 20 into $t1
    li $t2, 30       # Load immediate value 30 into $t2
    li $t6, 0x000000b4  # Load immediate value 180 into $t6

# Test BEQ (Branch if Equal)
    beq $t0, $t1, label1  # Branch to label1 if $t0 == $t1
    add $t3, $t0, $t1     # Delay slot: This instruction will execute regardless of the branch
    sw $t3, 0($t6)        # STORE

label1:
    sub $t3, $t1, $t0     # This will execute if branch is taken

# Test BNE (Branch if Not Equal)
    bne $t0, $t1, label2  # Branch to label2 if $t0 != $t1
    add $t4, $t1, $t2     # Delay slot: This instruction will execute regardless of the branch
    sw $t4, 4($t6)        # NOT STORE

label2:
    sub $t4, $t2, $t1     # This will execute if branch is taken

# Test BLEZ (Branch if Less than or Equal to Zero)
    blez $t0, label3      # Branch to label3 if $t0 <= 0
    add $t5, $t0, $t2     # Delay slot: This instruction will execute regardless of the branch
    sw $t5, 8($t6)        # STORE

label3:
    sub $t5, $t2, $t0     # This will execute if branch is taken

# Test BGTZ (Branch if Greater than Zero)
    bgtz $t0, label4      # Branch to label4 if $t0 > 0
    add $t5, $t1, $t2     # Delay slot: This instruction will execute regardless of the branch
    sw $t5, 12($t6)       # NOT STORE

label4:
    sub $t6, $t2, $t1     # This will execute if branch is taken

# End of script
    .word 0xfeedfeed    # End of program marker