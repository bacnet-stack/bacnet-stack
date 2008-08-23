/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2003 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330 
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#ifndef KEYLIST_H
#define KEYLIST_H

#include "key.h"

/* This is a key sorted linked list data library that */
/* uses a key or index to access the data. */
/* If the keys are duplicated, they can be added into the list like FIFO */

/* list data and datatype */
struct Keylist_Node {
    KEY key;    /* unique number that is sorted in the list */
    void *data; /* pointer to some data that is stored */
};

typedef struct Keylist {
    struct Keylist_Node **array;        /* array of nodes */
    int count;  /* number of nodes in this list - more effecient than loop */
    int size;   /* number of available nodes on this list - can grow or shrink */
} KEYLIST_TYPE;
typedef KEYLIST_TYPE *OS_Keylist;

/* returns head of the list or NULL on failure. */
OS_Keylist Keylist_Create(
    void);

/* delete specified list */
/* note: you should pop all the nodes off the list first. */
void Keylist_Delete(
    OS_Keylist list);

/* inserts a node into its sorted position */
/* returns the index where it was added */
int Keylist_Data_Add(
    OS_Keylist list,
    KEY key,
    void *data);

/* deletes a node specified by its key */
/* returns the data from the node */
void *Keylist_Data_Delete(
    OS_Keylist list,
    KEY key);

/* deletes a node specified by its index */
/* returns the data from the node */
void *Keylist_Data_Delete_By_Index(
    OS_Keylist list,
    int index);

/* returns the data from last node, and removes it from the list */
void *Keylist_Data_Pop(
    OS_Keylist list);

/* returns the data from the node specified by key */
void *Keylist_Data(
    OS_Keylist list,
    KEY key);

/* returns the data specified by key */
void *Keylist_Data_Index(
    OS_Keylist list,
    int index);

/* return the key at the given index */
KEY Keylist_Key(
    OS_Keylist list,
    int index);

/* returns the next empty key from the list */
KEY Keylist_Next_Empty_Key(
    OS_Keylist list,
    KEY key);

/* returns the number of items in the list */
int Keylist_Count(
    OS_Keylist list);

#endif
