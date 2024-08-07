ENTRY(_start)
SECTIONS
{
 . = 0x7c00,
 .text :
 {
  *(.entry);
  *(.text*);
 }

 .data : {
  *(.data);
  . = _start + 510;
  SHORT(0xAA55);
 }

}

