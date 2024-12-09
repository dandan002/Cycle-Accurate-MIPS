# zero_register_dependency.asm
# This program ensures that dependencies on the $zero register do not cause stalls.

main:
    # Initialize test values
    li   $t0, 10            
    add  $t1, $t0, $zero    

    lw   $zero, 0($t0)
    bne  $zero, $zero, done

    add  $t1, $t0, $zero    
    add  $t1, $t1, $zero    
    add  $t1, $t1, $zero    

    # again to check cache
    lw   $zero, 0($t0)
    bne  $zero, $zero, done
    
    addi $zero, $zero, 10      # $t1 = $t0 + 10
    addi $zero, $zero, 10      # $t1 = $t0 + 10
    beq  $zero, $t0, skip   

    li $t1, 0x9FFFFFFF     
    add  $t1, $t1, $t1        
    .word 0xfeedfeed

    .align 12                
    .org 0x8000              

exception_handler:
    li   $t1, 0xDEADBEEF     
    addi $t1, $t1, 1         
    .word 0xfeedfeed         

skip:
    li   $t1, 0xAAAAAAAA
    sub  $t3, $t1, $zero    

    # End of program
    .word 0xfeedfeed        # End marker

