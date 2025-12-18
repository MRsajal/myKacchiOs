.global ctxsw
.type ctxsw, @function

ctxsw:
    pusha          # Save all general-purpose registers
    movl 36(%esp), %eax  # Load old ESP from stack
    movl %eax, %esp      # Switch to old stack
    movl 40(%esp), %eax  # Load new ESP from stack
    movl %eax, %esp      # Switch to new stack
    popa           # Restore all general-purpose registers
    ret            # Return to the caller