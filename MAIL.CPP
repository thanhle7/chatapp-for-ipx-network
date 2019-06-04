/*      E-Mail  V1.00  (Netware Mail)          */ 
/*      Copyright (c) 1989 Barry R. Nance      */

#pragma  inline

#include <stdio.h>
#include <dos.h>
#include <dir.h>
#include <mem.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <conio.h>
#include <bios.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <time.h>
#include <ipx.h>

void    do_popup(void);
void    actual_popup(void);
int     get_vid_mode(void);
void    getkey(void);
void    vidtype(void);
int     kbdstring(char buff[], int max_chars);
int     fgetbuf(int fh);
int     fgetstring(int fh, char buff[], int max_chars);
void    message(char *s);
void    ask_yn(char *s);
void    beep(void);

#define     TRUE           1
#define     FALSE          0
#define     LT            <0
#define     EQ           ==0
#define     GT            >0
#define     NE           !=0
#define     ALT           56
#define     RIGHT_SHIFT   54
#define     BELL           7
#define     BS             8
#define     LINEFEED      10
#define     FORMFEED      12
#define     CR            13
#define     TAB            9
#define     BACKTAB       15
#define     CTRLQ         17
#define     CTRLS         19
#define     CTRLX         24
#define     CTRLZ         26
#define     ESC           27
#define     ALTX          45
#define     ALTC          46
#define     ALTS          31
#define     ALTD          32
#define     ALTE          18
#define     ALTF          33
#define     ALTT          20
#define     ALTM          50
#define     ALTH          35
#define     ALT1         120
#define     ALT2         121
#define     ALT3         122
#define     ALT4         123
#define     ALT5         124
#define     ALT6         125
#define     ALT7         126
#define     ALT8         127
#define     ALT9         128
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
#define     ALT_F10      113

/* ------------------------------------------- */
char    user_id[51];
char    addressee[16];
char    subject[31];

typedef struct
        {
        char    mailfile[13];
        char    mailname[16];
        char    maildate[17];
        char    subject[31];
        char    read_flag;
        } BASKET;

BASKET outbasket[50];
BASKET inbasket[50];

typedef struct
        {
        char    addressee[16];
        char    to_name[16];
        char    sender[16];
        char    maildate[17];
        char    subject[31];
        char    copy_flag;
        char    attachment_flag;
        char    attachment_name[66];
        char    cc_list[66];
        char    read_flag;
        char    crlf[2];
        } ENVELOPE;

ENVELOPE    envelope;

char    edit_area[1920];
char    *edit_line[24];

unsigned    paragraphs;
char        critical_error = FALSE;
int         i, j, k;
unsigned    temp1, temp2, temp3, temp4, temp5;
unsigned    temp_ax;
unsigned char temp_ah;
unsigned char temp_al;
unsigned    old_ss, old_sp, our_ss, our_sp;
char        far *ourdta_ptr;
char        far *olddta_ptr;
char        far *our_mcb;
unsigned    far *our_mcb_size;
char        far *next_mcb;
unsigned    far *next_mcb_owner;
unsigned    ourpsp;
unsigned    oldpsp;
int         break_state;
void     interrupt (*oldint08)(void);
void     interrupt (*oldint09)(void);
void     interrupt (*oldint10)(void);
void     interrupt (*oldint13)(void);
void     interrupt (*oldint16)(void);
void     interrupt (*oldint1b)(void);
void     interrupt (*oldint1c)(void);
void     interrupt (*oldint23)(void);
void     interrupt (*oldint24)(void);
void     interrupt (*oldint28)(void);
char far             *kbd_flag_ptr;
unsigned char far    *indos_ptr;
unsigned char far    *indos2_ptr;
unsigned char    in_int08 = FALSE;
unsigned char    in_int09 = FALSE;
unsigned char    in_int10 = FALSE;
unsigned char    in_int13 = FALSE;
unsigned char    in_int16 = FALSE;
unsigned char    in_int28 = FALSE;
unsigned char    in_popup = FALSE;
unsigned char    hotkey_flag = FALSE;
unsigned char    de_install = FALSE;
unsigned char    break_flag = FALSE;

long far        *outbasket_alarm_ptr;
unsigned char far *postman_deinstall_ptr;
unsigned char far *new_mail_ptr;

char            mail_path[65];
int             in_count, out_count;
int             top_inbasket, top_outbasket;
int             inbasket_choice;
int             outbasket_choice;
int             which_basket;
int             redraw_list;
char            io_buffer[512];
char            filename[81];
char            string[101];
int             handle    = 0;
int             io_len    = 0;
unsigned int    fbufndx   = 2000;
unsigned int    fbufbytes = 0;

int             curr_item, old_item;
unsigned char   reg_fg = LIGHTGRAY;
unsigned char   reg_bg = BLUE;
unsigned char   msg_fg = BLACK;
unsigned char   msg_bg = CYAN;
int             cursor_save;
int             cursor_type;
unsigned        vidmode;
unsigned char   curr_vid_mode;
unsigned char   key_char;
unsigned char   extended_char;
char            app_screen [4000];
unsigned long   file_bytes;

char    *empty_basket[7] =
    {
    "                   ",
    "                   ",
    "                   ",
    "    ÛßßßßßßßßßÛ    ",
    " Ý/    empty    \Þ ",
    " ßßßßßßßßßßßßßßßßß ",
    "                   "
    };

char    *one_letter[7] =
    {
    "                   ",
    "                   ",
    "   ÚÄÄÄÄÄÄÄÄÄÄÄ¿   ",
    "   ³÷÷÷       ß³   ",
    " Ý/³     ððððð ³\Þ ",
    " ßßßßßßßßßßßßßßßßß ",
    "                   "
    };

char    *multiple_letters[7] =
    {
    "                   ",
    "    ÚÄÄÄÄÄÄÄÄÄÄÄ¿  ",
    "  ÚÄÁÄÄÄÄÄÄÄÄÄ¿ß³  ",
    "  ³÷÷÷       ß³ ³  ",
    " Ý³     ððððð ³ÄÙÞ ",
    " ßßßßßßßßßßßßßßßßß ",
    "                   "
    };


char    *send_icon[7] =
    {
    "   ÚÄÄÄÄÄÄÄÄÄÄÄ¿   ",
    "   ³ ððð       ³   ",
    "   ³ ððððððððð ³   ",
    "   ³ ððððððððð ³   ",
    "   ³ ððððððððð ³   ",
    "   ³ ððððððððð ³   ",
    "   ÀÄÄÄÄÄÄÄÄÄÄÄÙ   "
    };

char    *list_icon[11] =
    {
"Ú                                                                        ¿",
"³ø                                                                      ø³",
"³ø                                                                      ø³",
"³ø                                                                      ø³",
"³ø                                                                      ø³",
"³ø                                                                      ø³",
"³ø                                                                      ø³",
"³ø                                                                      ø³",
"³ø                                                                      ø³",
"³ø                                                                      ø³",
"À                                                                        Ù"
    };

unsigned                *our_stack;

/* - - - - - - - - - - - - - - - - - - - - - - - - */

