; Transfered data to master contains:
;       1  byte         Slaver code     ==   (24)
;       4  bytes        Network number
;       6  bytes        Node address
;       1  byte         Object type
;       38 bytes        Object name     ==> 50 bytes for dataportion.

; Transfered data from master to slaver contains:
;       2  bytes        Master code     ==  (0x250x25)
;       1  byte         Command code    == 0x1 - 0x3: command codes.
;       2  bytes        reserved.       ==> 5 bytes

Main    proc near

        jmp   Init

        video_seg       dw    0B800h
        Old_08          dd    0
        socket          dw    05555h
        in_work         db    0
        ipx_fonc        dd    0

request dw      2               ; request length
        db      16              ; sub function
        db      0               ; con_num

reply   dw      61              ; reply length
        dd      0               ; object ID
        dw      0               ; obj type
myname  db      48 dup (0)      ; obj name
        db      7 dup (0)       ; login time
        dw      0               ; reserved
no_name db      0,'May be a NOT-LOGGED-IN  PC          ',0,0,0

execute dw      offset proc0
        dw      offset proc1
        dw      offset proc2

RECB:
        rLink_address         dw      0,0
        rESR                  dw      0,0           ;Event service rountine
        rIn_use               db      0
        rCompletion_code      db      0
        rSocket_number        dw      0             ;listen only from open proc
        rIPX_workspace        db      4  dup (0)
        rDriver_workspace     db      12 dup (0)
        rImmediate_address    db      6  dup (0)
        rpacket_count         dw      2             ; the same

        rIPX_addr             dw      0,0
        rIPX_size             dw      30            ;Constant
        rdata_addr            dw      0,0
        rdata_size            dw      5

RIPX:
        rCheckSum             dw      0                 ; 2
        rLength               dw      35                ; 2
        rTransport_Ctrl       db      0                 ; 1
        rPacket_type          db      4                 ; 1
        rDest_network_number  db      4 dup (0)         ; 4
        rDest_network_node    db      6 dup (0)         ; 6
        rDest_network_socket  dw      0                 ; 2
        rSour_network_number  db      4 dup (0)         ; 4
        rSour_network_node    db      6 dup (0)         ; 6
        rSour_network_socket  dw      0                 ; 2
        rdataportion          db      5 dup(0)          ;===> 30 bytes

SECB:
        sLink_address         dw      0,0
        sESR                  dw      0,0           ;Event service rountine
        sIn_use               db      0
        sCompletion_code      db      0
        sSocket_number        dw      0             ;listen only from open proc
        sIPX_workspace        db      4  dup (0)
        sDriver_workspace     db      12 dup (0)
        sImmediate_address    db      6  dup (0)
        spacket_count         dw      2             ; the same

        sIPX_addr             dw      0,0
        sIPX_size             dw      30            ;Constant
        sdata_addr            dw      0,0
        sdata_size            dw      50

SIPX:
        sCheckSum             dw      0                 ; 2
        sLength               dw      80                ; 2
        sTransport_Ctrl       db      0                 ; 1
        sPacket_type          db      4                 ; 1
        sDest_network_number  db      4 dup (0)         ; 4
        sDest_network_node    db      6 dup (0)         ; 6
        sDest_network_socket  dw      0                 ; 2
        sSour_network_number  db      4 dup (0)         ; 4
        sSour_network_node    db      6 dup (0)         ; 6
        sSour_network_socket  dw      0                 ; 2
        sdataportion          db      '', 49 dup(0)    ;===> 30 bytes
;========================

New_08  proc   near
        pushf
        call  dword ptr cs:old_08      ; Invoke the OLd Int 08

        push  ax
        push  bx
        push  cx
        push  dx
        push  si
        push  di
        push  ds
        push  es

        or      byte ptr in_work, 0      ; The gate of the game
        jnz     done
        call    process
done:
        pop   es
        pop   ds
        pop   di
        pop   si
        pop   dx
        pop   cx
        pop   bx
        pop   ax
        iret
New_08  endp
;========================

process proc    near

        mov     ax, cs                   ; DS -> CS
        mov     ds, ax

        mov     byte ptr in_work, 1      ; set flag on

        cmp     byte ptr sin_use, 0
        jne     Nothing

        ;======= Any change ?

        call    get_myname
        call    change_name              ; CX == 7 if name is changed

        or      cl, cl
        jz      pr1                      ; Nothing changed

        call    sproc
        jmp     Nothing
