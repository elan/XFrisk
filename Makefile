#
#   XFrisk - The classic board game for X
#   Copyright (C) 1993-1999 Elan Feingold (elan@aetherworks.com)
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
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#   $Id: Makefile,v 1.22 2000/01/23 19:01:55 tony Exp $
#
#   $Log: Makefile,v $
#   Revision 1.22  2000/01/23 19:01:55  tony
#   default to Xaw3d, with comment
#
#   Revision 1.21  2000/01/20 18:03:20  morphy
#   Cleanup, comments to aid the non-initiated and reordering for clarity
#
#   Revision 1.20  2000/01/18 19:55:06  morphy
#   Minor correction to Drop All Armies prevention
#
#   Revision 1.19  2000/01/18 18:56:04  morphy
#   Added note and option: -DNARROWPROTO fixes Xaw scrollbars
#
#   Revision 1.18  2000/01/17 21:11:34  tony
#   fix on dropping armies during fortification, made configurable from here
#

##################
# Language options
# Define only either one, not both
LANGUAGE=ENGLISH
#LANGUAGE=FRENCH


##################
# Gameplay options
# Dropping more than 1 army while fortifying:
# Use this one to disallow dropping all...
DROPALL=
# ... or this one to allow dropping all at once
#DROPALL=-DDROPALL
# The max number of armies to drop is set in game.c, DROP_ARMIES
# which will be ignored it DROPALL is not set

####################
# C compiler options
# GNU gcc is recommended.
CC=gcc
# Use these with gcc
CFLAGS=-g -Wall -W -fno-common -pedantic

# If you don't have gcc, don't use -Wall -W -fno-common
#CFLAGS=-g

# Debugging options - these don't all work
# Assertions catch "shouldn't happen" type errors and immediately terminate the game
# if one is caught
#CFLAGS+=-DASSERTIONS
#
# For debugging memory problems
#CFLAGS+=-DMEM_DEBUG
#
# For logging
#CFLAGS+=-DLOGGING
#
# Test games (quick start)
#CFLAGS+=-DTEST_GAME

## Initial linker options
LDFLAGS=

## Installation prefix
# Adjust to taste. Stuff gets installed here.
PREFIX=/usr/local
#PREFIX=/usr/local/X11

## X11 location and options
# for X11R6
# Point this at your X tree.
XDIR=/usr/X11R6
#XDIR=/usr/local/X11
#XDIR=/usr/openwin

# try Xaw if you don't have Xaw3d
#XAW=Xaw
XAW=Xaw3d

XLIBS=-L$(XDIR)/lib -L/opt/local/lib -l$(XAW) -lXext -lXmu -lXt -lSM -lICE -lX11

XINC=-I$(XDIR)/include
CFLAGS+=$(XINC)

# for X11R5
#XLIBS=-L$(XDIR)/lib -lXaw -lXext -lXmu -lXt -lX11

# System V (Solaris, Irix, etc.) will probably want -lsocket -lnsl.
#LIBS=-lsocket -lnsl
# on other systems leave LIBS blank for now
LIBS=

# On some systems -DNARROWPROTO is needed for working Xaw scrollbars
# This includes FreeBSD 3.x and recent Linux
CFLAGS+=-DNARROWPROTO

# If the link fails because it can't find snprintf, uncomment this.
#COMPAT_OBJS+=snprintf.o

# If you get complaints about ranlib not being found, exchange these.
RANLIB=ranlib
#RANLIB=true

# For HPUX
#CFLAGS+=-Ae

# For Suns with OpenWindows
#CFLAGS+=-I/usr/openwin/include

# On particularly lame systems (e.g., Solaris 2.5) you may need to 
# uncomment this.
#XLIBS+=-Wl,-rpath,$(XDIR)/lib

# For BSD/OS (BSD/OS's ranlib sometimes seems to munch files)
#XLIBS+=-lipc
#RANLIB=true

# For BSD and other nonbroken systems
INSTALL_DATA=install -c -m 644
INSTALL_BIN=install -c -m 755
# For systems without a BSD install
#INSTALL_DATA=cp
#INSTALL_BIN=cp


