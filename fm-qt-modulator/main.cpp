#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair programming
 *
 *    This file is part of the am modulator
 *
 *    am modulator is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    am modulator is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with am modulator; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include        <unistd.h>
#include        <stdio.h>
#include        <cstring>
#include        <signal.h>
#include        <stdlib.h>
#include        <stdint.h>
#include        <samplerate.h>
#include        <math.h>
#include        <atomic>
#include        <sys/time.h>

#include	<QApplication>
#include	<QDir>
#include	<QSettings>
#include	"fm-modulator.h"

#define DEFAULT_INI     ".fm-modulator.ini"
#define	CURRENT_VERSION	"0.2"
std::atomic<bool>	running;

QString fullPathfor (QString v) {
QString fileName;

	if (v == QString (""))
	   return QString ("/tmp/xxx");

	if (v. at (0) == QChar ('/'))           // full path specified
	   return v;

#ifdef	OSX_INIT_FILE
	char *PathFile;
	PathFile = getenv("HOME");
	fileName = PathFile;
	fileName.append(v);
	qDebug() << fileName;
#else

	fileName = QDir::homePath ();
	fileName. append ("/");
	fileName. append (v);
	fileName = QDir::toNativeSeparators (fileName);
#endif
	if (!fileName. endsWith (".ini"))
	   fileName. append (".ini");

	return fileName;
}
//
static
void    terminate (int num) {

        running. store (false);
        fprintf (stderr,"Caught signal - Terminating %x\n",num);
}
//

void	printHelp	(void) {
	fprintf (stderr, "am modulator\n"
"\toptions are:\n"
"\ti fileName\t: use fileName as ini file\n"
"\tp number\t: use number as port number for the output\n"
"\tr number\t: use number as output sample rate\n"
"\tu string\t: use string as url of the transmitter\n"
"\th\t:print this message\n");
}

int	main (int argc, char **argv) {

QString initFileName = fullPathfor (QString (DEFAULT_INI));
QSettings	*modulatorSettings;           // ini file
QString		url		= QString ("");
fmModulator	*myModulator;
int		opt;
int		port		= -1;
int		rate		= -1;

        QCoreApplication::setOrganizationName ("Lazy Chair Computing");
        QCoreApplication::setOrganizationDomain ("Lazy Chair Computing");
        QCoreApplication::setApplicationName ("hackrf transmitter");
        QCoreApplication::setApplicationVersion (QString (CURRENT_VERSION) + " Git: " + GITHASH);

        while ((opt = getopt (argc, argv, "i:p:r:u:h:")) != -1) {
           switch (opt) {
              case 'i':
                 initFileName = fullPathfor (QString (optarg));
                 break;

	      case 'p':
                 port		= atoi (optarg);
                 break;

	      case 'r':
	         rate		= atoi (optarg);
	         break;

	      case 'u':
	         url		= QString (optarg);
	         break;

	      case 'h':
              default:
	         printHelp ();
	         exit (1);
           }
        }

	for (int i = 0; i < 64; i++) {
	   struct sigaction sa;
	   std::memset (&sa, 0, sizeof(sa));
	   sa. sa_handler = terminate;
	   sigaction (i, &sa, NULL);
	}

	running. store (false);
	modulatorSettings =
	              new QSettings (initFileName, QSettings::IniFormat);
	if (url. isEmpty ())
	   url	= modulatorSettings -> value ("url", "127.0.0.1"). toString ();
	else
	   modulatorSettings -> setValue ("url", url);

	if (port == -1)
	   port = modulatorSettings -> value ("port", 8765). toInt ();
	else
	   modulatorSettings -> setValue ("port", port);

	if (rate == -1)
	   rate = modulatorSettings -> value ("rate", 96000). toInt ();
	else
	   modulatorSettings -> setValue ("rate", rate);

	QApplication a (argc, argv);
	tcpClient theGenerator (port, url);
	try {
	   fprintf (stderr, "trying to connect to %s (%d)\n",
	                                 url. toLatin1 (). data (), port);
           myModulator = new fmModulator (modulatorSettings,
	                                  rate,
	                                  &theGenerator);
	} catch (int e) {
	   fprintf (stderr, "setting up modulator failed, fatal\n");
	   exit (1);
	}
	myModulator -> show ();
        a. exec ();
/*
 *	done:
 */
	fflush (stdout);
        fflush (stderr);
	modulatorSettings	-> sync ();
	delete myModulator;
	delete modulatorSettings;
        qDebug ("It is done\n");
}

