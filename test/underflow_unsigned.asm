# test/unsigned_underflow.asm
# NOTE: Properly functioning code should store 0x00000000  
# at memory address 0x000000c8 and 0xFFFFFFFF at 0x000000cc 

# Initialize registers with values
    li $t0, 0           # $t0 = 0x00000000 (minimum unsigned 32-bit integer)
    li $t1, 1           # $t1 = 1
    li $t6, 0x000000b4  # $t6 = 180

# Subtract values and store results
    subu $t2, $t0, $t1  # This should cause an unsigned arithmetic underflow
    sw $t2, 0($t6)      # Store result in memory

# EXCEPTION: Unsigned subtraction underflow
    li $t0, 0           # $t0 = 0x00000000 (minimum unsigned 32-bit integer)
    li $t1, 1           # $t1 = 1
    subu $t2, $t0, $t1  # This should cause an unsigned arithmetic underflow
    sw $t2, 4($t6)      # This instruction should not execute
    .word 0xfeedfeed    # This instruction should not execute
    
# Align so the handler executes
    .align 12                # Align to a 0x1000-byte boundary
    .org 0x8000              # Start placing the subsequent instructions at 0x8000

exception_handler:
    li   $t3, 0xDEADBEEF    # Load a test value to confirm execution here
    li   $t6, 0x000000c8    # $t6 = 200
    sw   $t3, 0($t6)        # Store the value of $t3 into memory at address in $t6
    addi $t3, $t3, 1        # Increment to check exception behavior 
    sw   $t3, 4($t6)        # Store the value of $t3 into memory at address in $t6
    .word 0xfeedfeed        # End of exception handler