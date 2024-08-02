/**
 * @file
 * @brief Key List library
 * @details This is an enhanced array of pointers to data.
 * The list is sorted, indexed, and keyed. The array is much faster
 * than a linked list.  It stores a pointer to data, which you must
 * malloc and free on your own, or just use static data.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2003
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "bacnet/basic/sys/keylist.h"

/******************************************************************** */
/* Generic node routines */
/******************************************************************** */

/** Grab memory for a node (Keylist_Node).
 *
 * @return Pointer to the allocated memory or
 *         NULL under an Out Of Memory situation.
 */
static struct Keylist_Node *NodeCreate(void)
{
    return calloc(1, sizeof(struct Keylist_Node));
}

/** Grab memory for a list (Keylist).
 *
 * @return Pointer to the allocated memory or
 *         NULL under an Out Of Memory situation.
 */
static struct Keylist *KeylistCreate(void)
{
    return calloc(1, sizeof(struct Keylist));
}

/** Check to see if the array is big enough for an addition
 * or is too big when we are deleting and we can shrink.
 *
 * @param list  Pointer to the list to be tested.
 *
 * @return Returns true if success, false if failed
 */
static bool CheckArraySize(OS_Keylist list)
{
    int new_size = 0; /* set it up so that no size change is the default */
    const int chunk = 8; /* minimum number of nodes to allocate memory for */
    struct Keylist_Node **new_array = NULL; /* new array of nodes, if needed */
    int i; /* counter */
    if (!list) {
        return false;
    }

    /* indicates the need for more memory allocation */
    if (list->count == list->size) {
        new_size = list->size + chunk;

        /* allow for shrinking memory */
    } else if ((list->size > chunk) && (list->count < (list->size - chunk))) {
        new_size = list->size - chunk;
    }
    if (new_size > 0) {
        /* Allocate more room for node pointer array */
        new_array = calloc((size_t)new_size, sizeof(struct Keylist_Node *));

        /* See if we got the memory we wanted */
        if (!new_array) {
            return true;
        }

        /* copy the nodes from the old array to the new array */
        if (list->array) {
            for (i = 0; i < list->count; i++) {
                new_array[i] = list->array[i];
            }
            free(list->array);
        }
        list->array = new_array;
        list->size = new_size;
    }

    return true;
}

/** Find the index of the key that we are looking for.
 * Since it is sorted, we can optimize the search.
 * returns true if found, and false not found.
 * Returns the found key and the index where it was found in parameters.
 * If the key is not found, the nearest index from the bottom will be returned,
 * allowing the ability to find where an key should go into the list.
 *
 * @param list  Pointer to the list
 * @param key  Key to search for
 * @param pIndex  Pointer to the variable taking the index were the key
 *                had been found.
 *
 * @return true if found, and false if not
 */
static bool FindIndex(OS_Keylist list, KEY key, int *pIndex)
{
    struct Keylist_Node *node; /* holds the new node */
    int left = 0; /* the left branch of tree, beginning of list */
    int right = 0; /* the right branch on the tree, end of list */
    int index = 0; /* our current search place in the array */
    KEY current_key = 0; /* place holder for current node key */
    bool status = false; /* return value */

    if (!list) {
        *pIndex = 0;
        return false;
    }
    if (!list->array || !list->count) {
        *pIndex = 0;
        return false;
    }
    right = list->count - 1;
    /* assume that the list is sorted */
    do {
        /* A binary search */
        index = (left + right) / 2;
        node = list->array[index];
        if (!node) {
            break;
        }
        current_key = node->key;
        if (key < current_key) {
            right = index - 1;

        } else {
            left = index + 1;
        }
    } while ((key != current_key) && (left <= right));

    if (key == current_key) {
        status = true;
        *pIndex = index;

    } else {
        /* where the index should be */
        if (key > current_key) {
            *pIndex = index + 1;

        } else {
            *pIndex = index;
        }
    }
    return status;
}

