  /********************************************************************\
  * BitlBee -- An IRC to other IM-networks gateway                     *
  *                                                                    *
  * Copyright 2002-2004 Wilmer van der Gaast and others                *
  \********************************************************************/

/* Main file                                                            */

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License with
  the Debian GNU/Linux distribution in /usr/share/common-licenses/GPL;
  if not, write to the Free Software Foundation, Inc., 59 Temple Place,
  Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _BITLBEE_H
#define _BITLBEE_H

#define _GNU_SOURCE /* Stupid GNU :-P */

#define PACKAGE "BitlBee"
#define BITLBEE_VERSION "1.1dev"
#define VERSION BITLBEE_VERSION

#define MAX_STRING 128

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#ifndef _WIN32
#include <syslog.h>
#include <errno.h>
#endif

#include <glib.h>
#include <gmodule.h>

/* The following functions should not be used if we want to maintain Windows compatibility... */
#undef free
#define free		__PLEASE_USE_THE_GLIB_MEMORY_ALLOCATION_SYSTEM__
#undef malloc
#define malloc		__PLEASE_USE_THE_GLIB_MEMORY_ALLOCATION_SYSTEM__
#undef calloc
#define calloc		__PLEASE_USE_THE_GLIB_MEMORY_ALLOCATION_SYSTEM__
#undef realloc
#define realloc		__PLEASE_USE_THE_GLIB_MEMORY_ALLOCATION_SYSTEM__
#undef strdup
#define strdup		__PLEASE_USE_THE_GLIB_STRDUP_FUNCTIONS_SYSTEM__
#undef strndup
#define strndup		__PLEASE_USE_THE_GLIB_STRDUP_FUNCTIONS_SYSTEM__
#undef snprintf
#define snprintf	__PLEASE_USE_G_SNPRINTF__
#undef strcasecmp
#define strcasecmp	__PLEASE_USE_G_STRCASECMP__
#undef strncasecmp
#define strncasecmp	__PLEASE_USE_G_STRNCASECMP__

/* And the following functions shouldn't be used anymore to keep compatibility
   with other event handling libs than GLib. */
#undef g_timeout_add
#define g_timeout_add		__PLEASE_USE_B_TIMEOUT_ADD__
#undef g_timeout_add_full
#define g_timeout_add_full	__PLEASE_USE_B_TIMEOUT_ADD__
#undef g_io_add_watch
#define g_io_add_watch		__PLEASE_USE_B_INPUT_ADD__
#undef g_io_add_watch_full
#define g_io_add_watch_full	__PLEASE_USE_B_INPUT_ADD__
#undef g_source_remove
#define g_source_remove		__PLEASE_USE_B_EVENT_REMOVE__
#undef g_source_remove_by_user_data
#define g_source_remove_by_user_data	__PLEASE_USE_B_SOURCE_REMOVE_BY_USER_DATA__
#undef g_main_run
#define g_main_run		__PLEASE_USE_B_MAIN_RUN__
#undef g_main_quit
#define g_main_quit		__PLEASE_USE_B_MAIN_QUIT__

#ifndef F_OK
#define F_OK 0
#endif

#define _( x ) x

#define ROOT_NICK "root"
#define ROOT_CHAN "&bitlbee"
#define ROOT_FN "User manager"

#define NS_NICK "NickServ"

#define DEFAULT_AWAY "Away from computer"
#define CONTROL_TOPIC "Welcome to the control channel. Type \2help\2 for help information."
#define IRCD_INFO "BitlBee <http://www.bitlbee.org/>"

#define MAX_NICK_LENGTH 24

#define HELP_FILE VARDIR "help.txt"
#define CONF_FILE_DEF ETCDIR "bitlbee.conf"

extern char *CONF_FILE;

#include "irc.h"
#include "storage.h"
#include "set.h"
#include "nogaim.h"
#include "commands.h"
#include "account.h"
#include "nick.h"
#include "conf.h"
#include "log.h"
#include "ini.h"
#include "help.h"
#include "query.h"
#include "sock.h"
#include "misc.h"
#include "proxy.h"

typedef struct global {
	/* In forked mode, child processes store the fd of the IPC socket here. */
	int listen_socket;
	gint listen_watch_source_id;
	help_t *help;
	conf_t *conf;
	GList *storage; /* The first backend in the list will be used for saving */
	char *helpfile;
	int restart;
} global_t;

int bitlbee_daemon_init( void );
int bitlbee_inetd_init( void );

gboolean bitlbee_io_current_client_read( gpointer data, gint source, b_input_condition cond );
gboolean bitlbee_io_current_client_write( gpointer data, gint source, b_input_condition cond );

void root_command_string( irc_t *irc, user_t *u, char *command, int flags );
void root_command( irc_t *irc, char *command[] );
gboolean bitlbee_shutdown( gpointer data, gint fd, b_input_condition cond );

extern global_t global;

#endif
