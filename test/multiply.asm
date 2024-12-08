# matrix.asm
# This program multiplies two numbers and stores the result in memory

.data
result: .word 0  # Memory location to store the result

.text
main:
    # Load the first number into register $t0
    li $t0, 6  # Example number 6

    # Load the second number into register $t1
    li $t1, 7  # Example number 7

    # Initialize the result register $t2 to 0
    li $t2, 0

    # Initialize the counter register $t4 to 0
    li $t4, 0

multiply_loop:
    # Check if counter is equal to the second number
    beq $t4, $t1, end_multiply

    # Add the first number to the result
    add $t2, $t2, $t0

    # Increment the counter
    addi $t4, $t4, 1

    # Repeat the loop
    j multiply_loop

end_multiply:
    # Store the result in memory
    la $t3, result  # Load address of result into $t3
    sw $t2, 0($t3)  # Store the value of $t2 into memory at address in $t3

    .word 0xfeedfeed  # End of program marker