/******************************************************************** */
/* list data functions */
/******************************************************************** */
/** Inserts a node into its sorted position.
 *
 * @param list  Pointer to the list
 * @param key  Key to be inserted
 * @param data  Pointer to the data hold by the key.
 *              This pointer needs to be poiting to static memory
 *              as it will be stored in the list and later used
 *              by retrieving the key again.
 * @return Index of the key, or -1 if not found.
 */
int Keylist_Data_Add(OS_Keylist list, KEY key, void *data)
{
    struct Keylist_Node *node; /* holds the new node */
    int index = -1; /* return value */
    int i; /* counts through the array */

    if (list && CheckArraySize(list)) {
        /* figure out where to put the new node */
        if (list->count) {
            (void)FindIndex(list, key, &index);
            if (index < 0) {
                /* Add to the beginning of the list */
                index = 0;

            } else if (index > list->count) {
                /* Add to the end of the list */
                index = list->count;
            }
            /* Move all the items up to make room for the new one */
            for (i = list->count; i > index; i--) {
                list->array[i] = list->array[i - 1];
            }
        } else {
            index = 0;
        }

        /* create and add the node */
        node = NodeCreate();
        if (node) {
            list->count++;
            node->key = key;
            node->data = data;
            list->array[index] = node;
        }
    }
    return index;
}

/** Deletes a node specified by its index
 * returns the data from the node
 *
 * @param list  Pointer to the list
 * @param index Index of the key to be deleted
 *
 * @returns Pointer to the data of the deleted key or NULL.
 */
void *Keylist_Data_Delete_By_Index(OS_Keylist list, int index)
{
    struct Keylist_Node *node;
    void *data = NULL;

    if (list) {
        if (list->array && list->count && (index >= 0) &&
            (index < list->count)) {
            node = list->array[index];
            if (node) {
                data = node->data;
            }
            /* move the nodes to account for the deleted one */
            if (list->count == 1) {
                /* There is no node shifting to do */
            } else if (index == (list->count - 1)) {
                /* We are the last one */
                /* There is no node shifting to do */
            } else {
                /* Move all the nodes down one */
                int i; /* counter */
                int count = list->count - 1;
                for (i = index; i < count; i++) {
                    list->array[i] = list->array[i + 1];
                }
            }
            list->count--;
            if (node) {
                free(node);
            }

            /* potentially reduce the size of the array */
            (void)CheckArraySize(list);
        }
    }
    return (data);
}

/** Deletes a node specified by its key/
 * returns the data from the node
 *
 * @param list  Pointer to the list
 * @param key  Key to be deleted
 *
 * @returns Pointer to the data of the deleted key or NULL.
 */
void *Keylist_Data_Delete(OS_Keylist list, KEY key)
{
    void *data = NULL; /* return value */
    int index; /* where the node is in the array */

    if (list) {
        if (FindIndex(list, key, &index)) {
            data = Keylist_Data_Delete_By_Index(list, index);
        }
    }

    return data;
}

/**
 * @brief Pops every node data, removing it from the list,
 *  and frees the data memory
 * @param list Pointer to the list
 * */
void Keylist_Data_Free(OS_Keylist list)
{
    void *data;
    while (Keylist_Count(list) > 0) {
        data = Keylist_Data_Pop(list);
        free(data);
    }
}

/** Returns the data from last node, and
 * removes it from the list.
 *
 * @param list  Pointer to the list
 *
 * @return Pointer to the data that might be NULL.
 */
void *Keylist_Data_Pop(OS_Keylist list)
{
    void *data = NULL; /* return value */
    int index; /* position in the array */

    if (list) {
        if (list->count) {
            index = list->count - 1;
            data = Keylist_Data_Delete_By_Index(list, index);
        }
    }

    return data;
}

/** Returns the data from the node specified by key.
 *
 * @param list  Pointer to the list
 * @param key  Key to be deleted
 *
 * @returns Pointer to the data, that might be NULL.
 */
