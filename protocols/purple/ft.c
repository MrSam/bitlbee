/***************************************************************************\
*                                                                           *
*  BitlBee - An IRC to IM gateway                                           *
*  libpurple module - File transfer stuff                                   *
*                                                                           *
*  Copyright 2009-2010 Wilmer van der Gaast <wilmer@gaast.net>              *
*                                                                           *
*  This program is free software; you can redistribute it and/or modify     *
*  it under the terms of the GNU General Public License as published by     *
*  the Free Software Foundation; either version 2 of the License, or        *
*  (at your option) any later version.                                      *
*                                                                           *
*  This program is distributed in the hope that it will be useful,          *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*  GNU General Public License for more details.                             *
*                                                                           *
*  You should have received a copy of the GNU General Public License along  *
*  with this program; if not, write to the Free Software Foundation, Inc.,  *
*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.              *
*                                                                           *
\***************************************************************************/

/* Do file transfers via disk for now, since libpurple was really designed
   for straight-to/from disk fts and is only just learning how to pass the
   file contents the the UI instead (2.6.0 and higher it seems, and with
   varying levels of success). */

#include "bitlbee.h"

#include <stdarg.h>

#include <glib.h>
#include <purple.h>

struct prpl_xfer_data
{
	PurpleXfer *xfer;
	file_transfer_t *ft;
	gint ready_timer;
	char *buf;
	int buf_len;
};

static file_transfer_t *next_ft;

struct im_connection *purple_ic_by_pa( PurpleAccount *pa );

static gboolean prpl_xfer_write_request( struct file_transfer *ft )
{
	return FALSE;
}

static gboolean prpl_xfer_write( struct file_transfer *ft, char *buffer, unsigned int len )
{
	struct prpl_xfer_data *px = ft->data;
	
	return FALSE;
}

static void prpl_xfer_accept( struct file_transfer *ft )
{
	struct prpl_xfer_data *px = ft->data;
	purple_xfer_request_accepted( px->xfer, NULL );
	prpl_xfer_write_request( ft );
}

static void prpl_xfer_canceled( struct file_transfer *ft, char *reason )
{
	struct prpl_xfer_data *px = ft->data;
	purple_xfer_request_denied( px->xfer );
}

static gboolean prplcb_xfer_new_send_cb( gpointer data, gint fd, b_input_condition cond );

static void prplcb_xfer_new( PurpleXfer *xfer )
{
	if( purple_xfer_get_type( xfer ) == PURPLE_XFER_RECEIVE )
	{
		/* This should suppress the stupid file dialog. */
		purple_xfer_set_local_filename( xfer, "/tmp/wtf123" );
		
		/* Sadly the xfer struct is still empty ATM so come back after
		   the caller is done. */
		b_timeout_add( 0, prplcb_xfer_new_send_cb, xfer );
	}
	else
	{
		/*
		struct prpl_xfer_data *px = g_new0( struct prpl_xfer_data, 1 );
		
		px->ft = next_ft;
		px->ft->data = px;
		px->xfer = xfer;
		px->xfer->ui_data = px;
		
		purple_xfer_set_filename( xfer, px->ft->file_name );
		purple_xfer_set_size( xfer, px->ft->file_size );
		
		next_ft = NULL;
		*/
	}
}

static gboolean prplcb_xfer_new_send_cb( gpointer data, gint fd, b_input_condition cond )
{
	PurpleXfer *xfer = data;
	struct im_connection *ic = purple_ic_by_pa( xfer->account );
	struct prpl_xfer_data *px = g_new0( struct prpl_xfer_data, 1 );
	PurpleBuddy *buddy;
	const char *who;
	
	buddy = purple_find_buddy( xfer->account, xfer->who );
	who = buddy ? purple_buddy_get_name( buddy ) : xfer->who;
	
	/* TODO(wilmer): After spreading some more const goodness in BitlBee,
	   remove the evil cast below. */
	px->ft = imcb_file_send_start( ic, (char*) who, xfer->filename, xfer->size );
	px->ft->data = px;
	px->xfer = data;
	px->xfer->ui_data = px;
	
	px->ft->accept = prpl_xfer_accept;
	px->ft->canceled = prpl_xfer_canceled;
	px->ft->write_request = prpl_xfer_write_request;
	
	return FALSE;
}

static void prplcb_xfer_progress( PurpleXfer *xfer, double percent )
{
	fprintf( stderr, "prplcb_xfer_dbg 0x%p %f\n", xfer, percent );
}

static void prplcb_xfer_dbg( PurpleXfer *xfer )
{
	fprintf( stderr, "prplcb_xfer_dbg 0x%p\n", xfer );
}

PurpleXferUiOps bee_xfer_uiops =
{
	prplcb_xfer_new,
	prplcb_xfer_dbg,
	prplcb_xfer_dbg,
	prplcb_xfer_progress,
	prplcb_xfer_dbg,
	prplcb_xfer_dbg,
	NULL,
	NULL,
	prplcb_xfer_dbg,
};

static gboolean prplcb_xfer_send_cb( gpointer data, gint fd, b_input_condition cond );

void purple_transfer_request( struct im_connection *ic, file_transfer_t *ft, char *handle )
{
	PurpleAccount *pa = ic->proto_data;
	struct prpl_xfer_data *px;
	
	/* xfer_new() will pick up this variable. It's a hack but we're not
	   multi-threaded anyway. */
	next_ft = ft;
	serv_send_file( purple_account_get_connection( pa ), handle, ft->file_name );
	
	ft->write = prpl_xfer_write;
	
	px = ft->data;
	imcb_file_recv_start( ft );
}
