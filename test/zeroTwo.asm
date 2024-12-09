main:
    li $t6, 0x000000b4  # 0x000000b4
    li $t0, 10          # $t0 = 10
    add $t1, $t0, $zero # $t1 = $t0 + 0 (should be 10)
    sw $t1, 0($t6)      # 0x000000b4 = $t1 (should be 10)

    add $zero, $t1, $zero   # $zero = $t1 + 0 (should be 0)
    add $t1, $t0, $zero     # $t1 = $t0 + 0 (should be 10)
    sw $t1, 4($t6)          # 0x000000b4 = $t1 (should be 10)
    sw $zero, 8($t6)        # 0x000000b4 = $zero (should be 0)
    .word 0xfeedfeed        # End marker

    