void *Keylist_Data(OS_Keylist list, KEY key)
{
    struct Keylist_Node *node = NULL;
    int index = 0; /* used to look up the index of node */

    if (list) {
        if (list->array && list->count) {
            if (FindIndex(list, key, &index)) {
                node = list->array[index];
            }
        }
    }
    return node ? node->data : NULL;
}

/** Returns the index from the node specified by key.
 *
 * @param list  Pointer to the list
 * @param key  Key whose index shall be retrieved.
 *
 * @return Index of the key or -1, if not found.
 */
int Keylist_Index(OS_Keylist list, KEY key)
{
    int index = -1; /* used to look up the index of node */

    if (list) {
        if (list->array && list->count) {
            if (!FindIndex(list, key, &index)) {
                index = -1;
            }
        }
    }
    return index;
}

/** Returns the data specified by index
 *
 * @param list  Pointer to the list
 * @param index  Key whose data shall be retrieved.
 *
 * @return Pointer to the data that might be NULL.
 */
void *Keylist_Data_Index(OS_Keylist list, int index)
{
    struct Keylist_Node *node = NULL;

    if (list) {
        if (list->array && list->count && (index >= 0) &&
            (index < list->count)) {
            node = list->array[index];
        }
    }
    return node ? node->data : NULL;
}

/** Return the key at the given index.
 *
 * @param list  Pointer to the list
 * @param index  Index that shall be returned
 *
 * @return Key for the index or UINT32_MAX if not found.
 * @deprecated Use Keylist_Index_Key() instead
 */
KEY Keylist_Key(OS_Keylist list, int index)
{
    KEY key = UINT32_MAX; /* return value */
    struct Keylist_Node *node;

    if (list) {
        if (list->array && list->count && (index >= 0) &&
            (index < list->count)) {
            node = list->array[index];
            if (node) {
                key = node->key;
            }
        }
    }
    return key;
}

/**
 * Determine if there is a node key at the given index.
 *
 * @param list  Pointer to the list
 * @param index  Index that shall be returned
 * @param pKey  Pointer to the variable returning the key
 * @return True if the key is found, false if not.
 */
bool Keylist_Index_Key(OS_Keylist list, int index, KEY *pKey)
{
    bool status = false; /* return value */
    struct Keylist_Node *node;

    if (list) {
        if (list->array && list->count && (index >= 0) &&
            (index < list->count)) {
            node = list->array[index];
            if (node) {
                status = true;
                if (pKey) {
                    *pKey = node->key;
                }
            }
        }
    }

    return status;
}

/** Returns the next empty key from the list.
 *
 * @param list  Pointer to the list
 * @param key  Key whose index shall be retrieved.
 *
 * @return Next empty key or 'key=key' if there is none.
 */
KEY Keylist_Next_Empty_Key(OS_Keylist list, KEY key)
{
    int index;

    if (list) {
        while (FindIndex(list, key, &index)) {
            if (KEY_LAST(key)) {
                break;
            }
            key++;
        }
    }

    return key;
}

/** Return the number of nodes in this list.
 *
 * @param list  Pointer to the list
 *
 * @return Count of key in the list.
 */
int Keylist_Count(OS_Keylist list)
{
    int cnt = 0;

    if (list) {
        cnt = list->count;
    }

    return (cnt);
}

/******************************************************************** */
/* Public List functions */
/******************************************************************** */

/** Returns head of the list or NULL on failure.
 *
 * @return Pointer to the key list or NUL if creation failed.
 */
OS_Keylist Keylist_Create(void)
{
    struct Keylist *list;

    list = KeylistCreate();
    if (list) {
        CheckArraySize(list);
    }

    return list;
}

/** Delete specified list.
 *
 * @param list  Pointer to the list
 */
void Keylist_Delete(OS_Keylist list)
{ /* list number to be deleted */
    if (list) {
        /* clean out the list */
        while (list->count) {
            (void)Keylist_Data_Delete_By_Index(list, 0);
        }
        if (list->array) {
            free(list->array);
        }
        free(list);
    }

    return;
}
