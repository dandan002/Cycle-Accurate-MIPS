# test/test_exception.asm

# Initialize registers with values
    li $t0, 5      # $t0 = 5
    li $t1, 10     # $t1 = 10
    li $t2, 15     # $t2 = 15
    li $t3, 20     # $t3 = 20
    li $t4, 25     # $t4 = 25

# Add values and store results
    add $t5, $t0, $t1   # $t5 = $t0 + $t1 = 15
    sw $t5, 0($t6)      # Store result in memory

    add $t5, $t5, $t2   # $t5 = $t5 + $t2 = 30
    sw $t5, 4($t6)      # Store result in memory

    add $t5, $t5, $t3   # $t5 = $t5 + $t3 = 50
    sw $t5, 8($t6)      # Store result in memory

    add $t5, $t5, $t4   # $t5 = $t5 + $t4 = 75
    sw $t5, 12($t6)     # Store result in memory

# Illegal instruction (multiply not supported)
    mul $t5, $t5, $t0   # This should cause an illegal instruction exception
    sw $t5, 16($t6)     # This instruction should not execute