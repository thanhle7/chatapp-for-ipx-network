
/*  NETWORK.C  */

/*  a Local Area Network emulator  */
/*  written in Turbo C 1.5         */

#pragma  inline

#include <stdio.h>
#include <dos.h>
#include <fcntl.h>
#include <conio.h>
#include <io.h>
#include <bios.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <dir.h>
#include <mem.h>
#include <process.h>


void    main(int argc, char *argv[]);
void    init_program(void);
void    do_popup(void);
void    actual_popup(void);

void    open_file(void);
void    close_file(void);
void    lock_record(void);
void    unlock_record(void);
void    show_file(void);
void    show_status(void);

void    getkey(void);
int     kbdstring(char buff[], int max_chars);
int     get_vid_mode(void);
void    save_cursor(struct csavetype *csave);
void    restore_cursor(struct csavetype *csave);

void interrupt   int08 (void);
void interrupt   int09 (void);
void interrupt   int10 (unsigned bp,
                        unsigned di,
                        unsigned si,
                        unsigned ds,
                        unsigned es,
                        unsigned dx,
                        unsigned cx,
                        unsigned bx,
                        unsigned ax,
                        unsigned ip,
                        unsigned cs,
                        unsigned flags);
void interrupt  int13  (unsigned bp,
                        unsigned di,
                        unsigned si,
                        unsigned ds,
                        unsigned es,
                        unsigned dx,
                        unsigned cx,
                        unsigned bx,
                        unsigned ax,
                        unsigned ip,
                        unsigned cs,
                        unsigned flags);
void interrupt  int16  (unsigned bp,
                        unsigned di,
                        unsigned si,
                        unsigned ds,
                        unsigned es,
                        unsigned dx,
                        unsigned cx,
                        unsigned bx,
                        unsigned ax,
                        unsigned ip,
                        unsigned cs,
                        unsigned flags);
void interrupt  int1b  (void);
void interrupt  int1c  (void);
void    interrupt int21(unsigned bp,
                        unsigned di,
                        unsigned si,
                        unsigned ds,
                        unsigned es,
                        unsigned dx,
                        unsigned cx,
                        unsigned bx,
                        unsigned ax,
                        unsigned ip,
                        unsigned cs,
                        unsigned flags);
void interrupt  int23  (void);
void interrupt  int24  (unsigned bp,
                        unsigned di,
                        unsigned si,
                        unsigned ds,
                        unsigned es,
                        unsigned dx,
                        unsigned cx,
                        unsigned bx,
                        unsigned ax,
                        unsigned ip,
                        unsigned cs,
                        unsigned flags);
void interrupt  int28  (void);

#define     TRUE           1
#define     FALSE          0
#define     LT            <0
#define     EQ           ==0
#define     GT            >0
#define     NE           !=0
#define     MY_STK_SIZE 1500
#define     LINE_LENGTH   81
#define     MAX_LINES    100
#define     FOUR_SECS     73


#define     BELL           7
#define     BS             8
#define     LINEFEED      10
#define     FORMFEED      12
#define     CR            13
#define     BACKTAB       15
#define     CTRLQ         17
#define     CTRLS         19
#define     CTRLX         24
#define     CTRLZ         26
#define     ESC           27

#define     ALTX          45
#define     ALTC          46
#define     ALTD          32
#define     ALTE          18
#define     ALTF          33
#define     ALTT          20
#define     ALTM          50
#define     ALTH          35
#define     HOMEKEY       71
#define     ENDKEY        79
#define     UPKEY         72
#define     DOWNKEY       80
#define     PGUPKEY       73
#define     PGDNKEY       81
#define     LEFTKEY       75
#define     INSKEY        82
#define     RIGHTKEY      77
#define     DELKEY        83
#define     CTRLLEFTKEY  115
#define     CTRLRIGHTKEY 116

#define     ALT           56
#define     RIGHT_SHIFT   54
#define     F1            59
#define     F2            60
#define     F3            61
#define     F4            62
#define     F5            63
#define     F6            64
#define     F7            65
#define     F8            66
#define     F9            67
#define     F10           68


struct      csavetype
            {
            unsigned int curloc;
            unsigned int curmode;
            };

struct      FILE_DATA
            {
            char name[81];
            int  handle;
            int  rec_length;
            int  lock_count;
            unsigned char inherit_flag;
            unsigned char sharing_flag;
            unsigned char access_flag;
            };

struct  FILE_DATA file_data[5];

char            filename[81];
unsigned char   file_inherit;
unsigned char   file_sharing;
unsigned char   file_access;
int             file_rec_len;
int             open_count;

unsigned int    paragraphs;
unsigned int    temp1, temp2;
unsigned int    current_ss;
unsigned int    current_sp; 
unsigned int    save_ss; 
unsigned int    save_sp;

unsigned int    far *our_mcb_size;
unsigned int    far *next_mcb_owner;
unsigned char   far *ourdta_ptr;
unsigned char   far *olddta_ptr;
unsigned char   far *our_mcb;
unsigned char   far *next_mcb;
unsigned char   far *prtsc_flag_ptr;
unsigned char   far *indos_ptr;
unsigned char   far *indos2_ptr;
unsigned char   far *kbd_flag_ptr;

unsigned int    ourpsp;
unsigned int    oldpsp;
unsigned int    break_state;

union    REGS   regs;
struct   SREGS  sregs;

