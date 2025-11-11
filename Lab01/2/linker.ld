ENTRY(kernel_main)  /* Kernel entry point */

SECTIONS {
  . = 0x10000;       /* Load address: physical 0x10000 (what ES:BX = 0x1000:0000 points to) */

  .text : {
    *(.text)         /* Code section */
  }

  .data : {
    *(.data)         /* Initialized data */
  }

  .bss : {
    *(.bss)          /* Uninitialized data (zero-filled) */
  }
}