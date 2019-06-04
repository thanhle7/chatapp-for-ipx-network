
                .MODEL small

StdIn           =       0000
StdOut          =       0001
StdErr          =       0002

                .DATA
MachineName     DB      15 Dup(' ')
                DB      0

CheckName       DB      15 Dup(' ')
                DB      0

Usage           DB      13, 10
                DB      "USAGE: SetName <machinename>"
                DB      13, 10
                DB      "<machinename> can be up to 15 characters long."
                DB      13, 10
UsageL          =       $-Usage

Success         DB      13, 10
                DB      "SetName ended successfully."
                DB      13, 10
SuccessL        =       $-Success

ErrorMsg        DB      13, 10
                DB      "SetName ended due to error.  Machine Name not set."
                DB      13, 10
ErrorMsgL       =       $-ErrorMsg

                .STACK  200h

                .CODE
NoCmdLine:      Mov     DX, Offset Usage
                Mov     BX, StdErr
                Mov     CX, UsageL
                Mov     AX, 4000h
                Int     21h

Exit:
                Mov     AX, 4C00h
                Int     21h

; -------------------------------------------

Start:          Mov     AX, @Data
                Mov     DS, AX

                Mov     DI, 0081h
                Mov     BX, 0
                Mov     CX, 0
                Mov     CL, ES:[0080h]

FindStart:      JCXZ    NoCmdLine
                Cmp     Byte Ptr ES:[DI], 20h
                JE      NextChar
                Cmp     Byte Ptr ES:[DI], 22h
                JE      NextChar
                Cmp     Byte Ptr ES:[DI], 27h
                JE      NextChar

                Jmp     MoveCmdLine

NextChar:       Inc     DI
                Dec     CX
                Jmp     FindStart

MoveCmdLine:    JCXZ    GotName
                Cmp     BX, 15
                JE      GotName
                Mov     AL, ES:[DI]
                Cmp     AL, 0Dh
                JE      GotName
                Cmp     AL, 20h
                JE      GotName
                Cmp     AL, 22h
                JE      GotName
                Cmp     AL, 27h
                JE      GotName

                Mov     MachineName [BX], AL
                Inc     DI
                Inc     BX
                Dec     CX
                Jmp     MoveCmdLine

GotName:        Mov     AX, 5E01h
                Mov     CX, 0101h
                Mov     DX, Offset MachineName
                Int     21h
                JC      DidntWork

                Mov     AX, 5E00h
                Mov     DX, Offset CheckName
                Int     21h
                JC      DidntWork

                Push    DS
                Pop     ES
                Mov     SI, Offset MachineName
                Mov     DI, Offset CheckName
                Mov     CX, 15
        Repe    CmpSB
                JNE     DidntWork

                Mov     DX, Offset Success
                Mov     BX, StdErr
                Mov     CX, SuccessL
                Mov     AX, 4000h
                Int     21h

                Mov     AX, 4C00h
                Int     21h

DidntWork:      Mov     DX, Offset ErrorMsg
                Mov     BX, StdErr
                Mov     CX, ErrorMsgL
                Mov     AX, 4000h
                Int     21h

                Mov     AX, 4C00h
                Int     21h

                End     Start