void     interrupt (*oldint08)(void);
void     interrupt (*oldint09)(void);
void     interrupt (*oldint10)(void);
void     interrupt (*oldint13)(void);
void     interrupt (*oldint16)(void);
void     interrupt (*oldint1b)(void);
void     interrupt (*oldint1c)(void);
void     interrupt (*oldint21)(void);
void     interrupt (*oldint23)(void);
void     interrupt (*oldint24)(void);
void     interrupt (*oldint28)(void);
void     interrupt (*vectsave)(void);
void     interrupt (*vecthold)(void);

unsigned int     in_int08        = FALSE;
unsigned int     in_int09        = FALSE;
unsigned int     in_int10        = FALSE;
unsigned int     in_int13        = FALSE;
unsigned int     in_int16        = FALSE;
unsigned int     in_int21        = FALSE;
unsigned int     in_int28        = FALSE;
unsigned int     in_popup        = FALSE;
unsigned int     hot_flag        = FALSE;
unsigned int     de_install      = FALSE;
unsigned int     break_flag      = FALSE;
unsigned int     crit_err_flag   = FALSE;
unsigned int     we_are_last     = FALSE;
unsigned int     first_time      = TRUE;

unsigned int     tick_counter    = 0;
char             machine_name [16] = "01TEST         ";
unsigned char    network_drive;
char             string1 [LINE_LENGTH];

unsigned char    key_char;
unsigned char    extended_char;
unsigned int     i, j;
unsigned int     temp_ax;
unsigned int     temp_bx;
unsigned char    temp_ah;
unsigned char    temp_bl;

unsigned int     low_inten;
unsigned int     hi_inten;


unsigned char    *box_lines1[] =
                    {
                 "ÚÄ[ LAN EMULATOR ]ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿",
                 "³                                       ³",
                 "³  E)xit to application                 ³",
                 "³  R)emove Emulator from RAM            ³",
                 "³  O)pen a file                         ³",
                 "³  C)lose a file                        ³",
                 "³  L)ock a record                       ³",
                 "³  U)nlock a record                     ³",
                 "³  S)how current files/locks            ³",
                 "³                                       ³",
                 "³                                       ³",
                 "ÀÄ(Select option by first letter)ÄÄÄÄÄÄÄÙ"
                    };

unsigned char    *box_lines2[] =
                    {
                 "ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿",
                 "³                                       ³",
                 "³                                       ³",
                 "³                                       ³",
                 "³                                       ³",
                 "³                                       ³",
                 "³                                       ³",
                 "³                                       ³",
                 "³                                       ³",
                 "³                                       ³",
                 "³                                       ³",
                 "ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ"
                    };

unsigned char    app_window_save [42*12*2];
struct csavetype app_cursor;
unsigned int    *our_stack;

/************************************/

void    interrupt int08(void)
    {
    in_int08 = TRUE;
    oldint08();
    enable();
    tick_counter++;

int08_p1:
    if (!hot_flag && !de_install)
        goto exit08;

int08_p2:
    if (in_int09 || in_int10 || in_int13 
        || in_int16 || in_int21 || in_int28 || in_popup)
        goto exit08;

    if (*indos_ptr != 0)
         goto exit08;

    if (*indos2_ptr != 0)
         goto exit08;

    if (*prtsc_flag_ptr == 1)
         goto exit08;

    outportb(0x20, 0x0b);
    if (inportb(0x20)) goto exit08;

    in_popup = TRUE;
    do_popup();
    in_popup = FALSE;

exit08:
    in_int08 = FALSE;
    }

/************************************/

void interrupt  int09 (void)
    {
    in_int09 = TRUE;
    oldint09();
    enable();

    if ( (*kbd_flag_ptr & 0x09) == 0x09 )
        {
        de_install = FALSE;
        hot_flag   = TRUE;
        }

    in_int09 = FALSE;
    }

/************************************/

void    interrupt int10(unsigned bp,
                        unsigned di,
                        unsigned si,
                        unsigned ds,
                        unsigned es,
                        unsigned dx,
                        unsigned cx,
                        unsigned bx,
                        unsigned ax,
                        unsigned ip,
                        unsigned cs,
                        unsigned flags)
    {
    in_int10 = TRUE;
    enable();

    asm     push bp
	asm     pushf	
    asm     CLI
	asm     call dword ptr _oldint10
    asm     pop  bp

    asm     pushf
    asm     pop flags
    ax    = _AX;
    bx    = _BX;
    cx    = _CX;
    dx    = _DX;
    in_int10 = FALSE;
    }

/************************************/

void    interrupt int13(unsigned bp,
                        unsigned di,
                        unsigned si,
                        unsigned ds,
                        unsigned es,
                        unsigned dx,
                        unsigned cx,
                        unsigned bx,
                        unsigned ax,
                        unsigned ip,
                        unsigned cs,
                        unsigned flags)
    {
    in_int13 = TRUE;

    oldint13();
    enable();
    asm     pushf
    asm     pop flags
    ax    = _AX;

    in_int13 = FALSE;
    }

/************************************/

