#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair programming
 *
 *    This file is part of the hackrf transmitter
 *
 *    hackrf transmitter is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    hackrf transmitter is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with hackrf transmitter; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	<QApplication>
#include	<QDir>
#include	<QSettings>
#include	"transmitter-constants.h"
#include	"transmitter.h"

#define DEFAULT_INI     ".hackrf-transmitter.ini"


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
	fileName.append("/.qt-dab.ini");
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
//	to do:
//	adding handling parameters, i.e. port, url rate
//

void	printHelp	(void) {
	fprintf (stderr, "hackrf iq transmitter\n"
"\toptions are:\n"
"\ti fileName\t: use fileName as ini file\n"
"\tf number\t: use number as initial frequency setting (in KHz)\n"
"\tp number\t: use number as port number for the input\n"
"\tr number\t: use number as input sample rate\n"
"\tu number\t: upswing factor to get a reasonable transmissionrate\n"
"\th\t:print this message\n");
}

int	main (int argc, char **argv) {
Transmitter	*myTransmitter;
QString initFileName = fullPathfor (QString (DEFAULT_INI));
QSettings	*transmitterSettings;           // ini file
int	opt;
int	port		= -1;
int	vfoFrequency	= -1;
int	rate		= -1;
int	upFactor	= 1;
int	transmissionRate;

        QCoreApplication::setOrganizationName ("Lazy Chair Computing");
        QCoreApplication::setOrganizationDomain ("Lazy Chair Computing");
        QCoreApplication::setApplicationName ("hackrf transmitter");
        QCoreApplication::setApplicationVersion (QString (CURRENT_VERSION) + " Git: " + GITHASH);

        while ((opt = getopt (argc, argv, "i:f:p:r:h:")) != -1) {
           switch (opt) {
              case 'i':
                 initFileName = fullPathfor (QString (optarg));
                 break;

              case 'f':
	         vfoFrequency	= atoi (optarg);
	         break;

	      case 'p':
                 port		= atoi (optarg);
                 break;

	      case 'r':
	         rate		= atoi (optarg);
	         break;

	      case 'u':
	         upFactor	= atoi (optarg);
	         break;

	      case 'h':
              default:
	         printHelp ();
	         exit (1);
           }
        }

	transmitterSettings =
	              new QSettings (initFileName, QSettings::IniFormat);

	if (vfoFrequency == -1)
	   vfoFrequency =
	          transmitterSettings -> value ("vfo", MHz (110)). toInt ();
	if (port == -1)
	   port = transmitterSettings -> value ("port", 8765). toInt ();
	else
	   transmitterSettings -> setValue ("port", port);
	if (rate == -1)
	   rate = transmitterSettings -> value ("rate", 96000). toInt ();
	else
	   transmitterSettings -> setValue ("rate", rate);

	if (upFactor * rate < 4 * 96000)
	   transmissionRate	= 4 * 96000;
	else
	   transmissionRate	= upFactor * rate;

	QApplication a (argc, argv);
	try {
           myTransmitter = new Transmitter (transmitterSettings,
	                                    vfoFrequency,
	                                    port,
	                                    rate,
	                                    transmissionRate);
	} catch (int e) {
	   fprintf (stderr, "setting up transmitter failed, fatal\n");
	   exit (1);
	}
	myTransmitter -> show ();
        a. exec ();
/*
 *	done:
 */
	fflush (stdout);
        fflush (stderr);
	transmitterSettings	-> sync ();
	delete myTransmitter;
	delete transmitterSettings;
        qDebug ("It is done\n");
}

