/*
 * plist.c
 * Builds plist XML structures.
 *
 * Copyright (c) 2008 Zach C. All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#include <string.h>
#include <assert.h>
#include "plist.h"
#include <stdlib.h>
#include <stdio.h>

#include <node.h>
#include <node_iterator.h>

plist_t plist_new_node(plist_data_t data)
{
    return (plist_t) node_create(NULL, data);
}

plist_data_t plist_get_data(const plist_t node)
{
    if (!node)
        return NULL;
    return ((node_t*)node)->data;
}

plist_data_t plist_new_plist_data(void)
{
    plist_data_t data = (plist_data_t) calloc(sizeof(struct plist_data_s), 1);
    return data;
}

static void plist_free_data(plist_data_t data)
{
    if (data)
    {
        switch (data->type)
        {
        case PLIST_KEY:
        case PLIST_STRING:
            free(data->strval);
            break;
        case PLIST_DATA:
            free(data->buff);
            break;
        default:
            break;
        }
        free(data);
    }
}

static int plist_free_node(node_t* node)
{
    plist_data_t data = NULL;
    int index = node_detach(node->parent, node);
    data = plist_get_data(node);
    plist_free_data(data);
    node->data = NULL;

    node_iterator_t *ni = node_iterator_create(node->children);
    node_t *ch;
    while ((ch = node_iterator_next(ni))) {
        plist_free_node(ch);
    }
    node_iterator_destroy(ni);

    node_destroy(node);

    return index;
}

plist_t plist_new_dict(void)
{
    plist_data_t data = plist_new_plist_data();
    data->type = PLIST_DICT;
    return plist_new_node(data);
}

plist_t plist_new_array(void)
{
    plist_data_t data = plist_new_plist_data();
    data->type = PLIST_ARRAY;
    return plist_new_node(data);
}

//These nodes should not be handled by users
static plist_t plist_new_key(const char *val)
{
    plist_data_t data = plist_new_plist_data();
    data->type = PLIST_KEY;
    data->strval = strdup(val);
    data->length = strlen(val);
    return plist_new_node(data);
}

plist_t plist_new_string(const char *val)
{
    plist_data_t data = plist_new_plist_data();
    data->type = PLIST_STRING;
    data->strval = strdup(val);
    data->length = strlen(val);
    return plist_new_node(data);
}

plist_t plist_new_bool(uint8_t val)
{
    plist_data_t data = plist_new_plist_data();
    data->type = PLIST_BOOLEAN;
    data->boolval = val;
    data->length = sizeof(uint8_t);
    return plist_new_node(data);
}

plist_t plist_new_uint(uint64_t val)
{
    plist_data_t data = plist_new_plist_data();
    data->type = PLIST_UINT;
    data->intval = val;
    data->length = sizeof(uint64_t);
    return plist_new_node(data);
}

plist_t plist_new_uid(uint64_t val)
{
    plist_data_t data = plist_new_plist_data();
    data->type = PLIST_UID;
    data->intval = val;
    data->length = sizeof(uint64_t);
    return plist_new_node(data);
}

plist_t plist_new_real(double val)
{
    plist_data_t data = plist_new_plist_data();
    data->type = PLIST_REAL;
    data->realval = val;
    data->length = sizeof(double);
    return plist_new_node(data);
}

plist_t plist_new_data(const char *val, uint64_t length)
{
    plist_data_t data = plist_new_plist_data();
    data->type = PLIST_DATA;
    data->buff = (uint8_t *) malloc(length);
    memcpy(data->buff, val, length);
    data->length = length;
    return plist_new_node(data);
}

plist_t plist_new_date(int32_t sec, int32_t usec)
{
    plist_data_t data = plist_new_plist_data();
    data->type = PLIST_DATE;
    data->timeval.tv_sec = sec;
    data->timeval.tv_usec = usec;
    data->length = sizeof(struct timeval);
    return plist_new_node(data);
}

void plist_free(plist_t plist)
{
    if (plist)
    {
        plist_free_node(plist);
    }
}

static void plist_copy_node(node_t *node, void *parent_node_ptr)
{
    plist_type node_type = PLIST_NONE;
    plist_t newnode = NULL;
    plist_data_t data = plist_get_data(node);
    plist_data_t newdata = plist_new_plist_data();

    assert(data);				// plist should always have data

    memcpy(newdata, data, sizeof(struct plist_data_s));

    node_type = plist_get_node_type(node);
    if (node_type == PLIST_DATA || node_type == PLIST_STRING || node_type == PLIST_KEY)
    {
        switch (node_type)
        {
        case PLIST_DATA:
            newdata->buff = (uint8_t *) malloc(data->length);
            memcpy(newdata->buff, data->buff, data->length);
            break;
        case PLIST_KEY:
        case PLIST_STRING:
            newdata->strval = strdup((char *) data->strval);
            break;
        default:
            break;
        }
    }
    newnode = plist_new_node(newdata);

    if (*(plist_t*)parent_node_ptr)
    {
        node_attach(*(plist_t*)parent_node_ptr, newnode);
    }
    else
    {
        *(plist_t*)parent_node_ptr = newnode;
    }

    node_iterator_t *ni = node_iterator_create(node->children);
    node_t *ch;
    while ((ch = node_iterator_next(ni))) {
        plist_copy_node(ch, &newnode);
    }
    node_iterator_destroy(ni);
}

plist_t plist_copy(plist_t node)
{
    plist_t copied = NULL;
    plist_copy_node(node, &copied);
    return copied;
}

uint32_t plist_array_get_size(plist_t node)
{
    uint32_t ret = 0;
    if (node && PLIST_ARRAY == plist_get_node_type(node))
    {
        ret = node_n_children(node);
    }
    return ret;
}

plist_t plist_array_get_item(plist_t node, uint32_t n)
{
    plist_t ret = NULL;
    if (node && PLIST_ARRAY == plist_get_node_type(node))
    {
        ret = (plist_t)node_nth_child(node, n);
    }
    return ret;
}

uint32_t plist_array_get_item_index(plist_t node)
{
    plist_t father = plist_get_parent(node);
    if (PLIST_ARRAY == plist_get_node_type(father))
    {
        return node_child_position(father, node);
    }
    return 0;
}

void plist_array_set_item(plist_t node, plist_t item, uint32_t n)
{
    if (node && PLIST_ARRAY == plist_get_node_type(node))
    {
        plist_t old_item = plist_array_get_item(node, n);
        if (old_item)
        {
            int idx = plist_free_node(old_item);
	    if (idx < 0) {
		node_attach(node, item);
	    } else {
		node_insert(node, idx, item);
	    }
        }
    }
    return;
}

void plist_array_append_item(plist_t node, plist_t item)
{
    if (node && PLIST_ARRAY == plist_get_node_type(node))
    {
        node_attach(node, item);
    }
    return;
}

void plist_array_insert_item(plist_t node, plist_t item, uint32_t n)
{
    if (node && PLIST_ARRAY == plist_get_node_type(node))
    {
        node_insert(node, n, item);
    }
    return;
}

void plist_array_remove_item(plist_t node, uint32_t n)
{
    if (node && PLIST_ARRAY == plist_get_node_type(node))
    {
        plist_t old_item = plist_array_get_item(node, n);
        if (old_item)
        {
            plist_free(old_item);
        }
    }
    return;
}

uint32_t plist_dict_get_size(plist_t node)
{
    uint32_t ret = 0;
    if (node && PLIST_DICT == plist_get_node_type(node))
    {
        ret = node_n_children(node) / 2;
    }
    return ret;
}

void plist_dict_new_iter(plist_t node, plist_dict_iter *iter)
{
    if (iter && *iter == NULL)
    {
        *iter = malloc(sizeof(uint32_t));
        *((uint32_t*)(*iter)) = 0;
    }
    return;
}

void plist_dict_next_item(plist_t node, plist_dict_iter iter, char **key, plist_t *val)
{
    uint32_t* iter_int = (uint32_t*) iter;

    if (key)
    {
        *key = NULL;
    }
    if (val)
    {
        *val = NULL;
    }

    if (node && PLIST_DICT == plist_get_node_type(node) && *iter_int < node_n_children(node))
    {

        if (key)
        {
            plist_get_key_val((plist_t)node_nth_child(node, *iter_int), key);
        }

        if (val)
        {
            *val = (plist_t) node_nth_child(node, *iter_int + 1);
        }

        *iter_int += 2;
    }
    return;
}

void plist_dict_get_item_key(plist_t node, char **key)
{
    plist_t father = plist_get_parent(node);
    if (PLIST_DICT == plist_get_node_type(father))
    {
        plist_get_key_val( (plist_t) node_prev_sibling(node), key);
    }
}

plist_t plist_dict_get_item(plist_t node, const char* key)
{
    plist_t ret = NULL;

    if (node && PLIST_DICT == plist_get_node_type(node))
    {

        plist_t current = NULL;
        for (current = (plist_t)node_first_child(node);
                current;
                current = (plist_t)node_next_sibling(node_next_sibling(current)))
        {

            plist_data_t data = plist_get_data(current);
            assert( PLIST_KEY == plist_get_node_type(current) );

            if (data && !strcmp(key, data->strval))
            {
                ret = (plist_t)node_next_sibling(current);
                break;
            }
        }
    }
    return ret;
}

void plist_dict_set_item(plist_t node, const char* key, plist_t item)
{
    if (node && PLIST_DICT == plist_get_node_type(node)) {
        node_t* old_item = plist_dict_get_item(node, key);
        if (old_item) {
            int idx = plist_free_node(old_item);
            if (idx < 0) {
                node_attach(node, item);
            } else {
                node_insert(node, idx, item);
            }
        } else {
            node_attach(node, plist_new_key(key));
            node_attach(node, item);
        }
    }
    return;
}

void plist_dict_insert_item(plist_t node, const char* key, plist_t item)
{
    plist_dict_set_item(node, key, item);
}

void plist_dict_remove_item(plist_t node, const char* key)
{
    if (node && PLIST_DICT == plist_get_node_type(node))
    {
        plist_t old_item = plist_dict_get_item(node, key);
        if (old_item)
        {
            plist_t key_node = node_prev_sibling(old_item);
            plist_free(key_node);
            plist_free(old_item);
        }
    }
    return;
}

void plist_dict_merge(plist_t *target, plist_t source)
{
	if (!target || !*target || (plist_get_node_type(*target) != PLIST_DICT) || !source || (plist_get_node_type(source) != PLIST_DICT))
		return;

	char* key = NULL;
	plist_dict_iter it = NULL;
	plist_t subnode = NULL;
	plist_dict_new_iter(source, &it);
	if (!it)
		return;

	do {
		plist_dict_next_item(source, it, &key, &subnode);
		if (!key)
			break;

		if (plist_dict_get_item(*target, key) != NULL)
			plist_dict_remove_item(*target, key);

		plist_dict_set_item(*target, key, plist_copy(subnode));
		free(key);
		key = NULL;
	} while (1);
	free(it);	
}

plist_t plist_access_pathv(plist_t plist, uint32_t length, va_list v)
{
    plist_t current = plist;
    plist_type type = PLIST_NONE;
    uint32_t i = 0;

    for (i = 0; i < length && current; i++)
    {
        type = plist_get_node_type(current);

        if (type == PLIST_ARRAY)
        {
            uint32_t n = va_arg(v, uint32_t);
            current = plist_array_get_item(current, n);
        }
        else if (type == PLIST_DICT)
        {
            const char* key = va_arg(v, const char*);
            current = plist_dict_get_item(current, key);
        }
    }
    return current;
}

plist_t plist_access_path(plist_t plist, uint32_t length, ...)
{
    plist_t ret = NULL;
    va_list v;

    va_start(v, length);
    ret = plist_access_pathv(plist, length, v);
    va_end(v);
    return ret;
}

static void plist_get_type_and_value(plist_t node, plist_type * type, void *value, uint64_t * length)
{
    plist_data_t data = NULL;

    if (!node)
        return;

    data = plist_get_data(node);

    *type = data->type;
    *length = data->length;

    switch (*type)
    {
    case PLIST_BOOLEAN:
        *((char *) value) = data->boolval;
        break;
    case PLIST_UINT:
    case PLIST_UID:
        *((uint64_t *) value) = data->intval;
        break;
    case PLIST_REAL:
        *((double *) value) = data->realval;
        break;
    case PLIST_KEY:
    case PLIST_STRING:
        *((char **) value) = strdup(data->strval);
        break;
    case PLIST_DATA:
        *((uint8_t **) value) = (uint8_t *) malloc(*length * sizeof(uint8_t));
        memcpy(*((uint8_t **) value), data->buff, *length * sizeof(uint8_t));
        break;
    case PLIST_DATE:
        //exception : here we use memory on the stack since it is just a temporary buffer
        ((struct timeval*) value)->tv_sec = data->timeval.tv_sec;
        ((struct timeval*) value)->tv_usec = data->timeval.tv_usec;
        break;
    case PLIST_ARRAY:
    case PLIST_DICT:
    default:
        break;
    }
}

plist_t plist_get_parent(plist_t node)
{
    return node ? (plist_t) ((node_t*) node)->parent : NULL;
}

plist_type plist_get_node_type(plist_t node)
{
    if (node)
    {
        plist_data_t data = plist_get_data(node);
        if (data)
            return data->type;
    }
    return PLIST_NONE;
}

void plist_get_key_val(plist_t node, char **val)
{
    plist_type type = plist_get_node_type(node);
    uint64_t length = 0;
    if (PLIST_KEY == type)
        plist_get_type_and_value(node, &type, (void *) val, &length);
    assert(length == strlen(*val));
}

void plist_get_string_val(plist_t node, char **val)
{
    plist_type type = plist_get_node_type(node);
    uint64_t length = 0;
    if (PLIST_STRING == type)
        plist_get_type_and_value(node, &type, (void *) val, &length);
    assert(length == strlen(*val));
}

void plist_get_bool_val(plist_t node, uint8_t * val)
{
    plist_type type = plist_get_node_type(node);
    uint64_t length = 0;
    if (PLIST_BOOLEAN == type)
        plist_get_type_and_value(node, &type, (void *) val, &length);
    assert(length == sizeof(uint8_t));
}

void plist_get_uint_val(plist_t node, uint64_t * val)
{
    plist_type type = plist_get_node_type(node);
    uint64_t length = 0;
    if (PLIST_UINT == type)
        plist_get_type_and_value(node, &type, (void *) val, &length);
    assert(length == sizeof(uint64_t));
}

void plist_get_uid_val(plist_t node, uint64_t * val)
{
    plist_type type = plist_get_node_type(node);
    uint64_t length = 0;
    if (PLIST_UID == type)
        plist_get_type_and_value(node, &type, (void *) val, &length);
    assert(length == sizeof(uint64_t));
}

void plist_get_real_val(plist_t node, double *val)
{
    plist_type type = plist_get_node_type(node);
    uint64_t length = 0;
    if (PLIST_REAL == type)
        plist_get_type_and_value(node, &type, (void *) val, &length);
    assert(length == sizeof(double));
}

void plist_get_data_val(plist_t node, char **val, uint64_t * length)
{
    plist_type type = plist_get_node_type(node);
    if (PLIST_DATA == type)
        plist_get_type_and_value(node, &type, (void *) val, length);
}

void plist_get_date_val(plist_t node, int32_t * sec, int32_t * usec)
{
    plist_type type = plist_get_node_type(node);
    uint64_t length = 0;
    struct timeval val = { 0, 0 };
    if (PLIST_DATE == type)
        plist_get_type_and_value(node, &type, (void *) &val, &length);
    assert(length == sizeof(struct timeval));
    *sec = val.tv_sec;
    *usec = val.tv_usec;
}

int plist_data_compare(const void *a, const void *b)
{
    plist_data_t val_a = NULL;
    plist_data_t val_b = NULL;

    if (!a || !b)
        return FALSE;

    if (!((node_t*) a)->data || !((node_t*) b)->data)
        return FALSE;

    val_a = plist_get_data((plist_t) a);
    val_b = plist_get_data((plist_t) b);

    if (val_a->type != val_b->type)
        return FALSE;

    switch (val_a->type)
    {
    case PLIST_BOOLEAN:
    case PLIST_UINT:
    case PLIST_REAL:
    case PLIST_UID:
        if (val_a->intval == val_b->intval)	//it is an union so this is sufficient
            return TRUE;
        else
            return FALSE;

    case PLIST_KEY:
    case PLIST_STRING:
        if (!strcmp(val_a->strval, val_b->strval))
            return TRUE;
        else
            return FALSE;

    case PLIST_DATA:
        if (val_a->length != val_b->length)
            return FALSE;
        if (!memcmp(val_a->buff, val_b->buff, val_a->length))
            return TRUE;
        else
            return FALSE;
    case PLIST_ARRAY:
    case PLIST_DICT:
        //compare pointer
        if (a == b)
            return TRUE;
        else
            return FALSE;
        break;
    case PLIST_DATE:
        if (!memcmp(&(val_a->timeval), &(val_b->timeval), sizeof(struct timeval)))
            return TRUE;
        else
            return FALSE;
    default:
        break;
    }
    return FALSE;
}

char plist_compare_node_value(plist_t node_l, plist_t node_r)
{
    return plist_data_compare(node_l, node_r);
}

static void plist_set_element_val(plist_t node, plist_type type, const void *value, uint64_t length)
{
    //free previous allocated buffer
    plist_data_t data = plist_get_data(node);
    assert(data);				// a node should always have data attached

    switch (data->type)
    {
    case PLIST_KEY:
    case PLIST_STRING:
        free(data->strval);
        data->strval = NULL;
        break;
    case PLIST_DATA:
        free(data->buff);
        data->buff = NULL;
        break;
    default:
        break;
    }

    //now handle value

    data->type = type;
    data->length = length;

    switch (type)
    {
    case PLIST_BOOLEAN:
        data->boolval = *((char *) value);
        break;
    case PLIST_UINT:
    case PLIST_UID:
        data->intval = *((uint64_t *) value);
        break;
    case PLIST_REAL:
        data->realval = *((double *) value);
        break;
    case PLIST_KEY:
    case PLIST_STRING:
        data->strval = strdup((char *) value);
        break;
    case PLIST_DATA:
        data->buff = (uint8_t *) malloc(length);
        memcpy(data->buff, value, length);
        break;
    case PLIST_DATE:
        data->timeval.tv_sec = ((struct timeval*) value)->tv_sec;
        data->timeval.tv_usec = ((struct timeval*) value)->tv_usec;
        break;
    case PLIST_ARRAY:
    case PLIST_DICT:
    default:
        break;
    }
}

void plist_set_type(plist_t node, plist_type type)
{
    if ( node_n_children(node) == 0 )
    {
        plist_data_t data = plist_get_data(node);
        plist_free_data( data );
        data = plist_new_plist_data();
        data->type = type;
        switch (type)
        {
        case PLIST_BOOLEAN:
            data->length = sizeof(uint8_t);
            break;
        case PLIST_UINT:
        case PLIST_UID:
            data->length = sizeof(uint64_t);
            break;
        case PLIST_REAL:
            data->length = sizeof(double);
            break;
        case PLIST_DATE:
            data->length = sizeof(struct timeval);
            break;
        default:
            data->length = 0;
            break;
        }
    }
}

void plist_set_key_val(plist_t node, const char *val)
{
    plist_set_element_val(node, PLIST_KEY, val, strlen(val));
}

void plist_set_string_val(plist_t node, const char *val)
{
    plist_set_element_val(node, PLIST_STRING, val, strlen(val));
}

void plist_set_bool_val(plist_t node, uint8_t val)
{
    plist_set_element_val(node, PLIST_BOOLEAN, &val, sizeof(uint8_t));
}

void plist_set_uint_val(plist_t node, uint64_t val)
{
    plist_set_element_val(node, PLIST_UINT, &val, sizeof(uint64_t));
}

void plist_set_uid_val(plist_t node, uint64_t val)
{
    plist_set_element_val(node, PLIST_UID, &val, sizeof(uint64_t));
}

void plist_set_real_val(plist_t node, double val)
{
    plist_set_element_val(node, PLIST_REAL, &val, sizeof(double));
}

void plist_set_data_val(plist_t node, const char *val, uint64_t length)
{
    plist_set_element_val(node, PLIST_DATA, val, length);
}

void plist_set_date_val(plist_t node, int32_t sec, int32_t usec)
{
    struct timeval val = { sec, usec };
    plist_set_element_val(node, PLIST_DATE, &val, sizeof(struct timeval));
}

