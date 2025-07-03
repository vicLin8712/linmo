# HAL: Interrupt Management

## Overview
The Linmo kernel provides a comprehensive interrupt management system designed for RISC-V RV32I architecture.
Its interrupt control mechanisms do not require explicit interrupt status variables to be declared by the caller.
Instead, the system provides both low-level primitives and high-level critical section macros that automatically manage interrupt state.

## Interrupt Control Levels
Linmo provides two distinct levels of interrupt control to balance system responsiveness with data protection:

### 1. Global Interrupt Control (`CRITICAL_*`)
Controls ALL maskable interrupts, providing the strongest protection against concurrency.

### 2. Scheduler Interrupt Control (`NOSCHED_*`)
Controls ONLY the scheduler timer interrupt, allowing other hardware interrupts to be serviced while preventing task preemption.

## Low-Level Interrupt Control

### Basic Functions
```c
/* Enable/disable global interrupts */
#define _di() hal_interrupt_set(0)  /* Disable interrupts */
#define _ei() hal_interrupt_set(1)  /* Enable interrupts */

/* Fine-grained control with return value */
int32_t hal_interrupt_set(int32_t enable);
```

The `hal_interrupt_set()` function returns the previous interrupt enable state (1 if enabled, 0 if disabled), allowing for proper nesting.

### Correct Usage Example

```c
void low_level_function(void) {
    int32_t prev_state = _di();  /* Disable interrupts, save previous state */
    
    /* Critical code section */
    /* ... */
    
    hal_interrupt_set(prev_state);  /* Restore previous state */
}
```

### Incorrect Usage Example

```c
void bad_function(void) {
    _di();
    /* Critical code section */
    _ei();  /* WRONG: Always enables interrupts regardless of previous state */
}
```

Problem: The incorrect example always enables interrupts with `_ei()`,
even if interrupts were already disabled when the function was called.
This can lead to unexpected interrupt behavior in nested critical sections.

## High-Level Critical Section Macros

### Global Critical Sections

Use `CRITICAL_ENTER()` and `CRITICAL_LEAVE()` when protecting data shared with interrupt service routines (ISRs):

```c
void shared_data_access(void) {
    CRITICAL_ENTER();
    
    /* Access data shared with ISRs */
    global_counter++;
    shared_buffer[index] = value;
    
    CRITICAL_LEAVE();
}
```

When to use:
- Modifying data structures accessed by ISRs
- Hardware register manipulation
- Memory allocation/deallocation in ISR context
- Any operation that must be atomic with respect to ALL interrupts

**Warning**: Increases interrupt latency - use sparingly and keep critical sections short.

### Scheduler Critical Sections

Use `NOSCHED_ENTER()` and `NOSCHED_LEAVE()` when protecting data shared only between tasks:
```c
void task_shared_data_access(void) {
    NOSCHED_ENTER();
    
    /* Access data shared between tasks only */
    task_list_modify();
    update_task_counters();
    
    NOSCHED_LEAVE();
}
```

When to use:
- Modifying task-related data structures
- Protecting against task preemption only
- Operations that should be atomic with respect to the scheduler
- Most kernel synchronization primitives (semaphores, mutexes, etc.)

Advantage: Lower interrupt latency - hardware interrupts (UART, etc.) can still be serviced.

## Automatic State Management

Unlike systems requiring explicit `intsts` declarations, Linmo's macros automatically handle interrupt state:

```c
/* Linmo approach - automatic state management */
void linmo_function(void) {
    CRITICAL_ENTER();  /* Automatically saves current state */
    /* Critical section */
    CRITICAL_LEAVE();  /* Automatically restores previous state */
}

/* Compare with uT-Kernel approach */
void ut_kernel_function(void) {
    UINT intsts;       /* Must explicitly declare */
    DI(intsts);        /* Must pass intsts parameter */
    /* Critical section */
    EI(intsts);        /* Must pass intsts parameter */
}
```

## Implementation Details

### Preemptive vs Cooperative Mode
The critical section macros are mode-aware:
```c
#define CRITICAL_ENTER()     \
    do {                     \
        if (kcb->preemptive) \
            _di();           \
    } while (0)

#define NOSCHED_ENTER()          \
    do {                         \
        if (kcb->preemptive)     \
            hal_timer_disable(); \
    } while (0)
```

In cooperative mode, these macros become no-ops since tasks yield voluntarily.

### RISC-V Implementation
The interrupt control directly manipulates the `mstatus.MIE` bit:
```c
static inline int32_t hal_interrupt_set(int32_t enable)
{
    uint32_t mstatus_val = read_csr(mstatus);
    uint32_t mie_bit = (1U << 3); /* MSTATUS_MIE bit position */

    if (enable) {
        write_csr(mstatus, mstatus_val | mie_bit);
    } else {
        write_csr(mstatus, mstatus_val & ~mie_bit);
    }
    return (int32_t) ((mstatus_val >> 3) & 1);
}
```

## Common Pitfalls

### 1. Forgetting Critical Sections
```c
/* WRONG - race condition possible */
volatile int shared_var;
void unsafe_increment(void) {
    shared_var++;  /* Not atomic on most architectures */
}

/* CORRECT */
void safe_increment(void) {
    CRITICAL_ENTER();
    shared_var++;
    CRITICAL_LEAVE();
}
```

### 2. Using Wrong Protection Level
```c
/* WRONG - insufficient protection for ISR-shared data */
void isr_shared_access(void) {
    NOSCHED_ENTER();  /* Only blocks scheduler, not ISRs! */
    isr_shared_buffer[0] = value;
    NOSCHED_LEAVE();
}

/* CORRECT */
void isr_shared_access(void) {
    CRITICAL_ENTER();  /* Blocks all interrupts including ISRs */
    isr_shared_buffer[0] = value;
    CRITICAL_LEAVE();
}
```

### 3. Calling Blocking Functions in Critical Sections
```c
/* WRONG - can deadlock */
void bad_critical_section(void) {
    CRITICAL_ENTER();
    mo_sem_wait(semaphore);  /* May block indefinitely! */
    CRITICAL_LEAVE();
}

/* CORRECT */
void good_pattern(void) {
    mo_sem_wait(semaphore);  /* Block outside critical section */
    
    CRITICAL_ENTER();
    /* Quick critical work */
    CRITICAL_LEAVE();
}
```
