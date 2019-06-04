/*      E-Mail  V1.00   (Netware Postman)      */ 
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

void    send_mail(int first_item);
void far ESR_routine(void);
void    process_mail(void);
int     fgetbuf(int fh);
int     fgetstring(int fh, char buff[], int max_chars);
void    beep(void);

#define TRUE         1
#define FALSE        0
#define LINEFEED    10
#define CR          13

#define ACK          1
#define HEADER      10
#define MAIL_DATA   20
#define MAIL_EOF    30
#define FILE_HDR    50
#define FILE_DATA   60
#define FILE_EOF    70
#define TRAILER     99

struct ECB  send_ecb;
struct ECB  receive_ecb;
struct IPXHEADER send_header;
struct IPXHEADER receive_header;

unsigned int  socket;
char    user_id[48];
char    caller[16];
char    caller_network[4];
char    caller_node[6];
char    addressee[16];
char    addressee_network[4];
char    addressee_node[6];
char    subject[31];

typedef struct
        {
        char    type;
        int     sequence;
        int     data_length;
        char    data[500];
        } MAIL_PACKET;

MAIL_PACKET packet_in;
MAIL_PACKET packet_out;

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

ENVELOPE    envelope_in;
ENVELOPE    envelope_out;

int     errors_this_packet = 0;
int     send_handle = -1;
int     mail_handle = -1;
int     file_handle = -1;
int     expected_sequence = 1;
char    state = 0;

unsigned    paragraphs;
char        critical_error = FALSE;
int         i, j, k;
unsigned    temp1, temp2;
unsigned    temp_ax;
unsigned char temp_ah;
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
unsigned char    de_install = FALSE;
unsigned char    mail_flag = FALSE;
unsigned char    new_mail  = FALSE;
unsigned char    incoming_msg = FALSE;
unsigned char    break_flag = FALSE;

unsigned char    trigger_outbasket= FALSE;
unsigned char    incoming_timeout = FALSE;
unsigned char    no_answer        = FALSE;

unsigned char   first_packet = FALSE;
long            tick_counter = 0l;

long            outbasket_alarm = 10920l;
long            incoming_alarm  = 0x0FFFFFFFl;
long            no_answer_alarm = 0x0FFFFFFFl;

char            mail_path[65];
char            io_buffer[512];
char            outname[81];
char            mailname[81];
char            filename[81];
char            string[101];

struct ffblk    find_block;
int             io_len    = 0;
unsigned int    fbufndx   = 2000;
unsigned int    fbufbytes = 0;
unsigned        *our_stack;

/* - - - - - - - - - - - - - - - - - - - - - - - - */

void    interrupt int08(void)
    {
    in_int08 = TRUE;
    oldint08();
    tick_counter++;
    enable();

    if (tick_counter > outbasket_alarm)
        {
        mail_flag = TRUE;
        trigger_outbasket = TRUE;
        }

    if (tick_counter > incoming_alarm)
        {
        mail_flag = TRUE;
        incoming_timeout = TRUE;
        }

    if (tick_counter > no_answer_alarm)
        {
        mail_flag = TRUE;
        no_answer = TRUE;
        }

    if (!de_install && !mail_flag)
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
    process_mail();
    in_popup = FALSE;

exit08:
    in_int08 = FALSE;
    }

void interrupt  int09 (void)
    {
    in_int09 = TRUE;
    oldint09();
    enable();
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

    if (temp_ax == 'PO')
        {
        ax = 'po';
        es = FP_SEG( (void far *) mail_path);
        bx = FP_OFF( (void far *) mail_path);
        si = FP_OFF( (void far *) &outbasket_alarm);
        di = FP_OFF( (void far *) &new_mail);
        dx = FP_OFF( (void far *) &de_install);
        goto int16_exit;
        }

    if (temp_ah != 0)
        goto do_old16;

wait_for_key:
    _AH = 1;
    oldint16();
    asm     jz   check_flags

    goto do_old16;

check_flags:
    if (in_popup)
        oldint28();

    if (!mail_flag && !de_install)
        goto wait_for_key;

    if (*indos_ptr != 0)
         goto wait_for_key;

    if (*indos2_ptr != 0)
         goto wait_for_key;

    outportb(0x20, 0x0b);
    if (inportb(0x20)) goto wait_for_key;

    in_popup = TRUE;
    process_mail();
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

    if (!mail_flag && !de_install)
        goto exit28;

    if (in_popup)
        goto exit28;

    if (*indos_ptr > 1)
         goto exit28;

    if (*indos2_ptr != 0)
         goto exit28;

    outportb(0x20, 0x0b);
    if (inportb(0x20))
         goto exit28;

    in_popup = TRUE;
    process_mail();
    in_popup = FALSE;

exit28:
    in_int28 = FALSE;
    }

