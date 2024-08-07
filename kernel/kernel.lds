ENTRY(_start)
SECTIONS
{

	. = 0x100000;
	.text :
	{
		_text = .;
        *(.entry)
		*(.text)
		_etext = .;
	}

	. = ALIGN(8);

	.data :
	{
		_data = .;
		*(.data)		
		_edata = .;
	}

	.rodata : 
	{
		_rodata = .;	
		*(.rodata)
		_erodata = .;
	}


    _edata = . ;

    . = ALIGN(4096);


	.bss :
	{
        . = ALIGN(8);
		_bss = .;
		*(.bss)
        . = ALIGN(8);
		_ebss = .;
	}

    . = ALIGN(8); 
	_end = .;
}

