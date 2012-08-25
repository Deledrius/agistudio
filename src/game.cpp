/*
 *  QT AGI Studio :: Copyright (C) 2000 Helen Zommer
 *
 *  A big part of this code was adapted from the Windows AGI Studio
 *  developed by Peter Kelly.
 *
 *  LZW decompression code belongs to Lance Ewing.
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

#include "menu.h"
#include "util.h"
#include "resources.h"
#include "agicommands.h"

#include <string>
#include <stdio.h>
#include <sys/types.h>
#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include <windows.h>
#undef TEXT
#include "game.h"
#define TEXT 6
#else
#include <unistd.h>
#include <glob.h>
#include "game.h"
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#include <q3progressdialog.h>
#include <qmessagebox.h>
#include <qdir.h>

const char *ResTypeName[4] = {"logic","picture","view","sound"};
const char *ResTypeAbbrv[4] = {"log","pic","view","snd"};
static const char *files[5] = {"vol.0","viewdir","logdir","snddir","picdir"};
Game *game;

static TResource CompressedResource;

/******************************* LZW variables ****************************/
#define MAXBITS 12
#define TABLE_SIZE  18041
#define START_BITS  9

static int BITS, MAX_VALUE, MAX_CODE;
static unsigned int *prefix_code;
static byte *append_character;
static byte decode_stack[4000];  /* Holds the decoded string */
static int input_bit_count=0;    /* Number of bits in input bit buffer */
static unsigned long input_bit_buffer=0L;
//*******************************************

const char EncryptionKey[] = "Avis Durgan";
TResourceInfo ResourceInfo[4][256];

TResource ResourceData;
/* global buffer used for all resource I/0 ! (for this reason it wouldn't be
 wise to run several I/O operations simultaneously, like clicking compile
 very fast in several logic editor windows ("compile all" is ok - it is
 sequential) - fortunately the program works too fast to allow the user
 to do it...) */

//*******************************************
Game::Game()
{
  ResourceData.Data = (byte *)malloc(MaxResourceSize);
  defaults();
  read_settings();
}

//*******************************************
int Game::open(string name)
{
  byte DirData[3080];
  int CurResType,CurResNum,NumDirEntries;
  long DirSize=-1,DirOffset=-1;
  byte byte1,byte2,byte3;
  FILE *fptr;

  dir=name;

  ID = FindAGIV3GameID(dir.c_str());  // 'V2' if not found
  if(ID.length() == 0)return 1;

  isV3 = (ID != "V2");
  bool ErrorOccured = false;

  for(CurResType = 0;CurResType <= 3;CurResType++){
    for(CurResNum = 0;CurResNum <= 255; CurResNum++){
      ResourceInfo[CurResType][CurResNum].Exists = false;
    }
  }

  if(!isV3){
    //for V2 game: open logdir, picdir, viewdir, snddir
    for(CurResType = 0;CurResType <= 3; CurResType++){
      string DIRFilename = dir + "/"+ResTypeAbbrv[CurResType] + "dir";
      fptr=fopen(DIRFilename.c_str(),"rb");
      if(fptr==NULL){
        menu->errmes("Error: can't open %s !",DIRFilename.c_str());
        ErrorOccured = true;
        break;
      }
      else{
        struct stat buf;
        fstat(fileno(fptr),&buf);
        int size=buf.st_size;
        if(size >  768){
          menu->errmes("Error: %s  is too big (should not be mode than 768 bytes) !",DIRFilename.c_str());
          ErrorOccured = true;
          break;
        }
        else{      //read resource info
          fread(DirData,size,1,fptr);
          for(CurResNum = 0;CurResNum <= size/3 -1;CurResNum++){
            byte1 = DirData[CurResNum*3];
            byte2 = DirData[CurResNum*3+1];
            byte3 = DirData[CurResNum*3+2];
            if (!(byte1==0xff && byte2==0xff && byte3==0xff)){
              ResourceInfo[CurResType][CurResNum].Loc = (byte1 % 16)*0x10000 + byte2*0x100 + byte3;
              sprintf(ResourceInfo[CurResType][CurResNum].Filename,"vol.%d",byte1/16);
              ResourceInfo[CurResType][CurResNum].Exists = true;
            }
          }
        }
        fclose(fptr);
      }
    }
  }
  else{  //V3 game: open [GAME_ID]dir (e.g. grdir, mhdir)
    string DIRFilename = dir + "/"+ID + "dir";
    fptr=fopen(DIRFilename.c_str(),"rb");
    if(fptr==NULL){
      menu->errmes("Error: can't open %s ! ",DIRFilename.c_str());
      ErrorOccured = true;
    }
    else{
      struct stat buf;
      fstat(fileno(fptr),&buf);
      int size=buf.st_size;
      if(size > 3080){
        menu->errmes("Error: %s is too big (should not be mode than 3080 bytes) !",DIRFilename.c_str());
        ErrorOccured = true;
      }
      else{
        fread(DirData,size,1,fptr);
        //read resource info
        for(CurResType = 0;CurResType <= 3;CurResType++){
          if(!ErrorOccured){
            switch(CurResType){
            case LOGIC:
              DirOffset = DirData[0] + DirData[1]*256;
              DirSize = DirData[2] + DirData[3]*256 - DirOffset;
              break;
            case PICTURE:
              DirOffset = DirData[2] + DirData[3]*256;
              DirSize = DirData[4] + DirData[5]*256 - DirOffset;
              break;
            case VIEW:
              DirOffset = DirData[4] + DirData[5]*256;
              DirSize = DirData[6] + DirData[7]*256 - DirOffset;
              break;
            case SOUND:
              DirOffset = DirData[6] + DirData[7]*256;
              DirSize = size - DirOffset;
              break;
            }
            if(DirOffset < 0 || DirSize < 0){
              menu->errmes("Error: DIR file is invalid.");
              ErrorOccured = true;
              break;
            }
            else if(DirOffset + DirSize > size){
              menu->errmes("Error: Directory is beyond end of %s file !",DIRFilename.c_str());
              ErrorOccured = true;
              break;
            }
            else{
              if (DirSize > 768)NumDirEntries = 256;
              else NumDirEntries = DirSize/3;
              if (NumDirEntries > 0){
                for(CurResNum = 0;CurResNum < NumDirEntries;CurResNum++){
                   byte1 = DirData[DirOffset+CurResNum*3];
                   byte2 = DirData[DirOffset+CurResNum*3+1];
                   byte3 = DirData[DirOffset+CurResNum*3+2];
                   if (!(byte1==0xff && byte2==0xff && byte3==0xff)){
                     ResourceInfo[CurResType][CurResNum].Loc = (byte1 % 16)*0x10000 + byte2*0x100 + byte3;
                     sprintf(ResourceInfo[CurResType][CurResNum].Filename,"%svol.%d",ID.c_str(),byte1/16);
                     ResourceInfo[CurResType][CurResNum].Exists = true;
                   }
                }
              }
            }
          }
        }
      }
      fclose(fptr);
    }
  }

  if(!ErrorOccured){
    AGIVersionNumber = GetAGIVersionNumber();
    //printf("AGIVersion = %f\n",AGIVersionNumber);
    CorrectCommands(AGIVersionNumber);
    isOpen = true;
    make_source_dir();
    menu->status->message(dir.c_str());
    return 0;
  }
  else return 1;
}