void    interrupt int16(unsigned bp,
                        unsigned di,
                        unsigned si,
                        unsigned ds,
                        unsigned es,
                        unsigned dx,
                        unsigned cx,
                        unsigned bx,
                        unsigned ax,
                        unsigned ip,
                        unsigned cs,
                        unsigned flags)
    {
    temp_ax  = _AX;
    temp_bx  = _BX;
    temp_ah  = _AH;
    in_int16 = TRUE;
    enable();

    if (temp_ax == 'BN')
        {
        temp_ax = 'bn';
        goto int16_exit;
        }

    if (in_popup)
        goto do_old16;

    if (temp_ah != 0)
        goto do_old16;

wait_for_key:
    _AH = 1;
    oldint16();
    asm     jz   popup_16 

    goto do_old16;

popup_16:
    if (!hot_flag && !de_install)
        goto wait_for_key;

    if (in_int08 || in_int09 || in_int10 || in_int13 
        || in_int21 || in_int28 || in_popup)
        goto wait_for_key;

    if (*indos_ptr != 0)
        goto wait_for_key;

    if (*indos2_ptr != 0)
        goto wait_for_key;

    if (*prtsc_flag_ptr == 1)
        goto wait_for_key;

    outportb(0x20, 0x0b);
    if (inportb(0x20)) 
        goto wait_for_key;

    in_popup = TRUE;
    do_popup();
    in_popup = FALSE;

    goto wait_for_key;



do_old16:
    _AX = temp_ax;
    oldint16();
    asm     pushf
    asm     pop flags
    temp_ax = _AX;
    temp_bx = _BX;

int16_exit:
    ax = temp_ax;
    bx = temp_bx;
    in_int16 = FALSE;
    }

/************************************/

void interrupt  int1b (void)
    {
    enable();
    break_flag = TRUE;
    }

/************************************/

void interrupt  int1c (void)
    {
    enable();
    }

/************************************/

void    interrupt int21(unsigned bp,
                        unsigned di,
                        unsigned si,
                        unsigned ds,
                        unsigned es,
                        unsigned dx,
                        unsigned cx,
                        unsigned bx,
                        unsigned ax,
                        unsigned ip,
                        unsigned cs,
                        unsigned flags)
    {
    if (ax == 0x4409 || ax == 0x5e00)
        goto carry_on;

    asm     mov     sp, bp
    asm     pop     bp
    asm     pop     di
    asm     pop     si
    asm     pop     ds
    asm     pop     es
    asm     pop     dx
    asm     pop     cx
    asm     pop     bx
    asm     pop     ax

/*  jmp dword ptr cs:[0]  */
    asm     db      02eh,0ffh,02eh,0000h,000h


carry_on:
    in_int21        = TRUE;

    if (ax == 0x4409)       /* network drives */
        {
        if (temp_bl == network_drive)
            dx = 0x1000;
        else
            dx = 0x0000;
        ax = 0;
        goto int21exit;
        }
    else                    /* machine name */
        {
        movedata(_DS, (unsigned) machine_name, ds, dx, 16);
        cx = 0x0101;
        ax = 0;
        }

int21exit:
    flags &= 0xfffe;        /* clear the carry flag */
    in_int21 = FALSE;
    }

/************************************/
void interrupt  int23 (void)
    {
    enable();
    }

/************************************/
void interrupt  int28 (void)
    {
    in_int28 = TRUE;
    enable();
    oldint28();

    if (!hot_flag && !de_install)
        goto exit28;

    if (in_int08 || in_int09 || in_int10 || in_int13 
        || in_int16 || in_popup)
        goto exit28;

    if (*indos_ptr > 1)
         goto exit28;

    if (*indos2_ptr != 0)
         goto exit28;

    if (*prtsc_flag_ptr == 1)
         goto exit28;

    outportb(0x20, 0x0b);
    if (inportb(0x20)) goto exit28;

    in_popup = TRUE;
    do_popup();
    in_popup = FALSE;

exit28:
    in_int28 = FALSE;
    }

/************************************/

void    interrupt int24(unsigned bp,
                        unsigned di,
                        unsigned si,
                        unsigned ds,
                        unsigned es,
                        unsigned dx,
                        unsigned cx,
                        unsigned bx,
                        unsigned ax,
                        unsigned ip,
                        unsigned cs,
                        unsigned flags)
    {
    temp1 = _AX;
    crit_err_flag = TRUE;

    if (_osmajor < 3)
       ax = (temp1 & 0xFF00);
    else
       ax = (temp1 & 0xFF00) | 0x03;
    }

/************************************/

void    do_popup (void)
        {
        disable();
        current_ss = _SS;
        current_sp = _SP;
        _SS = save_ss;
        _SP = save_sp;
        enable();

        next_mcb       = MK_FP( (ourpsp) + *our_mcb_size, 0);
        next_mcb_owner = MK_FP(  ourpsp  + *our_mcb_size, 1);

        if (  *next_mcb_owner == 0x0000
           || *next_mcb_owner == 0xffff
           || *next_mcb_owner <  ourpsp  )
                we_are_last = TRUE;
        else
                we_are_last = FALSE;

        if (!de_install)
            goto process_popup;

        if (!we_are_last)
            goto do_popup_exit;

        regs.x.ax = 0x5000;
        regs.x.bx = ourpsp;
        intdos(&regs, &regs);
        setvect(0x08, oldint08);
        setvect(0x09, oldint09);
        setvect(0x10, oldint10);
        setvect(0x13, oldint13);
        setvect(0x16, oldint16);
        setvect(0x21, oldint21);
        setvect(0x28, oldint28);
        _AX = _CS;
        _ES = _AX;
        _AH = 0x49;
        geninterrupt(0x21);
        _ES = ourpsp;
        _BX = 0x2c;
        asm   mov es, es:[bx]
        _AH = 0x49;
        geninterrupt(0x21);
        _AX = 0x4c00;
        geninterrupt(0x21);            



process_popup:
        break_state = getcbrk();
        oldint1b = getvect(0x1b);
        setvect(0x1b, int1b);
        oldint1c = getvect(0x1c);
        setvect(0x1c, int1c);
        oldint23 = getvect(0x23);
        setvect(0x23, int23);
        oldint24 = getvect(0x24);
        setvect(0x24, int24);
        olddta_ptr = getdta();
        setdta(ourdta_ptr);
        regs.x.ax = 0x5100;
        intdos(&regs, &regs);
        oldpsp = regs.x.bx;
        regs.x.ax = 0x5000;
        regs.x.bx = ourpsp;
        intdos(&regs, &regs);

        if (hot_flag)
            {
            save_cursor(&app_cursor);
            gettext(19, 7, 59, 18, app_window_save);
            actual_popup();
            puttext(19, 7, 59, 18, app_window_save);
            restore_cursor(&app_cursor);
            }

        regs.x.ax = 0x5000;
        regs.x.bx = oldpsp;
        intdos(&regs, &regs);
        setdta(olddta_ptr);
        setvect(0x24, oldint24);
        setvect(0x23, oldint23);
        setvect(0x1c, oldint1c);
        setvect(0x1b, oldint1b);
        setcbrk(break_state);

do_popup_exit:
        disable();
        _SS = current_ss;
        _SP = current_sp;
        enable();
        }