void    interrupt int08(void)
    {
    in_int08 = TRUE;
    oldint08();
    enable();

    if (!hotkey_flag && !de_install)
        goto exit08;

    if (in_popup)
        goto exit08;

    if (in_int09 || in_int10 || in_int13 || in_int16 || in_int28)
        goto exit08;

    if (*indos_ptr != 0)
         goto exit08;

    if (*indos2_ptr != 0)
         goto exit08;

    outportb(0x20, 0x0b);
    if (inportb(0x20)) goto exit08;

    in_popup = TRUE;
    do_popup();
    in_popup = FALSE;

exit08:
    in_int08 = FALSE;
    }

void interrupt  int09 (void)
    {
    in_int09 = TRUE;
    oldint09();
    enable();
    if ( (*kbd_flag_ptr & 0x09) == 0x09 )
        {
        de_install = FALSE;
        hotkey_flag= TRUE;
        }
    in_int09 = FALSE;
    }

void    far int10(unsigned flags)
    {
    asm     pop  bp

    asm     push ax
    asm     push ds
    asm     mov  ax, DGROUP
    asm     mov  ds, ax
    asm     mov  _in_int10, 1
    asm     pop  ds
    asm     pop  ax

    asm     pushf
    asm     call    dword ptr cs:[0000h]
    asm     sti

    asm     push bp
    asm     mov  bp, sp
    asm     pushf
    asm     pop  flags
    asm     pop  bp

    asm     push ax
    asm     push ds
    asm     mov  ax, DGROUP
    asm     mov  ds, ax
    asm     mov  _in_int10, 0
    asm     pop  ds
    asm     pop  ax

    asm     iret
    }

void    far int13(unsigned flags)
    {
    asm     pop  bp

    asm     push ax
    asm     push ds
    asm     mov  ax, DGROUP
    asm     mov  ds, ax
    asm     mov  _in_int13, 1
    asm     pop  ds
    asm     pop  ax

    asm     pushf
    asm     call    dword ptr cs:[0004h]
    asm     sti

    asm     push bp
    asm     mov  bp, sp
    asm     pushf
    asm     pop  flags
    asm     pop  bp

    asm     push ax
    asm     push ds
    asm     mov  ax, DGROUP
    asm     mov  ds, ax
    asm     mov  _in_int13, 0
    asm     pop  ds
    asm     pop  ax

    asm     iret
    }

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
    in_int16 = TRUE;
    enable();
    temp_ax = _AX;
    temp_ah = _AH;

    if (temp_ax == 'MA')
        {
        ax = 'ma';
        goto int16_exit;
        }

    if (temp_ah != 0)
        goto do_old16;

wait_for_key:
    _AH = 1;
    oldint16();
    asm     jz   popup_16 
    goto do_old16;

popup_16:
    if (in_popup)
        {
        if (*new_mail_ptr)
            {
            ax = 0x0000;
            goto int16_exit;
            }
        oldint28();
        }

    if (!hotkey_flag && !de_install)
        goto wait_for_key;

    if (*indos_ptr != 0)
         goto wait_for_key;

    if (*indos2_ptr != 0)
         goto wait_for_key;

    outportb(0x20, 0x0b);
    if (inportb(0x20)) goto wait_for_key;

    in_popup = TRUE;
    do_popup();
    in_popup = FALSE;

    goto wait_for_key;

do_old16:
    _AX = temp_ax;
    oldint16();
    asm     pushf
    asm     pop flags
    ax    = _AX;
    bx    = _BX;
    cx    = _CX;
    dx    = _DX;

int16_exit:
    in_int16 = FALSE;
    }

void interrupt  int1b (void)
    {
    enable();
    break_flag = TRUE;
    }

void interrupt  int1c (void)
    {
    enable();
    }

void interrupt  int23 (void)
    {
    enable();
    }

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
    critical_error = TRUE;

    if (_osmajor < 3)
       ax = (temp1 & 0xFF00);
    else
       ax = (temp1 & 0xFF00) | 0x03;
    }


void    interrupt int28(void)
    {
    in_int28 = TRUE;
    oldint28();
    enable();

    if (!hotkey_flag && !de_install)
        goto exit28;

    if (in_popup)
        goto exit28;

    if (*indos_ptr > 1)
         goto exit28;

    if (*indos2_ptr != 0)
         goto exit28;

    outportb(0x20, 0x0b);
    if (inportb(0x20)) goto exit28;

    in_popup = TRUE;
    do_popup();
    in_popup = FALSE;

exit28:
    in_int28 = FALSE;
    }

/* ------------------------------------------- */

void    beep(void)
        {
        sound(880);
        delay(100);
        nosound();
        }

void    save_screen(unsigned left, unsigned top,
                        unsigned right, unsigned bottom, char *buf,
                        int *curpos, int *curtype)
        {
        gettext(left, top, right, bottom, buf);

        _AH = 3;
        _BH = 0;
        geninterrupt(0x10);
        *curpos  = _DX;
        *curtype = _CX;
        }

void    restore_screen(unsigned left, unsigned top,
                        unsigned right, unsigned bottom, char *buf,
                        int *curpos, int *curtype)
        {
        puttext(left, top, right, bottom, buf);

        _DX = *curpos;
        _AH = 2;
        _BH = 0;
        geninterrupt(0x10);
        _CX = *curtype;
        _AH = 1;
        geninterrupt(0x10);
        }

void    getkey(void)
        {
        int     k;

        k             = bioskey(0);
        key_char      = k & 0x00FF;
        extended_char = (k & 0xFF00) >> 8;
        }


