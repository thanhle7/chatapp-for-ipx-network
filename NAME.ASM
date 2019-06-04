                DOSSEG
                .MODEL small
                Include DOS.Inc
                Include BIOS.Inc

StdIn           =       0000
StdOut          =       0001
StdErr          =       0002

                .DATA
KeepSize        DW      0

                .STACK 100h

;---------------------------------------
                .CODE

;---------------------------------------

OurPSP          DW      0
MachineName     DB      "BARRY          "
                DB      0
oldint21        Label   DWord
oldint21_ofs    DW      0
oldint21_seg    DW      0

;---------------------------------------

Int_21:         PushF
                STI
                Cmp     AX, 5E00h
                JE      GetName

I21_10:         Cmp     AX, 5E01h
                JE      SetName

I21_20:         Cmp     AH, 5Ch
                JE      FixLock

NotOurs:        Popf
                Jmp     CS:oldint21

;---------------------------------------

FixLock:        Mov     SI, 0000h
                Jmp     NotOurs

GetName:        PopF
                CLD
                STI
                Push    ES
                Push    DI
                Push    SI

                Mov     CX, 8
                Push    DS
                Pop     ES
                Mov     DI, DX
                Mov     SI, Offset MachineName
                Push    CS
                Pop     DS
        Rep     MovSW

                Push    ES
                Pop     DS

                Mov     CX, 0101h
                Pop     SI
                Pop     DI
                Pop     ES
                Jmp     Short NameExit

SetName:        PopF
                CLD
                STI
                Push    ES
                Push    DI
                Push    SI
                Push    CX

                Mov     CX, 8
                Mov     DI, Offset MachineName
                Push    CS
                Pop     ES
                Mov     SI, DX
        Rep     MovSW

                Pop     CX
                Pop     SI
                Pop     DI
                Pop     ES

NameExit:       CLC
                Mov     AX, 0
                RetF    2

;---------------------------------------

EndISR          Label   Byte

;---------------------------------------

Start:          Mov     AX, @Data
                Mov     DS, AX
                Mov     CS:OurPSP, ES

GetVector:      @GetInt 21h
                cmp     BX, offset Int_21
                jne     install_it

                Push    DS
                @SetInt 21h, es:oldint21_ofs, es:oldint21_seg
                Pop     DS

                mov     bx, es:ourPSP
                Mov     AX, 5000h
                Int     21h

                Mov     ES, BX
                Mov     AH, 49h
                Int     21h

                Mov     ES, ES:[002Ch]
                Mov     AH, 49h
                Int     21h

                mov     bx, CS:ourPSP
                Mov     AX, 5000h
                Int     21h

                @Exit   0


install_it:     mov     CS:oldint21_seg, ES
                mov     CS:oldint21_ofs, BX

                Push    DS
                @SetInt 21h, Int_21
                Pop     DS

                Mov     AX, CS
                Sub     AX, CS:OurPSP

                Mov     BX, Offset EndISR
                Mov     CX, 4
                Shr     BX, CL
                Inc     BX

                Add     AX, BX

                Mov     KeepSize, AX
                @TSR    KeepSize, 0

                End     Start


