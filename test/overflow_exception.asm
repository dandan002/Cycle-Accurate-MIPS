# test/test_overflow-exception.asm
# NOTE: Properly functioning code should store 0xDEADBEEF  
# at memory address 0x000000c8 and 0xDEADBEF0 at 0x000000cc 

# Initialize registers with values
    li $t0, 5      # $t0 = 5
    li $t1, 10     # $t1 = 10
    li $t2, 15     # $t2 = 15
    li $t3, 20     # $t3 = 20
    li $t4, 25     # $t4 = 25
    li $t6, 0x000000b4  # $t6 = 180

# Add values and store results
    add $t5, $t0, $t1   # $t5 = $t0 + $t1 = 15
    sw $t5, 0($t6)      # Store result in memory

    add $t5, $t5, $t2   # $t5 = $t5 + $t2 = 30
    sw $t5, 4($t6)      # Store result in memory

    add $t5, $t5, $t3   # $t5 = $t5 + $t3 = 50
    sw $t5, 8($t6)      # Store result in memory

    add $t5, $t5, $t4   # $t5 = $t5 + $t4 = 75
    sw $t5, 12($t6)     # Store result in memory

# EXCEPTION: Addition overflow
    li $t0, 2147483647  # $t0 = 2^31 - 1 (maximum positive 32-bit integer)
    li $t1, 25          # $t1 = 25
    add $t2, $t0, $t1   # This should cause an arithmetic overflow exception
    sw $t2, 16($t6)     # This instruction should not execute
    .word 0xfeedfeed    # This instruction should not execute
    
# algin so the handler executes
    .align 12                # Align to a 0x1000-byte boundary
    .org 0x8000              # Start placing the subsequent instructions at 0x8000

exception_handler:
    li   $t3, 0xDEADBEEF    # Load a test value to confirm execution here
    li   $t6, 0x000000c8    # $t6 = 200
    sw   $t3, 0($t6)        # Store the value of $t3 into memory at address in $t6
    addi $t3, $t3, 1        # Increment to check exception behavior 
    sw   $t3, 4($t6)        # Store the value of $t3 into memory at address in $t6
    .word 0xfeedfeed        # End of exception handler