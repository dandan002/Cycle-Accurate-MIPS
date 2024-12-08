# Illegal instruction test
main: .word 0xFFFFFFFF           # Undefined instruction encoding (e.g., OP_ILLEGAL)
      add $t0, $t0, $t0
      addi $t1, $t1, 0
      .word 0xfeedfeed

      # Align the exception handler at address 0x8000
      .align 12                  # Align to a 0x1000-byte boundary (address 0x8000)
      .org 0x8000                # Start placing the subsequent instructions at 0x8000

exception_handler:
      li   $t3, 0xECEE374F       # Load a test value to confirm execution here
      addi $t3, $t3, 1           # Increment to check exception behavior 
      .word 0xfeedfeed           # End of exception handler