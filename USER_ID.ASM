main    proc    near
        jmp     m0

objtype db      'Unknown   $'
        db      'User      $'
        db      'User Group$'
        db      'Others    $'

request dw      2               ; request length
        db      16              ; sub function
        db      0               ; con_num

reply   dw      61              ; reply length
        dd      0               ; object ID
        dw      0               ; obj type
        db      48 dup ('A')    ; obj name
        db      7 dup (0)       ; login time
        dw      0               ; reversed
 m0:
        mov     ah, 0DCh
        int     021h

        mov     byte ptr request[3], al         ; connection number
        mov     word ptr request[0], 2
        mov     byte ptr request[2], 016h
        mov     word ptr reply[0], 61

        mov     ah, 0E3h
        push    cs
        push    cs
        pop     ds
        pop     es
        lea     si, request
        lea     di, reply
        int     021h

        mov     byte ptr reply[20], '$'
        lea     dx, reply
        add     dx, 8
        mov     ah, 09h
        int     021h
        mov     al, byte ptr reply[7]           ; low byte
        mov     cl, 11
        mul     cl
        lea     dx, objtype
        add     dx, ax
        mov     ah, 09h
        int     021h
        int     020h

main    endp
