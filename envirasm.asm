        Title   EnvirAsm - Environment check external routine
        Public  dvtest, apicall
Code    Segment Para Public 'Code'
        Assume  Cs:Code

ver dw 0
param dw 0

dvtest Proc Far
      mov cx,4445h
      mov dx,5351h
      mov ax,2b01h
      int 21h
      cmp al,0ffh
      jne dvok
      xor bx,bx
      dvok:
      mov ver,bx
      mov ax,bx
      push ax
      ret
dvtest endp

apicall Proc Far
      pop ax
      mov param,ax
      mov ax,101ah
      int 15h
      mov ax,param
      int 15h
      mov ax,1025h
      int 15h
      ret
apicall endp

Code  EndS
      End
