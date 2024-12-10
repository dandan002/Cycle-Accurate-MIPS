# Arithmetic overflow test
main:  li $t0, 0xFFFFFFFF      # Load value. switch first F to 7 for signed test
       addu  $t0, $t0, $t0        # $t1 = $t0 + $t0 -> Overflow occurs
       addi $t0, $t0, 2
       .word 0xfeedfeed
       
      # end of their test
      # Align the exception handler at address 0x8000
      .align 12                  # Align to a 0x1000-byte boundary (address 0x8000)
      .org 0x8000                # Start placing the subsequent instructions at 0x8000

exception_handler:
      li   $t3, 0xECEE374F       # Load a test value to confirm execution here
      addi $t3, $t3, 1           # Increment to check exception behavior 
      .word 0xfeedfeed  