/************************************/

void    actual_popup(void)
        {
        de_install      = FALSE;
        hot_flag        = FALSE;

show_menu1:
        window(1, 1, 80, 25);
        textattr(low_inten);
        for (i=0; i<12; i++)
            {
            gotoxy(19, i+7);
            cprintf("%s", box_lines1[i]);
            }
        window(20, 8, 58, 17);

win_key:
        gotoxy(1, 1);
        getkey();

        if (key_char == ESC)
            {
            goto exit_popup;
            }

        if (key_char == 'e' || key_char == 'E')
            goto exit_popup;

        if (key_char == 'r' || key_char == 'R')
            {
            de_install = TRUE;
            goto exit_popup;
            }

        if (key_char == 'o' || key_char == 'O')
            {
            open_file();
            goto show_menu1;
            }

        if (key_char == 'c' || key_char == 'C')
            {
            close_file();
            goto show_menu1;
            }

        if (key_char == 'l' || key_char == 'L')
            {
            lock_record();
            goto show_menu1;
            }

        if (key_char == 'u' || key_char == 'U')
            {
            unlock_record();
            goto show_menu1;
            }

        if (key_char == 's' || key_char == 'S')
            {
            show_status();
            goto show_menu1;
            }

        goto win_key;

exit_popup:
        window(1, 1, 80, 25);
        }

/************************************/
void    open_file(void)
        {
        window(1, 1, 80, 25);
        textattr(low_inten);
        for (i=0; i<12; i++)
            {
            gotoxy(19, i+7);
            cprintf("%s", box_lines2[i]);
            }

        gotoxy(21, 7);
        cprintf("[ OPEN A FILE ]");
        window(20, 8, 58, 18);

        if (open_count == 5)
            {
            gotoxy(1, 8);
            textattr(hi_inten);
            cprintf("Whoops. 5 files already open.");
            gotoxy(1, 9);
            cprintf("   (Press a key)");
            textattr(low_inten);
            gotoxy(1, 1);
            getkey();
            goto open_exit;
            }

        gotoxy(1, 2);
        cprintf("   Filename: ");
        gotoxy(1, 3);
        cprintf("Inheritance: ");
        gotoxy(1, 4);
        cprintf("    Sharing: ");
        gotoxy(1, 5);
        cprintf("     Access: ");
        gotoxy(1, 6);
        cprintf(" Record Len: ");

        gotoxy(1, 8);
        textattr(hi_inten);
        cprintf("Enter filename and press <enter>");
        gotoxy(1, 9);
        cprintf("(ESC if none)");
        textattr(low_inten);
        gotoxy(14, 2);
        memset(filename, 0, 80);
        kbdstring(filename, 20);
        if (filename[0] == '\0')
            goto open_exit;

get_inherit:
        gotoxy(1, 8);
        textattr(hi_inten);
        cprintf("File is inheritable? (Y or N)    ");
        gotoxy(1, 9);
        cprintf("                                 ");
        textattr(low_inten);
        gotoxy(14, 3);
        getkey();

        if (key_char == 'y' || key_char == 'Y')
            file_inherit = 0x00;
        else
        if (key_char == 'n' || key_char == 'N')
            file_inherit = 0x80;
        else
            goto get_inherit;
        cprintf("%c", key_char);

get_sharing:
        gotoxy(1, 8);
        textattr(hi_inten);
        cprintf("1=Compatibility Mode; 2=Deny R/W;");
        gotoxy(1, 9);
        cprintf("3=Deny Write; 4=Deny Read; 5=None");
        textattr(low_inten);
        gotoxy(14, 4);
        getkey();

        if (key_char == '1')
            {
            file_sharing = 0x00;
            cprintf("Compatibility Mode");
            }
        else
        if (key_char == '2')
            {
            file_sharing = 0x10;
            cprintf("Deny Read/Write");
            }
        else
        if (key_char == '3')
            {
            file_sharing = 0x20;
            cprintf("Deny Write");
            }
        else
        if (key_char == '4')
            {
            file_sharing = 0x30;
            cprintf("Deny Read");
            }
        else
        if (key_char == '5')
            {
            file_sharing = 0x40;
            cprintf("Deny None");
            }
        else
            goto get_sharing;

        gotoxy(1, 8);
        cprintf("                                 ");
        gotoxy(1, 9);
        cprintf("                                 ");

get_access:
        gotoxy(1, 8);
        textattr(hi_inten);
        cprintf("1=Read access;  2=Write access   ");
        gotoxy(1, 9);
        cprintf("3=Read/Write access              ");
        textattr(low_inten);
        gotoxy(14, 5);
        getkey();

        if (key_char == '1')
            {
            file_access = 0x00;
            cprintf("Read access");
            }
        else
        if (key_char == '2')
            {
            file_access = 0x01;
            cprintf("Write access");
            }
        else
        if (key_char == '3')
            {
            file_access = 0x02;
            cprintf("Read/Write access");
            }
        else
            goto get_access;

        gotoxy(1, 8);
        cprintf("                                 ");
        gotoxy(1, 9);
        cprintf("                                 ");

get_rec_len:
        gotoxy(1, 8);
        textattr(hi_inten);
        cprintf("Enter record length (1-32767)");
        gotoxy(1, 9);
        cprintf("and press <enter>");
        textattr(low_inten);
        gotoxy(14, 6);
        file_rec_len = 0;
        memset(string1, 0, 80);
        kbdstring(string1, 7);
        if (string1[0] == '\0')
            goto get_rec_len;
        file_rec_len = atoi(string1);
        if (file_rec_len < 1)
            goto get_rec_len;

        gotoxy(1, 8);
        cprintf("                                 ");
        gotoxy(1, 9);
        cprintf("                                 ");

        regs.h.ah = 0x3d;
        regs.h.al = file_inherit | file_sharing | file_access;
        regs.x.dx = (unsigned) filename;
        intdos(&regs, &regs);

        if (regs.x.cflag)
            {
            gotoxy(1, 8);
            textattr(hi_inten);
            cprintf("Failed.  DOS error %d", regs.x.ax);
            gotoxy(1, 9);
            cprintf("   (Press a key)");
            textattr(low_inten);
            }
        else
            {
            open_count++;
            i = 0;
            while (file_data[i].handle != 0)
                i++;
            strcpy(file_data[i].name, filename);
            file_data[i].handle       = regs.x.ax;
            file_data[i].lock_count   = 0;
            file_data[i].rec_length   = file_rec_len;
            file_data[i].inherit_flag = file_inherit;
            file_data[i].sharing_flag = file_sharing;
            file_data[i].access_flag  = file_access;
            gotoxy(1, 8);
            textattr(hi_inten);
            cprintf("File successfully opened");
            gotoxy(1, 9);
            cprintf("   (Press a key)");
            textattr(low_inten);
            }

        gotoxy(1, 1);
        getkey();

open_exit:
        window(1, 1, 80, 25);
        }

