
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <mem.h>
#include <bios.h>
#include <conio.h>
#include <string.h>
#include <process.h>
#include <netbios.h>

#define  MSG_SIZE  1001
#ifdef __cplusplus
    #define __CPPARGS ...
#else
    #define __CPPARGS
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - */

char            message_out[MSG_SIZE];
char            message_in [MSG_SIZE];
char            string     [MSG_SIZE];
char            string2    [MSG_SIZE];
char            localname  [17];
char            remotename [17];
unsigned char   lsn;
unsigned char   name_number;
int             choice;
int             x;
int             cursor_x;
int             cursor_y;

/* - - - - - - - - - - - - - - - - - - - - - - - */

unsigned            ncb_segments[10];
unsigned            ncb_offsets [10];
unsigned volatile   ncb_head  = 0;
unsigned            ncb_tail  = 0;

/* - - - - - - - - - - - - - - - - - - - - - - - */

NCB     reset_ncb;
NCB     adapter_status_ncb;
NCB     session_status_ncb;
NCB     call_ncb;
NCB     listen_ncb;
NCB     add_name_ncb;
NCB     add_group_name_ncb;
NCB     delete_name_ncb;
NCB     send_ncb;
NCB     receive_ncb;
NCB     receive_any_ncb;
NCB     hangup_ncb;
NCB     temp_ncb;

/* - - - - - - - - - - - - - - - - - - - - - - - */

typedef struct {
	unsigned char   card_id[6];
	unsigned char   release_level;
	unsigned char   reserved1;
	unsigned char   type_of_adapter;
	unsigned char   old_or_new_parameters;
        unsigned int    reporting_period_minutes;

        unsigned int    frame_reject_recvd_count;
        unsigned int    frame_reject_sent_count;
        unsigned int    recvd_I_frame_errors;

        unsigned int    unsuccessful_transmissions;
        unsigned long   good_transmissions;
        unsigned long   good_receptions;
        unsigned int    retransmissions;
        unsigned int    exhausted_resource_count;
	unsigned int    t1_timer_expired_count;
        unsigned int    ti_timer_expired_count;
        char            reserved2[4];
        unsigned int    available_ncbs;
        unsigned int    max_ncbs_configured;
        unsigned int    max_ncbs_possible;
        unsigned int    buffer_or_station_busy_count;
	unsigned int    max_datagram_size;
	unsigned int    pending_sessions;
        unsigned int    max_sessions_configured;
        unsigned int    max_sessions_possible;
        unsigned int    max_frame_size;
        int             name_count;
        struct {
            char            tbl_name[16];
            unsigned char   tbl_name_number;
            unsigned char   tbl_name_status;
            } name_table[20];
        }
        ADAPTER_DATA;

typedef struct {
         unsigned char  lsn;
         unsigned char  state;
         char           local_name[16];
         char           remote_name[16];
         unsigned char  recv_count;
	 unsigned char  send_count;
	 }
        A_SESSION;

typedef struct {
        unsigned char  name_num;
        unsigned char  session_count;
        unsigned char  junk1;
        unsigned char  junk2;
        A_SESSION      session_data[40];
        }
        STATUS_INFO;

ADAPTER_DATA    adapter_data;
STATUS_INFO     session_info;

/* - - - - - - - - - - - - - - - - - - - - - - - */

unsigned        es_reg, bx_reg;
void interrupt  (*int_5C_vector)(__CPPARGS);
char            screen_save[4000];

char    *state_msg [7] = {
        "                 ",
        "LISTEN pending",
        "CALL pending",
        "Active session",
        "HANG UP pending",
        "HANG UP complete",
        "Session abort"
        };

int     name_type;
char    *name_type_msg [2] = {
        "Unique name",
        "Group name "
        };

int     name_status;
char    *name_status_msg [8] = {
	"Reg. in progress   ",
	"                   ",
        "                   ",
        "                   ",
        "Registered         ",
        "De-registered      ",
        "Dupl detected      ",
        "Dupl; dereg pend.  "
        };

unsigned char    *box[18] = {
   "Ú[NetTest POST results]ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿",
   "³                                                                 ³",
   "³                                                                 ³",
   "³                                                                 ³",
   "³                                                                 ³",
   "³                                                                 ³",
   "³                                                                 ³",
   "³                                                                 ³",
   "³                                                                 ³",
   "³                                                                 ³",
   "³                                                                 ³",
   "³                                                                 ³",
   "³                                                                 ³",
   "³                                                                 ³",
   "³                                                                 ³",
   "³                                                                 ³",
   "³                                                                 ³",
   "ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ"
        };