/* - - - - - - - - - - - - - - - - - - - - - - - */

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

void    beep(void)
        {
        sound(880);
        delay(100);
        nosound();
        }

/* ----------------------------------------------- */

void    announce_mail(void)
        {
        char    linsav[160];
        int     n, cpos, ctype, vid_mode;

        _AH = 15;
        geninterrupt(0x10);
        _AH = 0;
        vid_mode = _AX;
        if (vid_mode != 2
            && vid_mode != 3
            && vid_mode != 7)
                {
                beep();
                delay(100);
                beep();
                delay(10);
                beep();
                return;
                }

        gettext(1, 25, 80, 25, linsav);
        _AH = 3;
        _BH = 0;
        geninterrupt(0x10);
        cpos  = _DX;
        ctype = _CX;

        gotoxy(1, 25);
        textcolor(BLACK);
        textbackground(LIGHTGRAY);
        cprintf("%-79.79s", "                     You have mail.");
        beep();

        for (n=0; n<20; n++)
            {
            delay(100);
            if (bioskey(1))
                {
                bioskey(0);
                break;
                }
            }

        puttext(1, 25, 80, 25, linsav);
        _DX = cpos;
        _AH = 2;
        _BH = 0;
        geninterrupt(0x10);
        _CX = ctype;
        _AH = 1;
        geninterrupt(0x10);
        }

void    send(char *network, char *node,
             void *packet_ptr, int packet_len)
        {
        int i;

        memset(&send_ecb, 0, sizeof(struct ECB));
        send_ecb.socket_number = socket;
        i = get_local_target(network, 
                             node,
                             socket,
                             send_ecb.immediate_address);
        if (i != 0) return;

        send_ecb.packet_count = 2;
        send_ecb.packet[0].address = &send_header;
        send_ecb.packet[0].length  = sizeof(struct IPXHEADER);
        send_ecb.packet[1].address = packet_ptr;
        send_ecb.packet[1].length  = packet_len;

        send_header.packet_type = 4;
        memcpy(send_header.dest_network_number, network, 4);
        memcpy(send_header.dest_network_node,   node,    6);
        send_header.dest_network_socket = socket;
        ipx_send_packet(&send_ecb);
        }

void    receive(void far (*esr)(), void *packet_ptr, int packet_len)
        {
        memset(&receive_ecb, 0, sizeof(struct ECB));
        receive_ecb.event_service_routine = esr;
        receive_ecb.socket_number = socket;
        receive_ecb.packet_count  = 2;
        receive_ecb.packet[0].address = &receive_header;
        receive_ecb.packet[0].length  = sizeof(struct IPXHEADER);
        receive_ecb.packet[1].address = packet_ptr;
        receive_ecb.packet[1].length  = packet_len;
        ipx_listen_for_packet(&receive_ecb);
        }

void    send_ack(void)
        {
        packet_in.type = ACK;
        packet_in.data_length = 0;
        send(caller_network, caller_node, &packet_in, 5);
        }

void    receive_incoming_mail(void)
        {
        long t;
        char filepart[10];
        char extpart[6];

        incoming_alarm = tick_counter + 1092l;

        switch (packet_in.type)
            {
            case HEADER :
                {
                memcpy(caller_network,
                    receive_header.source_network_number, 4);
                memcpy(caller_node,
                    receive_header.source_network_node,   6);
                state = 1;
                first_packet = TRUE;
                time(&t);
                sprintf(string, "%ld", t);
                strcpy(mailname, mail_path);
                strcat(mailname, &string[3]);
                strcat(mailname, ".IN");
                mail_handle = _creat(mailname, 0);
                if (mail_handle == -1)
                    return;
                send_ack();
                break;
                }
            case MAIL_DATA :
                {
                write(mail_handle, packet_in.data,
                        packet_in.data_length);
                if (first_packet)
                    {
                    memcpy(&envelope_in,
                           &packet_in.data, sizeof(ENVELOPE));
                    first_packet = FALSE;
                    }
                send_ack();
                break;
                }
            case MAIL_EOF :
                {
                close(mail_handle);
                mail_handle = -1;
                send_ack();
                break;
                }
            case FILE_HDR :
                {
                fnsplit(envelope_in.attachment_name,
                        NULL, NULL, filepart, extpart);
                strcpy(filename, mail_path);
                strcat(filename, filepart);
                strcat(filename, extpart);
                file_handle = _creat(filename, 0);
                if (file_handle != -1)
                    send_ack();
                break;
                }
            case FILE_DATA :
                {
                write(file_handle, packet_in.data,
                        packet_in.data_length);
                send_ack();
                break;
                }
            case FILE_EOF :
                {
                close(file_handle);
                file_handle = -1;
                send_ack();
                break;
                }
            case TRAILER :
                {
                new_mail = TRUE;
                incoming_alarm = 0x0FFFFFFFl;
                state = 0;
                expected_sequence = 0;
                send_ack();
                announce_mail();
                break;
                }
            default :
                {
                return;
                }
            };

        expected_sequence++;
        }

