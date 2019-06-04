
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <mem.h>
#ifdef __cplusplus
    #define __CPPARGS ...
#else
    #define __CPPARGS
#endif

typedef unsigned int    word;
typedef unsigned char   byte;
const   char  *mess="thanh" ;
struct  {
    word    buffer_length;
    byte    request_type;
    byte    last_object_id[4];
    byte    object_type[2];
    byte    name_length;
    byte    object_name[48];
    } object_request;

struct  {
    word    buffer_length;
    byte    object_id[4];
    byte    object_type[2];
    byte    object_name[48];
    byte    object_flag;
    byte    object_security;
    byte    object_has_properties;
    } object_reply;

struct  {
    word    buffer_length;
    byte    request_type;
    byte    object_type[2];
    byte    object_name_length;
    byte    object_name[48];
    byte    sequence_number[4];
    byte    property_name_length;
    byte    property_name[16];
    } property_request;

struct  {
    word    buffer_length;
    byte    property_name[16];
    byte    property_flags;
    byte    property_security;
    byte    sequence_number[4];
    byte    property_has_value;
    byte    more_properties;
    } property_reply;

struct  {
    word    buffer_length;
    byte    request_type;
    byte    object_type[2];
    byte    object_name_length;
    byte    object_name[48];
    byte    segment_number;
    byte    property_name_length;
    byte    property_name[16];
    } value_request;

struct  {
    word    buffer_length;
    byte    property_value[128];
    byte    more_segments;
    byte    property_flags;
    } value_reply;

