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

#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <cstring>

TStringList::TStringList ()
{

  num=0;
  max=0;
  inc=16;
  data=NULL;

}


void TStringList::toLower(void)
{

  for(int i=0;i<num;i++){
    for(int k=0;k<(int)data[i].length();k++){
      data[i][k]=tolower( (int)data[i][k]);
    }
  }


}

void TStringList::toLower(int i)
{


  for(int k=0;k<(int)data[i].length();k++){
    data[i][k]=tolower( (int)data[i][k]);
  }



}

void TStringList::print(void)
{

  for(int i=0;i<num;i++){
    printf("%d %s\n",i,data[i].c_str());
  }

}

void TStringList::print(int n)
{

  printf("num=%d\n",n);
  //  printf("data[%d]=%p\n",num,data[num]);
  printf("%s\n",data[n].c_str());


}
string TStringList::at(int n) const
{

  return (string(data[n].c_str()));


}

void TStringList::del(int n)
{

  for(int i=n+1;i<num;i++){
    data[i-1]=data[i];
  }
  data[num]="";
  num--;

}

void TStringList::replace(int n, const char *str)
{

  data[n]=string(str);

}

void TStringList::replace(int n, const string& str)
{

  data[n]=string(str);

}

void TStringList::add(const string& str)
{

  if(num>=max){
    max+=inc;
    string *data1 = new string[max];
    for(int i=0;i<num;i++){
      data1[i]=data[i];
    }
    delete [] data;
    data=data1;
  }

  // printf("add %d=%s\n",num,str.c_str());
  data[num]=string(str);
  num++;


}


void TStringList::addsorted(const char *str)
{

  addsorted(string(str));

}

void TStringList::addsorted(const string& s)
{

  if(num>=max){
    max+=inc;
    string *data1 = new string[max];
    for(int i=0;i<num;i++){
      data1[i]=data[i];
    }
    delete [] data;
    data=data1;

  }

  for(int i=0;i<num;i++){
    if(data[i] > s){
      for(int k=num;k>i;k--){
        data[k]=data[k-1];
      }
      data[i]=s;
      num++;
      return;
    }
  }
  data[num++]=s;

}

void TStringList::copy(const TStringList& list)
{

  num=list.num;
  max=list.max;
  delete [] data;
  data = new string[max];
  for(int i=0;i<num;i++){
    data[i]=list.data[i];
  }


}
void TStringList::lfree(void)
{

  if(max>0){
    delete [] data;
    data=NULL;
    num=0;
    max=0;
  }

}

//**************************************

string MultStr(const char *str,int NumCopies)
{
  char tmp[256];

  tmp[0]=0;
  for(int i=0;i<NumCopies;i++){
    strcat(tmp,str);
  }
  return string(tmp);
}


string IntToStr(int n)
{
  char tmp[256];
  sprintf(tmp,"%d",n);
  return string(tmp);

}

string IntToStr(byte n)
{
  char tmp[256];
  sprintf(tmp,"%d",(int)n);
  return string(tmp);

}

void toLower(string *str)
{

  for(int i=0;i<(int)str->length();i++){
    str->at(i)=tolower(str->at(i));
  }


}
void toLower(char *str)
{

  for(int i=0;i<(int)strlen(str);i++){
    str[i]=tolower(str[i]);
  }


}
//*********************************