void    cancel_incoming_mail(void)
        {
        if (mail_handle != -1)
            {
            close(mail_handle);
            unlink(mailname);
            mail_handle = -1;
            }
        if (file_handle != -1)
            {
            close(file_handle);
            unlink(filename);
            file_handle = -1;
            }
        state = 0;
        expected_sequence = 1;
        }

void    send_next_packet(void)
        {
        int rc, retry_count;

        no_answer_alarm = tick_counter + 91l;
        errors_this_packet = 0;
        packet_out.sequence++;

        if (packet_out.type == TRAILER)
            {
            packet_out.type = HEADER;
            send_mail(0);
            return;
            }

        if (packet_out.type == FILE_EOF)
            {
            retry_count = 0;
            do  rc = unlink(outname);
                while (rc != 0 && ++retry_count < 10);
            packet_out.type = TRAILER;
            send(addressee_network, addressee_node, &packet_out, 5);
            return;
            }

        if (packet_out.type == MAIL_EOF)
            {
            if (envelope_out.attachment_flag == 'N'
                || strcmp(envelope_out.addressee, 
                        envelope_out.sender) == 0)
                {
                retry_count = 0;
                do  rc = unlink(outname);
                    while (rc != 0 && ++retry_count < 10);
                packet_out.type = TRAILER;
                send(addressee_network, addressee_node, &packet_out, 5);
                return;
                }
            send_handle = 
                    open(envelope_out.attachment_name,
                    O_RDWR | O_DENYALL);
            if (send_handle == -1)
                {
                retry_count = 0;
                do  rc = unlink(outname);
                    while (rc != 0 && ++retry_count < 10);
                packet_out.type = TRAILER;
                send(addressee_network, addressee_node, &packet_out, 5);
                return;
                }
            packet_out.type = FILE_HDR;
            packet_out.data_length = 0;
            send(addressee_network, addressee_node, &packet_out, 5);
            return;
            }

        if (packet_out.type == FILE_DATA
            || packet_out.type == FILE_HDR)
            {
            packet_out.type = FILE_DATA;
            packet_out.data_length 
                = read(send_handle, packet_out.data, 500);
            if (packet_out.data_length == 0)
                {
                close(send_handle);
                send_handle = -1;
                packet_out.type = FILE_EOF;
                }
            send(addressee_network, addressee_node, 
                    &packet_out, packet_out.data_length+5);
            return;
            }

        if (packet_out.type == HEADER)
            {
            send_handle = open(outname, O_RDWR | O_DENYALL);
            if (send_handle == -1)
                return;
            }

        packet_out.type = MAIL_DATA;
        packet_out.data_length 
            = read(send_handle, packet_out.data, 500);
        if (packet_out.data_length == 0)
            {
            close(send_handle);
            send_handle = -1;
            packet_out.type = MAIL_EOF;
            }

        send(addressee_network, addressee_node, 
                &packet_out, packet_out.data_length+5);
        }

void    send_mail(int first_item)
        {
        int i, flag;

        if (!first_item && packet_out.type != HEADER)
            {
            if (++errors_this_packet < 10)
                {
                outbasket_alarm = tick_counter + 10920l;
                no_answer_alarm = tick_counter + 91l;
                send(addressee_network, addressee_node, 
                        &packet_out, packet_out.data_length+5);
                return;
                }
            if (send_handle != -1)
                close(send_handle);
            }

        if (first_item)
            {
            strcpy(outname, mail_path);
            strcat(outname, "*.out");
            flag = findfirst(outname, &find_block, 0);
            }
        else
            flag = findnext(&find_block);

        if (flag != 0)
            {
            no_answer_alarm = 0x0FFFFFFFl;
            return;
            }

        outbasket_alarm = tick_counter + 10920l;
        no_answer_alarm = tick_counter + 91l;
        errors_this_packet = 0;

        packet_out.type = HEADER;
        packet_out.sequence = 1;
        packet_out.data_length = 0;

        strcpy(outname, mail_path);
        strcat(outname, find_block.ff_name);
        send_handle = open(outname, O_RDWR | O_DENYALL);
        if (send_handle == -1)
            return;
        i = read(send_handle, &envelope_out, sizeof(ENVELOPE));
        close(send_handle);
        send_handle = -1;
        if (i != sizeof(ENVELOPE))
            return;
        strcpy(addressee, envelope_out.addressee);

        i = get_1st_connection_num(addressee);
        if (i == 0) return;
        i = get_internet_address(i, 
                addressee_network, addressee_node);
        if (i != 0) return;

        send(addressee_network, addressee_node, &packet_out, 5);
        }

