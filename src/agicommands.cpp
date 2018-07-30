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

#include "agicommands.h"


const char *ArgTypePrefix[9] = {"", "v", "f", "m", "o", "i", "s", "w", "c"};
const char *ArgTypeName[9] = {"number", "var", "flag", "message", "object",
                              "inventory item", "string", "word", "controller"};


CommandStruct TestCommand[19] = {
    { "",  0, { 0, 0, 0, 0, 0, 0, 0 } }, //dummy
    { "equaln", 2, { atVar, atNum, 0, 0, 0, 0, 0 } },
    { "equalv", 2, { atVar, atVar, 0, 0, 0, 0, 0 } },
    { "lessn", 2, { atVar, atNum, 0, 0, 0, 0, 0 } },
    { "lessv", 2, { atVar, atVar, 0, 0, 0, 0, 0 } },
    { "greatern", 2, { atVar, atNum, 0, 0, 0, 0, 0 } },
    { "greaterv", 2, { atVar, atVar, 0, 0, 0, 0, 0 } },
    { "isset", 1, { atFlag, 0, 0, 0, 0, 0, 0 } },
    { "issetv", 1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "has", 1, { atIObj, 0, 0, 0, 0, 0, 0 } },
    { "obj.in.room", 2, { atIObj, atVar, 0, 0, 0, 0, 0 } },
    { "posn", 5, { atSObj, atNum, atNum, atNum, atNum, 0, 0 } },
    { "controller", 1, { atCtrl, 0, 0, 0, 0, 0, 0 } },
    { "have.key", 0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "said", 0, { 0, 0, 0, 0, 0, 0, 0 } }, // special command so we don't need to set the right argument types for it
    { "compare.strings", 2, { atStr, atStr, 0, 0, 0, 0, 0 } },
    { "obj.in.box", 5, { atSObj, atNum, atNum, atNum, atNum, 0, 0 } },
    { "center.posn", 5, { atSObj, atNum, atNum, atNum, atNum, 0, 0 } },
    { "right.posn", 5, { atSObj, atNum, atNum, atNum, atNum, 0, 0 } }
};


