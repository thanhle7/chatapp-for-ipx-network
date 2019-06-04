
/*
 *          Remote -- A "Job" Dispatcher
 *
 *  "Remote" sends messages to "RPE" (which you run on a separate
 *  workstation), telling it to execute programs or DOS commands.
 *  A "job ticket" is issued back to REMOTE by RPE for each job.
 *
 *  Since the program or DOS command runs remotely, your computer
 *  is freed up immediately to do other work.
 *
 *  Copyright (c) 1989 Barry Nance
 *  All Rights Reserved
 *
 *
 *  Written for Turbo C version 2.0.
 *  Use the SMALL memory model to compile this program.
 *
*/

/* - - - - - - - - - - - - - - - - - - - - - - - */
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <dos.h>

#include <netbios.h>

NCB     status_ncb;
NCB     call_ncb;
NCB     send_ncb;
NCB     receive_ncb;
NCB     hangup_ncb;
NCB     temp_ncb;

void interrupt  (*int_5C_vector)(void);

char    string[201];
char    our_name[16];
char    other_name[16];
char    i;
int     j;

char far *command_line;
char far *byte_count;

struct  RUN_PACKET
        {
        char    packet_flag;
        char    program_and_commandline[133];
        };

struct  RUN_PACKET run_packet;

struct  QUEUE_INFO
	{
	int     queue_count;
        int     queue_head;
	int     queue_tail;
        struct  RUN_PACKET run_queue[50];
        int     next_jobnum[50];
        };

struct  QUEUE_INFO queue_info;

struct  JOB_PACKET
        {
        char    status_flag;
        char    machine_name[16];
        char    job[9];
        };

struct  JOB_PACKET job_packet;

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *  Call NetBIOS, via Interrupt 5C.
*/
void    NetBios(NCB far *ncb_ptr)
	{
	ncb_ptr->NCB_CMD_CPLT = 0xFF;
	ncb_ptr->NCB_RETCODE  = 0xFF;

        _ES    = FP_SEG(ncb_ptr);
        _BX    = FP_OFF(ncb_ptr);
        _AX    = 0x0100;

	geninterrupt(0x5c);
	}

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *  Expand a NetBIOS name by padding on the right
 *  with spaces.
