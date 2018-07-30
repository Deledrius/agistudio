/*
 *  QT AGI Studio :: Copyright (C) 2000 Helen Zommer
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef AGI_COMMANDS_H
#define AGI_COMMANDS_H

//argument types
#define atNum  0
#define atVar  1
#define atFlag 2
#define atMsg  3
#define atSObj 4
#define atIObj 5
#define atStr  6
#define atWord 7
#define atCtrl 8

extern const char *ArgTypePrefix[9] ;
extern const char *ArgTypeName[9] ;

typedef struct {
    const char *Name;
    char NumArgs;
    char argTypes[7];
} CommandStruct;

#define NumTestCommands 18
extern int NumAGICommands;   //changes in different AGI interpreter versions

extern CommandStruct TestCommand[19];  //tests for different flags, etc
extern CommandStruct AGICommand[182];  //all the other


extern void  CorrectCommands(long VerNum);

#endif
