TARGET	= mx6eapp mx6ectl

COM_SRCS = \
	mx6eapp_log.c \
	mx6eapp_socket.c \
	mx6eapp_util.c \
	mx6ectl_command.c \
	mx6eapp_pt.c \
	mx6eapp_network.c \
	mx6eapp_netlink.c \

APP_SRCS = \
	mx6eapp_main.c \
	mx6eapp_config.c \
	mx6eapp_tunnel.c \
	mx6eapp_pt_mainloop.c \
	mx6eapp_setup.c \
	mx6eapp_print_packet.c \
	mx6eapp_statistics.c \
	mx6eapp_dynamic_setting.c \
	mx6eapp_ct.c \

CTL_SRCS = \
	mx6ectl.c \

COM_OBJS = $(COM_SRCS:.c=.o)
APP_OBJS = $(APP_SRCS:.c=.o)
CTL_OBJS = $(CTL_SRCS:.c=.o)

OBJS	= $(COM_OBJS) $(APP_OBJS) $(CTL_OBJS)
DEPENDS	= $(COM_SRCS:.c=.d) $(APP_SRCS:.c=.d) $(CTL_SRCS:.c=.d)
LIBS	= -lpthread -lrt

CC	= gcc
# for release flag
CFLAGS	= -O2 -Wall -std=gnu99 -D_GNU_SOURCE
# for debug flag
#CFLAGS	= -Wall -std=gnu99 -D_GNU_SOURCE -DDEBUG -g
# for CT
#CFLAGS	= -DCT -Wall -std=gnu99 -D_GNU_SOURCE -DDEBUG -g
# for debug flag
#CFLAGS	= -O2 -Wall -std=gnu99 -D_GNU_SOURCE -DDEBUG_SYNC -g
INCDIR	= -I.
LD	= gcc
LDFLAGS	= 
LIBDIR	=

all: $(TARGET)

cleanall:
	rm -f $(OBJS) $(DEPENDS) $(TARGET)

clean:
	rm -f $(OBJS) $(DEPENDS)

cppcheck:
	cppcheck --enable=all --template='gcc' --force -I. -I`gcc --print-file-name=include` -I/usr/include  $(APP_SRCS) $(COM_SRCS) $(CTL_SRCS)
	#cppcheck --enable=all --template='gcc' --check-config -I. -I`gcc --print-file-name=include` -I/usr/include  $(APP_SRCS) $(COM_SRCS) $(CTL_SRCS)

mx6eapp: $(COM_OBJS) $(APP_OBJS)
	$(LD) $(LIBDIR) $(LDFLAGS) -o $@ $(COM_OBJS) $(APP_OBJS) $(LIBS)

mx6ectl: $(COM_OBJS) $(CTL_OBJS)
	$(LD) $(LIBDIR) $(LDFLAGS) -o $@ $(COM_OBJS) $(CTL_OBJS)

.c.o:
	$(CC) $(INCDIR) $(CFLAGS) -c $<

%.d: %.c
	@set -e; $(CC) -MM $(CINCS) $(CFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
		[ -s $@ ] || rm -f $@

-include $(DEPENDS)