void    post_office(void)
        {
        mail_flag = FALSE;

        if (incoming_msg)
            {
            incoming_msg = FALSE;
            if (receive_ecb.completion_code != 0)
                {
                gotoxy(50, 1);
                cprintf("Mail error!  code = %d.",
                    (int) receive_ecb.completion_code);
                beep();
                gotoxy(1,1);
                cancel_incoming_mail();
                goto post_office_exit;
                }
            receive(ESR_routine,
                    &packet_in,
                    sizeof(MAIL_PACKET));
            if (packet_in.type == ACK)
                send_next_packet();
            else
            if (packet_in.type == HEADER && state != 0)
                goto post_office_exit;
            else
            if (packet_in.sequence == expected_sequence)
                receive_incoming_mail();
            else
            if (packet_in.sequence == expected_sequence - 1)
                send_ack();
            else
                cancel_incoming_mail();

            goto post_office_exit;
            }

        if (incoming_timeout)
            {
            incoming_timeout = FALSE;
            incoming_alarm   = 0x0FFFFFFFl;
            cancel_incoming_mail();
            goto post_office_exit;
            }

        if (no_answer)
            {
            no_answer = FALSE;
            send_mail(0);
            }

        if (trigger_outbasket)
            {
            trigger_outbasket = FALSE;
            outbasket_alarm   = tick_counter + 10920l;
            send_mail(1);
            }

post_office_exit:
        ;
        }

void    process_mail(void)
        {
        disable();
        old_ss = _SS;
        old_sp = _SP;
        _SS = our_ss;
        _SP = our_sp;
        enable();

        if (de_install)
            {
            if (!okay_to_unload())
                goto process_mail_exit;
            _AX = 0x5000;
            _BX = ourpsp;
            geninterrupt(0x21);
            setvect(0x08, oldint08);
            setvect(0x28, oldint28);
            setvect(0x09, oldint09);
            setvect(0x10, oldint10);
            setvect(0x13, oldint13);
            setvect(0x16, oldint16);
            ipx_cancel_event(&receive_ecb);
            close_socket(socket);
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

        post_office();

        _AX = 0x5000;
        _BX = oldpsp;
        geninterrupt(0x21);
        setdta(olddta_ptr);
        setvect(0x24, oldint24);
        setvect(0x23, oldint23);
        setvect(0x1c, oldint1c);
        setvect(0x1b, oldint1b);
        setcbrk(break_state);

process_mail_exit:
        disable();
        _SS = old_ss;
        _SP = old_sp;
        enable();
        }

void    far ESR_routine(void)
        {
        _AX = _ES;
        _DS = _AX;

        incoming_msg = TRUE;
        mail_flag   = TRUE;
        }

/* ------------------------------------------- */

void    main(int argc, char *argv[])
        {
        _fmode = O_BINARY;

        if (_osmajor < 2)
            {
            cprintf("\r\n");
            cprintf("Early versions of DOS not supported...\r\n");
            exit(1);
            }

        _AX = 'PO';
        geninterrupt(0x16);
        if (_AX == 'po')
            {
            cprintf("\r\n");
            cprintf("The PostMan program was already loaded.\r\n");
            exit(1);
            }

        if ( (our_stack = malloc(1000)) == NULL)
            {
            cprintf("\r\n");
            cprintf("Insufficient memory...\r\n");
            exit(1);
            }

        getcwd(mail_path, 64);
        if (mail_path[strlen(mail_path) - 1] != '\\')
            strcat(mail_path, "\\");

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

        if (ipx_spx_installed() == -1)
            {
            cprintf("\r\n");
            cprintf("The network isn't active.\r\n");
            exit(1);
            }

        get_user_id(get_connection_number(), user_id);
        user_id[16] = '\0';

        socket = 0x6767;
        i = open_socket(socket);
        if (i != 0)
            {
            cprintf("\r\n");
            cprintf("Unable to open a socket.\r\n");
            exit(1);
            }

        cprintf("\r\n\r\n");
        cprintf("PostMan is loaded.\r\n");
        cprintf("Run the 'MAIL' program to see your IN/OUT baskets.\r\n");

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

        receive(ESR_routine,
                &packet_in,
                sizeof(MAIL_PACKET));

        paragraphs = (our_ss + (our_sp >> 4) + 1) - ourpsp;
        keep(0, paragraphs);
        }