/* - - - - - - - - - - - - - - - - - - - - - - - */

void    NetBios(NCB far *ncb_ptr)
        {
        _ES    = FP_SEG(ncb_ptr);
        _BX    = FP_OFF(ncb_ptr);
        _AX    = 0x0100;
	geninterrupt(0x5c);
	}

/* - - - - - - - - - - - - - - - - - - - - - - - */

void    expand_to_16_chars(char *name)
        {
        char *p;
        char tmp[17];
        int  i;

        memset(tmp, ' ', 15);
        p = name;
        i = 0;
	while (i < 15 && *p)
            {
            tmp[i] = *p;
            i++;
            p++;
            }
        tmp[15] = '\0';
	strcpy(name, tmp);
	}

/* - - - - - - - - - - - - - - - - - - - - - - - */

void    show_traffic_and_errors(void)
        {
        gotoxy(1, 6);
        cprintf("   Release level: %2.2X ", 
                    (int) adapter_data.release_level);
        gotoxy(1, 8);
        cprintf("    Adapter type: %2.2X ", 
                    (int) adapter_data.type_of_adapter);
	gotoxy(1, 10);
        cprintf("   Old/new parms: %2.2X ", 
                    (int) adapter_data.old_or_new_parameters);
        gotoxy(15, 16);
        cprintf("(Press a key)");
        bioskey(0);
        clrscr();

	gotoxy(1, 6);
        cprintf("   Reporting period (mins): %6u ", 
                    adapter_data.reporting_period_minutes);
        gotoxy(1, 8);
        cprintf(" Frame rejections received: %6u ", 
                    adapter_data.frame_reject_recvd_count);
        gotoxy(1, 10);
        cprintf("     Frame rejections sent: %6u ", 
                    adapter_data.frame_reject_sent_count);
        gotoxy(1, 12);
        cprintf("   Received I-frame errors: %6u ", 
                    adapter_data.recvd_I_frame_errors);
	gotoxy(1, 14);
        cprintf("Unsuccessful transmissions: %6u ", 
                    adapter_data.unsuccessful_transmissions);
        gotoxy(15, 16);
        cprintf("(Press a key)");
        bioskey(0);
        clrscr();

	gotoxy(1, 6);
        cprintf("Good transmissions: %10lu ", 
                    adapter_data.good_transmissions);
        gotoxy(1, 8);
        cprintf("   Good receptions: %10lu ", 
                    adapter_data.good_receptions);
        gotoxy(15, 16);
        cprintf("(Press a key)");
        bioskey(0);
        clrscr();

        gotoxy(1, 6);
	cprintf("        Retransmissions: %6u ",
                    adapter_data.retransmissions);
        gotoxy(1, 7);
        cprintf("    Exhausted resources: %6u ", 
                    adapter_data.exhausted_resource_count);
        gotoxy(1, 8);
        cprintf("        Available NCB's: %6u ", 
		    adapter_data.available_ncbs);
	gotoxy(1, 9);
        cprintf("   Max configured NCB's: %6u ", 
                    adapter_data.max_ncbs_configured);
        gotoxy(1, 10);
        cprintf("     Max possible NCB's: %6u ", 
                    adapter_data.max_ncbs_possible);
        gotoxy(1, 11);
        cprintf("       Pending sessions: %6u ", 
                    adapter_data.pending_sessions);
        gotoxy(1, 12);
        cprintf("Max configured sessions: %6u ", 
                    adapter_data.max_sessions_configured);
	gotoxy(1, 13);
        cprintf("  Max possible sessions: %6u ", 
                    adapter_data.max_sessions_possible);
        gotoxy(1, 14);
        cprintf("         Max frame size: %6u ", 
                    adapter_data.max_frame_size);
        gotoxy(15, 16);
	cprintf("(Press a key)");
	bioskey(0);
        clrscr();
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */

void    report_result(void)
        {
        int i, j;

        gettext(1, 1, 80, 25, screen_save);
        cursor_x = wherex();
	cursor_y = wherey();
        for (i=0; i<18; i++)
            {
            gotoxy(5, i+1);
            cprintf("%s", box[i]);
            }
        window(6, 2, 70, 17);

	movedata(ncb_segments[ncb_tail], ncb_offsets[ncb_tail],
                 FP_SEG(&temp_ncb), FP_OFF(&temp_ncb),
                 sizeof(NCB));
        ncb_tail++;
        if (ncb_tail == 10)
            ncb_tail = 0;

        if (temp_ncb.NCB_LENGTH == MSG_SIZE - 1)
            temp_ncb.NCB_LENGTH = 0;

        if (temp_ncb.NCB_LENGTH >  320)
            temp_ncb.NCB_LENGTH = 320;

        switch (temp_ncb.NCB_COMMAND)
            {
            case RESET         : {strcpy(string, "RESET ADAPTER"); break;}
            case STATUS        : {strcpy(string, "ADAPTER STATUS");break;}
            case ADD_NAME      : {strcpy(string, "ADD NAME");      break;}
            case ADD_GROUP_NAME: {strcpy(string, "ADD GRP NAME");  break;}
	    case DELETE_NAME   : {strcpy(string, "DELETE NAME");   break;}
	    case LISTEN        : {strcpy(string, "LISTEN");        break;}
            case CALL          : {strcpy(string, "CALL");          break;}
            case HANG_UP       : {strcpy(string, "HANG UP");       break;}
            case SEND          : {strcpy(string, "SEND");          break;}
            case RECEIVE       : {strcpy(string, "RECEIVE");       break;}
            case RECEIVE_ANY   : {strcpy(string, "RECEIVE ANY");   break;}
            case SESSION_STATUS: {strcpy(string, "SESSION STATUS");break;}
            default            : {strcpy(string, "<unknown>");     break;}
            }

        gotoxy(1, 1);
        cprintf("Command: %s", string);

        if (temp_ncb.NCB_RETCODE <= 0x26)
            strcpy(string, net_error_message[temp_ncb.NCB_RETCODE]);
        else
        if (temp_ncb.NCB_RETCODE == 0xff)
            strcpy(string, "command pending");
        else
	    strcpy(string, "adapter malfunction");

        if (temp_ncb.NCB_CMD_CPLT <= 0x26)
            strcpy(string2, net_error_message[temp_ncb.NCB_CMD_CPLT]);
        else
        if (temp_ncb.NCB_CMD_CPLT == 0xff)
            strcpy(string2, "command pending");
        else
            strcpy(string2, "adapter malfunction");

        gotoxy(1, 2);
        cprintf("Immed: %s.  Final: %s.", string, string2);

	if (temp_ncb.NCB_RETCODE > 0 || temp_ncb.NCB_CMD_CPLT > 0)
            goto report_exit;

	if (temp_ncb.NCB_COMMAND == STATUS)
            {
	    gotoxy(1, 4);
	    cprintf("Card ID (hex): ");
            for (i=0; i<6; i++)
                cprintf("%2.2X ", (int) adapter_data.card_id[i]);
            show_traffic_and_errors();
            gotoxy(1, 14);
            cprintf("Local name table items: %d", adapter_data.name_count);
            gotoxy(15, 16);
            cprintf("(Press a key)");
            bioskey(0);
            if (adapter_data.name_count == 0) goto report_exit;
            clrscr();
            for (i=0, j=2; i<adapter_data.name_count; i++)
		{
                adapter_data.name_table[i].tbl_name[15] = '\0';
                name_status = (int) adapter_data.name_table[i].tbl_name_status;
                name_status &= 0x0007;
                name_type = (int) adapter_data.name_table[i].tbl_name_status;
		name_type &= 0x0080;
                name_type = (name_type == 0x0080)? 1 : 0;
		if (i > 9)
		    {
                    gotoxy(15, 16);
                    cprintf("(Press a key)");
                    bioskey(0);
                    clrscr();
                    j = 2;
                    }
                gotoxy(1, j++);
                cprintf("%s  (#%d)  %s  %s",
                    adapter_data.name_table[i].tbl_name,
                    (int) adapter_data.name_table[i].tbl_name_number,
                    name_status_msg [name_status],
		    name_type_msg   [name_type]);
                }
            }
        else
        if (temp_ncb.NCB_COMMAND == ADD_NAME
	  || temp_ncb.NCB_COMMAND == ADD_GROUP_NAME)
            {
	    gotoxy(1, 5);
	    cprintf("Name number = %d.", (int) temp_ncb.NCB_NUM);
            }
        else
        if (temp_ncb.NCB_COMMAND == CALL)
            {
            gotoxy(1, 5);
            cprintf("Session established.  LSN = %d.", (int) temp_ncb.NCB_LSN);
            }
        else
        if (temp_ncb.NCB_COMMAND == LISTEN)
            {
            gotoxy(1, 5);
	    strncpy(string, temp_ncb.NCB_CALLNAME, 16);
            string[16] = '\0';
            cprintf("Session established with '%s'.  LSN = %d.",
                     string, (int) temp_ncb.NCB_LSN);
            }
	else
        if (temp_ncb.NCB_COMMAND == RECEIVE 
	  || temp_ncb.NCB_COMMAND == RECEIVE_ANY)
			{
            gotoxy(1, 5);
            cprintf("Message says: ");
            for (i=0, j=6; i<temp_ncb.NCB_LENGTH; i++)
                {
                if (i % 60 == 0)
                    gotoxy(1, j++);
                cprintf("%c", message_in[i]);
                }
            }
        else
        if (temp_ncb.NCB_COMMAND == SESSION_STATUS)
	    {
            gotoxy(1, 4);
            cprintf("Names: %d   sessions: %d",
                (int) session_info.name_num, (int) session_info.session_count);
            if (session_info.session_count == 0) goto report_exit;
	    j = 5;
            for (i=0; i<session_info.session_count; i++)
                {
		if (i > 5)
                    {
                    gotoxy(15, 16);
                    cprintf("(Press a key)");
                    bioskey(0);
                    clrscr();
                    j = 5;
                    }
                gotoxy(1, j++);
                cprintf("lsn = %d  (%s)  recvcount = %d  sendcount = %d",
                      (int) session_info.session_data[i].lsn,
                      state_msg[(int) session_info.session_data[i].state],
		      (int) session_info.session_data[i].recv_count,
                      (int) session_info.session_data[i].send_count );
                gotoxy(1, j++);
                session_info.session_data[i].local_name[15] = '\0';
                session_info.session_data[i].remote_name[15] = '\0';
		cprintf("localname = '%s'   remotename = '%s'",
                        session_info.session_data[i].local_name,
                        session_info.session_data[i].remote_name);
		}
            }
        else
        if (temp_ncb.NCB_COMMAND == HANG_UP)
            {
            gotoxy(1, 5);
            cprintf("Session closed.");
            }

report_exit:
        gotoxy(15, 16);
        cprintf("(Press a key)");
	bioskey(0);
        window(1, 1, 80, 25);
        gotoxy(cursor_x, cursor_y);
        puttext(1, 1, 80, 25, screen_save);
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */

void interrupt     post(void)
        {
        es_reg  = _ES;
        bx_reg  = _BX;

        ncb_segments[ncb_head] = es_reg;
        ncb_offsets [ncb_head] = bx_reg;
        ncb_head++;

        if (ncb_head == 10)
            ncb_head = 0;
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */

void    main(void)      //int argc, char *argv[]
	{
	int_5C_vector = getvect(0x5C);
	if (int_5C_vector == (void far *) NULL)
	    {
	    printf("NetBios not loaded (Int5C not present).\n");
	    exit(0);
	    }

	memset(&temp_ncb, 0, sizeof(NCB));
	temp_ncb.NCB_COMMAND = 0x7F;
	NetBios(&temp_ncb);
	if (temp_ncb.NCB_RETCODE != 0x03)
	    {
	    printf("NetBios not loaded (No response from Int5C).\n");
//	    exit(0);
	    }

show_menu:
	clrscr();
	choice = 0;

	printf("NET-TEST Menu:\n\n");
	printf(" 0...exit\n");
	printf(" 1...reset adapter\n");
	printf(" 2...adapter status\n");
	printf(" 3...add name\n");
	printf(" 4...add group name\n");
	printf(" 5...delete name\n");
	printf(" 6...call\n");
	printf(" 7...listen\n");
	printf(" 8...send\n");
	printf(" 9...receive\n");
	printf("10...receive any\n");
	printf("11...hang up\n");
	printf("12...session status\n\n");
	printf("    Choice? ");

get_choice:
	if (ncb_head != ncb_tail)
	    report_result();

	if (!bioskey(1))
	    goto get_choice;

	gets(string);
	printf("\n");
	if (strlen(string) == 0) goto show_menu;

	choice = atoi(string);
	if (choice < 0 || choice > 12) goto show_menu;

	switch (choice)
	    {
	    case 0 :{printf("Exiting.\n"); exit(0);}
	    case 1 :{
		    printf("Are you sure (Y/N)? ");
		    gets(string);
		    strlwr(string);
		    if (string[0] != 'y') break;
		    printf("Resetting adapter...\n");
		    memset(&reset_ncb, 0, sizeof(NCB));
		    reset_ncb.NCB_COMMAND = RESET;
		    NetBios(&reset_ncb);
		    printf("Return Code = %d.   (press a key)",
			(int) reset_ncb.NCB_RETCODE);
		    bioskey(0);
		    break;
		    }
	    case 2 :{
		    printf("GET STATUS -- Enter name of adapter: ");
		    gets(remotename);
		    if (strlen(remotename) == 0) break;
		    expand_to_16_chars(remotename);
		    memset(&adapter_status_ncb, 0, sizeof(NCB));
		    adapter_status_ncb.NCB_COMMAND = STATUS;
		    adapter_status_ncb.POST_FUNC = post;
		    strcpy(adapter_status_ncb.NCB_CALLNAME, remotename);
		    adapter_status_ncb.NCB_BUFFER_PTR = &adapter_data;
		    adapter_status_ncb.NCB_LENGTH  = sizeof(ADAPTER_DATA);
		    memset(&adapter_data, 0, sizeof(ADAPTER_DATA));
		    NetBios(&adapter_status_ncb);
		    break;
		    }
	    case 3 :{
		    printf("ADD NAME -- Enter name: ");
		    gets(localname);
		    if (strlen(localname) == 0) break;
		    expand_to_16_chars(localname);
		    memset(&add_name_ncb, 0, sizeof(NCB));
		    add_name_ncb.NCB_COMMAND = ADD_NAME;
		    strcpy(add_name_ncb.NCB_NAME, localname);
		    add_name_ncb.POST_FUNC = post;
		    NetBios(&add_name_ncb);
		    break;
		    }
	    case 4 :{
		    printf("ADD GROUP NAME -- Enter name: ");
		    gets(localname);
		    if (strlen(localname) == 0) break;
		    expand_to_16_chars(localname);
		    memset(&add_group_name_ncb, 0, sizeof(NCB));
		    add_group_name_ncb.NCB_COMMAND = ADD_GROUP_NAME;
		    strcpy(add_group_name_ncb.NCB_NAME, localname);
		    add_group_name_ncb.POST_FUNC = post;
		    NetBios(&add_group_name_ncb);
		    break;
		    }
	    case 5 :{
		    printf("DELETE NAME -- Enter name: ");
		    gets(localname);
		    if (strlen(localname) == 0) break;
		    expand_to_16_chars(localname);
		    memset(&delete_name_ncb, 0, sizeof(NCB));
		    delete_name_ncb.NCB_COMMAND = DELETE_NAME;
		    strcpy(delete_name_ncb.NCB_NAME, localname);
		    delete_name_ncb.POST_FUNC = post;
		    NetBios(&delete_name_ncb);
		    break;
		    }
	    case 6 :{
		    printf("CALL -- Enter remote name to call: ");
		    gets(remotename);
		    if (strlen(remotename) == 0) break;
		    printf("       Enter local (calling) name: ");
		    gets(localname);
		    if (strlen(localname) == 0) break;
		    expand_to_16_chars(remotename);
                    expand_to_16_chars(localname);
                    memset(&call_ncb, 0, sizeof(NCB));
                    call_ncb.NCB_COMMAND = CALL;
                    strcpy(call_ncb.NCB_NAME,     localname);
                    strcpy(call_ncb.NCB_CALLNAME, remotename);
                    call_ncb.NCB_RTO = 0;
                    call_ncb.NCB_STO = 0;
                    call_ncb.POST_FUNC = post;
                    NetBios(&call_ncb);
                    break;
                    }
            case 7 :{
                    printf("LISTEN -- Enter remote name to listen for: ");
                    gets(remotename);
                    if (strlen(remotename) == 0) break;
                    printf("             Enter local (listening) name: ");
                    gets(localname);
		    if (strlen(localname) == 0) break;
                    expand_to_16_chars(remotename);
		    expand_to_16_chars(localname);
		    memset(&listen_ncb, 0, sizeof(NCB));
		    listen_ncb.NCB_COMMAND = LISTEN;
		    strcpy(listen_ncb.NCB_NAME,     localname);
		    strcpy(listen_ncb.NCB_CALLNAME, remotename);
		    listen_ncb.NCB_RTO = 60;
		    listen_ncb.NCB_STO = 0;
		    listen_ncb.POST_FUNC = post;
		    NetBios(&listen_ncb);
		    break;
                    }
            case 8 :{
                    printf("SEND -- Enter message to be sent: ");
                    gets(message_out);
                    if (strlen(message_out) == 0) break;
                    printf("            Enter session number: ");
                    gets(string);
                    if (strlen(string) == 0) break;
		    x = atoi(string);
                    if (x < 1 || x > 254) break;
		    lsn = (unsigned char) x;
                    memset(&send_ncb, 0, sizeof(NCB));
                    send_ncb.NCB_COMMAND = SEND;
                    send_ncb.NCB_LSN     = lsn;
                    send_ncb.NCB_LENGTH  = strlen(message_out) + 1;
                    send_ncb.NCB_BUFFER_PTR  = message_out;
                    send_ncb.POST_FUNC = post;
                    NetBios(&send_ncb);
                    break;
                    }
            case 9 :{
                    printf("RECEIVE -- Enter session number: ");
                    gets(string);
                    if (strlen(string) == 0) break;
                    x = atoi(string);
                    if (x < 1 || x > 254) break;
                    lsn = (unsigned char) x;
                    memset(&receive_ncb, 0, sizeof(NCB));
		    receive_ncb.NCB_COMMAND = RECEIVE;
                    receive_ncb.NCB_LSN     = lsn;
		    receive_ncb.NCB_LENGTH  = MSG_SIZE - 1;
                    receive_ncb.NCB_BUFFER_PTR = message_in;
                    receive_ncb.POST_FUNC = post;
                    NetBios(&receive_ncb);
                    break;
                    }
            case 10:{
                    printf("RECEIVE ANY -- Enter name number: ");
                    gets(string);
                    if (strlen(string) == 0) break;
                    x = atoi(string);
                    if (x < 1 || x > 255) break;
                    name_number = (unsigned char) x;
                    memset(&receive_any_ncb, 0, sizeof(NCB));
                    receive_any_ncb.NCB_COMMAND = RECEIVE_ANY;
                    receive_any_ncb.NCB_NUM     = name_number;
                    receive_any_ncb.NCB_LENGTH  = MSG_SIZE - 1;
                    receive_any_ncb.NCB_BUFFER_PTR = message_in;
		    receive_any_ncb.POST_FUNC = post;
                    NetBios(&receive_any_ncb);
		    break;
                    }
            case 11:{
                    printf("HANG UP -- Enter session number: ");
                    gets(string);
                    if (strlen(string) == 0) break;
                    x = atoi(string);
                    if (x < 1 || x > 254) break;
                    lsn = (unsigned char) x;
                    memset(&hangup_ncb, 0, sizeof(NCB));
                    hangup_ncb.NCB_COMMAND = HANG_UP;
                    hangup_ncb.NCB_LSN     = lsn;
                    hangup_ncb.POST_FUNC   = post;
                    NetBios(&hangup_ncb);
                    break;
                    }
            case 12:{
                    printf("GET SESSION STATUS -- Enter name:");
		    gets(localname);
                    if (strlen(localname) == 0) break;
		    expand_to_16_chars(localname);
                    memset(&session_status_ncb, 0, sizeof(NCB));
                    session_status_ncb.NCB_COMMAND = SESSION_STATUS;
                    strcpy(session_status_ncb.NCB_NAME, localname);
                    session_status_ncb.NCB_LENGTH  = sizeof(STATUS_INFO);
                    session_status_ncb.NCB_BUFFER_PTR = &session_info;
                    session_status_ncb.POST_FUNC = post;
                    NetBios(&session_status_ncb);
                    break;
                    }
            default : break;
            }

        goto show_menu;
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */



