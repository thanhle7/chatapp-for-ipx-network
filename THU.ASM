main    proc    near
        jmp     thu

error   db      000h, 'OK. No error found.     $'
        db      096h, 'Server is out of memory $'
        db      0E9h, 'Member is already exist $'
        db      0EBh, 'Not a set property.     $'
        db      0F0h, 'Wild card not allowed.  $'
        db      0F1h, 'Invalid Bindery security$'
        db      0F8h, 'No privilege to modify. $'
        db      0FBh, 'No such property.       $'
        db      0FCh, 'No such object.         $'
        db      0FEh, 'Server bindery locked.  $'
        db      0FFh, 'Bindery hardware failure$'           ; 26 bytes
        db      011h, 'Error can''t be checked.$'

addbind dw      0               ; Length
        db      042h
        db      0, 2
        db      48
        db      'MANAGER'
        db      41 dup (0)
        db      16
        db      'GROUP_MEMBERS'
        db      3 dup (0)
        db      0, 1
        db      48
        db      'GUEST'
        db      43 dup (0)

reply   dw      0

chgbind dw      53              ; change bindery object security
        db      038h
        db      0               ; Change right for any
        db      0, 1            ; object type
        db      48
        db      'GUEST'
        db      43 dup (0)

chgpass dw      310
        db      040h            ; type
        db      0, 1
        db      48
        db      'GUEST'
        db      43 dup (0)      ;  52
        db      127             ; 258
        db      127 dup (0)     ;=====
        db      127             ; 310
        db      'TUC QUA DI'
        db      117 dup (0)

chgprop dw      70
        db      03Bh
        db      0, 1
        db      48
        db      'GUEST'
        db      43 dup (0)
        db      033h            ; new property value for any
        db      16
        db      'PASSWORD'
        db      8 dup (0)

crebind dw      54              ; creat bindery object
        db      032h            ; 033h to delete
        db      0               ; object flag static
        db      3               ; anyone
        db      0, 2            ; type
        db      48
        db      'TUC'
        db      45 dup (0)

creprop dw      71
        db      039h
        db      0, 2            ; object type
        db      48
        db      'EVERYONE'
        db      40 dup (0)
        db      2               ; property flag
        db      3               ; property security
        db      16
        db      'TUC'
        db      13 dup (0)


thu:    push    cs
        push    cs
        pop     ds
        pop     es

;       call    crebindproc
;       call    crepropproc

;       call    chgbindproc
;       call    showerror

;       call    chgpropproc
;       call    showerror

        call    chgpassproc
        call    showerror
        int     020h


        lea     dx, reply
        lea     bx, addbind
        sub     dx, bx
        sub     dx, 2
        mov     word ptr addbind, dx
        lea     DI, reply
        lea     SI, addbind
        mov     ah, 0E3h
        int     021h
        call    showerror

main    endp
;==========================
chgbindproc     proc    near
        lea     si, chgbind
        lea     di, reply
        mov     ah, 0E3h
        int     021h
        ret
chgbindproc     endp
;==========================

chgpassproc proc    near
        lea     dx, chgprop
        lea     bx, chgpass
        sub     dx, bx
        sub     dx, 2
        mov     word ptr chgpass, dx
        lea     si, chgpass
        les     di, reply
        mov     ah, 0E3h
        int     021h
        ret
chgpassproc endp
;==========================
chgpropproc     proc    near
        mov     ah, 0E3h
        lea     si, chgprop
        lea     di, reply
        int     021h
        ret
chgpropproc     endp
;==========================

crepropproc proc    near

        lea     si, creprop
        les     di, reply
        mov     ah, 0E3h
        int     021h
        ret
crepropproc endp
;==========================
crebindproc proc    near

        lea     si, crebind
        les     di, reply
        mov     ah, 0E3h
        int     021h
        ret
crebindproc endp
;==========================
showerror       proc    near
        mov     cx, 11
        lea     si, error
check:
        cmp     al, byte ptr ds:[si]
        je      continue
        add     si, 26
        loop    check
continue:
        mov     dx, si
        inc     dx
        mov     ah, 09
        int     021h
        ret
showerror       endp