############################################################
#          You shouldn't need to edit below here           #
############################################################

LIBDIR=$(PREFIX)/lib/xfrisk
BINDIR=$(PREFIX)/bin

# Data files, stored in LIBDIR
MAP=World.risk
COUNTRY=Countries.risk
HELP=Help.risk

# Flags & programs
DEFINES=-D$(LANGUAGE) -DMAPFILE=\"$(LIBDIR)/$(MAP)\" -DCOUNTRYFILE=\"$(LIBDIR)/$(COUNTRY)\" -DHELPFILE=\"$(LIBDIR)/$(HELP)\" $(DROP_ONE)

CFLAGS+=$(DEFINES)

# The targets and directives
all:  xfrisk friskserver aiDummy aiConway aiColson World.risk Countries.risk

# The client program
GAMEOBJS=client.o network.o gui.o callbacks.o utils.o dice.o cards.o \
	 game.o colormap.o help.o riskgame.o debug.o clientMain.o colorEdit.o \
	 registerPlayers.o addPlayer.o viewStats.o viewCards.o viewLog.o \
	 viewChat.o viewFeedback.o

xfrisk: $(GAMEOBJS) $(COMPAT_OBJS)
	$(CC) $(LDFLAGS) $^ $(XINC) $(XLIBS) $(LIBS) -o $@

# The server program
SERVEROBJS=server.o network.o deck.o riskgame.o debug.o clients.o serverMain.o

# these could be compiled without X as soon as callbacks gets fixed :-)
# right now compile will break if the X include files are not found
friskserver: $(SERVEROBJS) $(COMPAT_OBJS)
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

ai: aiDummy aiConway aiColson


# The map building program
buildmap: buildmap.o debug.o
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

# The architecture analysis program
findtypes: findtypes.o
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

# A framework for a computer player
aiDummy: aiDummy.o libfriskAI.a
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

# A real computer player, by Andrew Conway
aiConway: aiConway.o libfriskAI.a
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

# A real computer player, by Jean-Claude Colson
aiColson: aiColson.o libfriskAI.a
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

# The computer player library
libfriskAI.a: network.o aiDice.o game.o riskgame.o debug.o aiClientMain.o \
		aiController.o aiStubs.o $(COMPAT_OBJS)
	$(AR) -cruv $@ $^
	$(RANLIB) $@

install: all
	-(umask 022; mkdir -p -m755 $(LIBDIR) $(BINDIR) $(APPLOADDIR))
	$(INSTALL_DATA) $(MAP) $(LIBDIR)
	$(INSTALL_DATA) $(COUNTRY) $(LIBDIR)
	$(INSTALL_DATA) $(HELP) $(LIBDIR)
	$(INSTALL_BIN) risk $(BINDIR)
	$(INSTALL_BIN) xfrisk $(BINDIR)
	$(INSTALL_BIN) friskserver $(BINDIR)
	$(INSTALL_BIN) aiDummy $(BINDIR)
	$(INSTALL_BIN) aiConway $(BINDIR)
	$(INSTALL_BIN) aiColson $(BINDIR)


# Other targets
Countries.risk World.risk: World.ppm buildmap
	./buildmap World.ppm World.risk Countries.risk

uninstall:
	rm -rf $(LIBDIR)
	rm -f  $(BINDIR)/risk
	rm -f  $(BINDIR)/xfrisk
	rm -f  $(BINDIR)/friskserver
	rm -f  $(BINDIR)/aiDummy
	rm -f  $(BINDIR)/aiConway
	rm -f  $(BINDIR)/aiColson

types.h: findtypes
	./findtypes

version:
	touch version.h

include depend.mk
depend: types.h
	$(CC) $(CFLAGS) -MM *.c >depend.mk

depend.mk:
	touch depend.mk
	$(MAKE) depend

tidy:
	rm -f *.log *~ core

clean: tidy
	rm -f *.o *.a findtypes types.h buildmap Countries.risk World.risk

distclean: clean
	rm -f aiDummy aiConway aiColson xfrisk friskserver depend.mk core

check:
	find . -type f \! -perm 444 \! -perm 555 -print

