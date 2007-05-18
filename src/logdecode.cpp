/*
 *  QT AGI Studio :: Copyright (C) 2000 Helen Zommer
 *
 *  Almost all of this code was adapted from the Windows AGI Studio 
 *  developed by Peter Kelly.
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

#include "game.h"
#include "logedit.h"
#include "words.h"
#include "object.h"
#include "menu.h"
#include "agicommands.h"

#include <cstdlib>
#include <stdio.h>

static int EncryptionStart;
static int MessageSectionStart,MessageSectionEnd;
static bool MessageUsed[256],MessageExists[256];
static int ResPos;
static byte CurByte;
static int NumMessages;

static string Messages[MaxMessages];

static byte ThisCommand;
bool ShowArgTypes = true;
bool ShowNonExistingValues = true;  // Uses the number of an object, word or message instead of the text if it does not exist
byte SpecialSyntaxType = 1;  // 0 for v30 = v30 + 4;, 1 for v30 += 4;

static byte  BlockDepth;
static short BlockEnd[MaxBlockDepth+1];
static short BlockLength[MaxBlockDepth+1];
static bool BlockIsIf[MaxBlockDepth+1];
static short TempBlockLength,CurBlock;
static byte CurArg;
static unsigned int ArgsStart;

static TResource LabelIndex;
static int LabelLoc,NumLabels;
static bool DoGoto;
static string ThisLine;

static bool ErrorOccured;

static bool  FirstCommand, OROn ,  NOTOn;
static byte NumSaidArgs;

static byte IndentPos;
//***************************************************
static byte ReadByte(void)
{
  if(ResPos < ResourceData.Size){
    return ResourceData.Data[ResPos++];
  }
  return 0;
}
//***************************************************
static short ReadLSMSWord(void)
{
  byte MSbyte,LSbyte;
  
  LSbyte = ReadByte();
  MSbyte = ReadByte();

  return ((MSbyte<<8)|LSbyte);

}
//***************************************************
static byte ReadEncByte(void)
{
  return (ReadByte() ^ EncryptionKey[(ResPos-EncryptionStart+10)%11]) ;

}
//***************************************************
void Logic::ReadMessages(void)
{

  int MessageStart[256];
  string ThisMessage;
  int i;

  // NOTE: There is no message 0 (this is not supported by the file format).

  for(i=0;i<MaxMessages;i++){
    Messages[i]="";
    MessageExists[i] = false;
    MessageUsed[i] = false;
  }

  ResPos = MessageSectionStart;
  NumMessages = ReadByte();
  if (NumMessages > 0){
    MessageSectionEnd = ReadLSMSWord() + MessageSectionStart;
    for (i = 1;i <= NumMessages;i++){
      MessageStart[i] = ReadLSMSWord();
    }
    EncryptionStart = ResPos;
    for (i = 1;i <= NumMessages;i++){
      if (MessageStart[i] > 0){
        ThisMessage = "";
        ResPos = MessageSectionStart + MessageStart[i] + 1;
        do{
          CurByte = ReadEncByte();
          if (CurByte == 0 || ResPos >= ResourceData.Size)break;
          if(CurByte == 0x0a)ThisMessage += "\\n";
          else if (CurByte == 0x22)ThisMessage += "\\\"";
          else if (CurByte == 0x5c)ThisMessage += "\\\\";
          else ThisMessage += CurByte;
        }while(true);
        Messages[i]=ThisMessage;
        MessageExists[i] = true;
      }
    }
  }

}
//***************************************************
void Logic::DisplayMessages(void)
{  
  int i;

  if(game->show_all_messages){
    OutputText.append("// Messages\n");
    for(i=1;i<=255;i++){
      if (MessageExists[i]){
         OutputText.append("#message " + IntToStr(i) + " \""+Messages[i]+"\"\n");
      }
    }
  }
  else{  //(only those not used elsewhere in the logic are here)
     OutputText.append("// Messages\n");
     for(i=1;i<=255;i++){
       if (MessageExists[i] && !MessageUsed[i]){
         OutputText.append("#message " + IntToStr(i)+" \""+Messages[i]+"\"\n");
       }       
     }
  }

}
//***************************************************
int Logic::FindLabels_ReadIfs(void)
{
  byte NumSaidArgs;

  do{
    CurByte = ReadByte();
    if(CurByte == 0xFC)CurByte = ReadByte();
    if(CurByte == 0xFC)CurByte = ReadByte(); // we may have 2 0xFCs in a row, e.g. (a || b) && (c || d)
    if(CurByte == 0xFD)CurByte = ReadByte();

    if (CurByte > 0 && CurByte <= NumTestCommands){      
      ThisCommand = CurByte;
      if (ThisCommand == 14){ // said command
        NumSaidArgs = ReadByte();
        ResPos += NumSaidArgs*2;
      }
      else{
        ResPos += TestCommand[ThisCommand].NumArgs;
      }
    }
    else if(CurByte == 0xFF){
      if (BlockDepth >= MaxBlockDepth - 1){
        sprintf(tmp,"Too many nested blocks (%d)\n",BlockDepth);
        ErrorList.append(tmp);
        ErrorOccured=true;
        break;
      }
      BlockDepth++;
      BlockIsIf[BlockDepth] = true;
      BlockLength[BlockDepth] = ReadLSMSWord();
      BlockEnd[BlockDepth] = BlockLength[BlockDepth] + ResPos;
      if (BlockEnd[BlockDepth] > BlockEnd[BlockDepth-1]){
        sprintf(tmp,"Block too long (%d bytes longer than rest of previous block)",BlockEnd[BlockDepth]-BlockEnd[BlockDepth-1]);
        ErrorOccured=true; 
        ErrorList.append(string(tmp)+"\n");
      }
      break;
    }
    else{
      sprintf(tmp,"Unknown test command (%d)",CurByte);
      break;
    }
  }while(true);

  return 0;
}
//***************************************************
void Logic::AddBlockEnds(void)
{
  for(int CurBlock = BlockDepth;CurBlock>=1;CurBlock--){
    if (BlockEnd[CurBlock] <= ResPos){
        OutputText.append(MultStr("  ",CurBlock-1)+"}\n");
      BlockDepth--;
    }
  }
}
//***************************************************
int Logic::FindLabels(void)
{

  LabelIndex.Size = ResourceData.Size;
  LabelIndex.Data = (byte *)calloc(LabelIndex.Size,1);
  BlockDepth = 0;
  NumLabels = 0;
  do{
    for( CurBlock = BlockDepth;CurBlock>=1;CurBlock--){
      if (BlockEnd[CurBlock] <= ResPos)BlockDepth--;
    }
    CurByte = ReadByte();

    if (CurByte == 0xFF)FindLabels_ReadIfs();
    else if(CurByte <= NumAGICommands){
      ResPos += AGICommand[CurByte].NumArgs;
    }
    else if(CurByte == 0xFE){
      DoGoto = false;
      TempBlockLength = ReadLSMSWord();
      if ((BlockEnd[BlockDepth] == ResPos) && (BlockIsIf[BlockDepth]) && (BlockDepth > 0) && (!game->show_elses_as_gotos)){
         BlockIsIf[BlockDepth] = false;
         if (TempBlockLength + ResPos > BlockEnd[BlockDepth-1] || (TempBlockLength & 0x8000) || BlockLength[BlockDepth] <= 3){
           DoGoto = true;
         }
         else{
           BlockLength[BlockDepth] = TempBlockLength;
           BlockEnd[BlockDepth] = BlockLength[BlockDepth] + ResPos;
         }
      }
      else{
        DoGoto = true;
      }
      //goto
      if (DoGoto){
        LabelLoc = TempBlockLength + ResPos;
        if (LabelLoc > LabelIndex.Size - 1){
          sprintf(tmp,"Label past end of logic (%x %x)\n ",LabelLoc,LabelIndex.Size);
          ErrorList.append(tmp);
          ErrorOccured=true;
          break;
        } 
        if (LabelIndex.Data[LabelLoc] == 0){
          NumLabels++;
          LabelIndex.Data[LabelLoc] = NumLabels;
        }
      }      
    }
    else{
      sprintf(tmp,"Unknown command (%d)",CurByte);
      break;
    }
  }while(ResPos < MessageSectionStart);

  return 0;
}
//***************************************************
void Logic::AddArg(byte Arg, byte ArgType)
{
  int NumCharsToDisplay;
  string ThisMessage;
  if(ShowArgTypes){
    switch(ArgType){
    case atMsg:
      if (MessageExists[Arg]){
        string ThisMessage = Messages[Arg];
        do{
          if(ThisMessage.length() + ThisLine.length() > maxcol){
            NumCharsToDisplay = maxcol - ThisLine.length();
            do{
              NumCharsToDisplay--;
            }while(!(NumCharsToDisplay <= 0 || ThisMessage[NumCharsToDisplay]==' '));
            if (NumCharsToDisplay <= 0)
              NumCharsToDisplay = maxcol-ThisLine.length();
            if (NumCharsToDisplay <= 0)
                NumCharsToDisplay = ThisMessage.length();
            ThisLine += "\"" + ThisMessage.substr(0,NumCharsToDisplay) + "\"";
            if(NumCharsToDisplay < (int)ThisMessage.length()){
                ThisMessage = ThisMessage.substr(NumCharsToDisplay+1);
                OutputText.append(ThisLine+"\n");
            }
            else{
                ThisMessage = "";
                OutputText.append(ThisLine);
            }
            if (ArgsStart >= maxcol - 20)ArgsStart = maxcol - 20;
            ThisLine = MultStr(" ",ArgsStart);
          }
          else{
            ThisLine +=  "\"" + ThisMessage +  "\"" ;
            ThisMessage = "";
          }
        }while(ThisMessage.length()>0);
      }        
      else if(ShowNonExistingValues){
        ThisLine += ArgTypePrefix[atMsg] + IntToStr(Arg);
      }
      else{
        sprintf(tmp,"Unknown message (%d)\n",Arg);
        ErrorList.append(tmp);
        ErrorOccured=true;
      }
      MessageUsed[Arg] = true;
      break;
    case atIObj:
      if (Arg <= objlist->ItemNames.num - 1){
        ThisLine += "\"" + objlist->ItemNames.at(Arg) + "\"";
      }
      else if(ShowNonExistingValues){
        ThisLine += ArgTypePrefix[atIObj] + IntToStr(Arg);
      }
      else{
        sprintf(tmp,"Unknown inventory item (%d)\n",Arg);
        ErrorList.append(tmp);
        ErrorOccured=true;        
      }
      break;
    default:
      if(ArgType != 0){
        ThisLine += ArgTypePrefix[ArgType] + IntToStr(Arg);
      }
      else{
        ThisLine += IntToStr(Arg);
      }
      break;
    }
  }
  else{
    ThisLine += IntToStr(Arg);
  }

}
//***************************************************
void Logic::AddSpecialSyntaxCommand(void)
{
  int arg1;

  arg1 = ReadByte();
  switch(ThisCommand){
    // increment
  case 0x01: ThisLine += "v"+IntToStr(arg1)+"++"; break;
    // decrement
  case 0x02: ThisLine += "v"+IntToStr(arg1)+"--"; break;
    // assignn
  case 0x03: ThisLine += "v"+IntToStr(arg1)+" = "+IntToStr(ReadByte()); break;
    // assignv
  case 0x04: ThisLine += "v"+IntToStr(arg1)+" = v"+IntToStr(ReadByte()); break;
    // addn
  case 0x05: if (SpecialSyntaxType == 0)
    ThisLine += "v"+IntToStr(arg1)+" = v"+IntToStr(arg1)+" + "+IntToStr(ReadByte());
  else ThisLine += "v"+IntToStr(arg1)+" += "+IntToStr(ReadByte()); break;
  // addv
  case 0x06: if (SpecialSyntaxType == 0)
    ThisLine += "v"+IntToStr(arg1)+" = v"+IntToStr(arg1)+" + v"+IntToStr(ReadByte());
  else ThisLine += "v"+IntToStr(arg1)+" += v"+IntToStr(ReadByte()); break;
  // subn
  case 0x07: if (SpecialSyntaxType == 0)
    ThisLine += "v"+IntToStr(arg1)+" = v"+IntToStr(arg1)+" - "+IntToStr(ReadByte());
  else ThisLine += "v"+IntToStr(arg1)+" -= "+IntToStr(ReadByte()); break;
  // subv
  case 0x08: if (SpecialSyntaxType == 0)
    ThisLine += "v"+IntToStr(arg1)+" = v"+IntToStr(arg1)+" - v"+IntToStr(ReadByte());
  else ThisLine += "v"+IntToStr(arg1)+" -= v"+IntToStr(ReadByte()); break;
  // lindirectv
  case 0x09: ThisLine += "*v"+IntToStr(arg1)+" = v"+IntToStr(ReadByte()); break;
    // rindirect
  case 0x0A: ThisLine += "v"+IntToStr(arg1)+" = *v"+IntToStr(ReadByte()); break;
    // lindirectn
  case 0x0B: ThisLine += "*v"+IntToStr(arg1)+" = "+IntToStr(ReadByte()); break;
    // mul.n
  case 0xA5: if (SpecialSyntaxType == 0)
    ThisLine += "v"+IntToStr(arg1)+" = v"+IntToStr(arg1)+" * "+IntToStr(ReadByte());
  else ThisLine += "v"+IntToStr(arg1)+" *= "+IntToStr(ReadByte()); break;
  // mul.v
  case 0xA6: if (SpecialSyntaxType == 0)
    ThisLine += "v"+IntToStr(arg1)+" = v"+IntToStr(arg1)+" * v"+IntToStr(ReadByte());
  else ThisLine += "v"+IntToStr(arg1)+" *= v"+IntToStr(ReadByte()); break;
  // div.n
  case 0xA7: if (SpecialSyntaxType == 0)
    ThisLine += "v"+IntToStr(arg1)+" = v"+IntToStr(arg1)+" / "+IntToStr(ReadByte());
  else ThisLine += "v"+IntToStr(arg1)+" /= "+IntToStr(ReadByte()); break;
  // div.v
  case 0xA8: if (SpecialSyntaxType == 0)
    ThisLine += "v"+IntToStr(arg1)+" = v"+IntToStr(arg1)+" / v"+IntToStr(ReadByte());
  else ThisLine += "v"+IntToStr(arg1)+" /= v"+IntToStr(ReadByte()); break;
  }

}
//***************************************************
void Logic::AddSpecialIFSyntaxCommand(void)
{

  switch(ThisCommand){
  case 1: // equaln
    ThisLine += 'v' + IntToStr(ReadByte());
    if (NOTOn) ThisLine += " != ";
    else ThisLine += " == ";
    ThisLine += IntToStr(ReadByte());
    break;
  case 2: // equalv
    ThisLine += 'v' + IntToStr(ReadByte());
    if (NOTOn) ThisLine += " != v";
    else ThisLine += " == v";
    ThisLine += IntToStr(ReadByte());
    break;
  case 3: // lessn
    ThisLine += 'v' + IntToStr(ReadByte());
    if (NOTOn) ThisLine += " >= ";
    else ThisLine += " < ";
    ThisLine += IntToStr(ReadByte());
    break;
  case 4: // lessv
    ThisLine += 'v' + IntToStr(ReadByte());
    if (NOTOn) ThisLine += " >= v";
    else ThisLine += " < v";
    ThisLine += IntToStr(ReadByte());
    break;
  case 5: // greatern
    ThisLine += 'v' + IntToStr(ReadByte());
    if (NOTOn) ThisLine += " <= ";
    else ThisLine += " > ";
    ThisLine += IntToStr(ReadByte());
    break;
  case 6: // greaterv
     ThisLine += 'v' + IntToStr(ReadByte());
     if (NOTOn) ThisLine += " <= v";
     else ThisLine += " > v";
     ThisLine += IntToStr(ReadByte());
    break;
  }

}
//***************************************************
void Logic::ReadIfs(void)
{
  int ThisWordGroupIndex, ThisWordGroupNum;

  FirstCommand = true;
  OROn = false;
  ThisLine = MultStr("  ",BlockDepth)+"if (";
  do{
    NOTOn = false;
    CurByte = ReadByte();
    if (CurByte == 0xFC){
      OROn = !OROn;
      if (OROn){
        if (!FirstCommand){
          ThisLine += " &&";
          OutputText.append(ThisLine+"\n");
          ThisLine = MultStr("  ",BlockDepth)+"    ";
          FirstCommand = true;
        }
        ThisLine += '(';
      }
      else ThisLine += ')';
      CurByte = ReadByte();
    }
    if(CurByte == 0xFC && !OROn){ // we may have 2 0xFCs in a row, e.g. (a || b) && (c || d)
      ThisLine += " &&";
      OutputText.append(ThisLine+"\n");
      ThisLine = MultStr("  ",BlockDepth)+"    ";
      FirstCommand = true;
      ThisLine += "(";
      OROn = true;
      CurByte = ReadByte();
    }
    if (CurByte == 0xFD){   // NOT
      NOTOn = true;
      CurByte = ReadByte();
    }
    if (CurByte > 0 && CurByte <= NumTestCommands){
      if(!FirstCommand){
        if (OROn) ThisLine += " ||";
        else ThisLine += " &&";
        OutputText.append(ThisLine+"\n");
        ThisLine = MultStr("  ",BlockDepth)+"    ";
      }
      ThisCommand = CurByte;
      if (game->show_special_syntax && ThisCommand >=1 && ThisCommand <=6)
        AddSpecialIFSyntaxCommand();
      else{
        if (NOTOn) ThisLine += '!';
        ThisLine += string(TestCommand[ThisCommand].Name) + '(';
        ArgsStart = ThisLine.length();
        if (ThisCommand == 14){   // said command
          NumSaidArgs = ReadByte();
          for (CurArg = 1;CurArg<=NumSaidArgs; CurArg++){
            ThisWordGroupNum = ReadLSMSWord();
            ThisWordGroupIndex = wordlist->GetWordGroupIndex(ThisWordGroupNum);
            if (ThisWordGroupIndex < 0){
              if (ShowNonExistingValues){
                ThisLine += IntToStr(ThisWordGroupNum);
              }
              else{
                sprintf(tmp,"Unknown word group (%d)\n",ThisWordGroupNum);
                ErrorList.append(tmp);
                ErrorOccured=true;
                break;
              }
            }
            else{
              ThisLine += '"' + string(wordlist->WordGroup[ThisWordGroupIndex].Words.at(0)) + '"';
              if (CurArg < NumSaidArgs)ThisLine += ',';
            }
          }
        }
        else{
          for (CurArg = 0; CurArg < TestCommand[ThisCommand].NumArgs; CurArg++){
            CurByte = ReadByte();
            AddArg(CurByte,TestCommand[ThisCommand].argTypes[CurArg]);
            if (CurArg < TestCommand[ThisCommand].NumArgs-1)ThisLine += ',';
          }
        } // if ThisCommand != 14
        ThisLine += ')';
      }
      FirstCommand = false;
    }//if (CurByte > 0) && (CurByte <= NumTestCommands)
    else if (CurByte == 0xff){
      ThisLine += ") {";
      if (BlockDepth >= MaxBlockDepth - 1){
        sprintf(tmp,"Too many nested blocks (%d)\n",BlockDepth);
        ErrorList.append(tmp);
        ErrorOccured=true;
        break;
      }
      else{
        BlockDepth++;
        BlockIsIf[BlockDepth] = true;
        BlockLength[BlockDepth] = ReadLSMSWord();
        BlockEnd[BlockDepth] = BlockLength[BlockDepth] + ResPos;        
        if (BlockEnd[BlockDepth] > BlockEnd[BlockDepth-1]){
          sprintf(tmp,"Block too long (%d bytes longer than rest of previous block)\n",BlockEnd[BlockDepth]-BlockEnd[BlockDepth-1]);
          ErrorList.append(tmp);
          ErrorOccured=true;
          break;
        }        
      }
      OutputText.append(ThisLine+"\n");
      ThisLine = MultStr("  ",BlockDepth);
      break;
    }// if CurByte = 0xFF
    else{
      sprintf(tmp,"Unknown test command (%d)\n",CurByte);
      ErrorList.append(tmp);
      ErrorOccured=true;
      break;
    }
  }while(!ErrorOccured);
}
//***************************************************
int Logic::decode(int ResNum)
{
  int ret=0,i,j;

  OutputText = "";
  sprintf(tmp,"%s/words.tok",game->dir.c_str());
  ret = wordlist->read(tmp);
  if(ret)return 1;
 
  sprintf(tmp,"%s/object",game->dir.c_str());  
  ret = objlist->read(tmp,false);
  if(ret)return 1;
  objlist->ItemNames.toLower();
  // words already in lower case in file so we don't need to convert them
  for(i=0;i<objlist->ItemNames.num;i++){
    if(objlist->ItemNames.at(i).find_first_of("\"")==string::npos)continue;
    //replace " with \" 
    char *ptr=(char *)objlist->ItemNames.at(i).c_str();
    for(j=0;*ptr;ptr++){
      if(*ptr=='"'){
        tmp[j++]='\\';
        tmp[j++]='"';
      }
      else tmp[j++]=*ptr;
    }
    tmp[j]=0;
    objlist->ItemNames.replace(i,tmp);
  }
  
  ret = game->ReadResource(LOGIC,ResNum);
  if(ret)return 1;

  ErrorList="";
  ResPos = 0;
  MessageSectionStart = ReadLSMSWord() + 2;

  if (MessageSectionStart > ResourceData.Size - 1){
    sprintf(tmp,"Error: Message section start %x is beyond end of resource\n",
            MessageSectionStart);
    ErrorList.append(tmp);
    return 1;
  }
  ErrorOccured=false;
  ReadMessages();
  ResPos = 2;
  BlockEnd[0] = MessageSectionStart;
  BlockIsIf[0] = false;
  memset(BlockIsIf,0,sizeof(BlockIsIf));
  FindLabels();
  BlockDepth = 0;
  ResPos = 2;
  do{
    AddBlockEnds();
    if (LabelIndex.Data[ResPos] > 0){
      OutputText.append("Label" + IntToStr(LabelIndex.Data[ResPos]) + ":\n");
    }
    CurByte = ReadByte();
    if(CurByte == 0xFF)ReadIfs();
    else if(CurByte <= NumAGICommands){
      ThisCommand = CurByte;
      ThisLine = MultStr("  ",BlockDepth);
      if (game->show_special_syntax && (ThisCommand>=0x01 && ThisCommand<=0x0B)||(ThisCommand>=0xA5&&ThisCommand<=0xA8))AddSpecialSyntaxCommand();
      else{
        ThisLine+=(string(AGICommand[ThisCommand].Name) + "(");
        ArgsStart = ThisLine.length();
        IndentPos = ThisLine.length();
        for(CurArg = 1;CurArg<=AGICommand[ThisCommand].NumArgs;CurArg++){
           CurByte = ReadByte();
           AddArg(CurByte,AGICommand[ThisCommand].argTypes[CurArg-1]);
           if(CurArg < AGICommand[ThisCommand].NumArgs)ThisLine+=",";
        }
        ThisLine+=")";
      }
      ThisLine+=";";
      OutputText.append(ThisLine+"\n");
    }
    else if(CurByte == 0xfe){
      DoGoto = false;
      TempBlockLength = ReadLSMSWord();
      if (BlockEnd[BlockDepth] == ResPos && (BlockIsIf[BlockDepth]) && BlockDepth > 0 && (!game->show_elses_as_gotos)){
        //else
        BlockIsIf[BlockDepth] = false;
        if (TempBlockLength + ResPos > BlockEnd[BlockDepth-1] || TempBlockLength & 0x8000 || BlockLength[BlockDepth] <= 3){
          DoGoto = true;
        }
        else{
          OutputText.append(MultStr("  ",BlockDepth-1) + "}\n");
          OutputText.append(MultStr("  ",BlockDepth-1) + "else {\n");
          BlockLength[BlockDepth] = TempBlockLength;
          BlockEnd[BlockDepth] = BlockLength[BlockDepth] + ResPos;
        }
      }
      else DoGoto = true;
      // goto
      if (DoGoto){
         LabelLoc = TempBlockLength + ResPos;
         if (LabelLoc > LabelIndex.Size - 1){
          sprintf(tmp,"Label past end of logic (%x %x)\n ",LabelLoc,LabelIndex.Size);
           ErrorList.append(tmp);
           ErrorOccured=true;
           break;
         }
         else{
           OutputText.append(MultStr("  ",BlockDepth) + "goto(Label"+IntToStr(LabelIndex.Data[LabelLoc])+");\n");
         }
      }
    }
    else{
      sprintf(tmp,"Unknown action command (%d)\n",CurByte);
      ErrorList.append(tmp);
      ErrorOccured = true;
      break;
    }
  }while(ResPos < MessageSectionStart);
  if(!ErrorOccured)AddBlockEnds();
  free(LabelIndex.Data);
  OutputText.append("\n");
  DisplayMessages();
  return (ErrorOccured)?1:0;

}