int     kbdstring(char buff[], int max_chars)
        {
        int             i, j, insert_mode, ctype, res;
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

        textcolor(msg_fg);
        textbackground(msg_bg);
        cprintf("%-*s", max_chars-1, buff);
        gotoxy(col, row);

ks1:    getkey();
        tcol = wherex();
        trow = wherey();

        if (key_char == ESC)
            {
            memset(buff, 0, max_chars);
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
                buff[max_chars] = '\0';
                gotoxy(col, row);
                textcolor(msg_fg);
                textbackground(msg_bg);
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
            buff[max_chars] = '\0';
            _CX = ctype;
            _AH = 1;
            geninterrupt(0x10);
            gotoxy(col, row);
            textcolor(msg_fg);
            textbackground(msg_bg);
            cprintf("%-*s", max_chars-1, buff);
            gotoxy(++tcol, trow);
            _CX = cblock;
            _AH = 1;
            geninterrupt(0x10);
            }
        else
            {
            buff[i++] = key_char;
            textcolor(msg_fg);
            textbackground(msg_bg);
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

int     fgetbuf(int fh)
        {
fgetbuf01:
        if (fbufndx < fbufbytes)
            return( (int) io_buffer[fbufndx++]);

        io_len = read(fh, io_buffer, 512);
        if (io_len == -1)
            {
            fbufndx = 2000;
            return(-1);
            }

        fbufbytes = io_len;
        if (fbufbytes == 0)
            {
            fbufndx = 2000;
            return(-1);
            }

        fbufndx = 0;
        goto fgetbuf01;
        }

/************************************/

int     fgetstring(int fh, char buff[], int max_chars)
        {
        int i, c;
        char ch;

        setmem(buff, max_chars, 0);
        i = 0;

fgs1:   if (i == max_chars - 1)
            {
            buff[i] = '\0';
            return(0);
            }
        if ( (c = fgetbuf(fh)) == -1 )
            {
            buff[i] = '\0';
            return(-1);
            }
        ch = (char) c;
        if (ch == CR)
            goto fgs1;
        if (ch == LINEFEED)
            {
            buff[i] = '\0';
            return(0);
            }

        buff[i++] = ch;
        goto fgs1;
        }


void    ask_yn(char *s)
        {
        char    linsav[160];
        int     cpos, ctype;

        save_screen(1,25,80,25, linsav, &cpos, &ctype);
        gotoxy(1, 25);
        cprintf("%-79.79s", " ");
        gotoxy(1, 25);
        cprintf("%s (y/n) ", s);

ask10:
        getkey();
        if ( key_char != 'y' && key_char != 'Y'
          && key_char != 'n' && key_char != 'N' )
            goto ask10;

        key_char |= 0x20;
        restore_screen(1,25,80,25, linsav, &cpos, &ctype);
        }

void    message(char *s)
        {
        char    linsav[160];
        int     cpos, ctype;

        save_screen(1,25,80,25, linsav, &cpos, &ctype);
        gotoxy(1, 25);
        cprintf("%-64.64s<Press a key.> ", s);
        getkey();
        restore_screen(1,25,80,25, linsav, &cpos, &ctype);
        }

int     okay_to_unload(void)
        {
        next_mcb       = MK_FP( (ourpsp) + *our_mcb_size, 0);
        next_mcb_owner = MK_FP(  ourpsp  + *our_mcb_size, 1);

        if (  *next_mcb_owner == 0x0000
           || *next_mcb_owner == 0xffff
           || *next_mcb_owner <  ourpsp  )
                return TRUE;

        return FALSE;
        }

void    trim_spaces(char s[])
        {
        int i;

        i = strlen(s) - 1;
        while (i > 0 && s[i] == ' ')
            {
            s[i] = '\0';
            i--;
            }
        }

void    show_help(int context)
        {
        int     help_handle, row, j;
        char    *p;
        char    str80[81];
        char    itemid[4];
        char    pageid[4];

        if ( (p = searchpath("mail.hlp")) == NULL || critical_error )
            {
            critical_error = FALSE;
            beep();
            message("ERROR.  'MAIL.HLP' not found.");
            return;
            }

        strcpy(str80, p);
        help_handle = open(str80, O_RDONLY | O_DENYNONE);
        if (help_handle < 1)
            {
            beep();
            message("ERROR.  'MAIL.HLP' not found.");
            return;
            }

        sprintf(itemid, "%1d", context);
        strcpy(pageid, "pg");
        strcat(pageid, itemid);
        i = fgetstring(help_handle, str80, 80);
        while (strcmp(str80, pageid) NE && i != -1)
            i = fgetstring(help_handle, str80, 80);

another_one:
        i = fgetstring(help_handle, str80, 80);
        row = 0;
        textcolor(msg_fg);
        textbackground(msg_bg);
        clrscr();

        while (strchr(str80, FORMFEED) == NULL && i != -1 && row < 24)
            {
            gotoxy(1, ++row);
            for (j=0; j<strlen(str80); j++)
                {
                if (str80[j] == 01)
                    {
                    textcolor(msg_bg);
                    textbackground(msg_fg);
                    }
                else
                if (str80[j] == 02)
                    {
                    textcolor(msg_fg);
                    textbackground(msg_bg);
                    }
                else
                    cprintf("%c", str80[j]);
                }
            i = fgetstring(help_handle, str80, 80);
            }

        if (i != -1)
            {
            message(" ESC to leave Help; any other key to see more Help. ");
            if (key_char != ESC)
                {
                i = fgetstring(help_handle, str80, 80);
                goto another_one;
                }
            }
        else
            message(" ");

        close(help_handle);
        }

int     get_vid_mode(void)
        {
        _AH = 15;
        geninterrupt(0x10);
        _AH = 0;
        return _AX;
        }

void    sort_basket(BASKET basket[], int count)
        {
        int     i, n1, n2;
        BASKET  temp;

        i = 0;
        while (i < count - 1)
            {
            n1 = atoi(basket[i].mailfile);
            n2 = atoi(basket[i+1].mailfile);
            if (n2 < n1)
                {
                temp = basket[i];
                basket[i] = basket[i+1];
                basket[i+1] = temp;
                i = 0;
                }
            else
                {
                i++;
                }
            }
        }

void    check_inbasket(void)
        {
        int  i, flag;
        struct ffblk findblock;

        for (i=0; i<50; i++)
            memset(&inbasket[i], 0, sizeof(BASKET));

        in_count = 0;
        strcpy(filename, mail_path);
        strcat(filename, "*.in");
        flag = findfirst(filename, &findblock, 0);
        while (flag == 0 && in_count < 50)
            {
            strcpy(inbasket[in_count].mailfile, findblock.ff_name);
            strcpy(filename, mail_path);
            strcat(filename, findblock.ff_name);
            handle = open(filename, O_RDWR | O_DENYALL);
            if (handle == -1)
                return;
            i = read(handle, &envelope, sizeof(ENVELOPE));
            if (i == sizeof(ENVELOPE))
                {
                strcpy(inbasket[in_count].mailname,
                        envelope.sender);
                strcpy(inbasket[in_count].maildate,
                        envelope.maildate);
                strcpy(inbasket[in_count].subject,
                        envelope.subject);
                inbasket[in_count].read_flag = envelope.read_flag;
                }
            close(handle);
            in_count++;
            flag = findnext(&findblock);
            }
        sort_basket(inbasket, in_count);
        }

void    check_outbasket(void)
        {
        int  i, flag;
        struct ffblk findblock;

        for (i=0; i<50; i++)
            memset(&outbasket[i], 0, sizeof(BASKET));

        out_count = 0;
        strcpy(filename, mail_path);
        strcat(filename, "*.out");
        flag = findfirst(filename, &findblock, 0);
        while (flag == 0 && out_count < 50)
            {
            strcpy(outbasket[out_count].mailfile, findblock.ff_name);
            strcpy(filename, mail_path);
            strcat(filename, findblock.ff_name);
            handle = open(filename, O_RDWR | O_DENYALL);
            if (handle == -1)
                return;
            i = read(handle, &envelope, sizeof(ENVELOPE));
            if (i == sizeof(ENVELOPE))
                {
                strcpy(outbasket[out_count].mailname,
                        envelope.addressee);
                strcpy(outbasket[out_count].maildate,
                        envelope.maildate);
                strcpy(outbasket[out_count].subject,
                        envelope.subject);
                outbasket[out_count].read_flag = envelope.read_flag;
                }
            close(handle);
            out_count++;
            flag = findnext(&findblock);
            }
        sort_basket(outbasket, out_count);
        }

void    get_attached_filename(void)
        {
        int h;

get_name:
        textcolor(reg_fg);
        textbackground(reg_bg);
        clrscr();

        textcolor(msg_fg);
        textbackground(msg_bg);
        gotoxy(1, 25);
        cprintf(
" Enter filename to attach (if any), including drive:\path. Then press enter. ");

        gotoxy(1, 1);
        envelope.attachment_flag = 'N';
        cprintf("Attach file: ");
        kbdstring(envelope.attachment_name, 65);

        textcolor(reg_fg);
        textbackground(reg_bg);
        gotoxy(1, 25);
        clreol();

        if (strlen(envelope.attachment_name) > 0)
            {
            h = open(envelope.attachment_name, O_RDONLY);
            if (h == -1)
                {
                textcolor(msg_fg);
                textbackground(msg_bg);
                gotoxy(2, 25);
                if (errno == ENOENT)
                    cprintf("ERROR. No file by that name (press a key)");
                else
                    cprintf("ERROR. Cannot access file.  (press a key)");
                getkey();
                goto get_name;
                }
            close(h);
            envelope.attachment_flag = 'Y';
            }
        }

void    editor(char *filename)
        {
        int     i, column, row, ed_handle, count, insert_mode;
        char    CRLF[3] = {13, 10, 0};
        char    cc_sequence[2] = {'A', 0};
        char    cc_string[66];
        char    current_date[30];
        char    cc_filename[81];
        long    t;
        char    *p;

        textcolor(reg_fg);
        textbackground(reg_bg);
        clrscr();

        for (row=0; row<24; row++)
            memset(edit_line[row], 0, 80);
        ed_handle = open(filename, O_RDWR | O_DENYALL);
        fbufndx = 2000;
        if (ed_handle == -1)
            {
            if (errno != ENOENT)
                {
                clrscr();
                gotoxy(1, 1);
                cprintf("I/O Error; can't open file.  (Press a key)");
                getkey();
                return;
                }
            memset(&envelope, 0, sizeof(ENVELOPE));
            gotoxy(1, 1);
            cprintf("TO: ");
            memset(addressee, 0, 16);
            kbdstring(addressee, 15);
            if (strlen(addressee) == 0)
                {
                textcolor(reg_fg);
                textbackground(reg_bg);
                clrscr();
                return;
                }
            textcolor(reg_fg);
            textbackground(reg_bg);
            gotoxy(1, 3);
            cprintf("SUBJECT: ");
            memset(subject, 0, 31);
            kbdstring(subject, 30);
            strcpy(edit_line[0], "TO: ");
            strcat(edit_line[0], addressee);
            strcpy(edit_line[1], "SUBJECT: ");
            strcat(edit_line[1], subject);
            textcolor(reg_fg);
            textbackground(reg_bg);
            clrscr();
            gotoxy(1, 1);
            cprintf("%s", edit_line[0]);
            gotoxy(1, 2);
            cprintf("%s", edit_line[1]);
            }
        else
            {
            i = read(ed_handle, &envelope, sizeof(ENVELOPE));
            if (i != sizeof(ENVELOPE))
                {
                close(ed_handle);
                clrscr();
                gotoxy(1, 1);
                cprintf("Error...can't read envelope. (Press a key)");
                getkey();
                return;
                }
            if (envelope.copy_flag == 'Y')
                {
                textcolor(reg_fg);
                textbackground(reg_bg);
                clrscr();
                textcolor(msg_fg);
                textbackground(msg_bg);
                gotoxy(1, 25);
                cprintf(
" This is a CC: copy.  Respecify the destination name, then press enter. ");
                gotoxy(1, 3);
                cprintf("Destination: ");
                memset(addressee, 0, 16);
                strcpy(addressee, envelope.addressee);
                kbdstring(addressee, 15);
                textcolor(reg_fg);
                textbackground(reg_bg);
                gotoxy(1, 25);
                clreol();
                if (strlen(addressee) == 0)
                    {
                    close(ed_handle);
                    return;
                    }
                memset(envelope.addressee, 0, 16);
                strcpy(envelope.addressee, addressee);
                trim_spaces(envelope.addressee);
                lseek(ed_handle, 0l, SEEK_SET);
                write(ed_handle, &envelope, sizeof(ENVELOPE));
                close(ed_handle);
                return;
                }
            strcpy(edit_line[0], "TO: ");
            strcat(edit_line[0], envelope.addressee);
            strcpy(edit_line[1], "SUBJECT: ");
            strcat(edit_line[1], envelope.subject);
            textcolor(reg_fg);
            textbackground(reg_bg);
            clrscr();
            gotoxy(1, 1);
            cprintf("%s", edit_line[0]);
            gotoxy(1, 2);
            cprintf("%s", edit_line[1]);
            fbufndx = 2000;
            for (row=2; row<24; row++)
                {
                i = fgetstring(ed_handle, edit_line[row], 80);
                if (i == -1) break;
                gotoxy(1, row + 1);
                cprintf("%s", edit_line[row]);
                }
            close(ed_handle);
            }
        row = 4; column = 1; insert_mode = 0;

        textcolor(msg_fg);
        textbackground(msg_bg);
        gotoxy(2, 25);
        cprintf(
"ESC-Quit  F2-Save   F4-Erase EOL   F5-Insert Line   F10-Delete Line           ");
        textcolor(reg_fg);
        textbackground(reg_bg);

        window(1, 1, 80, 24);

editor_keypress:
        gotoxy(column, row);
        getkey();

        if (key_char == ESC)
            {
            window(1, 1, 80, 25);
            gotoxy(1, 25);
            clreol();
            gotoxy(2, 25);
            textcolor(msg_fg);
            textbackground(msg_bg);
            cprintf("Are you sure? (y/n) ");
            getkey();
            if (key_char == 'Y' || key_char == 'y')
                {
                textcolor(reg_fg);
                textbackground(reg_bg);
                clrscr();
                return;
                }
            textcolor(msg_fg);
            textbackground(msg_bg);
            gotoxy(2, 25);
            cprintf(
"ESC-Quit  F2-Save   F4-Erase EOL   F5-Insert Line   F10-Delete Line           ");
            insert_mode = 0;
            textcolor(reg_fg);
            textbackground(reg_bg);
            window(1, 1, 80, 24);
            }
        else
        if (key_char >= 32 && key_char <= 127)
            {
            if (row == 1 && column < 5)
                goto editor_keypress;
            if (row == 2 && column < 10)
                goto editor_keypress;
            if (row == 24 && column == 80)
                goto editor_keypress;
            while (strlen(edit_line[row-1]) < column - 1)
                strcat(edit_line[row-1], " ");
            if (insert_mode)
                {
                if (edit_line[row-1][78] != ' '
                   && edit_line[row-1][78] != '\0')
                    goto editor_keypress;
                for (i=78; i>=column-1; i--)
                    edit_line[row-1][i+1] = edit_line[row-1][i];
                edit_line[row-1][column-1] = key_char;
                edit_line[row-1][79] = '\0';
                gotoxy(1, row);
                cprintf("%s", edit_line[row-1]);
                }
            else
                {
                edit_line[row-1][column-1] = key_char;
                cprintf("%c", key_char);
                }
            if (column < 80)
                {
                column++;
                }
            else
            if (row < 24)
                {
                column = 1;
                row++;
                }
            }
        else
        if (key_char == CR)
            {
            if (insert_mode && row < 24)
                {
                if (row == 1 && column < 5)
                    goto editor_keypress;
                if (row == 2 && column < 10)
                    goto editor_keypress;
                for (i=23; i>row; i--)
                    strcpy(edit_line[i], edit_line[i-1]);
                memset(edit_line[row], 0, 80);
                if (edit_line[row-1][column-1] != '\0')
                    strcpy(edit_line[row], &edit_line[row-1][column-1]);
                for (i=column-1; i<80; i++)
                    edit_line[row-1][i] = '\0';
                clreol();
                row++;
                column = 1;
                gotoxy(1, row);
                insline();
                gotoxy(1, row);
                cprintf("%s", edit_line[row-1]);
                }
            else
                {
                if (row < 24)
                    {
                    column = 1;
                    row++;
                    }
                }
            }
        else
        if (key_char == BS)
            {
            key_char = 0;
            extended_char = LEFTKEY;
            }
        else
        if (key_char == TAB && column < 72)
            {
            column += 4;
            }

        if (key_char == 0)
            {
            if (extended_char == F2)
                {
                window(1, 1, 80, 25);
                ed_handle = _creat(filename, 0);
                if (ed_handle == -1)
                    {
                    clrscr();
                    gotoxy(1, 1);
                    cprintf("I/O Error; can't open file.  (Press a key)");
                    getkey();
                    return;
                    }
                count = 24;
                for (row=23; row >1; row--)
                    {
                    if (strlen(edit_line[row]) > 0)
                        break;
                    count--;
                    }
                edit_line[0][19] = '\0';
                time(&t);
                strcpy(current_date, ctime(&t));
                current_date[16] = '\0';
                edit_line[1][39] = '\0';
                get_attached_filename();
                textcolor(msg_fg);
                textbackground(msg_bg);
                gotoxy(1, 25);
                cprintf(
" Enter CC: names (if any) separated by commas or spaces.  Then press enter. ");
                gotoxy(1, 3);
                cprintf("CC: ");
                kbdstring(envelope.cc_list, 65);
                textcolor(reg_fg);
                textbackground(reg_bg);
                gotoxy(1, 25);
                clreol();
                strcpy(envelope.addressee, &edit_line[0][4]);
                trim_spaces(envelope.addressee);
                strcpy(envelope.to_name, envelope.addressee);
                strcpy(envelope.sender, user_id);
                strcpy(envelope.maildate, current_date);
                strcpy(envelope.subject, &edit_line[1][9]);
                envelope.read_flag = 'N';
                envelope.copy_flag = 'N';
                envelope.crlf[0] = 13;
                envelope.crlf[1] = 10;
                i = write(ed_handle, &envelope, sizeof(ENVELOPE));
                for (row=2; row<count; row++)
                    {
                    i = strlen(edit_line[row]);
                    if (i > 0)
                        write(ed_handle, edit_line[row], i);
                    write(ed_handle, CRLF, 2);
                    }
                write(ed_handle, CRLF, 2);
                close(ed_handle);
                strcpy(cc_string, envelope.cc_list);
                cc_sequence[0] = 'A';
                time(&t);
                p = strtok(cc_string, " ,");
                while (p != NULL && strlen(p) < 16)
                    {
                    strcpy(envelope.addressee, p);
                    envelope.copy_flag = 'Y';
                    sprintf(current_date, "%ld", t);
                    strcpy(cc_filename, mail_path);
                    strcat(cc_filename, &current_date[3]);
                    strcat(cc_filename, cc_sequence);
                    cc_sequence[0]++;
                    strcat(cc_filename, ".OUT");
                    ed_handle = _creat(cc_filename, 0);
                    write(ed_handle, &envelope, sizeof(ENVELOPE));
                    for (row=2; row<count; row++)
                        {
                        i = strlen(edit_line[row]);
                        if (i > 0)
                            write(ed_handle, edit_line[row], i);
                        write(ed_handle, CRLF, 2);
                        }
                    write(ed_handle, CRLF, 2);
                    close(ed_handle);
                    p = strtok(NULL, " ,");
                    }
                which_basket = 0;
                redraw_list = TRUE;
                *outbasket_alarm_ptr = 0l;
                clrscr();
                return;
                }
            else
            if (extended_char == F4)
                {
                if (row == 1 && column < 5)
                    goto editor_keypress;
                if (row == 2 && column < 10)
                    goto editor_keypress;
                for (i=column-1; i<80; i++)
                    edit_line[row-1][i] = '\0';
                clreol();
                }
            else
            if (extended_char == F5)
                {
                if (row < 3)
                    goto editor_keypress;
                for (i=23; i>row-1; i--)
                    strcpy(edit_line[i], edit_line[i-1]);
                memset(edit_line[row-1], 0, 80);
                insline();
                }
            else
            if (extended_char == F10)
                {
                if (row < 3)
                    goto editor_keypress;
                for (i=row-1; i<24; i++)
                    strcpy(edit_line[i], edit_line[i+1]);
                memset(edit_line[23], 0, 80);
                delline();
                }
            else
            if (extended_char == INSKEY)
                {
                insert_mode = ~insert_mode;
                window(1, 1, 80, 25);
                gotoxy(72, 25);
                textcolor(msg_fg);
                textbackground(msg_bg);
                if (insert_mode)
                    cprintf("INSERT");
                else
                    cprintf("      ");
                textcolor(reg_fg);
                textbackground(reg_bg);
                window(1, 1, 80, 24);
                }
            else
            if (extended_char == HOMEKEY)
                {
                if (row == 1)
                    column = 5;
                else
                if (row == 2)
                    column = 10;
                else
                column = 1;
                }
            else
            if (extended_char == ENDKEY)
                {
                column = strlen(edit_line[row-1]) + 1;
                }
            else
            if (extended_char == DELKEY)
                {
                if (row == 1 && column < 5)
                    goto editor_keypress;
                if (row == 2 && column < 10)
                    goto editor_keypress;
                for (i=column-1; i<79; i++)
                    edit_line[row-1][i] = edit_line[row-1][i+1];
                edit_line[row-1][79] = '\0';
                gotoxy(1, row);
                clreol();
                cprintf("%s", edit_line[row-1]);
                }
            else
            if (extended_char == RIGHTKEY)
                {
                if (column < 80)
                    {
                    column++;
                    }
                else
                    {
                    if (row < 24)
                        {
                        column = 1;
                        row++;
                        }
                    }
                }
            else
            if (extended_char == LEFTKEY)
                {
                if (column > 1)
                    {
                    column--;
                    }
                else
                    {
                    if (row > 1)
                        {
                        row--;
                        column = strlen(edit_line[row-1]) + 1;
                        }
                    }
                }
            else
            if (extended_char == UPKEY)
                {
                if (row > 1)
                    row--;
                }
            else
            if (extended_char == DOWNKEY)
                {
                if (row < 24)
                    row++;
                }
            }

        goto editor_keypress;
        }


void    browse(char *filename)
        {
        int     i, j, count, row, browse_handle;
        char    formfeed = 12;
        char    CRLF[3] = {13, 10, 0};
        long    t;

open_browse:
        textcolor(reg_fg);
        textbackground(reg_bg);
        clrscr();

        for (row=0; row<24; row++)
            memset(edit_line[row], 0, 80);
        browse_handle = open(filename, O_RDWR | O_DENYALL);
        if (browse_handle == -1)
            {
            clrscr();
            gotoxy(1, 1);
            cprintf("I/O Error; can't open file.  (Press a key)");
            getkey();
            return;
            }
        i = read(browse_handle, &envelope, sizeof(ENVELOPE));
        if (i != sizeof(ENVELOPE))
            {
            clrscr();
            gotoxy(1, 1);
            cprintf("Error...can't read envelope. (Press a key)");
            getkey();
            return;
            }
        sprintf(edit_line[0],
            "TO: %-16.16s   FROM: %-16.16s   DATE: %-16.16s",
                    envelope.to_name,
                    envelope.sender,
                    envelope.maildate);
        sprintf(edit_line[1], "SUBJECT: %s", envelope.subject);
        textcolor(reg_fg);
        textbackground(reg_bg);
        clrscr();
        gotoxy(1, 1);
        cprintf("%s", edit_line[0]);
        gotoxy(1, 2);
        cprintf("%s", edit_line[1]);
        fbufndx = 2000;
        for (row=2; row<24; row++)
            {
            i = fgetstring(browse_handle, edit_line[row], 80);
            if (i == -1) break;
            gotoxy(1, row + 1);
            cprintf("%s", edit_line[row]);
            }
        if (envelope.read_flag == 'N')
            {
            envelope.read_flag = 'Y';
            lseek(browse_handle, 0l, SEEK_SET);
            write(browse_handle, &envelope, sizeof(ENVELOPE));
            }
        close(browse_handle);

        gotoxy(1, 24);
        if (strlen(envelope.cc_list) > 0)
            cprintf("CC: %s", envelope.cc_list);

        gotoxy(70, 24);
        if (envelope.attachment_flag == 'Y')
            cprintf("ATTACHMENT");

browse_prompt:
        textcolor(msg_fg);
        textbackground(msg_bg);
        gotoxy(2, 25);

        if (which_basket == 0)
            {
            cprintf(
" ESC - Exit    D - Delete Mail     S - Save as file    P - Print   R - Reply  ");
            }
        else
            {
            cprintf(
"     ESC - Exit         C - Change Contents         D - Delete This Mail      ");
            }

        textcolor(reg_fg);
        textbackground(reg_bg);

browse_key:
        gotoxy(80, 25);
        getkey();

        if (key_char == ESC)
            {
            clrscr();
            return;
            }

        if (key_char == 'd' || key_char == 'D')
            {
            unlink(filename);
            clrscr();
            return;
            }

        if (which_basket == 1)
            if (key_char == 'c' || key_char == 'C')
                {
                editor(filename);
                clrscr();
                return;
                }

        if (which_basket == 0)
            {
            if (key_char == 'r' || key_char == 'R')
                {
                strcpy(string, envelope.addressee);
                strcpy(envelope.addressee, envelope.sender);
                strcpy(envelope.sender, string);
                envelope.attachment_flag = 'N';
                memset(envelope.attachment_name, 0, 66);
                time(&t);
                sprintf(string, "%ld", t);
                strcpy(io_buffer, mail_path);
                strcat(io_buffer, &string[3]);
                strcat(io_buffer, ".OUT");
                strcpy(string, io_buffer);
                handle = _creat(string, 0);
                if (handle == -1)
                    {
                    textcolor(msg_fg);
                    textbackground(msg_bg);
                    gotoxy(2, 25);
                    clreol();
                    cprintf(" Error.  Could not create file. ");
                    textcolor(reg_fg);
                    textbackground(reg_bg);
                    getkey();
                    goto open_browse;
                    }
                write(handle, &envelope, sizeof(ENVELOPE));
                close(handle);
                editor(string);
                goto open_browse;
                }
            if (key_char == 's' || key_char == 'S')
                {
                textcolor(reg_fg);
                textbackground(reg_bg);
                memset(filename, 0, 81);
                gotoxy(2, 25);
                clreol();
                cprintf("Save as: ");
                kbdstring(filename, 65);
                textcolor(reg_fg);
                textbackground(reg_bg);
                if (strlen(filename) == 0)
                    goto browse_prompt;
                handle = _creat(filename, 0);
                if (handle == -1)
                    {
                    textcolor(msg_fg);
                    textbackground(msg_bg);
                    gotoxy(2, 25);
                    clreol();
                    cprintf(" Error.  Could not create file. ");
                    textcolor(reg_fg);
                    textbackground(reg_bg);
                    getkey();
                    goto browse_prompt;
                    }
                count = 24;
                for (i=23; i>1; i--)
                    {
                    if (strlen(edit_line[i]) > 0)
                        break;
                    count--;
                    }
                for (i=0; i<count; i++)
                    {
                    j = strlen(edit_line[i]);
                    if (j > 0)
                        write(handle, edit_line[i], j);
                    write(handle, CRLF, 2);
                    }
                write(handle, CRLF, 2);
                close(handle);
                goto browse_prompt;
                }
            if (key_char == 'p' || key_char == 'P')
                {
                handle = open("LPT1", O_WRONLY);
                if (handle == -1)
                    {
                    textcolor(msg_fg);
                    textbackground(msg_bg);
                    gotoxy(2, 25);
                    clreol();
                    cprintf(" Error.  Could not open printer. ");
                    textcolor(reg_fg);
                    textbackground(reg_bg);
                    getkey();
                    goto browse_prompt;
                    }
                count = 24;
                for (i=23; i>1; i--)
                    {
                    if (strlen(edit_line[i]) > 0)
                        break;
                    count--;
                    }
                for (i=0; i<count; i++)
                    {
                    j = strlen(edit_line[i]);
                    if (j > 0)
                        write(handle, edit_line[i], j);
                    write(handle, CRLF, 2);
                    }
                write(handle, CRLF, 2);
                write(handle, &formfeed, 1);
                close(handle);
                goto browse_prompt;
                }
            }

        goto browse_key;
        }


void    show_icon(int x, int y, char *icon[])
        {
        int i;

        for (i=0; i<7; i++)
            {
            gotoxy(x, y++);
            cprintf("%s", icon[i]);
            }
        }


void    actual_popup(void)
        {
        long t;

        de_install = FALSE;
        hotkey_flag= FALSE;

reset_trays:
        redraw_list= TRUE;
        inbasket_choice = 0;
        outbasket_choice = 0;
        top_inbasket = 0;
        top_outbasket = 0;

show_menu:
        textcolor(reg_fg);
        textbackground(reg_bg);
        clrscr();
        gotoxy(2, 1);
        cprintf(
"           ,  (cursor keys) move light bar.  ÄÙ selects item.             ");

        gotoxy(2, 2);
        cprintf(
"ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ");


show_prompts:
        textcolor(reg_fg);
        textbackground(reg_bg);
        gotoxy(2, 24);
        cprintf(
"ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ");
        gotoxy(2, 25);
        cprintf(
" F1-Help    S-Send Mail    I-Inbasket    O-Outbasket    X-Exit     F10-Unload ");


show_send_item:
        show_icon(2, 3, send_icon);
        gotoxy(9, 10);
        cprintf("S)end");

show_list:
        for (i=0, j=12; i<11; i++)
            {
            gotoxy(4, j++);
            cprintf("%s", list_icon[i]);
            }

show_baskets:
        *new_mail_ptr = FALSE;
        check_inbasket();
        check_outbasket();
        textcolor(reg_fg);
        textbackground(reg_bg);

        if (in_count == 0)
            show_icon(25, 3, empty_basket);
        else
        if (in_count == 1)
            show_icon(25, 3, one_letter);
        else
            show_icon(25, 3, multiple_letters);
        gotoxy(28, 10);
        cprintf("I)n (%2d items)", in_count);

        if (out_count == 0)
            show_icon(52, 3, empty_basket);
        else
        if (out_count == 1)
            show_icon(52, 3, one_letter);
        else
            show_icon(52, 3, multiple_letters);
        gotoxy(54, 10);
        cprintf("O)ut (%2d undeliv.)", out_count);

        textcolor(reg_bg);
        textbackground(reg_fg);
        gotoxy(30, 3);
        cprintf("E-Mail for %s", user_id);

        if (which_basket == 0)
            {
            textcolor(reg_fg);
            textbackground(reg_bg);
            gotoxy(2, 23);
            cprintf("%-79.79s", " ");
            gotoxy(6, 13);
            textcolor(reg_bg);
            textbackground(reg_fg);
            cprintf(" %-16.16s   %-30.30s   %-17.17s",
                "FROM", "SUBJECT", "DATE");
            gotoxy(6, 21);
            cprintf("%-25.25s %-44.44s", 
                " ", "INBASKET STATUS");
            window(6, 14, 75, 20);
            textcolor(msg_fg);
            textbackground(msg_bg);
            if (redraw_list) clrscr();
            if (in_count == 0)
                {
                gotoxy(28, 1);
                cprintf("<empty>");
                }
            else
                {
                for (i=0, j=1; i<7; i++)
                    {
                    k = i + top_inbasket;
                    if (k == in_count) break;
                    gotoxy(1, j++);
                    if (k == inbasket_choice)
                        {
                        textcolor(msg_bg);
                        textbackground(msg_fg);
                        }
                    else
                        {
                        textcolor(msg_fg);
                        textbackground(msg_bg);
                        }
                    if (inbasket[k].read_flag == 'Y')
                        cprintf("*");
                    else
                        cprintf(" ");
                    cprintf("%-16.16s   %-30.30s   %-16.16s",
                        inbasket[k].mailname,
                        inbasket[k].subject,
                        inbasket[k].maildate);
                    }
                }
            }
        else
            {
            textcolor(reg_bg);
            textbackground(reg_fg);
            gotoxy(2, 23);
            cprintf(
"   Delivery of outbasket items is deferred while you browse the outbasket.    ");
            gotoxy(6, 13);
            cprintf(" %-16.16s   %-30.30s   %-17.17s",
                "TO", "SUBJECT", "DATE");
            gotoxy(6, 21);
            cprintf("%-25.25s %-44.44s", 
                " ", "OUTBASKET STATUS");
            window(6, 14, 75, 20);
            textcolor(msg_fg);
            textbackground(msg_bg);
            if (redraw_list) clrscr();
            if (out_count == 0)
                {
                gotoxy(28, 1);
                cprintf("<empty>");
                }
            else
                {
                for (i=0, j=1; i<7; i++)
                    {
                    k = i + top_outbasket;
                    if (k == out_count) break;
                    gotoxy(1, j++);
                    if (k == outbasket_choice)
                        {
                        textcolor(msg_bg);
                        textbackground(msg_fg);
                        }
                    else
                        {
                        textcolor(msg_fg);
                        textbackground(msg_bg);
                        }
                    cprintf(" %-16.16s   %-30.30s   %-16.16s",
                        outbasket[k].mailname,
                        outbasket[k].subject,
                        outbasket[k].maildate);
                    }
                }
            }

        textcolor(reg_fg);
        textbackground(reg_bg);
        window(1, 1, 80, 25);
        redraw_list = FALSE;
        gotoxy(80, 25);

get_menu_key:
        getkey();
        if (*new_mail_ptr)
            {
            *new_mail_ptr = FALSE;
            redraw_list   = TRUE;
            goto show_baskets;
            }

        if (key_char == CR)
            {
            if (which_basket == 0 && in_count > 0)
                {
                strcpy(filename, mail_path);
                strcat(filename,
                    inbasket[inbasket_choice].mailfile);
                browse(filename);
                goto reset_trays;
                }
            else
            if (which_basket == 1 && out_count > 0)
                {
                strcpy(filename, mail_path);
                strcat(filename,
                    outbasket[outbasket_choice].mailfile);
                browse(filename);
                goto reset_trays;
                }
            }

        if (key_char == 0 && extended_char == DOWNKEY)
            {
            if (which_basket == 0)
                {
                if (inbasket_choice < in_count - 1)
                    inbasket_choice++;
                if (inbasket_choice > top_inbasket + 6)
                    top_inbasket++;
                }
            else
                {
                if (outbasket_choice < out_count - 1)
                    outbasket_choice++;
                if (outbasket_choice > top_outbasket + 6)
                    top_outbasket++;
                }
            goto show_baskets;
            }

        if (key_char == 0 && extended_char == UPKEY)
            {
            if (which_basket == 0)
                {
                if (inbasket_choice > 0)
                    inbasket_choice--;
                if (inbasket_choice < top_inbasket)
                    top_inbasket--;
                }
            else
                {
                if (outbasket_choice > 0)
                    outbasket_choice--;
                if (outbasket_choice < top_outbasket)
                    top_outbasket--;
                }
            goto show_baskets;
            }

        if (key_char == 's' || key_char == 'S')
            {
            time(&t);
            sprintf(string, "%ld", t);
            strcpy(filename, mail_path);
            strcat(filename, &string[3]);
            strcat(filename, ".OUT");
            editor(filename);
            goto reset_trays;
            }

        if (key_char == 'i' || key_char == 'I')
            {
            if (*outbasket_alarm_ptr == 0x0FFFFFFFl)
                *outbasket_alarm_ptr = 0l;
            which_basket = 0;
            redraw_list  = TRUE;
            goto show_baskets;
            }

        if (key_char == 'o' || key_char == 'O')
            {
            *outbasket_alarm_ptr = 0x0FFFFFFFl;
            which_basket = 1;
            redraw_list  = TRUE;
            goto show_baskets;
            }

        if (key_char == 'x' || key_char == 'X')
            goto exit_popup;

        if (key_char == 0 && extended_char == F1)
            {
            show_help(1);
            redraw_list  = TRUE;
            goto show_menu;
            }

        if (key_char == 0 && extended_char == F10)
            {
            de_install = TRUE;
            goto exit_popup;
            }

        if (key_char == 0 && extended_char == ALT_F10)
            {
            *postman_deinstall_ptr = TRUE;
            de_install = TRUE;
            goto exit_popup;
            }

        goto get_menu_key;

exit_popup:
        hotkey_flag   = FALSE;
        if (*outbasket_alarm_ptr == 0x0FFFFFFFl)
            *outbasket_alarm_ptr = 0l;
        }


void    do_popup(void)
        {
        disable();
        old_ss = _SS;
        old_sp = _SP;
        _SS = our_ss;
        _SP = our_sp;
        enable();

        if (de_install)
            {
            if (!okay_to_unload()) goto do_popup_exit;
            _AX = 0x5000;
            _BX = ourpsp;
            geninterrupt(0x21);
            setvect(0x08, oldint08);
            setvect(0x28, oldint28);
            setvect(0x09, oldint09);
            setvect(0x10, oldint10);
            setvect(0x13, oldint13);
            setvect(0x16, oldint16);
            _ES = ourpsp;
            _BX = 0x2c;
            asm   mov es, es:[bx]
            _AH = 0x49;
            geninterrupt(0x21);
            _ES = ourpsp;
            _AH = 0x49;
            geninterrupt(0x21);
            asm   mov ax, word ptr next_mcb+2
            asm   inc ax
            asm   mov es, ax
            _AH = 0x49;
            geninterrupt(0x21);
            _AX = 0x4c00;
            geninterrupt(0x21);
            }

process_popup:
        curr_vid_mode = get_vid_mode();
        if (curr_vid_mode != 2
            && curr_vid_mode != 3
            && curr_vid_mode != 7)
                {
                beep();
                delay(100);
                beep();
                hotkey_flag = FALSE;
                goto do_popup_exit;
                }

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
        _AX = 0x5100;
        geninterrupt(0x21);
        oldpsp = _BX;
        _AX = 0x5000;
        _BX = ourpsp;
        geninterrupt(0x21);

        save_screen(1,1,80,25, app_screen, 
                           &cursor_save, &cursor_type);
        actual_popup();
        restore_screen(1,1,80,25, app_screen, 
                           &cursor_save, &cursor_type);

        _AX = 0x5000;
        _BX = oldpsp;
        geninterrupt(0x21);
        setdta(olddta_ptr);
        setvect(0x24, oldint24);
        setvect(0x23, oldint23);
        setvect(0x1c, oldint1c);
        setvect(0x1b, oldint1b);
        setcbrk(break_state);

do_popup_exit:
        disable();
        _SS = old_ss;
        _SP = old_sp;
        enable();
        }

/* ------------------------------------------- */

void    main(int argc, char *argv[])
        {
        char far *path_ptr;

        _fmode = O_BINARY;
        if (_osmajor < 2)
            {
            cprintf("\r\n");
            cprintf("Early versions of DOS not supported...\r\n");
            exit(1);
            }

        _AX = 'PO';
        geninterrupt(0x16);
        temp_ax = _AX;
        temp2   = _BX;
        temp1   = _ES;
        temp3   = _SI;
        temp4   = _DX;
        temp5   = _DI;
        if (temp_ax == 'po')
            {
            path_ptr = MK_FP(temp1, temp2);
            for (i=0; i<65; i++)
                {
                mail_path[i] = *path_ptr;
                path_ptr++;
                }
            outbasket_alarm_ptr = MK_FP(temp1, temp3);
            postman_deinstall_ptr = MK_FP(temp1, temp4);
            new_mail_ptr = MK_FP(temp1, temp5);
            }
        else
            {
            cprintf("\r\n");
            cprintf("You must run the PostMan program\r\n");
            cprintf("before you can send/receive mail.\r\n");
            exit(1);
            }

        _AX = 'MA';
        geninterrupt(0x16);
        if (_AX == 'ma')
            {
            cprintf("\r\n");
            cprintf("The E-Mail program was already loaded.\r\n");
            cprintf("Use ALT/RIGHT-SHIFT to activate it.\r\n");
            exit(1);
            }

        if ( (our_stack = malloc(1000)) == NULL)
            {
            cprintf("\r\n");
            cprintf("Insufficient memory...\r\n");
            exit(1);
            }

        curr_vid_mode = get_vid_mode();
        if (curr_vid_mode == 7)
            {
            reg_fg = LIGHTGRAY;
            reg_bg = BLACK;
            msg_fg = WHITE;
            msg_bg = BLACK;
            }

        for (i=0; i<24; i++)
            edit_line[i] = &edit_area[i*80];

        our_ss    = _DS;
        our_sp    = FP_OFF( (void far *) our_stack) + 998;

        _AX = 0x3400;
        geninterrupt(0x21);
        temp2 = _BX;
        temp1 = _ES;
        indos_ptr = MK_FP(temp1, temp2);
        if (_osmajor == 2)
            indos2_ptr = MK_FP(temp1, temp2 + 1);
        else
            indos2_ptr = MK_FP(temp1, temp2 - 1);

        delay(10);
        kbd_flag_ptr   = MK_FP(0x0040, 0x0017);
        cprintf("\r\n\r\n");
        cprintf("E-Mail Program is loaded.\r\n");
        cprintf("Press ALT/RIGHT-SHIFT to activate the program.\r\n\r\n");

        get_user_id(get_connection_number(), user_id);
        user_id[16] = '\0';

        ourdta_ptr = getdta();
        _AX = 0x5100;
        geninterrupt(0x21);
        ourpsp = _BX;
        our_mcb       = MK_FP(ourpsp-1, 0);
        our_mcb_size  = MK_FP(ourpsp-1, 3);
        oldint08 = getvect(0x08);
        oldint09 = getvect(0x09);
        oldint10 = getvect(0x10);
        oldint13 = getvect(0x13);
        oldint16 = getvect(0x16);
        oldint28 = getvect(0x28);

        asm     mov ax, word ptr oldint10
        asm     mov word ptr cs:[0000h], ax
        asm     mov ax, word ptr oldint10+2
        asm     mov word ptr cs:[0002h], ax
        asm     mov ax, word ptr oldint13
        asm     mov word ptr cs:[0004h], ax
        asm     mov ax, word ptr oldint13+2
        asm     mov word ptr cs:[0006h], ax

        setvect(0x10, (void interrupt (*)()) int10);
        setvect(0x13, (void interrupt (*)()) int13);
        setvect(0x16, int16);
        setvect(0x09, int09);
        setvect(0x28, int28);
        setvect(0x08, int08);

        de_install = FALSE;
        hotkey_flag= FALSE;

        paragraphs = (our_ss + (our_sp >> 4) + 1) - ourpsp;
        keep(0, paragraphs);
        }


