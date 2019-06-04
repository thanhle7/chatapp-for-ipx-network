
    /* NETBIOS.H */

typedef unsigned char byte;
typedef unsigned int  word;

/* Network Control Block (NCB)  */

typedef struct
    {
    byte NCB_COMMAND;               /* command id                         */
    byte NCB_RETCODE;               /* immediate return code              */
    byte NCB_LSN;                   /* local session number               */
    byte NCB_NUM;                   /* network name number                */
    void far *NCB_BUFFER_PTR;       /* address of message packet          */
    word NCB_LENGTH;                /* length of message packet           */
    byte NCB_CALLNAME[16];          /* name of the other computer         */
    byte NCB_NAME[16];              /* our network name                   */
    byte NCB_RTO;                   /* receive time-out in 500 ms. incrs. */
    byte NCB_STO;                   /* send time-out - 500 ms. increments */
    void interrupt (*POST_FUNC)(void); /* address of POST routine         */
    byte NCB_LANA_NUM;              /* adapter number (0 or 1)            */
    byte NCB_CMD_CPLT;              /* final return code                  */
    byte NCB_RESERVE[14];           /* Reserved area                      */
    } 
    NCB;


/* NetBIOS error return messages  */

char    *net_error_message[] = {
        "success",                      /*  00  */
        "invalid buffer length",        /*  01  */
        "ret code 02",                  /*  02  */
        "invalid command",              /*  03  */
        "ret code 04",                  /*  04  */
        "timed out",                    /*  05  */
        "buffer too small",             /*  06  */
        "ret code 07",                  /*  07  */
        "invalid session num",          /*  08  */
        "no resource",                  /*  09  */
        "session closed",               /*  0A  */
        "command cancelled",            /*  0B  */
        "ret code 0C",                  /*  0C  */
        "dupl. local name",             /*  0D  */
        "name table full",              /*  0E  */
        "active session",               /*  0F  */
        "ret code 10",                  /*  10  */
        "session table full",           /*  11  */
        "no one listening",             /*  12  */
        "invalid name num",             /*  13  */
        "no answer",                    /*  14  */
        "no local name",                /*  15  */
        "name in use",                  /*  16  */
        "name is deleted",              /*  17  */
        "abnormal end",                 /*  18  */
        "name conflict",                /*  19  */
        "ret code 1A",                  /*  1A  */
        "ret code 1B",                  /*  1B  */
        "ret code 1C",                  /*  1C  */
        "ret code 1D",                  /*  1D  */
        "ret code 1E",                  /*  1E  */
        "ret code 1F",                  /*  1F  */
        "ret code 20",                  /*  20  */
        "card busy",                    /*  21  */
        "too many cmds",                /*  22  */
        "invalid card num",             /*  23  */
        "cancel done",                  /*  24  */
        "ret code 25",                  /*  25  */
        "cannot cancel"                 /*  26  */
        };

/* Symbolic names for NetBIOS commands  */

#define RESET                           0x32
#define CANCEL                          0x35
#define STATUS                          0xb3
#define STATUS_WAIT                     0x33
#define TRACE                           0xf9
#define TRACE_WAIT                      0x79
#define UNLINK                          0x70
#define ADD_NAME                        0xb0
#define ADD_NAME_WAIT                   0x30
#define ADD_GROUP_NAME                  0xb6
#define ADD_GROUP_NAME_WAIT             0x36
#define DELETE_NAME                     0xb1
#define DELETE_NAME_WAIT                0x31
#define CALL                            0x90
#define CALL_WAIT                       0x10
#define LISTEN                          0x91
#define LISTEN_WAIT                     0x11
#define HANG_UP                         0x92
#define HANG_UP_WAIT                    0x12
#define SEND                            0x94
#define SEND_WAIT                       0x14
#define SEND_NO_ACK                     0xf1
#define SEND_NO_ACK_WAIT                0x71
#define CHAIN_SEND                      0x97
#define CHAIN_SEND_WAIT                 0x17
#define CHAIN_SEND_NO_ACK               0xf2
#define CHAIN_SEND_NO_ACK_WAIT          0x72
#define RECEIVE                         0x95
#define RECEIVE_WAIT                    0x15
#define RECEIVE_ANY                     0x96
#define RECEIVE_ANY_WAIT                0x16
#define SESSION_STATUS                  0xb4
#define SESSION_STATUS_WAIT             0x34
#define SEND_DATAGRAM                   0xa0
#define SEND_DATAGRAM_WAIT              0x20
#define SEND_BCST_DATAGRAM              0xa2
#define SEND_BCST_DATAGRAM_WAIT         0x22
#define RECEIVE_DATAGRAM                0xa1
#define RECEIVE_DATAGRAM_WAIT           0x21
#define RECEIVE_BCST_DATAGRAM           0xa3
#define RECEIVE_BCST_DATAGRAM_WAIT      0x23