//*******************************************
int copy(char *src,char *dest)
  //copy src file to dest file (to avoid using an external 'cp' command)
{
  FILE *filer, *filew;
  size_t numr;
  char buffer[1024];

  filer=fopen(src,"rb");
  if(!filer)
  {
    menu->errmes("Can't open src file %s !",src);
    return 1;
  }
  filew=fopen(dest,"wb");
  if(!filew)
  {
    menu->errmes("Can't open dst file %s !",dest);
    return 1;
  }

  while(feof(filer)==0)
  {
    if((numr=fread(buffer,1,1024,filer))!=1024)
    {
      if(ferror(filer)!=0)
      {
        menu->errmes("read file error: %s !",src);
        return 1;
      }
    }
    if(fwrite(buffer,1,numr,filew) != numr)
    {
      menu->errmes("write file error: %s !",dest);
      return 1;
    }
  }
  
  fclose(filew);
  fclose(filer);
  return 0;
}

//*******************************************

int Game::from_template(string name)
  //create a new game (in 'name' directory) from template
{

  int i;
  char *ptr;
  struct stat buf;
  char *cfilename;

  dir = name;
  make_source_dir();

  //check if template directory contains *dir files and vol.0
  for(i=0;i<5;i++){
    sprintf(tmp,"%s/%s",templatedir.c_str(),files[i]);
    if(stat(tmp,&buf)){
      menu->errmes("AGI Studio error", "Can't read %s in template directory %s !",files[i],templatedir.c_str());
      return 1;
    }
  }

  sprintf(tmp,"%s/*",templatedir.c_str());
#ifdef _WIN32
  struct _finddata_t c_file;
  long hFile;
  if ((hFile = _findfirst(tmp, &c_file)) != -1L) do {
    sprintf(tmp2,"%s/%s",templatedir.c_str(),c_file.name);
    cfilename = tmp2;
#else
  glob_t globbuf;
  glob(tmp, 0, NULL, &globbuf);
  for(i=0;i<(int)globbuf.gl_pathc;i++){  //copy template game files
    cfilename = globbuf.gl_pathv[i];
#endif
    ptr=strrchr(cfilename,'/');
    if(ptr){
      if(!strcmp(ptr,"/src"))continue;
	  if(!strcmp(ptr,"/."))continue;
	  if(!strcmp(ptr,"/.."))continue;
      sprintf(tmp,"%s%s",name.c_str(),ptr);
    }
    else{
      if(!strcmp(cfilename,"/src"))continue;
	  if(!strcmp(cfilename,"/."))continue;
	  if(!strcmp(cfilename,"/.."))continue;
      sprintf(tmp,"%s/%s",name.c_str(),cfilename);
    }
    if(copy(cfilename,tmp))return 1;
#ifdef _WIN32
  } while (_findnext(hFile, &c_file) == 0);
  _findclose(hFile);
#else
  }
  globfree(&globbuf);
#endif

  sprintf(tmp,"%s/src/*",templatedir.c_str());
#ifdef _WIN32
  if ((hFile = _findfirst(tmp, &c_file)) != -1L) do {
    sprintf(tmp2,"%s/src/%s",templatedir.c_str(),c_file.name);
    cfilename = tmp2;
#else
  glob(tmp, 0, NULL, &globbuf);
  for(i=0;i<(int)globbuf.gl_pathc;i++){  //copy template src subdirectory
    cfilename = globbuf.gl_pathv[i];
#endif
    ptr=strrchr(cfilename,'/');
    if(ptr){
	  if(!strcmp(ptr,"/."))continue;
	  if(!strcmp(ptr,"/.."))continue;
      sprintf(tmp,"%s%s",srcdir.c_str(),ptr);
    }
    else{
	  if(!strcmp(cfilename,"/."))continue;
	  if(!strcmp(cfilename,"/.."))continue;
      sprintf(tmp,"%s/%s",srcdir.c_str(),cfilename);
    }
    if(copy(cfilename,tmp))return 1;
#ifdef _WIN32
  } while (_findnext(hFile, &c_file) == 0);
  _findclose(hFile);
#else
    }
  globfree(&globbuf);
#endif

  return open(name);

}

//*******************************************
void Game::make_source_dir()
  //create directory for sources (extracting logic, etc)
{

  if(reldir)
    srcdir=dir+"/"+srcdirname;   //srcdir is inside the game directory
  else
    srcdir=srcdirname;           //srcdir can be anywhere

#ifdef _WIN32
  int ret=_mkdir(srcdir.c_str());
#else
  int ret=mkdir(srcdir.c_str(),0xfff);
#endif
  if(ret==-1 && errno != EEXIST){
    menu->errmes("Can't create the source directory %s !\nlogic text files will not be saved.",srcdirname.c_str());
  }

}

//*******************************************
int Game::newgame(string name)
  //create an empty game in 'name' directory
{

  static byte BlankObjectFile[8] = {0x42,0x76,0x79,0x70,0x20,0x44,0x4A,0x72};
  static byte BlankWordsFile [72] = {
0x00,0x34,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x41,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x9E,0x00,0x00,0x01,0x11,0x06,0x08,0x10,0x0D,0x9B,0x00,
0x01,0x00,0x0D,0x10,0x93,0x27,0x0F,0x00};

  static const char* files[5] = {"vol.0","viewdir","logdir","snddir","picdir"};
  FILE *fptr;
  int i,j;

  dir = name;

  for(i=0;i<5;i++){
    sprintf(tmp,"%s/%s",name.c_str(),files[i]);
    fptr=fopen(tmp,"wb");  //create empty game files
    if(fptr==NULL){
      menu->errmes("Can't create file %s !",files[i]);
      return 1;
    }
    fclose(fptr);
  }
  sprintf(tmp,"%s/object",dir.c_str());
  fptr=fopen(tmp,"wb");
  fwrite(BlankObjectFile,8,1,fptr);
  fclose(fptr);
  sprintf(tmp,"%s/words.tok",dir.c_str());
  fptr=fopen(tmp,"wb");
  fwrite(BlankWordsFile,72,1,fptr);
  fclose(fptr);
  make_source_dir();

  isOpen = true;
  isV3 = false;
  ID = "";
  AGIVersionNumber = 2.917;
  for(int _i = 0;_i <= 3;_i++){
    for(j = 0;j <= 255; j++){
      ResourceInfo[_i][j].Exists = false;
    }
  }
  menu->status->message(dir.c_str());
  return 0;
}

