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
#include "words.h"
#include "object.h"
#include "menu.h"

#include <stdio.h>
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>

//****************************************************
//list of inventory objects
ObjList::ObjList()
{

}
//****************************************************
bool ObjList::GetItems()
{
  string ThisItemName;
  int NamePos;
  int CurrentItem,ItemNamesStart,ThisNameStart;
  byte lsbyte,msbyte;

  CurrentItem = 0;
  lsbyte = ResourceData.Data[0];
  msbyte = ResourceData.Data[1];
  ItemNamesStart = msbyte * 256 + lsbyte + 3;
  MaxScreenObjects = ResourceData.Data[2];
  do{
    lsbyte = ResourceData.Data[3+CurrentItem*3];
    msbyte = ResourceData.Data[3+CurrentItem*3+1];
    ThisNameStart = (msbyte<<8)|lsbyte + 3;
    RoomNum[CurrentItem] = ResourceData.Data[3+CurrentItem*3+2];
    NamePos = ThisNameStart;
    if (NamePos > ResourceData.Size)return false; //object name past end of file
    ThisItemName = "";
    do{
      if(ResourceData.Data[NamePos] > 0){
        ThisItemName += ResourceData.Data[NamePos];
        NamePos++;
      }
    }while(ResourceData.Data[NamePos] != 0 && NamePos < ResourceData.Size);
    ItemNames.add(ThisItemName);
    CurrentItem++;

  }while((CurrentItem+1)*3 < ItemNamesStart && CurrentItem < MaxItems);
  return true;
}
//****************************************************
void ObjList::XORData ()
{

  for(int i=0;i<ResourceData.Size;i++){
    ResourceData.Data[i] ^= EncryptionKey[i%11];
  }

}
//****************************************************
int ObjList::read(char *filename,bool FileIsEncrypted)
{

  FILE *fptr=fopen(filename,"rb");
  if(fptr==NULL){
     menu->errmes("Error opening file %s",filename);
     return 1;
  }
  
  struct stat buf;
  fstat(fileno(fptr),&buf);
  int size=buf.st_size;
  if(size > MaxResourceSize){
    menu->errmes("Error:  File %s is too big (>%d bytes)",filename,MaxResourceSize);
    return 1;
  }

  ItemNames.lfree();
  ResourceData.Size = size;
  fread( ResourceData.Data, ResourceData.Size, 1, fptr);
  fclose(fptr);  
  if (FileIsEncrypted) {
    XORData();
  }
  if(!GetItems()){ 
    XORData();
    FileIsEncrypted = !FileIsEncrypted;
    if(!GetItems()){
      menu->errmes("Error! Invalid OBJECT file.");
      return 1;
    }
  }
  if (ItemNames.num == 0){
    menu->errmes("Error! 0 objects in file.");
    return 1; 
  }

  return 0;  
}

//********************************************
int ObjList::save(char *filename,bool FileIsEncrypted)
{
  byte lsbyte,msbyte;
  int ItemNamesStart,ObjectFilePos;
  int CurrentItem,CurrentChar;
  FILE *fptr;

  ResourceData.Size = ItemNames.num*3 + 5;  
  //3 bytes for each index entry, 3 bytes for header, 2 for '?' object
  for (CurrentItem = 1;CurrentItem<=ItemNames.num;CurrentItem++){
    if(ItemNames.at(CurrentItem-1) != "?")
      ResourceData.Size+=ItemNames.at(CurrentItem-1).length()+1;
  }
  
  //create data
  ItemNamesStart = ItemNames.num*3 + 3;
  msbyte = (ItemNamesStart-3) / 256;
  lsbyte = (ItemNamesStart-3) % 256;
  ResourceData.Data[0] = lsbyte;
  ResourceData.Data[1] = msbyte;
  ResourceData.Data[2] = MaxScreenObjects;
  ResourceData.Data[3] = lsbyte;
  ResourceData.Data[4] = msbyte;
  ResourceData.Data[5] = 0;
  ResourceData.Data[ItemNamesStart] = '?';
  ResourceData.Data[ItemNamesStart + 1] = 0;
  ObjectFilePos = ItemNamesStart + 2;
  for (CurrentItem = 1;CurrentItem<=ItemNames.num;CurrentItem++){ 
    if (ItemNames.at(CurrentItem-1) == "?"){
      ResourceData.Data[CurrentItem*3] = ResourceData.Data[0];
      ResourceData.Data[CurrentItem*3+1] = ResourceData.Data[1];
      ResourceData.Data[CurrentItem*3+2] = RoomNum[CurrentItem-1];
    } 
    else{
      msbyte = (ObjectFilePos-3) / 256;
      lsbyte = (ObjectFilePos-3) % 256;
      ResourceData.Data[CurrentItem*3] = lsbyte;;
      ResourceData.Data[CurrentItem*3+1] = msbyte;
      ResourceData.Data[CurrentItem*3+2] = RoomNum[CurrentItem-1];
      for (CurrentChar = 0; CurrentChar<(int)ItemNames.at(CurrentItem-1).length();CurrentChar++){
        ResourceData.Data[ObjectFilePos] = ItemNames.at(CurrentItem-1)[CurrentChar];
        ObjectFilePos++;
      }
      ResourceData.Data[ObjectFilePos] = 0;
      ObjectFilePos++;
    }
  }//end create data
  fptr = fopen(filename,"wb");
  if(fptr==NULL){
    menu->errmes("Error opening file %s !",filename);
    return 1;
  }
  if(FileIsEncrypted)XORData(); 

  fwrite(ResourceData.Data,ResourceData.Size,1,fptr);
  fclose(fptr);
  return 0;

}

//*****************************************
void ObjList::clear()
{

  ItemNames.lfree();
  ItemNames.add("?");
  memset(RoomNum,0,sizeof(RoomNum));

}
