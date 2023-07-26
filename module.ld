OUTPUT_ARCH(arm)
ENTRY(mod_start)

MEMORY
{
  RAM012L   (rwx) : ORIGIN = 0x20020000, LENGTH = 0x002E0000
}

SECTIONS
{

.reset loadable_mod_data :
{
  *startup.o (.text)
} > RAM012L

.text :
{
  text_start = .;
  *(.text)
  address_end_text = .;
} > RAM012L

.rodata :
{
  *(.rodata)
  *(.rodata.*)
  . = ALIGN(0x8);
  _data = .;
  *(.data)
  *(.data.*)
  _edata = .;

  *(.got.plt)
  *(.got)
  . = ALIGN(0x8);
} > RAM012L

.preinit_array :
{
  PROVIDE_HIDDEN (__mod_preinit_array_start = .);
  KEEP (*(.preinit_array*))
  PROVIDE_HIDDEN (__mod_preinit_array_end = .);
} >RAM012L

.init_array :
{
  PROVIDE_HIDDEN (__mod_init_array_start = .);
  KEEP (*(SORT(.init_array.*)))
  KEEP (*(.init_array*))
  PROVIDE_HIDDEN (__mod_init_array_end = .);
} >RAM012L

.fini_array :
{
  PROVIDE_HIDDEN (__mod_fini_array_start = .);
  KEEP (*(SORT(.fini_array.*)))
  KEEP (*(.fini_array))
  PROVIDE_HIDDEN (__mod_fini_array_end = .);
} >RAM012L

}