//*******************************************
string Game::FindAGIV3GameID(const char *name)
  //compare the prefix for vol.0 and dir - if they are same and non-NULL
  //it is a V3 game
{
  string ID1;
  char *ptr;
  char *cfilename;

  ID1 = "V2";  //default for V2 games

#ifdef _WIN32
  struct _finddata_t c_file;
  long hFile;
#else
  glob_t globbuf;
#endif
  char dirString[10]="", volString[10]="";

  sprintf(tmp,"%s/*dir",name);
#ifdef _WIN32
  if ((hFile = _findfirst(tmp, &c_file)) == -1L) {
#else
  if (glob (tmp, GLOB_ERR | GLOB_NOSORT, NULL, &globbuf)) {
    globfree(&globbuf);
#endif
    return ID1;
  }

#ifdef _WIN32
  cfilename = c_file.name;
#else
  cfilename = globbuf.gl_pathv[0];
#endif
  if((ptr=strrchr(cfilename,'/')))ptr++;
  else ptr=cfilename;
  strncpy (dirString, ptr, strlen (ptr) - 3);

#ifdef _WIN32
  _findclose(hFile);
#else
  globfree(&globbuf);
#endif

  sprintf(tmp,"%s/*vol.0",name);

#ifdef _WIN32
  if ((hFile = _findfirst(tmp, &c_file)) == -1L) {
#else
  if (glob (tmp, GLOB_ERR | GLOB_NOSORT, NULL, &globbuf)) {
    globfree(&globbuf);
#endif
    return ID1;
  }

#ifdef _WIN32
  cfilename = c_file.name;
#else
  cfilename = globbuf.gl_pathv[0];
#endif
  if((ptr=strrchr(cfilename,'/')))ptr++;
  else ptr=cfilename;
  strncpy (volString, ptr, strlen (ptr) - 5);

#ifdef _WIN32
  _findclose(hFile);
#else
  globfree(&globbuf);
#endif

  if ((strcmp(volString, dirString) == 0) && (volString != NULL))
    ID1=volString;

  return ID1;
}

//*******************************************************
double Game::GetAGIVersionNumber(void)
{
  double VerNum;
  int ResPos,x;
  byte VerLen;
  bool ISVerNum;
  char VerNumText[16];
  string InFileName = dir + "/agidata.ovl";
  char VersionNumBuffer[] = "A_CDE_GHI";
  double ret = 2.917;
  // This is what we use if we can't find the version number.
  // Version 2.917 is the most common interpreter and
  // the one that all the "new" AGI games should be based on.

  ResourceData.Size = 0;
  FILE *fptr=fopen(InFileName.c_str(),"rb");
  if(fptr!=NULL){
    struct stat buf;
    fstat(fileno(fptr),&buf);
    int size=buf.st_size;
    if(size < MaxResourceSize){
      ResourceData.Size = size;
      fread(ResourceData.Data,ResourceData.Size,1,fptr);
    }
    fclose(fptr);
  }

  if(ResourceData.Size > 0){
    ResPos = 0;
    VerLen = 0;
    while(ResPos < ResourceData.Size && VerLen == 0){
      memcpy(VersionNumBuffer,&ResourceData.Data[ResPos],9);
      ResPos++;
      ISVerNum = true;
      if (VersionNumBuffer[1]=='.'){
        if(VersionNumBuffer[0]<'0'||VersionNumBuffer[2]>'9')ISVerNum = false;
        for(x=2;x<=4;x++){
          if(VersionNumBuffer[x]<'0'||VersionNumBuffer[x]>'9')ISVerNum = false;
        }
        if (ISVerNum && (VersionNumBuffer[0]<='2')) VerLen=5;  // 2.xxx format
        if(VersionNumBuffer[5] != '.') ISVerNum = false;
        for(x=6;x<=8;x++){   // 3.xxx.xxx format
          if(VersionNumBuffer[x]<'0'||VersionNumBuffer[x]>'9')ISVerNum = false;
        }
        if(ISVerNum)VerLen=9;
      }
      else ISVerNum = false;
      if (VerLen>0){
        if (VersionNumBuffer[5] == '.'){
          strcpy(VerNumText,VersionNumBuffer);
          strcpy(VerNumText+5,VersionNumBuffer+6); // remove second .
        }
        else{
          strncpy(VerNumText,VersionNumBuffer,VerLen);
          VerNumText[VerLen]=0;
        }
        VerNum=atof(VerNumText);
        if(VerNum!=0){
          ret = VerNum;
          break;
        }
      }
    }
  }
  return ret;
}

//***************************************
int Game::GetResourceSize(char ResType_c,int ResNum)
{
  byte lsbyte,msbyte;
  int ResType = ResType_c;

  if(ResourceInfo[ResType][ResNum].Exists){
    sprintf(tmp,"%s/%s",dir.c_str(),ResourceInfo[ResType][ResNum].Filename);
    FILE *fptr=fopen(tmp,"rb");
    if(fptr!=NULL){
      struct stat buf;
      fstat(fileno(fptr),&buf);
      int size=buf.st_size;
      if(size >= ResourceInfo[ResType][ResNum].Loc+5){
        fseek(fptr,ResourceInfo[ResType][ResNum].Loc,SEEK_SET);
        fread(&lsbyte,1,1,fptr);
        fread(&msbyte,1,1,fptr);
        if(lsbyte == 0x12 && msbyte == 0x34){
          fread(&lsbyte,1,1,fptr);
          fread(&lsbyte,1,1,fptr);
          fread(&msbyte,1,1,fptr);
          return (msbyte*256 + lsbyte);
        }
      }
      fclose(fptr);
    }
  }
  return -1;
}

//***************************************
int Game::ReadResource(char ResType_c, int ResNum)
  //read a resource number ResNum from the vol file
{
  byte msbyte,lsbyte;
  int ResType = (int)ResType_c;

  if(isV3){
    return ReadV3Resource(ResType,ResNum);
  }

  sprintf(tmp,"%s/%s",dir.c_str(),ResourceInfo[ResType][ResNum].Filename);
  FILE *fptr=fopen(tmp,"rb");
  if(fptr==NULL){
    menu->errmes("Error reading file %s/%s",dir.c_str(),ResourceInfo[ResType][ResNum].Filename);
    return 1;
  }

  struct stat buf;
  fstat(fileno(fptr),&buf);
  int size=buf.st_size;
  if(ResourceInfo[ResType][ResNum].Loc > size){
    menu->errmes("Error: %s: Specified resource location is past end of file",ResourceInfo[ResType][ResNum].Filename);
    return 1;
  }

  fseek(fptr,ResourceInfo[ResType][ResNum].Loc,SEEK_SET);
  fread(&msbyte,1,1,fptr);
  fread(&lsbyte,1,1,fptr);
  if(!(msbyte==0x12 && lsbyte==0x34)){
    menu->errmes("Error: %s: Resource signature not found",ResourceInfo[ResType][ResNum].Filename);
    return 1;
  }

  fseek(fptr,ResourceInfo[ResType][ResNum].Loc+3,SEEK_SET);
  fread(&lsbyte,1,1,fptr);
  fread(&msbyte,1,1,fptr);
  ResourceData.Size = msbyte * 256 + lsbyte;

  if(ResourceData.Size==0){
    menu->errmes("Error: %s: Resource size 0 !",ResourceInfo[ResType][ResNum].Filename);
    return 1;
  }

  fread(ResourceData.Data, ResourceData.Size,1,fptr);
  fclose(fptr);
  return 0;

}

//***************************************
FILE * Game::OpenPatchVol(int PatchVol,int *filesize)
{
  FILE *fptr;

  if(isV3)
    sprintf(tmp,"%s/%svol.%d",dir.c_str(),ID.c_str(),PatchVol);
  else
    sprintf(tmp,"%s/vol.%d",dir.c_str(),PatchVol);
  fptr = fopen(tmp,"a+b");
  if(fptr==NULL)return NULL;
  struct stat buf;
  fstat(fileno(fptr),&buf);
  *filesize=buf.st_size;
  return fptr;

}
//***********************************************
static int RewriteDirFile(FILE *dir,int dirsize)
  //used for V3 games
{
  byte DirData[4][768];
  int CurResType;
  byte lsbyte,msbyte;
  int Offset[4],Size[4];
  byte OffsetArray[8];

  fseek(dir,0,SEEK_SET);
  for(CurResType=3;CurResType>=0;CurResType--){
    fseek(dir,CurResType*2,SEEK_SET);
    fread(&lsbyte,1,1,dir);
    fread(&msbyte,1,1,dir);
    Offset[CurResType]=(msbyte<<8)|lsbyte;
    if(CurResType == 3)
      Size[CurResType]=dirsize-Offset[CurResType];
    else
      Size[CurResType]=Offset[CurResType+1]-Offset[CurResType];
    if(Offset[CurResType]>dirsize || Size[CurResType]>768 || Offset[CurResType]+Size[CurResType]>dirsize){
      menu->errmes("DIR file is invalid");
      fclose(dir);
      return 1;
    }

  }
  for(CurResType =0;CurResType<=3;CurResType++){
    fseek(dir,Offset[CurResType],SEEK_SET);
    memset(DirData[CurResType],0xff,768);
    fread(DirData[CurResType],Size[CurResType],1,dir);
  }

  OffsetArray[0] = 8;
  OffsetArray[1] = 0;
  Offset[0] = 8;

  for(CurResType =1;CurResType<=3;CurResType++){
     Offset[CurResType] = Offset[CurResType-1] + 768;
     OffsetArray[CurResType*2] = Offset[CurResType] % 256;
     OffsetArray[CurResType*2+1] = Offset[CurResType] / 256;
  }
  fseek(dir,0,SEEK_SET);
  fwrite(OffsetArray,8,1,dir);
  for(CurResType =0;CurResType<=3;CurResType++){
    fwrite(DirData[CurResType],768,1,dir);
  }
  fflush(dir);

  return 0;

}

//***********************************************
FILE * Game::OpenDirUpdate(int *dirsize,int ResType)
{
  FILE *fptr;

  if(isV3)
    dirname = dir + "/" + ID + "dir";
  else
    dirname = dir + "/" + ResTypeAbbrv[ResType] + "dir";

  fptr = fopen(dirname.c_str(),"r+b");
  if(fptr==NULL){
    menu->errmes("Error writing file %s ! ",dirname.c_str());
    return NULL;
  }
  struct stat buf;
  fstat(fileno(fptr),&buf);
  *dirsize=buf.st_size;
  return fptr;
}

//***********************************************
int Game::AddResource(int ResType,int ResNum)
{
  FILE *fptr,*dirf;
  int filesize,dirsize;
  int PatchVol;
  byte ResHeader[7],DirByte[3];
  int off;
  byte lsbyte,msbyte;

  if((dirf = OpenDirUpdate(&dirsize,ResType))==NULL)return 1;

  PatchVol = 0;
  fptr = OpenPatchVol(PatchVol,&filesize);  //open vol.0
  if(fptr==NULL){
    menu->errmes("Can't open vol.%d !",PatchVol);
    return 1;
  }

  do{
    if(filesize + ResourceData.Size > 1048000){
//current volume is too big (to fit a diskette) - create the next one
      fclose(fptr);
      PatchVol++;
      fptr=OpenPatchVol(PatchVol,&filesize);
      if(fptr==NULL){
        if(isV3)
          menu->errmes("Can't open %svol.%d !",ID.c_str(),PatchVol);
        else
          menu->errmes("Can't open vol.%d !",PatchVol);
        return 1;
      }
    }
  }while(filesize + ResourceData.Size > 1048000);

  //write the resource to the patch volume and update the DIR file
  if(isV3){
    if(RewriteDirFile(dirf,dirsize)){
      fclose(dirf);
      fclose(fptr);
      return 1;
    }
  }
  else{
    if(ResNum*3 > dirsize){
      DirByte[1] = 0xff;
      fseek(dirf,dirsize,SEEK_SET);
      do{
        fwrite(&DirByte[1],1,1,dirf);
      }while(ftell(dirf)!=ResNum*3);
    }
  }
  fseek(fptr,filesize,SEEK_SET);
  off = ftell(fptr);
  DirByte[0] = PatchVol*0x10 + off / 0x10000;
  DirByte[1] = (off % 0x10000) / 0x100;
  DirByte[2] = off % 0x100;
  ResourceInfo[ResType][ResNum].Exists = true;
  ResourceInfo[ResType][ResNum].Loc = off;
  if(isV3)sprintf(ResourceInfo[ResType][ResNum].Filename,"%svol.%d",ID.c_str(),PatchVol);
  else sprintf(ResourceInfo[ResType][ResNum].Filename,"vol.%d",PatchVol);
  int n=0;
  ResHeader[n++] = 0x12;
  ResHeader[n++] = 0x34;
  ResHeader[n++] = PatchVol;
  ResHeader[n++] = ResourceData.Size % 256;
  ResHeader[n++] = ResourceData.Size / 256;
  if(isV3){
    ResHeader[n++] = ResourceData.Size % 256;  // no compression so compressed size
    ResHeader[n++] = ResourceData.Size / 256;  // and uncompressed size are the same
  }
  fwrite(ResHeader,n,1,fptr);
  fwrite(ResourceData.Data,ResourceData.Size,1,fptr);

  if(isV3){
    fseek(dirf,ResType*2,SEEK_SET);
    fread(&lsbyte,1,1,dirf);
    fread(&msbyte,1,1,dirf);
    off = (msbyte<<8)|lsbyte;
    fseek(dirf,off+ResNum*3,SEEK_SET);
    fwrite(DirByte,3,1,dirf);
    fclose(dirf);
  }
  else{
    fseek(dirf,ResNum*3,SEEK_SET);
    fwrite(DirByte,3,1,dirf);
    fclose(dirf);
  }
  fflush(fptr);

  return 0;
}

//************************************************
int Game::DeleteResource(int ResType,int ResNum)
{
  FILE *dirf;
  int dirsize;
  byte lsbyte,msbyte;
  int off;

  if((dirf = OpenDirUpdate(&dirsize,ResType))==NULL)return 1;

  if(isV3){
    if(dirsize<8){
      menu->errmes("Error: %s file invalid!",dirname.c_str());
      fclose(dirf);
      return 1;
    }
    fseek(dirf,ResType*2,SEEK_SET);
    fread(&lsbyte,1,1,dirf);
    fread(&msbyte,1,1,dirf);
    off = (msbyte<<8)|lsbyte;
    if(dirsize < off+ResNum*3+2){
      menu->errmes("Error: %s file invalid!",dirname.c_str());
      fclose(dirf);
      return 1;
    }
    fseek(dirf,off+ResNum*3,SEEK_SET);
  }
  else{
    if(dirsize < ResNum*3 + 2){
      menu->errmes("Error: %s file invalid!",dirname.c_str());
      fclose(dirf);
      return 1;
    }
    fseek(dirf,ResNum*3,SEEK_SET);
  }
  byte b = 0xff;
  fwrite(&b,1,1,dirf);
  fwrite(&b,1,1,dirf);
  fwrite(&b,1,1,dirf);
  ResourceInfo[ResType][ResNum].Exists = false;
  fclose(dirf);

  return 0;
}

//************************************************
int Game::RebuildVOLfiles()
{
  int step=0,steps=0;
  int ResType,ResNum,VolFileNum;
  byte b=0xff,byte1,byte2,byte3;
  FILE *dirf=NULL,*fptr;
  byte ResHeader[7];
  long off;
#define MaxVOLFileSize  1023*1024
  TResourceInfo NewResourceInfo[4][256];
  int ResourceNum[4];
  bool cancel=false;
  char tmp1[1024];
  int DirOffset[4];
#ifdef _WIN32
  struct _finddata_t c_file;
  long hFile;
#endif
  char volname[8]="vol";

  if(isV3){
    sprintf(volname,"%svol",ID.c_str());
  }

  for(ResType = 0; ResType <= 3;ResType++){
    ResourceNum[ResType]=0;
    for(ResNum=0;ResNum<256;ResNum++){
      if(ResourceInfo[ResType][ResNum].Exists){
        steps++;  //number of steps for the progress bar (if necessary)
        ResourceNum[ResType]=ResNum;
      }
    }
  }

  Q3ProgressDialog progress( "Rebuilding VOL files...", "Cancel", steps,0, 0, TRUE );  //shows up if the operation is taking more than 3 sec
  //(so it never shows up...)

  ResHeader[0]=0x12;
  ResHeader[1]=0x34;

  VolFileNum=0;
  sprintf(tmp,"%s/%s.%d.new",dir.c_str(),volname,VolFileNum);
  if((fptr=fopen(tmp,"wb"))==NULL){
    menu->errmes("Error creating file %s ! ",tmp);
    progress.cancel();
    return 1;
  }

  if(isV3){
    sprintf(tmp,"%s/%sdir.new",dir.c_str(), ID.c_str());
    if((dirf=fopen(tmp,"wb"))==NULL){
      menu->errmes("Error creating file %s ! ",tmp);
      progress.cancel();
      return 1;
    }
    for (ResType = 0;ResType<=3;ResType++){
      DirOffset[ResType] = 8 + ResType*0x300;
      byte1 = DirOffset[ResType] % 0x100;
      byte2 = DirOffset[ResType] / 0x100;
      fwrite(&byte1,1,1,dirf);
      fwrite(&byte2,1,1,dirf);
    }
    for(ResType = 0; ResType <= 3;ResType++){
      for(ResNum=0;ResNum<256;ResNum++){
        fwrite(&b,1,1,dirf);
        fwrite(&b,1,1,dirf);
        fwrite(&b,1,1,dirf);
      }
    }
    fflush(dirf);
    fseek(dirf,0,SEEK_SET);
  }

  for(ResType = 0; ResType <= 3;ResType++){
    if(!isV3){
      sprintf(tmp,"%s/%sdir.new",dir.c_str(), ResTypeAbbrv[ResType]);
      if((dirf=fopen(tmp,"wb"))==NULL){
        menu->errmes("Error creating file %s !",tmp);
        progress.cancel();
        return 1;
      }
      for(ResNum=0;ResNum<ResourceNum[ResType];ResNum++){
        fwrite(&b,1,1,dirf);
        fwrite(&b,1,1,dirf);
        fwrite(&b,1,1,dirf);
      }
      fflush(dirf);
      fseek(dirf,0,SEEK_SET);
    }

    for(ResNum=0;ResNum<256;ResNum++){
      if(!ResourceInfo[ResType][ResNum].Exists){
        NewResourceInfo[ResType][ResNum].Exists = false;
        continue;
      }
      if(ReadResource(ResType,ResNum)){
        menu->errmes("Error saving %s.%d !",ResTypeAbbrv[ResType],ResNum);
        progress.cancel();
        return 1;
      }
      off=ftell(fptr);
      if(off + ResourceData.Size + 5 > MaxVOLFileSize){
        fclose(fptr);
        VolFileNum++;
        sprintf(tmp,"%s/%s.%d.new",dir.c_str(),volname,VolFileNum);
        if((fptr=fopen(tmp,"wb"))==NULL){
          menu->errmes("Error creating file %s !",tmp);
          progress.cancel();
          return 1;
        }
        off=ftell(fptr);
      }
      NewResourceInfo[ResType][ResNum].Exists = true;
      NewResourceInfo[ResType][ResNum].Loc = off;
      sprintf(NewResourceInfo[ResType][ResNum].Filename,"%s.%d",volname,VolFileNum);
      byte1 = VolFileNum*0x10 + off / 0x10000;
      byte2 = (off % 0x10000) / 0x100;
      byte3 = off % 0x100;
      ResHeader[2] = VolFileNum;
      ResHeader[3] = ResourceData.Size % 256;
      ResHeader[4] = ResourceData.Size / 256;
      if(isV3){
        ResHeader[5] = ResourceData.Size % 256;
        ResHeader[6] = ResourceData.Size / 256;
        fwrite(ResHeader,7,1,fptr);
      }
      else{
        fwrite(ResHeader,5,1,fptr);
      }

      fwrite(ResourceData.Data,ResourceData.Size,1,fptr);
      if(isV3)
        fseek(dirf,DirOffset[ResType]+ResNum*3,SEEK_SET);
      else
        fseek(dirf,ResNum*3,SEEK_SET);
      fwrite(&byte1,1,1,dirf);
      fwrite(&byte2,1,1,dirf);
      fwrite(&byte3,1,1,dirf);
      progress.setProgress( step++ );
      if ( progress.wasCancelled() ){
        cancel=true;
        break;
      }
    }
    if(!isV3)
      fclose(dirf);

    if ( progress.wasCancelled() ){
      cancel=true;
      break;
    }
  }
  progress.setProgress( steps );

  fclose(fptr);
  if(isV3)fclose(dirf);


  if(cancel)return 1;

  //cleanup temporary files
  if(isV3){
    sprintf(tmp,"%s/%sdir.new",dir.c_str(), ID.c_str());
    sprintf(tmp1,"%s/%sdir",dir.c_str(), ID.c_str());
    rename(tmp,tmp1);
  }
  else{
    for(ResType = 0; ResType <= 3;ResType++){
      sprintf(tmp,"%s/%sdir.new",dir.c_str(), ResTypeAbbrv[ResType]);
      sprintf(tmp1,"%s/%sdir",dir.c_str(), ResTypeAbbrv[ResType]);
      rename(tmp,tmp1);
    }
  }

  QDir d( dir.c_str());

  // Delete old VOLs...
  QStringList list = d.entryList(
    QString(volname) + ".?;" + QString(volname) + ".??" );
  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
        d.remove( *it );

  // ...and replace them with the new ones:
  list = d.entryList( QString(volname) + ".*.new" );
  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
  {
    QString new_name = *it;
    new_name.replace(".new", "");
    d.rename( *it, new_name );
  }

  memcpy(ResourceInfo,NewResourceInfo,sizeof(ResourceInfo));
  QMessageBox::information( menu, "AGI studio","Rebuilding is complete !");
  return 0;
}

//***********************************************
//V3 decompression code from QT AGI Utilities

static void initLZW()
{
  if(prefix_code==NULL){
    prefix_code = (unsigned int *)malloc(TABLE_SIZE*sizeof(unsigned int));
    append_character = (byte *)malloc(TABLE_SIZE*sizeof(byte));
  }
}

static void resetLZW()
{
   input_bit_count=0;
   input_bit_buffer=0L;
}


/***************************************************************************
** setBITS
**
** Purpose: To adjust the number of bits used to store codes to the value
** passed in.
***************************************************************************/
static int setBITS(int value)
{
   if (value == MAXBITS) return TRUE;

   BITS = value;
   MAX_VALUE = (1 << BITS) - 1;
   MAX_CODE = MAX_VALUE - 1;
   return FALSE;
}

/***************************************************************************
** decode_string
**
** Purpose: To return the string that the code taken from the input buffer
** represents. The string is returned as a stack, i.e. the characters are
** in reverse order.
***************************************************************************/
static byte *decode_string(byte *buffer,unsigned int code)
{
   int i;

   i=0;
   while (code > 255) {
      *buffer++ = append_character[code];
      code=prefix_code[code];
      if (i++>=4000) {
         menu->errmes("Fatal error during code expansion !");
         return NULL;
      }
   }
   *buffer=code;
   return(buffer);
}
/***************************************************************************
** input_code
**
** Purpose: To return the next code from the input buffer.
***************************************************************************/
static unsigned int input_code(byte **input)
{
   unsigned int return_value;

   while (input_bit_count <= 24) {
      input_bit_buffer |= (unsigned long) *(*input)++ << input_bit_count;
      input_bit_count += 8;
   }

   return_value = (input_bit_buffer & 0x7FFF) % (1 << BITS);
   input_bit_buffer >>= BITS;
   input_bit_count -= BITS;
   return(return_value);
}

/***************************************************************************
** expand
**
** Purpose: To uncompress the data contained in the input buffer and store
** the result in the output buffer. The fileLength parameter says how
** many bytes to uncompress. The compression itself is a form of LZW that
** adjusts the number of bits that it represents its codes in as it fills
** up the available codes. Two codes have special meaning:
**
**  code 256 = start over
**  code 257 = end of data
***************************************************************************/
static void expand(byte *input, byte *output, int fileLength)
{
   int next_code, new_code, old_code;
   int character, /* counter=0, index, */ BITSFull /*, i */;
   byte *string, *endAddr;

   BITSFull = setBITS(START_BITS);  /* Starts at 9-bits */
   next_code = 257;                 /* Next available code to define */

   endAddr = (byte *)((long)output + (long)fileLength);

   old_code = input_code(&input);    /* Read in the first code */
   character = old_code;
   new_code = input_code(&input);

   while ((output < endAddr) && (new_code != 0x101)) {

      if (new_code == 0x100) {      /* Code to "start over" */
	      next_code = 258;
	      BITSFull = setBITS(START_BITS);
	      old_code = input_code(&input);
	      character = old_code;
         *output++ = (char)character;
	      new_code = input_code(&input);
      }
      else {
	      if (new_code >= next_code) { /* Handles special LZW scenario */
	         *decode_stack = character;
	         string = decode_string(decode_stack+1, old_code);
	      }
	      else
	         string = decode_string(decode_stack, new_code);

         /* Reverse order of decoded string and store in output buffer. */
	      character = *string;
	      while (string >= decode_stack)
            *output++ = *string--;

	      if (next_code > MAX_CODE)
	         BITSFull = setBITS(BITS + 1);

         prefix_code[next_code] = old_code;
	      append_character[next_code] = character;
	      next_code++;
	      old_code = new_code;

	      new_code = input_code(&input);
      }
   }
}

//***********************************************
static void convertLOG(byte *logBuf, int logLen)
{
   int startPos, endPos, i, avisPos=0, numMessages;

   /* Find the start and end of the message section */
   startPos = *logBuf + (*(logBuf+1))*256 + 2;
   numMessages = logBuf[startPos];
   endPos = logBuf[startPos+1] + logBuf[startPos+2]*256;
   logBuf += (startPos + 3);
   startPos = (numMessages * 2) + 0;

   /* Encrypt the message section so that it compiles with AGIv2 */
   for (i=startPos; i<endPos&&i<logLen; i++)
	   logBuf[i] ^= EncryptionKey[avisPos++ % 11];

}

//***********************************************
static void DecompressPicture(byte *picBuf,byte *outBuf,int picLen,int *outLen)
{
#define  NORMAL     0
#define  ALTERNATE  1

  byte data, oldData = 0, outData;
  int mode = NORMAL, bufPos = 0;
  byte *out = outBuf;

  while (bufPos < picLen) {

    data = picBuf[bufPos++];

    if (mode == ALTERNATE)
      outData = ((data & 0xF0) >> 4) + ((oldData & 0x0F) << 4);
    else
      outData = data;

    if ((outData == 0xF0) || (outData == 0xF2)) {
      *out++ = outData;
      if (mode == NORMAL) {
        data = picBuf[bufPos++];
        *out++ = (data & 0xF0) >> 4;
        mode = ALTERNATE;
      }
      else {
        *out++  = (data & 0x0F);
        mode = NORMAL;
      }
    }
    else
      *out++ = outData;

    oldData = data;
  }

  *outLen = int(out - outBuf);
}
//***********************************************
int Game::ReadV3Resource(char ResourceType1_c, int ResourceID1)
{

  byte msbyte,lsbyte;
  bool ResourceIsPicture;
  byte VolNumByte;
  int ResourceType1 = (int)ResourceType1_c;

  if(CompressedResource.Data==NULL)
    CompressedResource.Data=(byte *)malloc(MaxResourceSize);


  sprintf(tmp,"%s/%s",dir.c_str(),ResourceInfo[ResourceType1][ResourceID1].Filename);
  FILE *fptr=fopen(tmp,"rb");
  if(fptr==NULL){
    menu->errmes("Error reading file %s/%s",dir.c_str(),ResourceInfo[ResourceType1][ResourceID1].Filename);
    return 1;
  }

  struct stat buf;
  fstat(fileno(fptr),&buf);
  int size=buf.st_size;
  if(ResourceInfo[ResourceType1][ResourceID1].Loc > size){
    menu->errmes("Error: %s: Specified resource location is past end of file",ResourceInfo[ResourceType1][ResourceID1].Filename);
    return 1;
  }
  fseek(fptr,ResourceInfo[ResourceType1][ResourceID1].Loc,SEEK_SET);
  fread(&msbyte,1,1,fptr);
  fread(&lsbyte,1,1,fptr);
  if(!(msbyte==0x12 && lsbyte==0x34)){
    menu->errmes("Error: %s: Resource signature not found",ResourceInfo[ResourceType1][ResourceID1].Filename);
    return 1;
  }
  fread(&VolNumByte,1,1,fptr);
  ResourceIsPicture = ((VolNumByte & 0x80) == 0x80);
  fread(&lsbyte,1,1,fptr);
  fread(&msbyte,1,1,fptr);
  ResourceData.Size = msbyte * 256 + lsbyte;
  fread(&lsbyte,1,1,fptr);
  fread(&msbyte,1,1,fptr);
  CompressedResource.Size = msbyte * 256 + lsbyte;
  fread(CompressedResource.Data,CompressedResource.Size,1,fptr);

  if(ResourceIsPicture){
    DecompressPicture(CompressedResource.Data,ResourceData.Data,CompressedResource.Size,&ResourceData.Size);
  }
  else if(CompressedResource.Size != ResourceData.Size){
    initLZW();
    resetLZW();
    expand(CompressedResource.Data,ResourceData.Data,ResourceData.Size);
    if (ResourceType1 == LOGIC) convertLOG( ResourceData.Data,ResourceData.Size);
  }
  else{
    ResourceData.Size = CompressedResource.Size;
    memcpy( ResourceData.Data, CompressedResource.Data,ResourceData.Size);
  }

  return 0;

}
//*********************************************************
void Game::defaults()
{

  res_default=VIEW;
  save_logic_as_text=true;
  show_elses_as_gotos=false;
  show_all_messages=true;
  show_special_syntax=true;
  reldir=true;
  srcdirname="src";
#ifdef _WIN32
  command="sarien -e -H 0 ./";
  char abspath[256];  // absolute path to the program file
  _fullpath(abspath,_pgmptr,255);
  char *mydir = (char *)(malloc(strlen(abspath)+1));  // will store the program's directory
  strcpy(mydir,abspath);
  char *lastslash = 0;
  lastslash = strrchr(mydir,'\\');
  if (!lastslash)
    lastslash = strrchr(mydir,'/');
  *lastslash = '\0';
  char templatedir_c[256];
  char helpdir_c[256];
  sprintf(templatedir_c,"%s/template",mydir);
  sprintf(helpdir_c,"%s/help",mydir);
  templatedir = templatedir_c;
  helpdir = helpdir_c;
#else
  command="nagi ./ || sarien -e -H 0 ./";
  templatedir="/usr/share/agistudio/template";
  helpdir="/usr/share/agistudio/help";
#endif
  picstyle=P_ONE;
}
//*********************************************************
void Game::read_settings()
  //read ~/.agistudio file
{
  char *home;
  FILE *fptr;
  int n;
  char *ptr;

#ifdef _WIN32
  home = (char *)malloc(256);
  GetWindowsDirectoryA(home,256);
#else
  home = getenv("HOME");
#endif
  if(!home)home=getenv("home");
  if(!home)return;

  sprintf(tmp,"%s/.agistudio",home);
  fptr=fopen(tmp,"rb");
  if(!fptr)return;

  while(fgets(tmp,MAX_TMP,fptr)!=NULL){
    if((ptr=strchr(tmp,0x0a)))*ptr=0;
    if((ptr=strchr(tmp,0x0d)))*ptr=0;
    if(!strncmp(tmp,"res_default=",12)){
      res_default=atoi(tmp+12);
    }
    else if(!strncmp(tmp,"save_logic_as_text=",20)){
      n=atoi(tmp+20);
      save_logic_as_text=(n==1);
    }
    else if(!strncmp(tmp,"show_all_messages=",18)){
      n=atoi(tmp+18);
      show_all_messages=(n==1);
    }
    else if(!strncmp(tmp,"show_elses_as_gotos=",20)){
      n=atoi(tmp+20);
      show_elses_as_gotos=(n==1);
    }
    else if(!strncmp(tmp,"show_special_syntax=",20)){
      n=atoi(tmp+20);
      show_special_syntax=(n==1);
    }
    else if(!strncmp(tmp,"reldir=",8)){
      n=atoi(tmp+8);
      reldir=(n==1);
    }
    else if(!strncmp(tmp,"command=",8)){
      command=string(tmp+8);
    }
    else if(!strncmp(tmp,"srcdirname=",11)){
      srcdirname=string(tmp+11);
    }
    else if(!strncmp(tmp,"template=",9)){
      templatedir=string(tmp+9);
    }
    else if(!strncmp(tmp,"help=",5)){
      helpdir=string(tmp+5);
    }
    else if(!strncmp(tmp,"picstyle=",9)){
      picstyle=atoi(tmp+9);
    }
  }

  fclose(fptr);

}

//*********************************************************
void Game::save_settings()
  //save ~/.agistudio file
{

  char *home;
  FILE *fptr;

#ifdef _WIN32
  home = (char *)malloc(256);
  GetWindowsDirectoryA(home,256);
#else
  home = getenv("HOME");
#endif
  if(!home)home=getenv("home");
  if(!home){
    menu->errmes("Can't determine HOME environment variable !\nSettings were not saved.");
    return;
  }

  sprintf(tmp,"%s/.agistudio",home);
  fptr=fopen(tmp,"wb");
  if(!fptr){
    menu->errmes("Can't open file %s for writing !\nSettings were not saved.",tmp);
    return;
  }
  fprintf(fptr,"res_default=%d\n",res_default);
  fprintf(fptr,"save_logic_as_text=%d\n",save_logic_as_text);
  fprintf(fptr,"show_elses_as_gotos=%d\n",show_elses_as_gotos);
  fprintf(fptr,"show_all_messages=%d\n",show_all_messages);
  fprintf(fptr,"show_special_syntax=%d\n",show_special_syntax);
  fprintf(fptr,"reldir=%d\n",reldir);
  fprintf(fptr,"command=%s\n",command.c_str());
  fprintf(fptr,"srcdirname=%s\n",srcdirname.c_str());
  fprintf(fptr,"template=%s\n",templatedir.c_str());
  fprintf(fptr,"help=%s\n",helpdir.c_str());
  fprintf(fptr,"picstyle=%d\n",picstyle);
  fclose(fptr);

}

//************************************************
int Game::RecompileAll()
{
  int i,ResNum,err;
  FILE *fptr;
  Logic logic;
  char tmp1[16],*ptr;
  extern TStringList InputLines;

  for(i=0;i<MAXWIN;i++){
    if(winlist[i].type==TEXT){
      if(winlist[i].w.t->filename != ""){
        winlist[i].w.t->save();
        winlist[i].w.t->status->message("");
      }
    }
    else if(winlist[i].type==LOGIC){
      if(winlist[i].w.l->filename != ""){
        winlist[i].w.l->save_logic();
        winlist[i].w.l->status->message("");
      }
    }

  }

  int step=0,steps=0;
  for(ResNum=0;ResNum<256;ResNum++){
    if(game->ResourceInfo[LOGIC][ResNum].Exists){
      steps++;
    }
  }


  Q3ProgressDialog progress( "Recompiling all logics...", "Cancel", steps,0, 0, TRUE );
  progress.setMinimumDuration(0);

  for(ResNum=0;ResNum<256;ResNum++){
    if(game->ResourceInfo[LOGIC][ResNum].Exists){
       //look for the source file first
      sprintf(tmp,"%s/logic.%03d",game->srcdir.c_str(),ResNum);
      fptr = fopen(tmp,"rb");
      if(fptr==NULL){
        sprintf(tmp,"%s/logic.%d",game->srcdir.c_str(),ResNum);
        fptr = fopen(tmp,"rb");
      }
      if(fptr==NULL){
        sprintf(tmp,"%s/logic%d.txt",game->srcdir.c_str(),ResNum);
        fptr = fopen(tmp,"rb");
      }
      if(fptr!=NULL){
        InputLines.lfree();
        while(fgets(tmp,MAX_TMP,fptr)!=NULL){
          if((ptr=strchr(tmp,0x0a)))*ptr=0;
          if((ptr=strchr(tmp,0x0d)))*ptr=0;
          //strcat(tmp,"\n");
          InputLines.add(tmp);
        }
        fclose(fptr);
      }
      else{  //source file not found - reading from the game
        err=logic.decode(ResNum);
        if(err){
          sprintf(tmp,"logic.%d",ResNum);
          menu->errmes(tmp,"Errors:\n%s",logic.ErrorList.c_str());
          continue;
        }
        InputLines.lfree();
        string::size_type pos;
        string str=logic.OutputText;
        while((pos=str.find_first_of("\n"))!=string::npos){
          InputLines.add(str.substr(0,pos));
          str=str.substr(pos+1);
        }
        if(str!=""){
          InputLines.add(str);
        }
      }
      err=logic.compile();
      if(!err){
        game->AddResource(LOGIC,ResNum);
      }
      else{
        if(logic.ErrorList != ""){
          sprintf(tmp1,"logic.%d",ResNum);
          menu->errmes(tmp1,"Errors:\n%s",logic.ErrorList.c_str());
        }
      }
      progress.setProgress( step++ );
      if ( progress.wasCancelled() )return 1;
    }
  }

  progress.setProgress( steps );
  QMessageBox::information( menu, "AGI studio","Recompilation is complete !");

  return 0;

}
