#!/usr/bin/env python
# 
#   skyped.py
#  
#   Copyright (c) 2007, 2008 by Miklos Vajna <vmiklos@frugalware.org>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
# 
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#  
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
#   USA.
#

import sys
import os
import signal
import locale
import time
import gobject
import socket
import getopt
import Skype4Py
import sha
from ConfigParser import ConfigParser
from OpenSSL import SSL
from traceback import print_exception

__version__ = "0.1.1"

SKYPE_SERVICE = 'com.Skype.API'
CLIENT_NAME = 'SkypeApiPythonShell'

def eh(type, value, tb):
	if type != KeyboardInterrupt:
		print_exception(type, value, tb)
	gobject.MainLoop().quit()
	skype.skype.Client.Shutdown()
	sys.exit("Exiting.")

sys.excepthook = eh

def input_handler(fd, io_condition):
	global options
	if options.buf:
		for i in options.buf:
			skype.send(i.strip())
		options.buf = None
	else:
		try:
			input = fd.recv(1024)
		except SysCallError:
			return True
		for i in input.split("\n"):
			skype.send(i.strip())
		return True

def idle_handler(skype):
	try:
		c = skype.skype.Command("PING", Block=True)
		skype.skype.SendCommand(c)
	except Skype4Py.SkypeAPIError, s:
		dprint("Warning, pinging Skype failed (%s)." % (s))
		time.sleep(2)
	return True

def server(host, port):
	global options

	ctx = SSL.Context(SSL.TLSv1_METHOD)
	ctx.use_privatekey_file(options.config.sslkey)
	ctx.use_certificate_file(options.config.sslcert)
	sock = SSL.Connection(ctx, socket.socket())
	sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	sock.bind((host, port))
	sock.listen(1)
	gobject.io_add_watch(sock, gobject.IO_IN, listener)

def listener(sock, *args):
	global options
	options.conn, addr = sock.accept()
	ret = 0
	line = options.conn.recv(1024)
	if line.startswith("USERNAME") and line.split(' ')[1].strip() == options.config.username:
		ret += 1
	line = options.conn.recv(1024)
	if line.startswith("PASSWORD") and sha.sha(line.split(' ')[1].strip()).hexdigest() == options.config.password:
		ret += 1
	if ret == 2:
		dprint("Username and password OK.")
		options.conn.send("PASSWORD OK\n")
		gobject.io_add_watch(options.conn, gobject.IO_IN, input_handler)
		return True
	else:
		dprint("Username and/or password WRONG.")
		options.conn.send("PASSWORD KO\n")
		return False

def dprint(msg):
	global options

	if options.debug:
		print msg

class SkypeApi():
	def __init__(self):
		self.skype = Skype4Py.Skype()
		self.skype.OnNotify = self.recv
		self.skype.Client.Start()

	def recv(self, msg_text):
		global options
		if msg_text == "PONG":
			return
		if "\n" in msg_text:
			# crappy skype prefixes only the first line for
			# multiline messages so we need to do so for the other
			# lines, too. this is something like:
			# 'CHATMESSAGE id BODY first line\nsecond line' ->
			# 'CHATMESSAGE id BODY first line\nCHATMESSAGE id BODY second line'
			prefix = " ".join(msg_text.split(" ")[:3])
			msg_text = ["%s %s" % (prefix, i) for i in " ".join(msg_text.split(" ")[3:]).split("\n")]
		else:
			msg_text = [msg_text]
		for i in msg_text:
			# use utf-8 here to solve the following problem:
			# people use env vars like LC_ALL=en_US (latin1) then
			# they complain about why can't they receive latin2
			# messages.. so here it is: always use utf-8 then
			# everybody will be happy
			e = i.encode('UTF-8')
			dprint('<< ' + e)
			if options.conn:
				try:
					options.conn.send(e + "\n")
				except IOError, s:
					dprint("Warning, sending '%s' failed (%s)." % (e, s))

	def send(self, msg_text):
		if not len(msg_text):
			return
		e = msg_text.decode(locale.getdefaultlocale()[1])
		dprint('>> ' + e)
		try:
			c = self.skype.Command(e, Block=True)
			self.skype.SendCommand(c)
			self.recv(c.Reply)
		except Skype4Py.SkypeError:
			pass
		except Skype4Py.SkypeAPIError, s:
			dprint("Warning, sending '%s' failed (%s)." % (e, s))

class Options:
	def __init__(self):
		self.cfgpath = "/usr/local/etc/skyped/skyped.conf"
		self.daemon = True
		self.debug = False
		self.help = False
		self.host = "0.0.0.0"
		self.port = 2727
		self.version = False
		# well, this is a bit hackish. we store the socket of the last connected client
		# here and notify it. maybe later notify all connected clients?
		self.conn = None
		# this will be read first by the input handler
		self.buf = None


	def usage(self, ret):
		print """Usage: skyped [OPTION]...

skyped is a daemon that acts as a tcp server on top of a Skype instance.

Options:
	-c      --config        path to configuration file (default: %s)
	-d	--debug		enable debug messages
	-h	--help		this help
	-H	--host		set the tcp host (default: %s)
	-n	--nofork	don't run as daemon in the background
	-p	--port		set the tcp port (default: %d)
	-v	--version	display version information""" % (self.cfgpath, self.host, self.port)
		sys.exit(ret)

if __name__=='__main__':
	options = Options()
	try:
		opts, args = getopt.getopt(sys.argv[1:], "c:dhH:np:v", ["config=", "daemon", "help", "host=", "nofork", "port=", "version"])
	except getopt.GetoptError:
		options.usage(1)
	for opt, arg in opts:
		if opt in ("-c", "--config"):
			options.cfgpath = arg
		elif opt in ("-d", "--debug"):
			options.debug = True
		elif opt in ("-h", "--help"):
			options.help = True
		elif opt in ("-H", "--host"):
			options.host = arg
		elif opt in ("-n", "--nofork"):
			options.daemon = False
		elif opt in ("-p", "--port"):
			options.port = arg
		elif opt in ("-v", "--version"):
			options.version = True
	if options.help:
		options.usage(0)
	elif options.version:
		print "skyped %s" % __version__
		sys.exit(0)
	# parse our config
	if not os.path.exists(options.cfgpath):
		print "Can't find configuration file at '%s'." % options.cfgpath
		print "Use the -c option to specify an alternate one."
		sys.exit(1)
	options.config = ConfigParser()
	options.config.read(options.cfgpath)
	options.config.username = options.config.get('skyped', 'username').split('#')[0]
	options.config.password = options.config.get('skyped', 'password').split('#')[0]
	options.config.sslkey = options.config.get('skyped', 'key').split('#')[0]
	options.config.sslcert = options.config.get('skyped', 'cert').split('#')[0]
	dprint("Parsing config file '%s' done, username is '%s'." % (options.cfgpath, options.config.username))
	if options.daemon:
		pid = os.fork()
		if pid == 0:
			nullin = file('/dev/null', 'r')
			nullout = file('/dev/null', 'w')
			os.dup2(nullin.fileno(), sys.stdin.fileno())
			os.dup2(nullout.fileno(), sys.stdout.fileno())
			os.dup2(nullout.fileno(), sys.stderr.fileno())
		else:
			print 'skyped is started on port %s, pid: %d' % (options.port, pid)
			sys.exit(0)
	server(options.host, options.port)
	try:
		skype = SkypeApi()
	except Skype4Py.SkypeAPIError, s:
		sys.exit("%s. Are you sure you have started Skype?" % s)
	gobject.idle_add(idle_handler, skype)
	gobject.MainLoop().run()