pr1:
        cmp     byte ptr rin_use, 0
        jne     nothing                  ; last receiving pending...

        ;======= Is receive anything ?   ; checking received data buffer
        cmp     byte ptr rCompletion_code, 0
        jne     pr2

        mov     al, ''                         ; is it a master's message ?
        xor     byte ptr rdataportion[0], al
        jnz     pr1
        xor     byte ptr rdataportion[1], al
        jnz     pr2

        mov     al, byte ptr rdataportion[2]    ; execute command
        cmp     al, 2
        ja      pr2
        xor     ah,ah
        shl     ax, 1
        mov     bx, ax
        call    word ptr execute[bx]
pr2:
        ;======= Not either sending or receiving.

        call    rproc

Nothing:
        mov     byte ptr in_work, 0
        ret
process endp
;========================

proc0   proc    near
        mov     ax, word ptr video_seg
        mov     es, ax
        mov     byte ptr es:[158], '0'
        ret
proc0   endp
;========================

proc1   proc    near
        mov     ax, word ptr video_seg
        mov     es, ax
        mov     byte ptr es:[158], '1'
        ret
proc1   endp
;========================

proc2   proc    near
        ; Logout from file server.
        mov     ah, 0D7
        int     021h                    ; logout from server.
        ret
proc2   endp
;========================

change_name     proc    near
        push    cs
        push    cs
        pop     es
        pop     ds
        lea     si, myname
        lea     di, sdataportion + 12
        push    si                              ; store string pointer
        push    di
        mov     cx, 38
        cld
        repe    cmpsb
        pop     di                              ; restore string pointer
        pop     si
        dec     di                              ; set back to get
        dec     si                              ; object type.
        or      cx, 0
        jz      No
        mov     cx, 38
        rep     movsb                           ; use the last 0 char
        mov     cl, 7                           ; flag
 No:
        ret
change_name     endp
;========================

get_myname      proc    near
        push    cs
        pop     ds

        mov     ah, 0DCh                        ; Get connection number
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
        lea     si, request                     ; request buffer
        lea     di, reply                       ; receive buffer
        int     021h
        or      al, al
        jz      normal
        lea     di, reply+7                     ; Object type and name.
        lea     si, no_name
        mov     cx, 38
        repne   movsb
normal:
        ret
get_myname      endp
;========================

sproc   proc    near
        ;======= SIPX setting ...
        mov     byte ptr sPacket_type, 4
        mov     word ptr slength, 80

        mov     word ptr sDest_network_number[0], 0FFFFh
        mov     word ptr sDest_network_number[2], 0FFFFh

        mov     word ptr sDest_network_node[0], 0FFFFh
        mov     word ptr sDest_network_node[2], 0FFFFh
        mov     word ptr sDest_network_node[4], 0FFFFh

        mov     word ptr sDest_network_socket, 07777h   ; to WS with socket
                                                        ; opened 0x7777 only.
        ;======= SECB setting; immediate address already getting...
        mov     word ptr sdata_size, 50
        mov     word ptr sipx_size, 30

        push    cs
        pop     bx
        mov     word ptr sIPX_addr[2], bx       ; segment of IPX
        mov     word ptr sdata_addr[2], bx      ; segment of data
        lea     bx, sIPX
        mov     word ptr sIPX_addr[0], bx       ; offset of IPX
        lea     bx, sdataportion
        mov     word ptr sdata_addr[0], bx      ; offset of data

        push    cs
        pop     es
        lea     si, sECB                        ; send now
        mov     bx, 03
        call    dword ptr ipx_fonc
        ret
sproc   endp
;===========================

