main    proc    near
        jmp     debut

request dw      0
_type   db      014h            ; type
        db      0, 1            ; obj type
        db      48              ; obj name length 48
        db      ''
        db      48 dup (0)
        db      0               ; password length
        db      128 dup (0FFh)   ; None-password

reply   dw      0               ; buffer length

ok      db      'Login OK ! $'

debut:
        push    cs
        push    cs
        pop     ds
        pop     es
        lea     si, request
        lea     dx, reply
        sub     dx, si
        sub     dx, 2
        mov     word ptr request, dx
        lea     di, reply

        mov     ah, 0E3h
        int     021h

        or      al, al
        jnz     kt
        lea     dx, ok
        mov     ah, 09
        int     021h
kt:
        int     020h
main    endp