CommandStruct AGICommand[182] = {

    { "return",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "increment",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "decrement",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "assignn",  2, { atVar, atNum, 0, 0, 0, 0, 0 } },
    { "assignv",  2, { atVar, atVar, 0, 0, 0, 0, 0 } },
    { "addn",  2, { atVar, atNum, 0, 0, 0, 0, 0 } },
    { "addv",  2, { atVar, atVar, 0, 0, 0, 0, 0 } },
    { "subn",  2, { atVar, atNum, 0, 0, 0, 0, 0 } },
    { "subv",  2, { atVar, atVar, 0, 0, 0, 0, 0 } },
    { "lindirectv",  2, { atVar, atVar, 0, 0, 0, 0, 0 } },
    { "rindirect",  2, { atVar, atVar, 0, 0, 0, 0, 0 } },
    { "lindirectn",  2, { atVar, atNum, 0, 0, 0, 0, 0 } },
    { "set",  1, { atFlag, 0, 0, 0, 0, 0, 0 } },
    { "reset",  1, { atFlag, 0, 0, 0, 0, 0, 0 } },
    { "toggle",  1, { atFlag, 0, 0, 0, 0, 0, 0 } },
    { "set.v",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "reset.v",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "toggle.v",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "new.room",  1, { atNum, 0, 0, 0, 0, 0, 0 } },
    { "new.room.v",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "load.logics",  1, { atNum, 0, 0, 0, 0, 0, 0 } },
    { "load.logics.v",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "call",  1, { atNum, 0, 0, 0, 0, 0, 0 } },
    { "call.v",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "load.pic",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "draw.pic",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "show.pic",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "discard.pic",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "overlay.pic",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "show.pri.screen",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "load.view",  1, { atNum, 0, 0, 0, 0, 0, 0 } },
    { "load.view.v",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "discard.view",  1, { atNum, 0, 0, 0, 0, 0, 0 } },
    { "animate.obj",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "unanimate.all",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "draw",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "erase",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "position",  3, { atSObj, atNum, atNum, 0, 0, 0, 0 } },
    { "position.v",  3, { atSObj, atVar, atVar, 0, 0, 0, 0 } },
    { "get.posn",  3, { atSObj, atVar, atVar, 0, 0, 0, 0 } },
    { "reposition",  3, { atSObj, atVar, atVar, 0, 0, 0, 0 } },
    { "set.view",  2, { atSObj, atNum, 0, 0, 0, 0, 0 } },
    { "set.view.v",  2, { atSObj, atVar, 0, 0, 0, 0, 0 } },
    { "set.loop",  2, { atSObj, atNum, 0, 0, 0, 0, 0 } },
    { "set.loop.v",  2, { atSObj, atVar, 0, 0, 0, 0, 0 } },
    { "fix.loop",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "release.loop",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "set.cel",  2, { atSObj, atNum, 0, 0, 0, 0, 0 } },
    { "set.cel.v",  2, { atSObj, atVar, 0, 0, 0, 0, 0 } },
    { "last.cel",  2, { atSObj, atVar, 0, 0, 0, 0, 0 } },
    { "current.cel",  2, { atSObj, atVar, 0, 0, 0, 0, 0 } },
    { "current.loop",  2, { atSObj, atVar, 0, 0, 0, 0, 0 } },
    { "current.view",  2, { atSObj, atVar, 0, 0, 0, 0, 0 } },
    { "number.of.loops",  2, { atSObj, atVar, 0, 0, 0, 0, 0 } },
    { "set.priority",  2, { atSObj, atNum, 0, 0, 0, 0, 0 } },
    { "set.priority.v",  2, { atSObj, atVar, 0, 0, 0, 0, 0 } },
    { "release.priority",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "get.priority",  2, { atSObj, atVar, 0, 0, 0, 0, 0 } },
    { "stop.update",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "start.update",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "force.update",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "ignore.horizon",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "observe.horizon",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "set.horizon",  1, { atNum, 0, 0, 0, 0, 0, 0 } },
    { "object.on.water",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "object.on.land",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "object.on.anything",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "ignore.objs",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "observe.objs",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "distance",  3, { atSObj, atSObj, atVar, 0, 0, 0, 0 } },
    { "stop.cycling",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "start.cycling",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "normal.cycle",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "end.of.loop",  2, { atSObj, atFlag, 0, 0, 0, 0, 0 } },
    { "reverse.cycle", 1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "reverse.loop",  2, { atSObj, atFlag, 0, 0, 0, 0, 0 } },
    { "cycle.time",  2, { atSObj, atVar, 0, 0, 0, 0, 0 } }, // arg2 Num???
    { "stop.motion",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "start.motion",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "step.size",  2, { atSObj, atVar, 0, 0, 0, 0, 0 } }, // arg2 Num???
    { "step.time",  2, { atSObj, atVar, 0, 0, 0, 0, 0 } }, // arg2 Num???
    { "move.obj",  5, { atSObj, atNum, atNum, atNum, atFlag, 0, 0 } },
    { "move.obj.v",  5, { atSObj, atVar, atVar, atNum, atFlag, 0, 0 } }, // arg 3 var???
    { "follow.ego",  3, { atSObj, atNum, atFlag, 0, 0, 0, 0 } },
    { "wander",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "normal.motion",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "set.dir",  2, { atSObj, atVar, 0, 0, 0, 0, 0 } },
    { "get.dir",  2, { atSObj, atVar, 0, 0, 0, 0, 0 } },
    { "ignore.blocks",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "observe.blocks",  1, { atSObj, 0, 0, 0, 0, 0, 0 } },
    { "block",  4, { atNum, atNum, atNum, atNum, 0, 0, 0 } },
    { "unblock",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "get",  1, { atIObj, 0, 0, 0, 0, 0, 0 } },
    { "get.v",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "drop",  1, { atIObj, 0, 0, 0, 0, 0, 0 } },
    { "put",  2, { atIObj, atVar, 0, 0, 0, 0, 0 } },
    { "put.v",  2, { atVar, atVar, 0, 0, 0, 0, 0 } },
    { "get.room.v",  2, { atVar, atVar, 0, 0, 0, 0, 0 } },
    { "load.sound",  1, { atNum, 0, 0, 0, 0, 0, 0 } },
    { "sound",  2, { atNum, atFlag, 0, 0, 0, 0, 0 } },
    { "stop.sound",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "print",  1, { atMsg, 0, 0, 0, 0, 0, 0 } },
    { "print.v",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "display",  3, { atNum, atNum, atMsg, 0, 0, 0, 0 } },
    { "display.v",  3, { atVar, atVar, atVar, 0, 0, 0, 0 } },
    { "clear.lines",  3, { atNum, atNum, atNum, 0, 0, 0, 0 } },
    { "text.screen",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "graphics",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "set.cursor.char",  1, { atMsg, 0, 0, 0, 0, 0, 0 } },
    { "set.text.attribute",  2, { atNum, atNum, 0, 0, 0, 0, 0 } },
    { "shake.screen",  1, { atNum, 0, 0, 0, 0, 0, 0 } },
    { "configure.screen",  3, { atNum, atNum, atNum, 0, 0, 0, 0 } },
    { "status.line.on",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "status.line.off",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "set.string",  2, { atStr, atMsg, 0, 0, 0, 0, 0 } },
    { "get.string",  5, { atStr, atMsg, atNum, atNum, atNum, 0, 0 } },
    { "word.to.string",  2, { atWord, atStr, 0, 0, 0, 0, 0 } },
    { "parse",  1, { atStr, 0, 0, 0, 0, 0, 0 } },
    { "get.num",  2, { atMsg, atVar, 0, 0, 0, 0, 0 } },
    { "prevent.input",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "accept.input",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "set.key",  3, { atNum, atNum, atCtrl, 0, 0, 0, 0 } },
    { "add.to.pic",  7, { atNum, atNum, atNum, atNum, atNum, atNum, atNum } },
    { "add.to.pic.v",  7, { atVar, atVar, atVar, atVar, atVar, atVar, atVar } },
    { "status",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "save.game",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "restore.game",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "init.disk",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "restart.game",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "show.obj",  1, { atNum, 0, 0, 0, 0, 0, 0 } },
    { "random",  3, { atNum, atNum, atVar, 0, 0, 0, 0 } },
    { "program.control",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "player.control",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "obj.status.v",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "quit",  1, { atNum, 0, 0, 0, 0, 0, 0 } }, // 0 args for 2.089
    { "show.mem",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "pause",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "echo.line",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "cancel.line",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "init.joy",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "toggle.monitor",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "version",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "script.size",  1, { atNum, 0, 0, 0, 0, 0, 0 } },
    { "set.game.id",  1, { atMsg, 0, 0, 0, 0, 0, 0 } },
    { "log",  1, { atMsg, 0, 0, 0, 0, 0, 0 } },
    { "set.scan.start",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "reset.scan.start",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "reposition.to",  3, { atSObj, atNum, atNum, 0, 0, 0, 0 } },
    { "reposition.to.v",  3, { atSObj, atVar, atVar, 0, 0, 0, 0 } },
    { "trace.on",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "trace.info",  3, { atNum, atNum, atNum, 0, 0, 0, 0 } },
    { "print.at",  4, { atMsg, atNum, atNum, atNum, 0, 0, 0 } }, // 3 args before 2.440
    { "print.at.v",  4, { atMsg, atVar, atVar, atVar, 0, 0, 0 } }, // 3 args before 2.440
    { "discard.view.v",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "clear.text.rect",  5, { atNum, atNum, atNum, atNum, atNum, 0, 0 } },
    { "set.upper.left",  2, { 0, 0, 0, 0, 0, 0, 0 } }, // ?????
    { "set.menu",  1, { atMsg, 0, 0, 0, 0, 0, 0 } },
    { "set.menu.item",  2, { atMsg, atCtrl, 0, 0, 0, 0, 0 } },
    { "submit.menu",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "enable.item",  1, { atCtrl, 0, 0, 0, 0, 0, 0 } },
    { "disable.item",  1, { atCtrl, 0, 0, 0, 0, 0, 0 } },
    { "menu.input",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "show.obj.v",  1, { atVar, 0, 0, 0, 0, 0, 0 } },
    { "open.dialogue",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "close.dialogue",  0, { 0, 0, 0, 0, 0, 0, 0 } },
    { "mul.n",  2, { atVar, atNum, 0, 0, 0, 0, 0 } },
    { "mul.v",  2, { atVar, atVar, 0, 0, 0, 0, 0 } },
    { "div.n",  2, { atVar, atNum, 0, 0, 0, 0, 0 } },
    { "div.v",  2, { atVar, atVar, 0, 0, 0, 0, 0 } },
    { "close.window",  0, { 0, 0, 0, 0, 0, 0, 0 } },

// The formerly unknown commands
    { "set.simple", 1, {0, 0, 0, 0, 0, 0, 0 } }, // unknown 170
    { "push.script", 0, {0, 0, 0, 0, 0, 0, 0 } }, // unknown 171
    { "pop.script", 0, {0, 0, 0, 0, 0, 0, 0 } }, // unknown 172
    { "hold.key", 0, {0, 0, 0, 0, 0, 0, 0 } }, // unknown 173
    { "set.pri.base", 1, {0, 0, 0, 0, 0, 0, 0 } }, // unknown 174
    { "discard.sound", 1, {0, 0, 0, 0, 0, 0, 0 } }, // unknown 175
    { "hide.mouse", 0, {0, 0, 0, 0, 0, 0, 0 } }, // unknown 176 - 1 arg for 3.002.086
    { "allow.menu", 1, {0, 0, 0, 0, 0, 0, 0 } }, // unknown 177
    { "show.mouse", 0, {0, 0, 0, 0, 0, 0, 0 } }, // unknown 178
    { "fence.mouse", 4, {0, 0, 0, 0, 0, 0, 0 } }, // unknown 179
    { "mouse.posn", 2, {0, 0, 0, 0, 0, 0, 0 } }, // unknown 180
    { "release.key", 0, {0, 0, 0, 0, 0, 0, 0 } } // unknown 181
};


int NumAGICommands  = 181; // not counting return()

void CorrectCommands(long VerNum)
{
    if (VerNum <= 2089000) {
        AGICommand[134].NumArgs = 0;  //quit
    }
    if (VerNum < 2400000) {
        AGICommand[151].NumArgs = 3;  // print.at
        AGICommand[152].NumArgs = 3;  // print.at.v
    }
    if (VerNum <= 3002086)
        AGICommand[176].NumArgs = 1;
    if (VerNum <= 2089000)
        NumAGICommands = 155;
    else if (VerNum <= 2272000)
        NumAGICommands = 161;
    else if (VerNum <= 2440000)
        NumAGICommands = 169;
    else if (VerNum <= 2917000)
        NumAGICommands = 173;
    else if (VerNum <= 2936000)
        NumAGICommands = 175;
    else if (VerNum <= 3002086)
        NumAGICommands = 177;
    else
        NumAGICommands = 181;
}