rproc   proc    near

        ;======= Receive ECB setting ...
        mov     word ptr rESR[0], 0
        mov     word ptr rESR[2], 0
        mov     word ptr rpacket_count, 2       ; two data buffers for packet

        push    cs
        pop     bx
        mov     word ptr rIPX_addr[2], bx       ; segment of IPX
        mov     word ptr rdata_addr[2], bx      ; segment of data
        lea     bx, rIPX
        mov     word ptr rIPX_addr[0], bx       ; offset of IPX
        lea     bx, rdataportion
        mov     word ptr rdata_addr[0], bx      ; offset of data

        mov     bx, word ptr socket
        mov     word ptr rSour_Network_Socket, bx

        ;======= Listen for packet
        push    cs
        pop     es
        lea     si, RECB
        xor     al, al
        mov     bx, 04h
        call    dword ptr ipx_fonc

        ;======= Let ipx breath
        mov     bx, 0Ah
        call    dword ptr ipx_fonc
        ret
rproc   endp

;======= Resident Install
Init    proc    near

        push    cs
        pop     ds

        mov     ah,0Fh
        int     010h                            ; Test video mode
        cmp     al,7
        jne     I1
        mov     video_seg,0B000h                ; Monochrome mode
I1:
        mov     ax,03508
        int     021h
        mov     word ptr Old_08[0],bx           ; Get vecter 08
        mov     word ptr Old_08[2],es

        mov     ax, 07A00
        int     02Fh
        cmp     al, 0FFh
        je      i10
        jmp     not_ins
 i10:
        mov     word ptr ipx_fonc[0], di
        mov     ax, es
        mov     word ptr ipx_fonc[2], ax

        ;======== Open socket
        mov     al, 0FFh                        ; long-live socket
        mov     dx, word ptr socket
        xor     bx, bx
        call    dword ptr ipx_fonc

        cmp     al, 0FEh                        ; error
        jb      i11
        jmp     not_ins
 i11:
        mov     word ptr rSocket_number, dx     ; DX = Ok  Socket Number
        mov     word ptr sSocket_number, dx     ; for sending.

        ;======= Get my network_number and node_address

        push    cs
        pop     es
        lea     si, sdataportion                ; store here for sending
        inc     si                              ; avoid slaver code
        mov     bx, 09
        call    dword ptr ipx_fonc
        call    get_myname
        call    change_name

        ;======= Send network_address to master
I2:
        lea     dx, link
        mov     ah, 09
        int     021h

        mov     word ptr sImmediate_address[0], 0FFFFh
        mov     word ptr sImmediate_address[2], 0FFFFh
        mov     word ptr sImmediate_address[4], 0FFFFh
        call    sproc
I3:
        ;======= Let ipx breath
        mov     bx, 0Ah
        call    dword ptr ipx_fonc
        or      byte ptr sin_use, 0
        jne     i3

        or      byte ptr scompletion_code, 0    ; can not send
        je      i31
        jmp     not_ins
 i31:

        ;======= Receive master's answer

        mov     dword ptr timeout, 0
        call    rproc
I4:
        mov     bx, 0Ah
        call    dword ptr ipx_fonc
        or      byte ptr rin_use, 0
        je      i41
        add     dword ptr timeout, 1

        cmp     dword ptr timeout, 900000000
        jb      i4
 i40:
        lea     dx, wrong
        mov     ah, 09h
        int     021h
        jmp     i2                              ; Re_request

        ;======= Is receive anything ?
 i41:
        or      byte ptr rCompletion_code, 0
        jne     i40                             ; resend again to master

        mov     al, ''                         ; flag of master's message
        lea     di, rdataportion
        mov     cx, 3
        repne   scasb
        or      cx, cx
        jz      i40                             ; not a master's message

        push    cs                              ; we can get the immediate
        push    cs                              ; address of master now
        pop     ds                              ; for later sending.
        pop     es
        lea     di, simmediate_address
        lea     si, rimmediate_address
        cld
        mov     cx, 6
        rep     movsb

        lea     dx, result
        mov     ah, 09h
        int     021h

        ;======= New Int08 installed
        mov     ax, 02508
        lea     dx, New_08
        int     021h

        mov     byte ptr in_work, 0

        ;======= Stop prog and resident
        lea     dx, Init
        int     027h

not_ins:
        cmp     al, 0FFh
        jne     kt
        lea     dx, reason
        mov     ah, 09h
        int     021h
 kt:
        int     020h
Init    endp
;===========================

result  db     'Communication Installed  OK !',10,13,'$'
link    db     10,13,'Linking to Master PC . . . . . . . $'
wrong   db     'failed.$'
reason  db     'Communication already exist ...$'
timeout dd     0
main    endp
