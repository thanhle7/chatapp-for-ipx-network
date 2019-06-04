
                .MODEL small

StdIn           =       0000
StdOut          =       0001
StdErr          =       0002

                .DATA
NameMsg         DB      13, 10
                DB      "Machine name: "
MachineName     DB      16 Dup(' ')
                DB      13, 10
NameMsgL        =       $-NameMsg

ErrorMsg        DB      13, 10
                DB      "No name, or GetName ended due to error."
                DB      13, 10
ErrorMsgL       =       $-ErrorMsg

                .STACK  200h

; -------------------------------------------
                .CODE

Start:          Mov     AX, @Data
                Mov     DS, AX

                Mov     AX, 5E00h
                Mov     DX, Offset MachineName
                Int     21h
                JC      DidntWork
                Cmp     CH, 0
                JE      DidntWork

                Mov     DX, Offset NameMsg
                Mov     BX, StdOut
                Mov     CX, NameMsgL
                Mov     AX, 4000h
                Int     21h

                Mov     AX, 4C00h
                Int     21h

DidntWork:      Mov     DX, Offset ErrorMsg
                Mov     BX, StdOut
                Mov     CX, ErrorMsgL
                Mov     AX, 4000h
                Int     21h

                Mov     AX, 4C00h
                Int     21h

                End     Start



