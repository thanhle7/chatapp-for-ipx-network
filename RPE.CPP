
/*
 *          Remote Program Executioner  (RPE)
 *
 *  The "Executioner" takes calls from other workstations consisting
 *  of messages that tell it to execute programs or DOS commands.
 *  It returns a "job ticket" to each caller.  A queue of up to 50
 *  pending jobs is maintained.
 *
 *  Copyright (c) 1989 Barry Nance
 *  All Rights Reserved
 *
 *
 *  Written for Turbo C version 2.0.
 *  Use the HUGE memory model to compile this program.
 *
*/

/* - - - - - - - - - - - - - - - - - - - - - - - */
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <bios.h>
#include <mem.h>
#include <fcntl.h>
#include <io.h>
#include <conio.h>
#include <string.h>
#include <time.h>
#include <process.h>
#include <alloc.h>

#include <netbios.h>
void    interrupt background_listen(void);

#define  TRUE   1
#define  FALSE  0

/* - - - - - - - - - - - - - - - - - - - - - - - */

unsigned extern _stklen = 2000;

NCB     cancel_ncb;
NCB     listen_ncb;
NCB     add_name_ncb;
NCB     delete_name_ncb;
NCB     send_ncb;
NCB     receive_ncb;
NCB     hangup_ncb;

NCB     temp_ncb;

void interrupt  (*int_5C_vector)(void);

char    ch;
char    string[201];
char    *exec_prog;
char    *exec_parms;
char    path[81];
char    logname[81];
char    outname[81];
char    machine_name[16];
int     log_handle;
int     job_number;
int     background_activity;
int     executing;
int     i;
unsigned u;
unsigned char general_error;

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
        char    our_name[16];
        char    job[9];
        };

struct  JOB_PACKET job_packet;

long    start_time;
long    stop_time;

unsigned paragraphs;
char     *transient;

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *  A function to call NetBIOS (via Int 5C).
 *
 *
