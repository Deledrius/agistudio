Qt AGI Studio
=============

Latest release: 1.3.0

About
=====

AGI (Adventure Game Interpreter) is the adventure game engine used by
Sierra On-Line(tm) to create some of their early games. Qt AGI Studio
(formerly known as Linux AGI Studio) is a program which allows you to
view, create and edit AGI games. Basically, it is an enhanced port of
the Windows AGI Studio developed by Peter Kelly.

The program contains very little platform specific code so brave ones
are encouraged to try it on other operating systems that have Qt
library support.


Availability
============

Qt AGI Studio is available at https://github.com/Deledrius/agistudio/
The project is licensed under the GPL (GNU General Public License).
See LICENSE for details.


System requirements
===================

Linux:   GNU make, g++, cmake, X Window System, Qt library
OS X:    GNU make or XCode, g++ or clang, cmake, Qt library
Windows: MinGW or Visual Studio, cmake, Qt library

The program is now being developed with Qt version 6.

NAGI, Sarien or ScummVM AGI interpreter is recommended to run games.
ScummVM is available at https://www.scummvm.org/
Sarien is at https://sourceforge.net/projects/sarien/
NAGI is at http://www.agidev.com/projects/nagi/
Sarien is more focused on portability and NAGI on compatibility
with Sierra's original AGI interpreter. Both are free.
ScummVM is actively developed and is widely cross-platform.


Building
========

CMake is used to create the necessary project files. Run "cmake" in a build
subdirectory of your choice to generate a makefile, Visual Studio Solution,
or XCode project, etc. appropriate for your platform:

 mkdir build;cd build
 cmake ..

If you don't have CMake, you can download it at https://cmake.org/

CMake will detect that Qt6 is installed.


Installation and setup
======================

The binary is called agistudio and will be built in the src subdirectory; you
can copy it to any path convenient to use (e.g. /usr/local/bin). In order to
use the help and an example game template, copy them to any convenient place
and specify the appropriate paths in the "Settings" menu when you'll run
agistudio.

Qt AGI Studio has its own help viewer, but you can also view the help with
any HTML browser.


Using
=====

Please read the online help. Note that if you want to use existing games'
files, all the filenames must be in lower case.


Credits
=======

 * Helen Zommer <helen@cc.huji.ac.il> - primary development
 * Jarno Elonen <elonen@iki.fi> - bitmap import
 * Nat Budin <natb@brandeis.edu> - Win32 port
 * Claudio Matsuoka <claudio@helllabs.org> - sound support
 * Peter Kelly <pmk@post.com> - the original Windows version
 * Lance Ewing <lance.e@ihug.co.nz> - the original (DOS) Picedit
 * Joseph Davies <joseph.davies@zero-factorial.com> - Qt6 Port, current maintainer

Additional fixes provided by:
  Mikael Agren, Chris Cromer, Michael Hansen, Mike Rombout.


Feedback
========

Please visit project website at https://github.com/Deledrius/agistudio/ to
submit bug reports, comments, suggestions, etc.

Hope you'll have as much fun using it as we've had developing it! :-)
