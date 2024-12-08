# Matrix multiplication of two 4x4 matrices
main:   la   $t0, Matrix1    # load address of first matrix
    la   $t1, Matrix2    # load address of second matrix
    la   $t2, Result     # load address of result matrix

    li   $s0, 0          # i = 0 (row counter)
loop1:  li   $s1, 0          # j = 0 (column counter)
loop2:  li   $s2, 0          # k = 0 (dot product counter)
    li   $t7, 0          # sum = 0

loop3:  # Calculate address for Matrix1[i][k]
    mul  $t3, $s0, 4     # i * 4
    add  $t3, $t3, $s2   # + k
    sll  $t3, $t3, 2     # * 4 (word size)
    add  $t3, $t3, $t0   # add base address
    lw   $t4, 0($t3)     # load Matrix1[i][k]

    # Calculate address for Matrix2[k][j]
    mul  $t3, $s2, 4     # k * 4
    add  $t3, $t3, $s1   # + j
    sll  $t3, $t3, 2     # * 4 (word size)
    add  $t3, $t3, $t1   # add base address
    lw   $t5, 0($t3)     # load Matrix2[k][j]

    mul  $t6, $t4, $t5   # multiply elements
    add  $t7, $t7, $t6   # add to sum

    addi $s2, $s2, 1     # k++
    blt  $s2, 4, loop3   # if k < 4, continue inner loop

    # Store result in Result[i][j]
    mul  $t3, $s0, 4     # i * 4
    add  $t3, $t3, $s1   # + j
    sll  $t3, $t3, 2     # * 4 (word size)
    add  $t3, $t3, $t2   # add base address
    sw   $t7, 0($t3)     # store sum in Result[i][j]

    addi $s1, $s1, 1     # j++
    blt  $s1, 4, loop2   # if j < 4, continue middle loop

    addi $s0, $s0, 1     # i++
    blt  $s0, 4, loop1   # if i < 4, continue outer loop

    .word 0xfeedfeed     # end program

# Data section
Matrix1: .word 1, 2, 3, 4    # First 4x4 matrix
    .word 5, 6, 7, 8
    .word 9, 10, 11, 12
    .word 13, 14, 15, 16

Matrix2: .word 1, 0, 0, 0    # Second 4x4 matrix
    .word 0, 1, 0, 0
    .word 0, 0, 1, 0
    .word 0, 0, 0, 1

Result: .word 0, 0, 0, 0     # Result matrix
    .word 0, 0, 0, 0
    .word 0, 0, 0, 0
    .word 0, 0, 0, 0