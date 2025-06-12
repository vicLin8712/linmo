/* RISC‑V linker script for QEMU 'virt' machine */

OUTPUT_ARCH(riscv)
OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv", "elf32-littleriscv")

/* Memory Layout:
 * The QEMU 'virt' machine provides 128 MiB of RAM starting at 0x80000000.
 */
MEMORY
{
    ram (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

/* Entry point of the kernel */
ENTRY(_entry)

/* Stack Configuration:
 * The stack grows downwards from the very top of RAM.
 * _stack_top: Highest address in RAM, the initial value of the stack pointer.
 * _stack_bottom: The lowest address allocated to the stack (stack limit).
 */
_stack_size   = 4M;                          /* 4 MiB total stack space */
_stack_top    = ORIGIN(ram) + LENGTH(ram);   /* Top of RAM */
_stack_bottom = _stack_top - _stack_size;
_stack        = _stack_top;                  /* Initial SP value */

/* Heap Configuration:
 * The heap grows upwards from the end of the .bss section towards the stack.
 * _heap_start: Start of the heap, immediately after static data.
 * _heap_end: End of available heap memory, just below the stack.
 */
_heap_start  = _end;
_heap_end    = _stack_bottom;
_heap_size   = _heap_end - _heap_start;

/* Ensure a minimum heap size is available for the kernel to function. */
ASSERT(_heap_size >= 1M, "ERROR: Insufficient heap space (minimum 1MB required)")

/* Program headers for ELF segments. These describe how sections are mapped
 * into memory segments for the loader.
 * FLAGS: R=4 (read), W=2 (write), X=1 (execute)
 */
PHDRS
{
    text PT_LOAD FLAGS(0x5);  /* R+X: Read + Execute segment for code and rodata */
    data PT_LOAD FLAGS(0x6);  /* R+W: Read + Write segment for data and bss    */
}

/* Section Layout */
SECTIONS
{
    /* .text section: Code and read-only data.
     * Mapped to the executable 'text' segment.
     */
    .text : ALIGN(4) {
        _stext = .;
        _text = .;

        /* Startup code must be placed first to handle reset. */
        KEEP(*(.text.prologue))             /* Boot/reset vector (_entry) */
        KEEP(*(.text.init))                 /* Early initialization code */

        /* Main program code sections */
        *(.text.unlikely .text.*_unlikely .text.unlikely.*)
        *(.text.exit .text.exit.*)
        *(.text.startup .text.startup.*)
        *(.text.hot .text.hot.*)
        *(.text .stub .text.* .gnu.linkonce.t.*)

        . = ALIGN(4);
        _etext_code = .;

        /* Read-only data sections are placed with code. */
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        *(.rodata1)
        *(.rdata)
        *(.srodata .srodata.*)
        *(.data.rel.ro.local* .gnu.linkonce.d.rel.ro.local.*)
        *(.data.rel.ro .data.rel.ro.* .gnu.linkonce.d.rel.ro.*)

        . = ALIGN(4);
        _etext = .;
    } > ram :text

    /* .data section: Initialized read-write data.
     * Mapped to the read-write 'data' segment.
     */
    .data : ALIGN(4) {
        _sidata = LOADADDR(.data); /* LMA for copying from ROM to RAM if needed */
        _sdata = .;
        _data = .;

        *(.data .data.* .gnu.linkonce.d.*)
        *(.data1)

        /* Global Pointer (gp) setup for RISC-V. 'gp' provides a base for
         * efficient access to a 4KB block of global variables. It is
         * conventionally placed in the middle of this block.
         */
        . = ALIGN(8);
        PROVIDE(__global_pointer$ = . + 0x800);
        _gp = . + 0x800;

        /* Small data sections (accessed relative to gp) */
        *(.sdata .sdata.* .gnu.linkonce.s.*)
        *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)

        . = ALIGN(4);
        _edata = .;
    } > ram :data

    /* .bss section: Zero-initialized (or uninitialized) data.
     * Occupies memory but has no data in the ELF file (NOLOAD).
     * The startup code is responsible for clearing this section to zero.
     */
    .bss (NOLOAD) : ALIGN(4) {
        _sbss = .;
        _bss = .;

        /* Small BSS sections (accessed relative to gp) */
        *(.sbss .sbss.* .gnu.linkonce.sb.*)
        *(.sbss2 .sbss2.* .gnu.linkonce.sb2.*)
        *(.scommon)

        /* Standard BSS sections */
        *(.dynbss)
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(COMMON)

        . = ALIGN(4);
        _ebss = .;
    } > ram :data

    /* End marker for static data. Used to locate the start of the heap. */
    _end = .;
    PROVIDE(end = .);

    /* Debug sections (not loaded into target memory) */
    .stab          0 : { *(.stab) }
    .stabstr       0 : { *(.stabstr) }
    .stab.excl     0 : { *(.stab.excl) }
    .stab.exclstr  0 : { *(.stab.exclstr) }
    .stab.index    0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment       0 : { *(.comment) }
    .gnu.build.attributes : { *(.gnu.build.attributes .gnu.build.attributes.*) }

    /* DWARF debug information sections */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line .debug_line.* .debug_line_end) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }
    .debug_pubtypes 0 : { *(.debug_pubtypes) }
    .debug_ranges   0 : { *(.debug_ranges) }
    .debug_macro    0 : { *(.debug_macro) }
    .debug_addr     0 : { *(.debug_addr) }
    .debug_line_str 0 : { *(.debug_line_str) }
    .debug_loclists 0 : { *(.debug_loclists) }
    .debug_rnglists 0 : { *(.debug_rnglists) }
    .debug_str_offsets 0 : { *(.debug_str_offsets) }
    .debug_sup      0 : { *(.debug_sup) }

    /* RISC-V specific sections */
    .riscv.attributes 0 : { *(.riscv.attributes) }

    /* Discarded sections that are not needed in the final kernel image */
    /DISCARD/ : {
        *(.note.GNU-stack)
        *(.gnu_debuglink)
        *(.gnu.lto_*)
        *(.discard)
        *(.discard.*)
    }
}
