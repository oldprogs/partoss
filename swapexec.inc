;
; Conditional copmilation definitions
;
GenMsg	Equ	1	; Generate text messages & support routine if NonZero
;
; Include file with actions bit mask definitions
;
Ac_Ems	Equ	1		; Allow swapping to EMS
Ac_Xms	Equ	2		; Allow swapping to XMS
Ac_Dsk	Equ	4		; Allow swapping to disk
Ac_EmXm Equ	8		; EMS first & XMS after instead of XMS first & EMS after
Ac_Path Equ	10h		; Perform search_on_PATH
Ac_Used Equ	20h		; Swap used memory only, entire memory else
Ac_War	Equ	40h		; Ignore warnings
Ac_Err	Equ	80h		; Ignore errors & warnings
Ac_Sev	Equ	100h		; Ignore severe errors, errors & warnings
Ac_Fat	Equ	200h		; Ignore fatal errors, severe errors, errors & warnings
Ac_SPrm Equ	400h		; Do not display swap parameters
Ac_CoEx Equ	800h		; Search for .Com & .Exe files automatically
Ac_FDsk Equ	1000h		; Force disk swapping if EMS/XMS allocation failure
Ac_FExp Equ	2000h		; Resolve file name to execute using full path
;
; Possible error codes
;
Er_LAlo Equ	-1		; Last memory allocation error
Er_LFre Equ	-2		; Last memory block free error
Er_I01	Equ	-3		; Internal: negative swappable size !
Er_I02	Equ	-4		; Internal: can not free XMS after allocation
Er_Bdv	Equ	-5		; Bad device name in swap file name
Er_SwN	Equ	-6		; Not enough room for swapping in specified swap areas
Er_FCre Equ	-7		; Swap file creation error
Er_FWrt Equ	-8		; Unexpected write error while swapping
Er_WEof Equ	-9		; Unexpected end of file while swapping
Er_Cwe	Equ	-10		; Work file close error
Er_XAf	Equ	-11		; XMS allocation error
Er_Xwe	Equ	-12		; XMS write data error
Er_I03	Equ	-13		; Internal: Invalid swap destination
Er_Eaf	Equ	-14		; EMS allocation failure
Er_ESg	Equ	-15		; EMS segment request error
Er_Eme	Equ	-16		; EMS map error
Er_I04	Equ	-17		; Internal: can not truncate self code
Er_Exe	Equ	-18		; DOS EXEC function failed
Er_SAlo Equ	-19		; Swapped block allocation error
;
; Fatal errors ID's
;
Fa_Suc	Equ	0		; No error, success
Fa_SwAl Equ	1		; Can not re-allocate swapped area
Fa_Tsr	Equ	2		; Memory structure still occupied after EXEC, possible TSR
Fa_I01	Equ	3		; Internal: Restoring from nothing
Fa_OpnF Equ	4		; Can not re-open swap file
Fa_FRdE Equ	5		; Error reading swap file
Fa_Eof	Equ	6		; Unexpected end of swap file
Fa_ClSf Equ	7		; Can not close swap file
Fa_Del	Equ	8		; Can not delete swap file
Fa_XMov Equ	9		; Unexpected error during XMS move
Fa_XFre Equ	10		; Can not free XMS memory
Fa_I02	Equ	11		; Internal:Invalid restore source
Fa_GSeg Equ	12		; Can not get EMS mapping address
Fa_Map	Equ	13		; Can not map EMS page
Fa_EFre Equ	14		; Can not free EMS memory
Fa_CkSm Equ	15		; Swapped code checksum error
Fa_CEnd Equ	16		; Internal: Can not find end of command string
;
; Last error sub-code types
;
Ln_None Equ	0		; No sub-code
Ln_Dos	Equ	1		; DOS error sub-code
Ln_Xms	Equ	2		; XMS error sub-code
Ln_Ems	Equ	3		; EMS error sub-code
;
; Last swap destination
;
Fr_None	Equ	0		; Not defined
Fr_Dsk	Equ	1		; Disk
Fr_Xms	Equ	2		; XMS
Fr_Ems	Equ	4		; EMS
;
