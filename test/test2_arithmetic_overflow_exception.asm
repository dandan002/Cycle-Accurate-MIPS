# overflow_exception_modified.asm
# This program triggers an arithmetic overflow exception using the SUBU instruction

main:
    li   $t0, -1               
    lui  $t4, 0x7FFF
    ori  $t4, 0xFFFF
    la   $t1, Arr              # loading this address prevents memory access error as Arr is defined below
    la   $t5, Arr              # loading this address prevents memory access error as Arr is defined below
    lw   $t1, 0($t5)           
    sw   $t6, 0($t1)           
    sub $t2, $t4, $t0          # Overflow: = 0x80000000 (exceeds signed 32-bit range)
    .word 0xfeedfeed           # End of program (should be skipped by exception)

    # Align the exception handler at address 0x8000
    .align 12                  # Align to a 0x1000-byte boundary (address 0x8000)
    .org 0x8000                # Start placing the subsequent instructions at 0x8000

exception_handler:
    li   $t3, 0xDEADBEEF       # Load a test value to confirm execution here
    addi $t3, $t3, 1           # Increment to check exception behavior 
    .word 0xfeedfeed           # End of exception handler

Arr: .word 0x1             # Initial value in array
