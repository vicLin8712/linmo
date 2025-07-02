# HAL: Context Switching for RISC-V

## Context Switching
Context switching is essential to Linmo's preemptive multitasking kernel,
facilitating smooth task transitions.
In Linmo, context switching is implemented through a clean separation of concerns architecture that combines the portability of standard C library functions with the performance requirements of real-time systems.
This approach provides both `setjmp` and `longjmp` functions following standard C library semantics for application use,
and dedicated HAL routines (`hal_context_save` and `hal_context_restore`) for optimized kernel scheduling.

## Separation of Concerns Architecture
The context switching implementation follows a clean layered approach that separates execution context management from processor state management:

### Standard C Library Layer
Portable, standards-compliant context switching for applications
- `setjmp` - Saves execution context only (elements 0-15)
- `longjmp` - Restores execution context only
- Semantics: Pure C library behavior, no processor state management
- Use Cases: Exception handling, coroutines, application-level control flow
- Performance: Standard overhead, optimized for portability

### HAL Context Switching Layer
Context switching for kernel scheduling
- `hal_context_save` - Saves execution context AND processor state
- `hal_context_restore` - Restores complete task state
- Semantics: System-level optimization with interrupt state management
- Use Cases: Preemptive scheduling, cooperative task switching
- Performance: Optimized for minimal overhead

### Unified Context Buffer
Both layers use the same `jmp_buf` structure but access different portions:

```c
typedef uint32_t jmp_buf[17];

/* Layout:
 * [0-11]:  s0-s11 (callee-saved registers) - both layers
 * [12]:    gp (global pointer) - both layers
 * [13]:    tp (thread pointer) - both layers
 * [14]:    sp (stack pointer) - both layers
 * [15]:    ra (return address) - both layers
 * [16]:    mstatus (processor state) - HAL layer only
 */
```

## Context Switch Process

### 1. Save Current Task State
The `hal_context_save` function captures complete task state including both execution context and processor state.
The function saves all callee-saved registers as required by the RISC-V ABI,
plus essential pointers (gp, tp, sp, ra).
For processor state, it performs sophisticated interrupt state reconstruction and ensures that tasks resume with correct interrupt state,
maintaining system responsiveness and preventing interrupt state corruption.

### 2. Select Next Task
The scheduler, invoked via `dispatcher()` during machine timer interrupts,
uses a priority-based round-robin algorithm or user-defined scheduler to select the next ready task.
The scheduling logic evaluates task priorities and readiness states to ensure optimal system responsiveness.

### 3. Restore Next Task State
The `hal_context_restore` function performs complete state restoration with processor state restored first to establish correct execution environment:

```c
lw  t0, 16*4(%0)        // Load saved mstatus from jmp_buf[16]
csrw mstatus, t0        // Restore processor state FIRST
// ... then restore all execution context registers
```

This ordering ensures that interrupt state and privilege mode are correctly established before resuming task execution.

## Processor State Management

### Interrupt State Reconstruction
The HAL context switching routines include sophisticated interrupt state management that handles the complexities of RISC-V interrupt processing:

During Timer Interrupts:
- `mstatus.MIE` is automatically cleared by hardware when entering the trap
- `mstatus.MPIE` preserves the previous interrupt enable state
- HAL functions reconstruct the original interrupt state from `MPIE`
- This ensures consistent interrupt behavior across context switches

State Preservation:
- Each task maintains its own interrupt enable state
- Context switches preserve privilege mode (Machine mode for kernel tasks)
- Interrupt state is reconstructed accurately for reliable task resumption

### Task Initialization
New tasks are initialized with proper processor state:

```c
void hal_context_init(jmp_buf *ctx, size_t sp, size_t ss, size_t ra)
{
    /* Set execution context */
    (*ctx)[CONTEXT_SP] = (uint32_t) stack_top;  // Stack pointer
    (*ctx)[CONTEXT_RA] = (uint32_t) ra;         // Entry point
    /* Set processor state */
    (*ctx)[CONTEXT_MSTATUS] = MSTATUS_MIE | MSTATUS_MPP_MACH;
}
```

This ensures new tasks start with interrupts enabled in machine mode.

## Implementation Details

### Kernel Integration
The kernel scheduler uses the context switching routine:

```c
/* Preemptive context switching */
void dispatch(void)
{
    /* Save current task with processor state */
    if (hal_context_save(current_task->context) != 0)
        return;  /* Restored from context switch */
 
    /* ... scheduling logic ... */

    /* Restore next task with processor state*/
    hal_context_restore(next_task->context, 1);
}
```

## Best Practices

### Architecture Principles
- Layer Separation: Keep application and system contexts separate
- Standard Compliance: Use standard functions for portable code
- Performance Optimization: Use HAL functions for time-critical system code
- State Management: Let HAL functions handle processor state automatically

### Development Guidelines
- Application Code: Always use `setjmp` and `longjmp` for exception handling
- System Code: Always use `hal_context_save` and `hal_context_restore` for scheduling
- Mixed Use: Both can operate on the same `jmp_buf` without interference
- Testing: Verify interrupt state preservation across context switches

### Interrupt State Machinery
RISC-V Interrupt Behavior During Traps:
1. Hardware automatically clears `mstatus.MIE` on trap entry
2. Previous interrupt state saved in `mstatus.MPIE`
3. Privilege level preserved in `mstatus.MPP`
4. HAL functions reconstruct original interrupt state for task resumption

State Reconstruction Logic:
```
Original MIE state = Current MPIE bit
Reconstructed mstatus = (current_mstatus & ~MIE) | (MPIE >> 4)
```

This ensures tasks resume with their original interrupt enable state rather than the disabled state from trap entry.

### Task State Lifecycle
New Task Creation:
1. `hal_context_init` sets up initial execution context
2. Stack pointer positioned with ISR frame reservation
3. Return address points to task entry function
4. Processor state initialized with interrupts enabled

First Task Launch:
1. `hal_dispatch_init` transfers control from kernel to first task
2. Global interrupts enabled just before task execution
3. Timer interrupts activated for preemptive scheduling
4. Task begins execution at its entry point

Context Switch Cycle:
1. Timer interrupt triggers scheduler entry
2. `hal_context_save` preserves complete current task state
3. Scheduler selects next ready task based on priority
4. `hal_context_restore` resumes selected task execution
5. Task continues from its previous suspension point
