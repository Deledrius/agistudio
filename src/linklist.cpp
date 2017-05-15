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

/*************************************************************************
** linklist.c
**
** The AGI PICTURE codes will be stored in a doubly linked list which
** will make the editing a lot more simple and quicker.
**
** (c) Lance Ewing 1997
*************************************************************************/


#include <stdio.h>
#include <stdlib.h>

#include "linklist.h"
#include "picture.h"

void Picture::dldelete()
{
    struct picCodeNode *temp;

    if (picPos == NULL)
        return;

    if (picPos == picStart)
        picStart = picPos->next;
    if (picPos == picLast)
        picLast = picPos->prior;

    if (picPos->prior != NULL)
        picPos->prior->next = picPos->next;
    if (picPos->next != NULL)
        picPos->next->prior = picPos->prior;

    temp = picPos->next;
    free(picPos);
    picPos = temp;

    bufLen--;
    /* bufPos should still be the same */
}

void Picture::removeAction()
{
    if (picPos != NULL) {
        dldelete();
        while ((picPos != NULL) && (picPos->node < 0xF0))
            dldelete();
    }
}

void Picture::wipeAction()
{
    if (picPos != NULL) {
        dldelete();
        while (picPos != NULL)
            dldelete();
    }
}

void Picture::dlstore(struct picCodeNode *i)
{
    if ((picStart == NULL) && (picLast == NULL))   {
        picStart = picLast = i;
        i->next = NULL;
        i->prior = NULL;
        picPos = NULL;
    } else if (picPos == NULL) { // End Node
        i->next = NULL;
        i->prior = picLast;
        picLast->next = i;
        picLast = i;
        picPos = NULL;
    } else {
        switch (addMode) {
            case INS_MODE:
                i->prior = picPos->prior;   // works for picStart as well
                i->next = picPos;
                if (picPos != picStart)
                    picPos->prior->next = i;
                if (picPos == picStart)
                    picStart = i;
                picPos->prior = i;
                // picPos = i;
                break;

            case OVR_MODE:
                i->prior = picPos->prior;    // link from i
                i->next = picPos->next;
                if (picPos != picStart)
                    picPos->prior->next = i;
                if (picPos == picStart)
                    picStart = i;
                if (picPos != picLast)
                    picPos->next->prior = i;
                if (picPos == picLast)
                    picLast = i;
                free(picPos);
                picPos = i->next;
                break;
        }
    }

    bufLen++;
    bufPos++;
}

void Picture::displayList()
{
    struct picCodeNode *temp;

    temp = picStart;
    printf("%02X ", temp->node);

    do {
        temp = temp->next;
        printf("%X ", temp->node);
    } while (temp->next != NULL);

    printf("\n");
}

void Picture::freeList()
{
    struct picCodeNode *temp, *store;

    if ((picStart != NULL) && (picLast != NULL)) {

        temp = picStart;
        store = temp->next;
        free(temp);

        do {
            temp = store;
            store = temp->next;
            free(temp);
        } while (store != NULL);

        picStart = picLast = picPos = NULL;
    }

    bufPos = 0;
    bufLen = 0;
}

void Picture::moveBack()
{
    if (picPos == NULL) {
        picPos = picLast;
        if (bufPos > 0)
            bufPos--;
    } else {
        if (picPos->prior != NULL) {
            picPos = picPos->prior;
            bufPos--;
        }
    }
}

void Picture::moveForward()
{
    if (picPos != NULL) {
        picPos = picPos->next;
        bufPos++;
    }
}

void Picture::moveToStart()
{
    picPos = picStart;
    bufPos = 0;
}

void Picture::moveToEnd()
{
    picPos = NULL;
    bufPos = bufLen;
}

void Picture::moveBackAction()
{
    do {
        moveBack();
        if (picPos == picStart)
            break;
    } while (picPos->node < 0xF0);
}

void Picture::moveForwardAction()
{
    do {
        moveForward();
        if (picPos == NULL)
            break;
    } while (picPos->node < 0xF0);
}


/* LIST TEST PROGRAM
void main()
{
   struct picCodeNode *trial, *trial2, *trial3, *temp;
   int i;

   trial = (struct picCodeNode *)malloc(sizeof(picCodes));
   trial2 = (struct picCodeNode *)malloc(sizeof(picCodes));
   trial3 = (struct picCodeNode *)malloc(sizeof(picCodes));

   trial->node = 0x64;
   trial2->node = 0x50;
   trial3->node = 0x40;
   dlstore(trial);
   dlstore(trial2);
   dlstore(trial3);

   for (i=0; i<10; i++) {
      temp = (struct picCodeNode *)malloc(sizeof(picCodes));
      temp->node = i;
      dlstore(temp);
   }

   clrscr();
   displayList();
   moveBack();
   temp = (struct picCodeNode *)malloc(sizeof(picCodes));
   temp->node = 0x3F;
   dlstore(temp);
   displayList();

   moveBack();
   moveBack();
   moveBack();
   addMode = OVR_MODE;
   temp = (struct picCodeNode *)malloc(sizeof(picCodes));
   temp->node = 0x4F;
   dlstore(temp);
   displayList();

   moveToStart();
   temp = (struct picCodeNode *)malloc(sizeof(picCodes));
   temp->node = 0x8F;
   dlstore(temp);
   displayList();

   moveToStart();
   addMode = INS_MODE;
   temp = (struct picCodeNode *)malloc(sizeof(picCodes));
   temp->node = 0x7F;
   dlstore(temp);
   displayList();

   moveForward();
   moveForward();
   moveForward();
   moveForward();
   temp = (struct picCodeNode *)malloc(sizeof(picCodes));
   temp->node = 0x6F;
   dlstore(temp);
   displayList();

   moveToEnd();
   temp = (struct picCodeNode *)malloc(sizeof(picCodes));
   temp->node = 0x5F;
   dlstore(temp);
   displayList();

   freeList();
}
*/