void    close_file(void)
        {
        window(1, 1, 80, 25);
        textattr(low_inten);
        for (i=0; i<12; i++)
            {
            gotoxy(19, i+7);
            cprintf("%s", box_lines2[i]);
            }

        gotoxy(21, 7);
        cprintf("[ CLOSE A FILE ]");
        window(20, 8, 58, 18);

        if (open_count == 0)
            {
            gotoxy(1, 8);
            textattr(hi_inten);
            cprintf("Whoops. No files are open.");
            gotoxy(1, 9);
            cprintf("   (Press a key)");
            textattr(low_inten);
            gotoxy(1, 1);
            getkey();
            goto close_exit;
            }

        for (i=0; i<5; i++)
            {
            gotoxy(1, i+2);
            cprintf(" %d. ", i+1);
            if (file_data[i].handle)
                cprintf("%s", file_data[i].name);
            }

get_close:
        gotoxy(1, 8);
        textattr(hi_inten);
        cprintf("Which file? (1-5) ");
        gotoxy(1, 9);
        cprintf("(ESC if none)");
        textattr(low_inten);
        gotoxy(19, 8);
        getkey();

        if (key_char == ESC)
            goto close_exit;

        i = (int) key_char - '1';
        if (i < 0 || i > 4)
            goto get_close;

        if (file_data[i].handle == 0)
            goto get_close;

        if (file_data[i].lock_count != 0)
            {
            gotoxy(1, 8);
            textattr(hi_inten);
            cprintf("%d records are still locked!", file_data[i].lock_count);
            gotoxy(1, 9);
            cprintf("File not closed.  (Press a key)");
            textattr(low_inten);
            gotoxy(1, 1);
            getkey();
            goto close_exit;
            }

        regs.h.ah = 0x3e;
        regs.x.bx = file_data[i].handle;
        intdos(&regs, &regs);

        if (regs.x.cflag)
            {
            gotoxy(1, 8);
            textattr(hi_inten);
            cprintf("Failed.  DOS error %d", regs.x.ax);
            gotoxy(1, 9);
            cprintf("   (Press a key)");
            textattr(low_inten);
            }
        else
            {
            open_count--;
            file_data[i].handle = 0;
            gotoxy(1, 8);
            textattr(hi_inten);
            cprintf("%s closed", file_data[i].name);
            gotoxy(1, 9);
            cprintf("   (Press a key)");
            textattr(low_inten);
            }

        gotoxy(1, 1);
        getkey();

close_exit:
        window(1, 1, 80, 25);
        }

