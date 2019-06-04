#include <netbios.h>
#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <bios.h>

#ifdef __cplusplus
    #define __CPPARGS ...
#else
    #define __CPPARGS
#endif

const	char	test[8] = {'Ä',0x8,'\\',0x8,'³',0x8,'/',0x8};

void	interrupt (*int_5c)(__CPPARGS);

char 	lname[16], rname[16], sdata[512], rdata[512], chain[5];
NCB	ncb_add_name, ncb_receive, ncb_send, ncb_temp;
unsigned int	i, send_ok = 0, receive_ok = 0;
int	cur_x, cur_y, screen[80*25];
/*struct
	{
	  int	solan;
	  int	nhandien[50];
	  int	quatrinh[50];
	} rdata;*/

//============================================

void    tran_to_16cs(char *name)
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
//============================================

void    NetBios(NCB far *ncb_ptr)
	{
	_ES    = FP_SEG(ncb_ptr);
	_BX    = FP_OFF(ncb_ptr);
	_AX    = 0x0100;
	geninterrupt(0x5C);
	}

//============================================

void interrupt     post(void)
    {
      movedata(_ES, _BX, FP_SEG(&ncb_temp), FP_OFF(&ncb_temp), sizeof(NCB));

      if (ncb_temp.NCB_COMMAND == SEND_DATAGRAM)
	  if (ncb_temp.NCB_RETCODE || ncb_temp.NCB_CMD_CPLT)
	     send_ok = 0;
	  else
	     send_ok = 2;

      if (ncb_temp.NCB_COMMAND == RECEIVE_DATAGRAM)
	  if (ncb_temp.NCB_RETCODE || ncb_temp.NCB_CMD_CPLT)
	     receive_ok = 0;
	  else
	     receive_ok = 2;
    }

//============================================

void report(int fonc)

{
   gettext(1, 1, 80, 25, screen);	// Save screen
   cur_x = wherex(); cur_y = wherey();	// and cursor.

   gotoxy(7, 9);
   printf("Report info:");
   gotoxy(7, 11);
   switch (fonc)
   {
      case 1:
	  send_ok = 0;
	  cprintf("Sending successed . . .");
	  break;
      case 2:
	  receive_ok = 0;
	  cprintf("Receiving successed . . .");
	  gotoxy(7, 13);
	  rdata[70] = '\0';
	  printf("Message say: %s",rdata);
	  break;
      default:
	  printf("No job completted...");
   };
   gotoxy(7, 15);
   cprintf("Press a key...");
   bioskey(0);
   puttext(1, 1, 80, 25, screen);
   gotoxy(cur_x, cur_y);
}

//============================================

void main(void)
{
	int_5c = getvect(0x5c);
	if (int_5c == (void far *)NULL)
	   {
		printf("Your PC is off line...\n");
		exit(1);
	   }
	memset(&ncb_add_name, 0, sizeof(NCB));
	ncb_add_name.NCB_COMMAND = 0x7F;    // Not valid function
	NetBios(&ncb_add_name);
	if (ncb_add_name.NCB_RETCODE != 0x3)
	   {
		printf("No response from NetBios. . .\n");
		exit(1);
	   }
	clrscr();
	printf("Local name: "); gets(lname);
	tran_to_16cs(lname);
	printf("Remote name: "); gets(rname);
	tran_to_16cs(rname);

	memset(&ncb_add_name, 0, sizeof(NCB));
	ncb_add_name.NCB_COMMAND = ADD_NAME;
	strcpy(ncb_add_name.NCB_NAME, lname);
	NetBios(&ncb_add_name);

	i = 0; printf("\nWait ");
	while (ncb_add_name.NCB_CMD_CPLT == 0xFF) putchar(test[i++ % 8]);
	if (ncb_add_name.NCB_CMD_CPLT != 0x0)
	{
	   printf("\rAdd name failure...%25s\n",
		net_error_message[ncb_add_name.NCB_CMD_CPLT]);
	   exit(1);
	}
	do
	{
	  clrscr();
	  printf("                  --- Le Ngoc Thanh ---\n");
	  printf(" NETbios Datagram Protocol Supported Communucative Program.\n");
	  printf(" 1 .. Send\n 2 .. Receive\n Enter your choice ? ");
	  do {
		if (send_ok == 2) report(1);
		if (receive_ok == 2) report(2);
	     } while (!bioskey(1));

	  gets(chain);
	  if (strlen(chain) == 0) break;
	  i = atoi(chain);

	  gotoxy(1, 7);
	  switch (i)
	   {
	    case 0:
	       printf("Normal shutdown ...");
	       break;
	    case 1:
	       if (send_ok == 1)
		  {
		      printf("The last sending not completed yet...");
		      bioskey(0);
		      break;
		  };
	       printf("\nMessage to send: ");
	       gets(sdata);
	       memset(&ncb_send, 0, sizeof(NCB));
	       ncb_send.NCB_COMMAND = SEND_DATAGRAM;
	       ncb_send.NCB_NUM = ncb_add_name.NCB_NUM;
	       strcpy(ncb_send.NCB_CALLNAME, rname);
	       ncb_send.NCB_BUFFER_PTR = (void far *) &sdata;
	       ncb_send.NCB_LENGTH = 512;
	       ncb_send.POST_FUNC = post;
	       ncb_send.NCB_LANA_NUM = 0;
	       ncb_send.NCB_STO = 20;
	       NetBios(&ncb_send);
	       send_ok = 1;        		// sending flag
	       break;
	    case 2:
	       gotoxy(1, 5);
	       if (receive_ok == 1)
		  {
		      printf("The last receiving not completed yet...");
		      bioskey(0);
		      break;
		  };
	       memset(&ncb_receive, 0, sizeof(NCB));
	       ncb_receive.NCB_COMMAND = RECEIVE_DATAGRAM;
	       ncb_receive.NCB_NUM  = ncb_add_name.NCB_NUM;
	       ncb_receive.NCB_BUFFER_PTR = (void far *) &rdata;
	       ncb_receive.NCB_LENGTH = sizeof(rdata);
	       ncb_receive.POST_FUNC = post;
	       ncb_receive.NCB_LANA_NUM = 0;
	       ncb_receive.NCB_RTO = 20;
	       NetBios(&ncb_receive);
	       receive_ok = 1;
	       break;
	    default:
	       printf(" No such a function, sorry!\n Press a key....");
	       bioskey(0);
	   };
	} while (i);

	memset(&ncb_add_name, 0, sizeof(NCB));
	ncb_add_name.NCB_COMMAND = DELETE_NAME;
	strcpy(ncb_add_name.NCB_NAME, lname);
	ncb_add_name.NCB_CMD_CPLT = 0;
	NetBios(&ncb_add_name);
}