*/
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
/*
 *  Build and send an 'adapter status' command
*/
void    net_status(char far *buffer, int len)
        {
        memset(&status_ncb, 0, sizeof(NCB));
        status_ncb.NCB_COMMAND = STATUS;
        strcpy(status_ncb.NCB_CALLNAME, "*");
        expand_to_16_chars(status_ncb.NCB_CALLNAME);
        status_ncb.NCB_LENGTH = len;
        status_ncb.NCB_BUFFER_PTR = buffer;
        NetBios(&status_ncb);
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *  "Call" another workstation.
*/
void    net_call(char *who, char *us, unsigned char rto, unsigned char sto)
        {
        memset(&call_ncb, 0, sizeof(NCB));
	call_ncb.NCB_COMMAND = CALL;
	memcpy(call_ncb.NCB_NAME,     us, 16);
	strcpy(call_ncb.NCB_CALLNAME, who);
        expand_to_16_chars(call_ncb.NCB_CALLNAME);
        call_ncb.NCB_RTO = rto;
        call_ncb.NCB_STO = sto;
        NetBios(&call_ncb);
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *  Issue a "receive" command to NetBIOS.
*/
void    net_receive(unsigned char lsn, void far *packet_ptr, int packet_len)
        {
        memset(&receive_ncb, 0, sizeof(NCB));
        receive_ncb.NCB_COMMAND = RECEIVE;
        receive_ncb.NCB_LSN = lsn;
        receive_ncb.NCB_LENGTH = packet_len;
        receive_ncb.NCB_BUFFER_PTR = packet_ptr;
	NetBios(&receive_ncb);
	}

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *  Build and send a message packet via NetBIOS.
*/
void    net_send(unsigned char lsn, void far *packet_ptr, int packet_len)
        {
        memset(&send_ncb, 0, sizeof(NCB));
        send_ncb.NCB_COMMAND = SEND;
        send_ncb.NCB_LSN = lsn;
        send_ncb.NCB_LENGTH = packet_len;
        send_ncb.NCB_BUFFER_PTR = packet_ptr;
        NetBios(&send_ncb);
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *  Close a session with a "hang up" command.
*/
void    net_hangup(unsigned char lsn)
	{
        memset(&hangup_ncb, 0, sizeof(NCB));
        hangup_ncb.NCB_COMMAND = HANG_UP;
        hangup_ncb.NCB_LSN = lsn;
        NetBios(&hangup_ncb);
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *  The program starts here.
 *
 *  Basically, all we do is send our command line to "RPE"
 *  so that it can treat it as a program (or DOS command)
 *  to be executed remotely.
 *
*/
//(int argc, char *argv[])
void    main(void)
	{
/*
 *  First, make sure that we're running under DOS 3.0 or later.
 *  Then check to make sure that SHARE.EXE has been run and that
 *  NetBIOS is active.
*/
Step_1:
        if (_osmajor < 3)
	    {
            printf("ERROR. Early versions of DOS not supported.\n");
            exit(1);
            }

        _AX = 0x1000;
	geninterrupt(0x2F);
	if (_AL != 0xFF)
	    {
	    printf("ERROR. 'Share.Exe' (file sharing support) not loaded.\n");
	    exit(1);
	    }
	// make a void far interrupt:
	int_5C_vector = (void (interrupt far *)()) getvect(0x5C);
	if (int_5C_vector == (void far *) NULL)
	    {
	    printf("ERROR. NetBios not loaded (Int5C not present).\n");
	    exit(1);
	    }

	memset(&temp_ncb, 0, sizeof(NCB));
	temp_ncb.NCB_COMMAND = 0x7F;
	NetBios(&temp_ncb);
	if (temp_ncb.NCB_RETCODE != 0x03)
	    {
	    printf("ERROR. NetBios not loaded (No response from Int5C).\n");
	    exit(1);
	    }

/*
 *  We need a name to identify ourselves when we later issue the
 *  "call" command.  Let's use the "Permanent Node Name"...we build
 *  it by getting the first six bytes of the data area returned by
 *  the "adapter status" command, and prefixing them with ten bytes
 *  of binary zeroes.
 *
 *  Note that we only give the 'adapter status' command a buffer of
 *  60 bytes to fill.  We then deliberately ignore the almost certain
 *  NCB_CMD_CPLT return code of 6 ("data area too small").
*/
Step_2:
	net_status(string, 60);
	while (status_ncb.NCB_CMD_CPLT == 0xFF)
	    ;
	if (status_ncb.NCB_CMD_CPLT != 0x00 && status_ncb.NCB_CMD_CPLT != 0x06)
	    {
	    printf("ERROR.  NetBios said: %s.\n",
		net_error_message[(int)status_ncb.NCB_CMD_CPLT]);
	    exit(1);
	    }

	memset(our_name, 0, 16);
	for (i=0; i<6; i++)
	    our_name[i+10] = string[i];

/*
 *  REMOTE expects to be invoked like this:
 *
 *      C:> remote <progname> <command line parameters>
 *
 *  or this:
 *
 *      C:> remote <dos command> <command parameters>
 *
 *  So we go down inside the Program Segment Prefix to pick up
 *  REMOTE's command line, which will be sent to RPE.  If REMOTE
 *  is run without a command line, it simply exits after reminding
 *  the user that a command line is required.
 *
 *  The length of the command line is found in byte 0x80 (byte 128)
 *  of the PSP.  The bytes following the length are the command
 *  line, terminated by a carriage return.  For example:
 *
 *   (11)     C   O   P   Y       A   :   *   .   *  (13)
 *   --- --- --- --- --- --- --- --- --- --- --- --- ---
 *  0x80  81  82  83  84  85  86  87  88  89  8A  8B  8C
 *
 *  Note that the first byte following the length byte is normally
 *  a space character.  We bypass it.
 *
*/
Step_3:
	byte_count   = (char far *)MK_FP(_psp, 0x0080);
	command_line = (char far *)MK_FP(_psp, 0x0081);

	if (*byte_count == 0)
	    {
	    printf("Usage: REMOTE <progname> <command line>\n");
	    printf("    or REMOTE <dos command> <parms>\n");
	    exit(1);
	    }

	memset(run_packet.program_and_commandline, 0, 133);
        while (*command_line == ' ')
	    ++command_line;

        i = 0;
        while (*command_line != 13 && *command_line != 0)
            {
            run_packet.program_and_commandline[i] = *command_line;
            ++command_line;
            ++i;
            }

/*
 *  Issue a NetBIOS 'call' to establish a session with "RPE".
 *
 *
*/
Step_5:
        net_call("RPE", our_name, 20, 20);
        while (call_ncb.NCB_CMD_CPLT == 0xFF)
	    ;

	if (call_ncb.NCB_CMD_CPLT != 0)
            {
            printf("Couldn't connect: %s\n", 
                    net_error_message[call_ncb.NCB_CMD_CPLT]);
            printf("'Remote' ended (unsuccessfully).\n");
            exit(1);
            }

/*
 *  "RPE" always starts the conversation by sending us the machine-
 *  name of the computer it's running on (as part of 'job_packet').
 *  So the first thing we do is issue a 'receive' command in order 
 *  to get that first message.
*/
Step_6:
        net_receive(call_ncb.NCB_LSN, &job_packet, sizeof(job_packet));
        while (receive_ncb.NCB_CMD_CPLT == 0xFF)
            ;

        if (receive_ncb.NCB_CMD_CPLT != 0)
	    {
            printf("RPE aborted--%s\n", 
                    net_error_message[receive_ncb.NCB_CMD_CPLT]);
            net_hangup(call_ncb.NCB_LSN);
            while (hangup_ncb.NCB_CMD_CPLT == 0xFF)
                ;
            printf("Remote ended due to error.\n");
            exit(1);
            }

        strcpy(other_name, job_packet.machine_name);
        j = strlen(other_name) - 1;
        while (j > 0 && other_name[j] == ' ')
            {
            other_name[j] = '\0';
            j--;
            }

/*
 *  If the job packet returned by "RPE" contains a 'packet_flag'
 *  of 'Q', it means that our job request will go into the queue.
 *  Let's ask if this is okay.
 *
 *  If 'packet_flag' is a 'Z', the queue is full.
 *
 *  If the user doesn't want the job placed in the awaiting-
 *  execution queue, we'll tell "RPE" to forget it.
 *
 *  If the command line contains 'quit', 'status', or 'cancel',
 *  pass it immediately to "RPE".
 *
*/
Step_7:
        if (strnicmp(run_packet.program_and_commandline, "quit", 4) == 0)
            goto Step_8;
        if (strnicmp(run_packet.program_and_commandline, "status", 6) == 0)
            goto Step_8;
        if (strnicmp(run_packet.program_and_commandline, "cancel", 6) == 0)
            goto Step_8;

	if (job_packet.status_flag == 'Z')
            {
            printf("The queue is full at this time.  Try later.\n");
            net_hangup(call_ncb.NCB_LSN);
            while (hangup_ncb.NCB_CMD_CPLT == 0xFF)
                ;
            printf("'Remote' ended due to error.\n");
            exit(1);
            }

        if (job_packet.status_flag == 'Q')
            {
            printf("\n");
            printf("RPE (%s) is busy...your request will go into the queue.\n",
                    other_name);
            printf("Is this okay (Yes/No)? ");
            do
                i = (char) getch();
                while (i != 'N' && i != 'n' && i != 'Y' && i != 'y');
            if (i == 'y' || i == 'Y')
		{
                printf("Yes...\n");
                goto Step_8;
                }
            printf("No...\n");
            printf("Job request cancelled.\n");
            run_packet.packet_flag = 'X';
            net_send(call_ncb.NCB_LSN, &run_packet, sizeof(run_packet));
            while (send_ncb.NCB_CMD_CPLT == 0xFF)
                ;
            if (send_ncb.NCB_CMD_CPLT != 0)
                {
                printf("NetBios error msg: %s\n", 
                        net_error_message[send_ncb.NCB_CMD_CPLT]);
                net_hangup(call_ncb.NCB_LSN);
                while (hangup_ncb.NCB_CMD_CPLT == 0xFF)
                    ;
                printf("'Remote' ended due to error.\n");
                exit(1);
                }
	    net_hangup(call_ncb.NCB_LSN);
            while (hangup_ncb.NCB_CMD_CPLT == 0xFF)
                ;
            exit(0);
            }

/*
 *  Send "RPE" a message containing REMOTE's command line.
 *
*/
Step_8:
        run_packet.packet_flag = 'P';

        net_send(call_ncb.NCB_LSN, &run_packet, sizeof(run_packet));
        while (send_ncb.NCB_CMD_CPLT == 0xFF)
            ;

        if (send_ncb.NCB_CMD_CPLT != 0)
            {
            printf("NetBios error msg: %s\n", 
		    net_error_message[send_ncb.NCB_CMD_CPLT]);
            net_hangup(call_ncb.NCB_LSN);
            while (hangup_ncb.NCB_CMD_CPLT == 0xFF)
                ;
            printf("'Remote' ended due to error.\n");
            exit(1);
            }


/*
 *  If we sent a 'quit' command, we're stopping "RPE".  So we can
 *  quit, too.
 *
*/
Step_9:
        if (strnicmp(run_packet.program_and_commandline, "quit", 4) == 0)
            {
            printf("RPE (%s) will stop when the queue is empty.\n",
                    other_name);
            net_hangup(call_ncb.NCB_LSN);
	    while (hangup_ncb.NCB_CMD_CPLT == 0xFF)
                ;
            exit(0);
            }

/*
 *  Receive a response from "RPE".
 *
*/
Step_10:
        if (strnicmp(run_packet.program_and_commandline, "status", 6) == 0)
            net_receive(call_ncb.NCB_LSN, &queue_info, sizeof(queue_info));
        else
        if (strnicmp(run_packet.program_and_commandline, "cancel", 6) == 0)
            net_receive(call_ncb.NCB_LSN, &job_packet, sizeof(job_packet));
        else
            net_receive(call_ncb.NCB_LSN, &job_packet, sizeof(job_packet));


        while (receive_ncb.NCB_CMD_CPLT == 0xFF)
	    ;

        if (receive_ncb.NCB_CMD_CPLT != 0)
            {
            printf("RPE aborted--%s\n", 
                    net_error_message[receive_ncb.NCB_CMD_CPLT]);
            net_hangup(call_ncb.NCB_LSN);
            while (hangup_ncb.NCB_CMD_CPLT == 0xFF)
                ;
            printf("Remote ended due to error.\n");
            exit(1);
            }

/*
 *  The dialog is finished; hang up on "RPE".
 *
*/
        net_hangup(call_ncb.NCB_LSN);
        while (hangup_ncb.NCB_CMD_CPLT == 0xFF)
            ;


/*
 *  Display the awaiting-execution queue.
 *
*/
Step_11:
        if (strnicmp(run_packet.program_and_commandline, "status", 6) == 0)
            {
            printf("\n");
            printf("There are %d job(s) in %s's queue.\n\n",
                    queue_info.queue_count,
                    other_name);
            if (queue_info.queue_count > 0)
                {
                printf("  JOB   COMMAND\n");
                printf(" -----  -----------------------------------\n");
                for (j=0; j<queue_info.queue_count; j++)
                    {
                    if (queue_info.run_queue
			    [queue_info.queue_tail].packet_flag == 'C')
                        strcpy(string, "CANCELLED");
                    else
                    if (queue_info.run_queue
                            [queue_info.queue_tail].packet_flag == 'E')
                        strcpy(string, "EXECUTING");
                    else
                        strcpy(string, "PENDING");
                    printf(" J%4.4d  %-40.40s  %s\n",
                        queue_info.next_jobnum[queue_info.queue_tail],
                        queue_info.run_queue
                            [queue_info.queue_tail].program_and_commandline,
                        string);
                    if (++queue_info.queue_tail == 50)
                        queue_info.queue_tail = 0;
                    }
		printf("\n");
                }
            printf("Remote ended successfully.\n");
            exit(0);
	    }

/*
 *  Display whether the job was cancelled or not found in the queue.
 *
*/
Step_12:
        if (strnicmp(run_packet.program_and_commandline, "cancel", 6) == 0)
            {
            if (job_packet.status_flag == 'E')
                printf("Job %s not found in %s's queue.\n",
                        job_packet.job,
                        other_name);
            else
                printf("Job %s cancelled in %s's queue.\n",
                        job_packet.job,
			other_name);
            printf("Remote ended successfully.\n");
            exit(0);
            }

/*
 *  Display the assigned job number.
 *
*/
Step_13:
        printf("\n");
        printf("Job # %s in queue on machine %s.\n",
                job_packet.job,
                other_name);
        printf("Remote ended successfully.\n");
        exit(0);
        }