void    lock_record(void)
        {
        window(1, 1, 80, 25);
        textattr(low_inten);
        for (i=0; i<12; i++)
            {
            gotoxy(19, i+7);
            cprintf("%s", box_lines2[i]);
            }

        gotoxy(21, 7);
        cprintf("[ LOCK A RECORD ]");
        window(20, 8, 58, 18);

        if (open_count == 0)
            {
            gotoxy(1, 8);
            textattr(hi_inten);
            cprintf("Whoops. No files are open.");
            gotoxy(1, 9);
            cprintf("   (Press a key)");
            textattr(low_inten);
            gotoxy(1, 1);
            getkey();
            goto lock_exit;
            }

        for (i=0; i<5; i++)
            {
            gotoxy(1, i+2);
            cprintf(" %d. ", i+1);
            if (file_data[i].handle)
                cprintf("%s", file_data[i].name);
            }

get_lock_file:
        gotoxy(1, 8);
        textattr(hi_inten);
        cprintf("Which file? (1-5) ");
        gotoxy(1, 9);
        cprintf("(ESC if none)");
        textattr(low_inten);
        gotoxy(19, 8);
        getkey();

        if (key_char == ESC)
            goto lock_exit;

        i = (int) key_char - '1';
        if (i < 0 || i > 4)
            goto get_lock_file;

        if (file_data[i].handle == 0)
            goto get_lock_file; 

get_lock_rec:
        gotoxy(1, 8);
        textattr(hi_inten);
        cprintf("Which record number? ");
        gotoxy(1, 9);
        cprintf("(numbers start at 1) ");
        textattr(low_inten);

        j = 0;
        gotoxy(22, 8);
        memset(string1, 0, 80);
        kbdstring(string1, 5);
        j = atoi(string1);
        if (j < 1)
            goto get_lock_rec;

        regs.h.ah = 0x5c;
        regs.h.al = 0x00;
        regs.x.bx = file_data[i].handle;
        regs.x.di = file_data[i].rec_length;
        regs.x.si = 0;
        regs.x.cx = 0;
        regs.x.dx = (j - 1) * file_data[i].rec_length;
        intdos(&regs, &regs);

        if (regs.x.cflag)
            {
            gotoxy(1, 8);
            textattr(hi_inten);
            cprintf("Failed.  DOS error %d     ", regs.x.ax);
            gotoxy(1, 9);
            cprintf("   (Press a key)          ");
            textattr(low_inten);
            }
        else
            {
            file_data[i].lock_count++;
            gotoxy(1, 8);
            textattr(hi_inten);
            cprintf("record %d locked             ", j);
            gotoxy(1, 9);
            cprintf("   (Press a key)             ");
            textattr(low_inten);
            }

        gotoxy(1, 1);
        getkey();

lock_exit:
        window(1, 1, 80, 25);
        }

void    unlock_record(void)
        {
        window(1, 1, 80, 25);
        textattr(low_inten);
        for (i=0; i<12; i++)
            {
            gotoxy(19, i+7);
            cprintf("%s", box_lines2[i]);
            }

        gotoxy(21, 7);
        cprintf("[ UNLOCK A RECORD ]");
        window(20, 8, 58, 18);

        if (open_count == 0)
            {
            gotoxy(1, 8);
            textattr(hi_inten);
            cprintf("Whoops. No files are open.");
            gotoxy(1, 9);
            cprintf("   (Press a key)");
            textattr(low_inten);
            gotoxy(1, 1);
            getkey();
            goto unlock_exit;
            }

        for (i=0; i<5; i++)
            {
            gotoxy(1, i+2);
            cprintf(" %d. ", i+1);
            if (file_data[i].handle)
                cprintf("%s", file_data[i].name);
            }

get_unlock_file:
        gotoxy(1, 8);
        textattr(hi_inten);
        cprintf("Which file? (1-5) ");
        gotoxy(1, 9);
        cprintf("(ESC if none)");
        textattr(low_inten);
        gotoxy(19, 8);
        getkey();

        if (key_char == ESC)
            goto unlock_exit;

        i = (int) key_char - '1';
        if (i < 0 || i > 4)
            goto get_unlock_file;

        if (file_data[i].handle == 0)
            goto get_unlock_file; 

        if (file_data[i].lock_count == 0)
            {
            gotoxy(1, 8);
            textattr(hi_inten);
            cprintf("No records are locked!");
            gotoxy(1, 9);
            cprintf("  (Press a key)");
            textattr(low_inten);
            gotoxy(1, 1);
            getkey();
            goto unlock_exit;
            }

get_unlock_rec:
        gotoxy(1, 8);
        textattr(hi_inten);
        cprintf("Which record number? ");
        gotoxy(1, 9);
        cprintf("(numbers start at 1) ");
        textattr(low_inten);

        j = 0;
        gotoxy(22, 8);
        memset(string1, 0, 80);
        kbdstring(string1, 5);
        j = atoi(string1);
        if (j < 1)
            goto get_unlock_rec;

        regs.h.ah = 0x5c;
        regs.h.al = 0x01;
        regs.x.bx = file_data[i].handle;
        regs.x.di = file_data[i].rec_length;
        regs.x.si = 0;
        regs.x.cx = 0;
        regs.x.dx = (j - 1) * file_data[i].rec_length;
        intdos(&regs, &regs);

        if (regs.x.cflag)
            {
            gotoxy(1, 8);
            textattr(hi_inten);
            cprintf("Failed.  DOS error %d     ", regs.x.ax);
            gotoxy(1, 9);
            cprintf("   (Press a key)          ");
            textattr(low_inten);
            }
        else
            {
            file_data[i].lock_count--;
            gotoxy(1, 8);
            textattr(hi_inten);
            cprintf("record %d unlocked          ", j);
            gotoxy(1, 9);
            cprintf("   (Press a key)            ");
            textattr(low_inten);
            }

        gotoxy(1, 1);
        getkey();

unlock_exit:
        window(1, 1, 80, 25);
        }

