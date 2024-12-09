# MIPS Test for Load-Use and Load-Branch Stalls
# The simulator should count:
# Load-Use stalls: 3
# Load-Branch stalls: 2

.text
main:
    # Initialize registers
    li $t0, 0          # Index
    la $t1, array      # Base address of array

    # Load-Use Stall Test
    lw $t2, 0($t1)     # Load array[0] into $t2
    add $t3, $t2, $t2  # Use $t2 immediately (1st stall)
    sub $t4, $t2, $t2  # Use $t2 immediately (2nd stall)
    add $t5, $t2, $t2  # Use $t2 immediately (3rd stall)

    # Load-Branch Stall Test
    lw $t6, 4($t1)     # Load array[1] into $t6
    beq $t6, $zero, label1  # Branch based on $t6 (1st stall)
    bne $t6, $zero, label2  # Branch based on $t6 (2nd stall)

label1:
    # Do something
    addi $t0, $t0, 1

label2:
    # Do something else
    addi $t0, $t0, 2

    # End of program
    .word 0xfeedfeed

.data
array: .word 10, 20, 30, 40, 50