byte    all_ffs[48] =
   { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

int     obj_1st_time;
int     prop_1st_time;
int     val_1st_time;
void  interrupt  (*int_5C_vector)(__CPPARGS);

int     get_value(void)
	   {
	   struct SREGS sregs;
	   union  REGS  regs;

	   if (val_1st_time)
		  {
		  val_1st_time = 0;
		  value_request.segment_number = 0;
		  }

	   value_request.buffer_length = sizeof(value_request) - 2;
	   value_request.request_type  = 0x3D;
	   memcpy(value_request.object_type, object_reply.object_type, 2);
	   value_request.object_name_length = 48;
	   strcpy(value_request.object_name, object_reply.object_name);
	   value_request.segment_number++;
	   value_request.property_name_length = 16;
	   strcpy(value_request.property_name, property_reply.property_name);

	   value_reply.buffer_length = sizeof(value_reply) - 2;

	   regs.h.ah = 0xE3;
	   sregs.ds  = FP_SEG( (void far *) &value_request);
	   regs.x.si = FP_OFF( (void far *) &value_request);
	   sregs.es  = FP_SEG( (void far *) &value_reply);
	   regs.x.di = FP_OFF( (void far *) &value_reply);
	   int86x(0x21, &regs, &regs, &sregs);

	   regs.h.ah = 0;
	   return regs.x.ax;
	   }

int     get_next_property(void)
	   {
	   struct SREGS sregs;
	   union  REGS  regs;

	   if (prop_1st_time)
		  {
		  prop_1st_time = 0;
		  memcpy(property_reply.sequence_number, all_ffs, 4);
		  }

	   property_request.buffer_length = sizeof(property_request) - 2;
	   property_request.request_type  = 0x3C;
	   memcpy(property_request.object_type, object_reply.object_type, 2);
	   property_request.object_name_length = 48;
	   memcpy(property_request.object_name, object_reply.object_name, 48);
	   memcpy(property_request.sequence_number,
			 property_reply.sequence_number, 4);
	   property_request.property_name_length = 16;
	   strcpy(property_request.property_name, "*");

	   property_reply.buffer_length   = sizeof(property_reply) - 2;

	   regs.h.ah = 0xE3;
	   sregs.ds  = FP_SEG( (void far *) &property_request);
	   regs.x.si = FP_OFF( (void far *) &property_request);
	   sregs.es  = FP_SEG( (void far *) &property_reply);
	   regs.x.di = FP_OFF( (void far *) &property_reply);
	   int86x(0x21, &regs, &regs, &sregs);

	   regs.h.ah = 0;
	   return regs.x.ax;
	   }

int     get_next_object(void)
	   {
	   struct SREGS sregs;
	   union  REGS  regs;

	   if (obj_1st_time)
		  {
		  obj_1st_time = 0;
		  memcpy(object_reply.object_id, &all_ffs, 4);
		  }

	   object_reply.buffer_length   = sizeof(object_reply) - 2;

	   object_request.buffer_length = sizeof(object_request) - 2;
	   object_request.request_type  = 0x37;
	   memcpy(object_request.last_object_id, object_reply.object_id, 4);
	   memcpy(object_request.object_type, &all_ffs, 2);
	   strcpy(object_request.object_name, "*");
	   object_request.name_length = 48;

	   regs.h.ah = 0xE3;
	   sregs.ds  = FP_SEG( (void far *) &object_request);
	   regs.x.si = FP_OFF( (void far *) &object_request);
	   sregs.es  = FP_SEG( (void far *) &object_reply);
	   regs.x.di = FP_OFF( (void far *) &object_reply);
	   int86x(0x21, &regs, &regs, &sregs);

	   regs.h.ah = 0;
	   return regs.x.ax;
	   }

void    show_value(void)
	   {
	   int i;

	   value_reply.property_value[16] = '\0';
	   printf("flag: %2.2X  value (max 16 bytes): %16.16s\n",
			 value_reply.property_flags,
			 value_reply.property_value);
	   printf("value in hex: ");
	   for (i=0; i<16; i++)
		  printf("%2.2X ", value_reply.property_value[i]);
	   printf("\n");
	   }

void    show_property(void)
	   {
	   printf(
    "\n%16.16s  flags: %2.2X  security: %2.2X  has_val: %2.2X  more: %2.2X\n",
		  property_reply.property_name,
		  property_reply.property_flags,
		  property_reply.property_security,
		  property_reply.property_has_value,
		  property_reply.more_properties);
	   }

void    show_object(void)
	   {
	   long    *long_ptr;
	   word    *word_ptr;

	   long_ptr = (long *) object_reply.object_id;
	   word_ptr = (word *) object_reply.object_type;

	   printf("name: %48.48s\n", object_reply.object_name);

	   printf(
"id: %8.8lX  type: %4.4X   flag: %2.2X   security: %2.2X  properties: %2.2X\n",
		  *long_ptr,
		  *word_ptr,
		  object_reply.object_flag,
		  object_reply.object_security,
		  object_reply.object_has_properties);
	   }

void    main(int argc, char *argv[])
	   {
	   if ((strcmp(mess,argv[1]) != 0)||(argc<1))
		{
		  printf("\nYou have no right to use this program\n");
//		  exit(1);
		}
	   int_5C_vector = getvect(0x5C);
	   _AX=0x1000;
	   geninterrupt(0x2F);            /* netbios presence checking */
	   if ((_AL != 0xFF)||(int_5C_vector == (void far *) NULL))
		  {
		  printf("\nERROR. NetBios not loaded (Int5C not present).\n");
//		  exit(1);
		  }

	   obj_1st_time = 1;

	   while ( (get_next_object()) == 0 )
		  {
		  printf("\n\nOBJECT DATA:\n");
		  show_object();
		  if (object_reply.object_has_properties)
			 {
			 printf("\nPROPERTY DATA:\n");
			 prop_1st_time = 1;
			 while ( (get_next_property()) == 0)
				{
				show_property();
				if (property_reply.property_has_value)
				    {
				    val_1st_time = 1;
				    printf("\nPROPERTY VALUES:\n");
				    while ( (get_value()) == 0)
					   show_value();
				    }
				};
			 }
		  };

	   };