void    show_file(void)
        {
        j = i;

        window(1, 1, 80, 25);
        textattr(low_inten);
        for (i=0; i<12; i++)
            {
            gotoxy(19, i+7);
            cprintf("%s", box_lines2[i]);
            }

        gotoxy(21, 7);
        cprintf("[ FILE/LOCK STATUS ]");
        window(20, 8, 58, 18);

        i = j;
        gotoxy(1, 2);
        cprintf("      Name: %s", file_data[i].name);
        gotoxy(1, 3);
        cprintf("    Handle: %5d", file_data[i].handle);
        gotoxy(1, 4);
        cprintf("Rec Length: %5d", file_data[i].rec_length);
        gotoxy(1, 5);
        cprintf("Lock Count: %5d", file_data[i].lock_count);
        gotoxy(1, 6);
        j =   file_data[i].inherit_flag 
            | file_data[i].sharing_flag
            | file_data[i].access_flag;
        cprintf(" Open Mode:    %2.2x (hex)", j);

        gotoxy(1, 8);
        textattr(hi_inten);
        cprintf("(Press a key) ");
        textattr(low_inten);
        gotoxy(1, 1);
        getkey();
        }

void    show_status(void)
        {
        window(1, 1, 80, 25);
        textattr(low_inten);
        for (i=0; i<12; i++)
            {
            gotoxy(19, i+7);
            cprintf("%s", box_lines2[i]);
            }

        gotoxy(21, 7);
        cprintf("[ FILE/LOCK STATUS ]");
        window(20, 8, 58, 18);

        if (open_count == 0)
            {
            gotoxy(1, 8);
            textattr(hi_inten);
            cprintf("Whoops. No files are open.");
            gotoxy(1, 9);
            cprintf("   (Press a key)");
            textattr(low_inten);
            gotoxy(1, 1);
            getkey();
            goto status_exit;
            }

        for (i=0; i<5; i++)
            {
            gotoxy(1, i+2);
            cprintf(" %d. ", i+1);
            if (file_data[i].handle)
                cprintf("%s", file_data[i].name);
            }

get_status_file:
        gotoxy(1, 8);
        textattr(hi_inten);
        cprintf("Which file? (1-5) ");
        gotoxy(1, 9);
        cprintf("(ESC if none)");
        textattr(low_inten);
        gotoxy(19, 8);
        getkey();

        if (key_char == ESC)
            goto status_exit;

        i = (int) key_char - '1';
        if (i < 0 || i > 4)
            goto get_status_file;

        if (file_data[i].handle == 0)
            goto get_status_file; 

        show_file();

status_exit:
        window(1, 1, 80, 25);
        }
/************************************/

int     kbdstring(char buff[], int max_chars)
        {
        unsigned int    i, j, insert_mode, ctype, res;
        unsigned char   row, col, trow, tcol;
        unsigned int    cblock;

        i = j = insert_mode = 0;
        if (get_vid_mode() == 7)
            cblock = 0x000D;
        else
            cblock = 0x0007;

        _AH = 3;
        _BH = 0;
        geninterrupt(0x10);
        ctype = _CX;
        col = wherex();
        row = wherey();
        cprintf("%-*s", max_chars-1, buff);
        gotoxy(col, row);

ks1:    getkey();
        tcol = wherex();
        trow = wherey();
        if (key_char == ESC)
            {
            buff[0] = '\0';
            res = 0;
            goto kbdstring_exit;
            }

        if (key_char == 0)
            {
            if (extended_char == INSKEY)
                {
                if (insert_mode)
                    {
                    insert_mode = FALSE;
                    _CX = ctype;
                    _AH = 1;
                    geninterrupt(0x10);
                    }
                else
                    {
                    insert_mode = TRUE;
                    _CX = cblock;
                    _AH = 1;
                    geninterrupt(0x10);
                    }
                }
            else
            if (extended_char == HOMEKEY)
                {
                i = 0;
                gotoxy(col, row);
                }
            else
            if (extended_char == ENDKEY)
                {
                i = strlen(buff);
                gotoxy(col+strlen(buff), row);
                }
            else
            if (extended_char == DELKEY)
                {
                for (j = i; j < strlen(buff); j++)
                    buff[j] = buff[j+1];
                gotoxy(col, row);
                cprintf("%-*s", max_chars-1, buff);
                gotoxy(tcol, trow);
                }
            else
            if (extended_char == RIGHTKEY)
                {
                if (i < strlen(buff))
                    {
                    i++;
                    gotoxy(tcol+1, trow);
                    }
                }
            else
            if (extended_char == LEFTKEY)
                {
                if (i > 0)
                    {
                    i--;
                    gotoxy(tcol-1, trow);
                    }
                }
            }

        if (key_char == 0)
            goto ks1;

        if (key_char == BS)
            {
            if (i > 0)
                {
                i--;
                gotoxy(tcol-1, trow);
                }
            }

        if (key_char == CR)
            {
            res = 0;
            goto kbdstring_exit;
            }
        if (key_char < 32)
            goto ks1;
        if (i == max_chars-1)
            goto ks1;

        if (insert_mode)
            {
            for (j = strlen(buff)-1; j >= i; j--)
                if (j < max_chars-2)
                    buff[j+1] = buff[j];
            buff[i++] = key_char;
            _CX = ctype;
            _AH = 1;
            geninterrupt(0x10);
            gotoxy(col, row);
            cprintf("%-*s", max_chars-1, buff);
            gotoxy(++tcol, trow);
            _CX = cblock;
            _AH = 1;
            geninterrupt(0x10);
            }
        else
            {
            buff[i++] = key_char;
            cprintf("%c", key_char);
            }

        goto ks1;


kbdstring_exit:
        _CX = ctype;
        _AH = 1;
        geninterrupt(0x10);
        return(res);
        }