*/
void    NetBios(NCB *ncb_ptr)
        {
        ncb_ptr->NCB_CMD_CPLT = 0xFF;

        _ES    = FP_SEG(ncb_ptr);
        _BX    = FP_OFF(ncb_ptr);
        _AX    = 0x0100;

        geninterrupt(0x5c);
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *  Expand 'name' to be a 16 byte string, padded
 *  on the right with spaces, and null-terminated.
 *  (Doesn't work with 'permanent node names'.)
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
 *  Format a log file entry, lock the file, write
 *  the entry, and unlock the file.  Abort if a serious
 *  error occurs.
*/
void    log(char *log_message)
        {
        char   line[201];
        char   *str_time;
        long   now;
        int    retry_count;

        time(&now);
        str_time = ctime(&now);
        str_time[strlen(str_time) - 1] = '\0';
        sprintf(line, "%s <%s> ==> %s\n",
                str_time,
                machine_name,
                log_message);

        printf("%s", line);

        retry_count = 0;
        while ( lock(log_handle, 0x00000000l, 0x0FFFFFFFl) )
            if (++retry_count > 100)
                {
                printf("SYSTEM ERROR.  Could not lock the log file.\n");
                close(log_handle);
                exit(1);
                }

        write(log_handle, line, strlen(line));

        retry_count = 0;
        while ( unlock(log_handle, 0x00000000l, 0x0FFFFFFFl) )
            if (++retry_count > 100)
                {
                printf("SYSTEM ERROR.  Could not unlock the log file.\n");
                close(log_handle);
                exit(1);
                }

        close(dup(log_handle));
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *  Build the 'add_name' NCB and send it out
 *  across the network.
 *
*/
void    net_add_name(char *name)
        {
        memset(&add_name_ncb, 0, sizeof(NCB));
        add_name_ncb.NCB_COMMAND = ADD_NAME;
        strcpy(add_name_ncb.NCB_NAME, name);
        expand_to_16_chars(add_name_ncb.NCB_NAME);
        NetBios(&add_name_ncb);
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *    Build the 'delete_name' NCB
 *
*/
void    net_delete_name(char *name)
        {
        memset(&delete_name_ncb, 0, sizeof(NCB));
        delete_name_ncb.NCB_COMMAND = DELETE_NAME;
        strcpy(delete_name_ncb.NCB_NAME, name);
        expand_to_16_chars(delete_name_ncb.NCB_NAME);
        NetBios(&delete_name_ncb);
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *    Build the 'listen' NCB and send it out
 *    across the network.  Set the POST address to
 *    point to a 'background' routine to handle a caller.
*/
void    net_listen_post(char *caller, char *us,
                        void interrupt (*post_function)(),
                        unsigned char rto, unsigned char sto)
        {
        memset(&listen_ncb, 0, sizeof(NCB));
        listen_ncb.NCB_COMMAND = LISTEN;
        strcpy(listen_ncb.NCB_NAME,     us);
        strcpy(listen_ncb.NCB_CALLNAME, caller);
        expand_to_16_chars(listen_ncb.NCB_NAME);
        expand_to_16_chars(listen_ncb.NCB_CALLNAME);
        listen_ncb.POST_FUNC = post_function;
        listen_ncb.NCB_RTO = rto;
        listen_ncb.NCB_STO = sto;
        NetBios(&listen_ncb);
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *    Build the 'cancel' NCB and send it out
 *    across the network.
 *
*/
void    net_cancel(NCB *np)
        {
        memset(&cancel_ncb, 0, sizeof(NCB));
        cancel_ncb.NCB_COMMAND = CANCEL;
        cancel_ncb.NCB_BUFFER_PTR = np;
        NetBios(&cancel_ncb);
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *    Build the 'receive' NCB and send it out
 *    across the network.  When the operation completes,
 *    let NetBIOS call the POST routine to handle it.
*/
void    net_receive_post(unsigned char lsn,
                         void interrupt (*post_function)(),
                         void *packet_ptr, int packet_len)
        {
        memset(&receive_ncb, 0, sizeof(NCB));
        receive_ncb.NCB_COMMAND = RECEIVE;
        receive_ncb.NCB_LSN = lsn;
        receive_ncb.NCB_LENGTH = packet_len;
        receive_ncb.NCB_BUFFER_PTR = packet_ptr;
        receive_ncb.POST_FUNC = post_function;
        NetBios(&receive_ncb);
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *    Build the 'send' NCB and send it out
 *    across the network.
 *
*/
void    net_send(unsigned char lsn,void *packet_ptr, int packet_len)
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
 *    Build the 'hang up' NCB and send it out
 *    across the network.  Wait for completion.
 *
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
 *  Find job in the pending queue and remove it
 *  by marking it with a packet flag of 'C'
 *  (or return a 'job not found' indication).
*/

void    cancel_job (int job)
        {
        int  x;

        sprintf(job_packet.job, "J%4.4d", job);
        x = queue_info.queue_tail;
        while (x != queue_info.queue_head)
            {
            if (queue_info.next_jobnum[x] == job)
                {
                queue_info.run_queue[x].packet_flag = 'C';
                job_packet.status_flag = 'C';
                return;
                }
            if (++x == 50) x = 0;
            }
        job_packet.status_flag = 'E';
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *  This function is called by NetBIOS when a POSTed
 *  receive completes (meaning that someone sent us a
 *  status packet, a cancel packet, or a run packet
 *  that goes into the queue).
 *
*/
void    interrupt background_receive(void)
        {
        char *job_token;

        general_error = receive_ncb.NCB_CMD_CPLT;
        if (general_error != 0)
            goto bg_receive_exit;

forget_it:
        if (run_packet.packet_flag == 'X')
            goto bg_close_session;

        if (queue_info.queue_count > 45)
            {
            job_packet.status_flag = 'Z';
            net_send(listen_ncb.NCB_LSN, &job_packet, sizeof(job_packet));
            goto bg_close_session;
            }

status_response:
        if (strnicmp(run_packet.program_and_commandline, "status", 6) == 0)
            {
            net_send(listen_ncb.NCB_LSN, &queue_info, sizeof(queue_info));
            goto bg_close_session;
            }

cancel_a_job:
        if (strnicmp(run_packet.program_and_commandline, "cancel", 6) == 0)
            {
            job_token = strtok(run_packet.program_and_commandline, "Jj");
            if (job_token != NULL)
                i = atoi( strtok(NULL, " \n") ); 
            cancel_job(i);
            net_send(listen_ncb.NCB_LSN, &job_packet, sizeof(job_packet));
            goto bg_close_session;
            }

queue_the_job:
        strcpy(queue_info.run_queue
                    [queue_info.queue_head].program_and_commandline,
                run_packet.program_and_commandline);
        queue_info.run_queue[queue_info.queue_head].packet_flag = 'Q';
        sprintf(job_packet.job, "J%4.4d", ++job_number);
        job_packet.status_flag = 'J';
        queue_info.next_jobnum[queue_info.queue_head] = job_number;

        if (++queue_info.queue_head == 50) queue_info.queue_head = 0;
        queue_info.queue_count++;

        net_send(listen_ncb.NCB_LSN, &job_packet, sizeof(job_packet));

bg_close_session:
        net_hangup(listen_ncb.NCB_LSN);
        if (strnicmp(run_packet.program_and_commandline, "quit", 4) == 0)
            goto bg_receive_exit;

reissue_listen:
        net_listen_post("*", "RPE", background_listen, 20, 20);

bg_receive_exit:
        background_activity = FALSE;
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *  This function is activated when a 'call RPE' command
 *  is issued by another workstation.
 *
*/
void    interrupt background_listen(void)
        {
        general_error = listen_ncb.NCB_CMD_CPLT;
        if (general_error != 0)
            goto bg_listen_exit;

        background_activity = TRUE;

        strcpy(job_packet.our_name, machine_name);
        strcpy(job_packet.job, "        ");

        if (executing)
            job_packet.status_flag = 'Q';
        else
            job_packet.status_flag = 'J';

        net_send(listen_ncb.NCB_LSN, &job_packet, sizeof(job_packet));

        net_receive_post(listen_ncb.NCB_LSN,
                         background_receive,
                         &run_packet, sizeof(run_packet));

bg_listen_exit:
        ;
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *  Execute the next item in the queue.
*/

void    execute_program(int job)
        {
/*
 *  
 *  if a call is still underway, let it finish.
 *
*/
        while (background_activity)
            ;

/*
 *  As mentioned below, we want to force Command.Com to re-load
 *  its transient (high-memory) portion.  This avoids an obscure
 *  DOS bug and makes RPE a little more bullet-proof.
*/
        for (i=0; i<8000; i++)
            transient[i] = 0;

/*
 *
 *  Mention how many jobs are awaiting execution.  Log this job.
 *
*/
        if (queue_info.queue_count != 0)
            {
            sprintf(string, "%d job(s) awaiting execution", 
                    queue_info.queue_count);
            log(string);
            }

         sprintf(job_packet.job, "J%4.4d", job);
         log(job_packet.job);


/*
 *  If the caller is instructing us to quit (remotely), we do so.
 *
 *
*/

        if (strnicmp(run_packet.program_and_commandline, "quit", 4) == 0)
            {
            log("RPE ended remotely.");
            close(log_handle);
            net_hangup(listen_ncb.NCB_LSN);
            while (hangup_ncb.NCB_CMD_CPLT == 0xFF)
                ;
            net_delete_name("RPE");
            while (delete_name_ncb.NCB_CMD_CPLT == 0xFF)
                ;
            exit(0);
            }

/*
 *  Now construct the DOS command line that we'll pass to the
 *  system() function, just as if it were to be executed at a
 *  DOS prompt.  Redirect 'stdout' output to a job-specific file.
*/

        sprintf(string, "%s >%sJ%4.4d.OUT",
            run_packet.program_and_commandline,
            path,
            job);
        log(string);

/*
 *  Execute the program (or DOS command).  For statistical purposes,
 *  keep track of elapsed time.
 *
*/

        executing = TRUE;
        time(&start_time);
        system(string);
        time(&stop_time);
        executing = FALSE;

/*
 *  Log the completion of the program/command.
 *
*/

        sprintf(string, "Job %4.4d completed.  %ld elapsed second(s).", 
                job, (long) stop_time - start_time );
        log(string);
        }

/* - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *  The program starts here.
 *
 *  Initialize job_number, then figure out where the transient
 *  portion of Command.Com is located.  We'll use this later to
 *  zero that area, because we want to force Command.Com to 
 *  reload it.  This avoids an obscure bug in PC/DOS.
 *
*/
void    main (int argc, char *argv[])
        {
        general_error = 0;

        job_number = 0;
        paragraphs = (biosmemory() - 8) * 64;
        transient  = MK_FP(paragraphs, 0);

        background_activity = FALSE;
        queue_info.queue_head = 0;
        queue_info.queue_tail = 0;

/*
 *  Abort if we're running on top of a DOS version earlier than 3.0.
 *  Also, check to see if SHARE.EXE has been run (to support file-
 *  sharing).  Finally, make sure NetBIOS is present.
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

        _DX = (unsigned) &machine_name[0];
        _AX = 0x5E00;
        geninterrupt(0x21);
        if (_CH == 0)
            {
            printf("ERROR.  Machine name not set.\n");
            exit(1);
            }

        i = strlen(machine_name) - 1;
        while (i > 0 && machine_name[i] == ' ')
            {
            machine_name[i] = '\0';
            i--;
            }

        int_5C_vector = getvect(0x5C);
        if (int_5C_vector == (void far *) NULL)
            {
            printf("ERROR. NetBios not loaded (Int5C not present).\n");
            exit(1);
            }

        memset(&temp_ncb, 0, sizeof(NCB));
        temp_ncb.NCB_COMMAND = 0x7F;
        NetBios(&temp_ncb);
        if (temp_ncb.NCB_RETCODE != 03)
            {
            printf("ERROR. NetBios not loaded (No response from Int5C).\n");
            exit(1);
            }

/*
 *  Do an 'Add Name' call, to make us available on
 *  the network.
 *
*/
Step_2:
        printf("Adding name 'RPE' to the network...");

        net_add_name("RPE");
        while (add_name_ncb.NCB_CMD_CPLT == 0xFF)
            ;

        printf("\n");
        if (add_name_ncb.NCB_CMD_CPLT != 0)
            {
            printf("ERROR.  NetBios said: %s.\n",
                net_error_message[(int)add_name_ncb.NCB_CMD_CPLT]);
            exit(1);
            }

/*
 *  Now find out the drive and directory in which the log file
 *  should be written.  Open it for shared access (if it's 
 *  necessary to create it, we'll have to close it and re-open
 *  it because file-creation confers exclusive access, which we
 *  don't want).
*/
Step_3:
        printf("Specify network Drive:\\Path for log file: ");
        gets(path);
        if (strlen(path) == 0)
            goto Step_3;

        if (path[strlen(path)-1] != '\\')
            strcat(path, "\\");
        strcpy(logname, path);
        strcat(logname, "RPE.LOG");
        strupr(logname);

        printf("Log file is %s (append/shared)\n", logname);

        if ( (log_handle = creatnew(logname, 0)) != -1)
            close(log_handle);
        log_handle = open(logname, O_RDWR | O_APPEND | O_TEXT | O_DENYNONE);

        if (log_handle == -1)
            {
            net_delete_name("RPE");
            while (delete_name_ncb.NCB_CMD_CPLT == 0xFF)
                ;
            printf("ERROR. Could not open log file.\n");
            exit(1);
            }

        log("RPE started.");

/*
 *  Issue a Listen-to-anyone command.  Set the receive-timeout
 *  and send-timeout fields to 20 500-ms periods (10 seconds)
 *  each.  If someone calls, invoke the 'background_listen()'
 *  routine.
*/
Step_4:
        net_listen_post("*", "RPE", background_listen, 20, 20);


/*
 *  Let people know we're waiting for something to do,
 *  and tell how to stop the program.
*/

Step_5:
        log("Waiting for work.  Press 'ESC' to stop RPE. ");

/*
 *  While we're waiting for a call, look for a press of the
 *  ESCape key, to see if someone wants to stop the program.
 *  Confirm their response.  To stop RPE, we need to leave
 *  things the way we found them, so we cancel the
 *  outstanding 'listen' command, delete our name from the
 *  name table, and then go back to DOS.
 *
*/
Step_6:
        while (!general_error)
            {
            if (bioskey(1))
                if ( (ch = (char) getch()) == 27 )
                    {
                    log("Terminate RPE (Yes/No)? ");
                    gets(string);
                    if (string[0] == 'Y' || string[0] == 'y')
                        {
                        log("RPE ended.");
                        close(log_handle);
                        net_cancel(&listen_ncb);
                        while (cancel_ncb.NCB_CMD_CPLT == 0xFF)
                            ;
                        net_delete_name("RPE");
                        while (delete_name_ncb.NCB_CMD_CPLT == 0xFF)
                            ;
                        exit(0);
                        }
                    goto Step_5;
                    }
            if (queue_info.queue_head != queue_info.queue_tail)
                {
                strcpy(run_packet.program_and_commandline, 
                        queue_info.run_queue
                            [queue_info.queue_tail].program_and_commandline);
                job_number = queue_info.next_jobnum[queue_info.queue_tail];
                if (queue_info.run_queue
                        [queue_info.queue_tail].packet_flag != 'C')
                    {
                    queue_info.run_queue
                        [queue_info.queue_tail].packet_flag = 'E';
                    execute_program(job_number);
                    }
                queue_info.queue_count--;
                if (++queue_info.queue_tail == 50) queue_info.queue_tail = 0;
                }
            }

/*
 *  If an error occurs, we want to stop the program as
 *  gracefully as possible.  So, we make sure we cancel
 *  any outstanding 'listen' and delete the name 'RPE'
 *  before going back to DOS.
*/

        sprintf(string, "RPE aborted--%s", 
                net_error_message[general_error]);
        log(string);
        close(log_handle);
        net_cancel(&listen_ncb);
        while (cancel_ncb.NCB_CMD_CPLT == 0xFF)
            ;
        net_delete_name("RPE");
        while (delete_name_ncb.NCB_CMD_CPLT == 0xFF)
            ;
        exit(1);
        }


