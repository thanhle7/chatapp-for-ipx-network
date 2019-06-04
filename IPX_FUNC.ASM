;--------------------------------------------------------
;  Cac ham cua IPX duoc viet bang hop ngu Assembler
;--------------------------------------------------------


;--------------------------------------------------------
;  Muc dich: kiem tra IPX da install ?
;  Goi voi : nothing
;  Tra ve  : Code_Return = 0 : IPX chua install
;                        = 1 : IPX da install
;            ES:DI --> _IPX
;--------------------------------------------------------
Test_IPXInstall      proc   near

        mov     byte ptr Code_Return,0

  ; Test ipx
        mov     ax,07A00
        int     02F
        cmp     al,0FFh
        jne     Not_IPXI
        mov     byte ptr Code_Return,1

Not_IPXI:
        ret
Test_IPXInstall      endp

;----------------------------------------------------------------
;  Muc dich: xac dinh gia tri Immediate address cho this Packet
;  Goi voi : ECB entries  (ES:SI --> ECB)
;  Tra ve  : Immediate Address, AL : 00  success (ECB added to pool)
;                                    0FF Listening socket dose not exist
;            Commpletion code,       00  Packet successfully recieved
;                                    0FC This event has been canceled
;                                    0FD Buffer too small
;                                    0FF Socket not open
;----------------------------------------------------------------
ListenforPacket    proc  near
        mov     byte ptr Code_Return,0

        xor     al,al
        mov     bx,04h
        int     07Ah

ListenP1:
        mov     ah,01h
        int     016h
        cmp     al,01Bh
        je      Exit_Listen

        cmp     byte ptr In_use,0
        jne     ListenP1

        cmp     byte ptr Completion_code,0
        jne     Exit_Listen
        mov     byte ptr Code_Return,1
Exit_Listen:
        ret
ListenforPacket    endp

;----------------------------------------------------------------
;  Muc dich: xac dinh node address cua destination workstation
;  Goi voi :   ES:SI --> request buffer
;                :DI --> reply buffer
;  Tra ve  : Completion code, AL = 00  success
;                                  0FA no path to destination
;            Estimated transport time, CX
;            ES:DI --> reply buffer
;----------------------------------------------------------------
Request_Buffers db  4 dup (0)
                db  6 dup (0)
                db  2 dup (0)

Reply_Buffers   db  6 dup (0)

GetLocalTarget     proc  near
        mov     byte ptr Code_Return,0

        push    es
        push    di

        push    cs
        pop     es
        mov     di, offset Request_Buffers

   ; DS:SI --> ES:DI
   ; Request Buffer to Request Temp
        mov     cx,12
        cld
        rep     movsb


        mov     si, offset Request_Buffers
        mov     di, offset Reply_Buffers
        mov     bx,0200
        int     07Ah

        or      al,al
        jnc     Exit_LocalTarget

        pop     di
        pop     es

Exit_LocalTarget:
        ret
GetLocalTarget     endp


;----------------------------------------------------------------
;  Muc dich: Open the Socket
;  Goi voi : AL : Live
;            DX : socketed number
;            ES:DI --> ECB
;  Tra ve  : Completion code, AL = 00 : success
;                                  0FE: socket table is full
;                                  0FF: socket already open
;            Assigned socket number,  DX
;----------------------------------------------------------------
OpenSocket   proc  near
        mov     byte ptr Code_return,0

        xor     bx,bx
        int     07ah

        mov     byte ptr Completion_code,al    ;completion code

        cmp     al,0FEh
        je      Exit_OpnSoc
        mov     byte ptr Code_Return,1

        mov     word ptr Socket_Number,dx            ;socket number

Exit_OpnSoc:
        ret
OpenSocket   endp

;----------------------------------------------------------------
;  Muc dich: Close socket
;  Goi voi : DX = socket number
;  Tra ve  : nothing
;----------------------------------------------------------------
CloseSocket    proc  near

        mov     bx,0100
        int     07Ah

        ret
CloseSocket    endp

;----------------------------------------------------------------
;  Muc dich : Send data
;  Goi voi  : ES:SI --> ECB
;  Tra ve   : Completion codes, 00  : message sent (not guaranteed)
;                               0FC : this event has been canceled
;                               0FD : bad packet
;                               0FE : Undeliverable
;                               0FF : Hardware or network failure
;----------------------------------------------------------------
SendPacket    proc near

        mov     byte ptr Code_Return,0

        mov     bx,03h
        int     07ah
        mov     cx,50000
_SendP1:
        cmp     byte ptr In_use,0
        jne     _SendP2

        cmp     byte ptr Completion_code,0
        jne     Exit_SendP

        mov     byte ptr Code_Return,1

_SendP2:
        Loop    _SendP1

Exit_SendP:
        ret
SendPacket    endp

Code_Return     db      0


