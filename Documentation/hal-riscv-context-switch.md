# HAL: Context Switching for RISC-V

## Context Switching

Context switching is essential to Linmo's preemptive multitasking kernel, facilitating smooth task transitions. In Linmo, this is managed through `setjmp` and `longjmp` functions, implemented in [arch/riscv/hal.c](../arch/riscv/hal.c) and declared in [arch/riscv/hal.h](../arch/riscv/hal.h). These functions enable the kernel to save and restore task states, supporting reliable multitasking. The process involves unique considerations due to the non-standard use of these functions, requiring careful handling to ensure system stability.

### Repurposed setjmp and longjmp

The `setjmp` and `longjmp` functions, typically used for exception handling, are repurposed in Linmo for context switching. They save additional states beyond standard registers, including CSRs like `mcause`, `mepc`, and `mstatus`, which may lead to unexpected behavior if not properly managed. During timer interrupts, `setjmp` must handle `mstatus.MIE` being cleared, relying on `MPIE` reconstruction, a departure from their usual role. Similarly, `longjmp` restores context without reinitializing local variables or the call stack, posing risks of resource leaks in tasks with dynamic memory. These deviations demand precise implementation to maintain task switching integrity.

### Context Switch Process

1. Save Current Task State: The `setjmp` function captures the current task's CPU state, storing it in a `jmp_buf` structure defined as `uint32_t jmp_buf[19]` in [arch/riscv/hal.h](../arch/riscv/hal.h). This includes callee-saved general-purpose registers (such as `s0` to `s11`), essential pointers (`gp`, `tp`, `sp`, `ra`), and CSRs (`mcause`, `mepc`, `mstatus`). The layout is defined by `CONTEXT_*` macros in [arch/riscv/hal.c](../arch/riscv/hal.c). Failing to reconstruct `mstatus` from `MPIE` during this step can cause incorrect interrupt settings, leading to missed timer interrupts or stalls, which is mitigated by ensuring proper `mstatus` reconstruction.

2. Select Next Task: The scheduler, invoked via `dispatcher()` during a machine timer interrupt, selects the next task using a priority-based round-robin algorithm or a user-defined scheduler. This evaluates task priorities and readiness, ensuring system responsiveness. The process relies on accurate `mstatus` restoration to avoid timing issues.

3. Restore Next Task State: The `longjmp` function restores the CPU state from the selected task’s `jmp_buf`, resuming execution where `setjmp` was called. For new tasks, `hal_context_init` initializes the `jmp_buf` with `sp`, `ra`, and `mstatus` set to `MSTATUS_MIE` and `MSTATUS_MPP_MACH`, while `hal_dispatch_init` launches the task. The `hal_interrupt_tick` function enables interrupts (`_ei()`) after the first task, ensuring a consistent environment. Premature restoration of `mstatus` can disrupt scheduling, addressed by prioritizing it before other registers.

## Machine Status Management

The machine status in RISC-V is managed through the `mstatus` CSR, which controls critical system states, such as the Machine Interrupt Enable (`MIE`) bit. Proper handling of `mstatus` during context switching is essential to maintain correct interrupt behavior and ensure tasks execute as expected.

### Role of `mstatus`

The `mstatus` register includes the `MIE` bit (bit 3), which enables or disables machine-mode interrupts globally, and the `MPIE` bit (bit 7), which preserves the previous interrupt enable state during a trap, allowing `MIE` to be reconstructed afterward. Other fields, such as those for privilege mode and memory protection, are less relevant in Linmo’s single-address-space model but must still be preserved. The `hal_interrupt_set` function in [arch/riscv/hal.h](../arch/riscv/hal.h) manipulates `MIE` using `read_csr(mstatus)` and `write_csr(mstatus, val)`, with convenience macros `_di()` (disable interrupts) and `_ei()` (enable interrupts).

### Saving and Restoring `mstatus`

1. Saving in `setjmp`: The `setjmp` function reads `mstatus` using the `csrr` instruction. Since `mstatus.MIE` is cleared during a trap, the previous interrupt state is reconstructed from `MPIE` by shifting bit 7 to bit 3, clearing the original `MIE`, and restoring it, then storing the result in `jmp_buf` at `CONTEXT_MSTATUS` (18). This ensures accurate interrupt context preservation, preventing issues from incorrect handling.

2. Restoring in `longjmp`: The `longjmp` function loads `mstatus` from `jmp_buf` and writes it back with `csrw` before other registers, ensuring early interrupt state establishment. This prevents premature interrupt handling and maintains consistency.

3. Timing and drift considerations: Incorrect `mstatus` restoration can disrupt scheduling by enabling interrupts at the wrong time, resolvable through testing with scheduler-stressing applications like message queues. Timer interrupt drift, if not handled carefully, is addressed by `do_trap` scheduling relative to the previous `mtimecmp` value, requiring `mstatus.MIE` to be enabled for effective reception.

### Best Practices for Machine Status

- Prioritize `mstatus` restoration: Restore `mstatus` before other registers in `longjmp` to establish the correct interrupt state early.

- Use safe CSR access: Use `read_csr` and `write_csr` macros in [arch/riscv/hal.h](../arch/riscv/hal.h) for consistent CSR manipulation.

- Initialize `mstatus` for new tasks: Set `MSTATUS_MIE` and `MSTATUS_MPP_MACH` in `hal_context_init` to ensure new tasks start with interrupts enabled.