/************************************/

void    getkey(void)
        {
        unsigned int    k;

        k             = bioskey(0);
        key_char      = k & 0x00FF;
        extended_char = (k & 0xFF00) >> 8;
        }

/************************************/

int     get_vid_mode(void)
        {
        regs.h.ah = 15;
        int86(0x10, &regs, &regs);
        regs.h.ah = 0;
        return(regs.x.ax);
        }

/************************************/

void    save_cursor(struct csavetype *csave)
        {
        _AH = 3;
        _BH = 0;
        geninterrupt(0x10);
        csave->curloc  = _DX;
        csave->curmode = _CX;
        }

/************************************/

void    restore_cursor(struct csavetype *csave)
        {
        _DX = csave->curloc;
        _AH = 2;
        _BH = 0;
        geninterrupt(0x10);

        _CX = csave->curmode;
        _AH = 1;
        geninterrupt(0x10);
        }

/************************************/
void    init_program(void)
        {
        if (get_vid_mode() == 7)
            {
            low_inten = 0x07;
            hi_inten  = 0x0f;
            }
        else
            {
            low_inten = 0x17;
            hi_inten  = 0x1f;
            }

        file_data[0].handle = 0;
        file_data[1].handle = 0;
        file_data[2].handle = 0;
        file_data[3].handle = 0;
        file_data[4].handle = 0;
        file_data[0].lock_count = 0;
        file_data[1].lock_count = 0;
        file_data[2].lock_count = 0;
        file_data[3].lock_count = 0;
        file_data[4].lock_count = 0;
        open_count = 0;

        clrscr();

        _AX = 'BN';
        geninterrupt(0x16);
        if (_AX == 'bn')
            {
            gotoxy(1, 2);
            cprintf("The Local Area Network Emulator is already loaded.");
            gotoxy(1, 3);
            cprintf("Use ALT/RIGHT-SHIFT to activate it.");
            gotoxy(1, 5);
            exit(1);
            }

        if (_osmajor < 3)
            {
            gotoxy(1, 2);
            cprintf("Early versions of DOS not supported...");
            gotoxy(1, 4);
            exit(1);
            }

        regs.x.ax = 0x1000;
        int86(0x2f, &regs, &regs);
        if (regs.h.al != 0xff)
            {
            gotoxy(1, 2);
            cprintf("You must run SHARE.EXE before using the LAN Emulator.");
            gotoxy(1, 4);
            exit(1);
            }

        sregs.ds = _DS;
        regs.x.ax = 0x3400;
        intdosx(&regs, &regs, &sregs);
        indos_ptr = MK_FP(sregs.es, regs.x.bx);
        if (_osmajor == 2)
            indos2_ptr = MK_FP(sregs.es, regs.x.bx + 1);
        else
            indos2_ptr = MK_FP(sregs.es, regs.x.bx - 1);

        prtsc_flag_ptr = MK_FP(0x0050, 0x0000);
        kbd_flag_ptr   = MK_FP(0x0040, 0x0017);

        if ( (our_stack = malloc(MY_STK_SIZE)) == NULL)
            {
            gotoxy(1, 2);
            cprintf("Insufficient memory.");   
            gotoxy(1, 5);
            exit(1);
            }

get_net_drive:
        gotoxy(1, 24);
        cprintf("Drive letter that will be your network drive: ");
        network_drive = 0;
        gotoxy(1, 25);
        memset(string1, 0, 80);
        kbdstring(string1, 5);
        strupr(string1);
        network_drive = string1[0] - '@';
        if (network_drive < 1 || network_drive > 20)
            goto get_net_drive;

        }
/************************************/

void    main(int argc, char *argv[])
        {
        init_program();

        clrscr();
        gotoxy(1, 3);
        cprintf("NETWORK--a LAN Emulator.  Copyright (c) 1988 Barry R. Nance");
        gotoxy(1, 4);
        cprintf("Press ALT/RIGHT-SHIFT to activate this program.");
        gotoxy(1, 6);
        cprintf("As you test, the following conditions will exist:");
        gotoxy(1, 7);
        cprintf("     Machine Name: %s", machine_name);
        gotoxy(1, 8);
        cprintf("    Network Drive: %c:", network_drive + '@');
        gotoxy(1, 12);

        save_ss  = _SS;
        save_sp  = ( (unsigned) our_stack + MY_STK_SIZE ) - 2;

        ourdta_ptr = getdta();
        regs.x.ax = 0x5100;
        intdos(&regs, &regs);
        ourpsp = regs.x.bx;

        our_mcb       = MK_FP(ourpsp-1, 0);
        our_mcb_size  = MK_FP(ourpsp-1, 3);

        oldint08 = getvect(0x08);
        oldint09 = getvect(0x09);
        oldint10 = getvect(0x10);
        oldint13 = getvect(0x13);
        oldint16 = getvect(0x16);
        oldint28 = getvect(0x28);
        oldint21 = getvect(0x21);
        asm     mov ax, word ptr oldint21
        asm     mov word ptr cs:[0000], ax
        asm     mov ax, word ptr oldint21+2
        asm     mov word ptr cs:[0002], ax

        setvect(0x10, int10);
        setvect(0x13, int13);
        setvect(0x16, int16);
        setvect(0x21, int21);
        setvect(0x28, int28);
        setvect(0x09, int09);
        setvect(0x08, int08);

        paragraphs = (save_ss + (save_sp >> 4) + 0x10) - ourpsp;
        keep(0, paragraphs);
        }

/************************************/

