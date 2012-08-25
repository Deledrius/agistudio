Summary: AGI integrated development environment
Name: agistudio
Version: 1.3.0
Release: 0
Copyright: GPL
Group: Development/Tools
Source: agistudio-1.3.0.tar.gz
URL: http://agistudio.sourceforge.net/
%description
AGI (Adventure Game Interpreter) is the adventure game engine used by
Sierra On-Line(tm) to create some of their early games. QT AGI Studio
is a program which allows you to view, create and edit AGI games.
%prep
%setup
%build
cd src
qmake-qt4 agistudio.pro -o Makefile
make RPM_OPT_FLAGS="$RPM_OPT_FLAGS"

%install
mkdir -p /usr/local/agistudio/bin
mkdir -p /usr/local/bin
install -m 755 -o 0 -g 0 src/agistudio /usr/local/agistudio//bin/agistudio
ln -fs /usr/local/agistudio/bin/agistudio /usr/local/bin/agistudio
install -m 755 -d -o 0 -g 0 template /usr/local/agistudio/template
install -m 755 -d -o 0 -g 0 help /usr/local/agistudio/help
install -m 755 -o 0 -g 0 README /usr/local/agistudio/README
install -m 755 -o 0 -g 0 relnotes /usr/local/agistudio/relnotes
cp help/* /usr/local/agistudio/help
cp -r template/* /usr/local/agistudio/template

%files
/usr/local/agistudio/bin/agistudio
/usr/local/bin/agistudio
/usr/local/agistudio/template
/usr/local/agistudio/help
/usr/local/agistudio/relnotes
/usr/local/agistudio/README

