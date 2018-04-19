/*
 * json-out.c -- JSON output
 *
 * Copyright (c) 2018 Virtuozzo International GmbH
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Public
 * License.
 * %End-Header%
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "json-out.h"

static void *xmalloc(size_t size)
{
	void *p = malloc(size);

	if (!p)
		exit(1);

	return p;
}

static void *xstrdup(const char *str)
{
	size_t size = strlen(str) + 1;
	char *p = xmalloc(size);

	return strcpy(p, str);
}

struct json_list *json_list_create(enum json_val_type type)
{
	struct json_list *list = xmalloc(sizeof(struct json_list));

	list->node = NULL;
	list->type = type;

	return list;
}

struct json_list *json_list_create_in_obj(struct json_obj *parent_obj, char *key, enum json_val_type type)
{
	struct json_list *list = json_list_create(type);

	json_obj_add_list(parent_obj, key, list);

	return list;
}

void json_list_delete(struct json_list *list)
{
	struct json_list_node *node = list->node, *next;

	while (node) {
		switch (list->type) {
		case JSON_VAL_STRING:
			free(node->val.str);
			break;
		case JSON_VAL_LIST:
			json_list_delete(node->val.list);
			break;
		case JSON_VAL_OBJECT:
			json_obj_delete(node->val.obj);
			break;
		default:
			break;
		}
		next = node->next;
		free(node);
		node = next;
	}
	free(list);
}

struct json_obj *json_obj_create(void)
{
	struct json_obj *obj = xmalloc(sizeof(struct json_obj));

	obj->pair = NULL;

	return obj;
}

struct json_obj *json_obj_create_in_obj(struct json_obj *parent_obj, char *key)
{
	struct json_obj *obj = json_obj_create();

	json_obj_add_obj(parent_obj, key, obj);

	return obj;
}

static void json_pair_delete(struct json_pair *pair)
{
	switch (pair->type)
	{
	case JSON_VAL_STRING:
		free(pair->val.str);
		break;
	case JSON_VAL_LIST:
		json_list_delete(pair->val.list);
		break;
	case JSON_VAL_OBJECT:
		json_obj_delete(pair->val.obj);
		break;
	default:
		break;
	}
	free(pair->key);
	free(pair);
}

void json_obj_delete(struct json_obj *obj)
{
	struct json_pair *pair = obj->pair, *next;

	while (pair) {
		next = pair->next;
		json_pair_delete(pair);
		pair = next;
	}
	free(obj);
}

void json_obj_delete_pair(struct json_obj *obj, char *key)
{
	struct json_pair *pair = obj->pair, *next, *prev = NULL;

	while (pair) {
		next = pair->next;
		if (!strcmp(pair->key, key)) {
			json_pair_delete(pair);
			if (prev)
				prev->next = next;
			else
				obj->pair = next;
			break;
		}
		prev = pair;
		pair = next;
	}
}

static struct json_list_node *json_list_add_node(struct json_list *list)
{
	struct json_list_node *new_node = xmalloc(sizeof(struct json_list_node));

	new_node->next = NULL;

	if (list->node) {
		struct json_list_node *node;

		for (node = list->node; node && node->next; node = node->next);
		node->next = new_node;
	} else
		list->node = new_node;

	return new_node;
}

void json_list_add_str(struct json_list *list, const char *str)
{
	struct json_list_node *new_node = json_list_add_node(list);

	new_node->val.str = xstrdup(str);
}

void json_list_add_obj(struct json_list *list, struct json_obj *obj)
{
	struct json_list_node *new_node = json_list_add_node(list);

	new_node->val.obj = obj;
}

static struct json_pair *json_obj_add_pair(struct json_obj *obj, const char *key, enum json_val_type type)
{
	struct json_pair *new_pair = xmalloc(sizeof(struct json_pair));
	new_pair->key = xstrdup(key);
	new_pair->next = NULL;
	new_pair->type = type;

	if (obj->pair) {
		struct json_pair *pair;

		for (pair = obj->pair; pair && pair->next; pair = pair->next);
		pair->next = new_pair;
	} else
		obj->pair = new_pair;

	return new_pair;
}

void json_obj_add_str(struct json_obj *obj, const char *key, const char *str)
{
	struct json_pair *new_pair = json_obj_add_pair(obj, key, JSON_VAL_STRING);

	new_pair->val.str = xstrdup(str);
}

void json_obj_add_fmt_buf_str(struct json_obj *obj, const char *key, char *buf, size_t size, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, size, fmt, ap);
	va_end(ap);

	json_obj_add_str(obj, key, buf);
}

void json_obj_add_fmt_str(struct json_obj *obj, const char *key, size_t size, const char *fmt, ...)
{
	va_list ap;
	char *buf = malloc(size);

	va_start(ap, fmt);
	vsnprintf(buf, size, fmt, ap);
	va_end(ap);

	json_obj_add_str(obj, key, buf);
	free(buf);
}

void json_obj_add_list(struct json_obj *obj, const char *key, struct json_list *list)
{
	struct json_pair *new_pair = json_obj_add_pair(obj, key, JSON_VAL_LIST);

	new_pair->val.list = list;
}

void json_obj_add_obj(struct json_obj *obj, const char *key, struct json_obj *new_obj)
{
	struct json_pair *new_pair = json_obj_add_pair(obj, key, JSON_VAL_OBJECT);

	new_pair->val.obj = new_obj;
}

static void json_pair_print_json(struct json_pair *pair, int ind_lvl);
static void json_list_node_print_json(struct json_list_node *node, enum json_val_type type, int ind_lvl);

static void
print_ind(int ind_lvl)
{
	int i;

	putchar('\n');
	for (i = 0; i < ind_lvl; i++)
	fputs("  ", stdout);
}

void json_obj_print_json(struct json_obj *obj, int ind_lvl)
{
	struct json_pair *pair;

	printf("{");
	for (pair = obj->pair; pair; pair = pair->next) {
		json_pair_print_json(pair, ind_lvl+1);
		if (pair->next)
			printf(", ");
	}
	if (obj->pair) /* Do not indent if object was empty */
		print_ind(ind_lvl);
	printf("}");
}

void json_list_print_json(struct json_list *list, int ind_lvl)
{
	struct json_list_node *node;

	printf("[");
	for (node = list->node; node; node = node->next) {
		json_list_node_print_json(node, list->type, ind_lvl+1);
		if (node->next)
			printf(", ");
	}
	if (list->node) /* Do not indent if list was empty */
		print_ind(ind_lvl);
	printf("]");
}

static void
json_list_node_print_json(struct json_list_node *node, enum json_val_type type, int ind_lvl)
{
	print_ind(ind_lvl);
	switch (type) {
	case JSON_VAL_STRING:
		printf("\"%s\"", node->val.str);
		break;
	case JSON_VAL_LIST:
		json_list_print_json(node->val.list, ind_lvl);
		break;
	case JSON_VAL_OBJECT:
		json_obj_print_json(node->val.obj, ind_lvl);
		break;
	case JSON_VAL_FLAG:
		printf("%s", node->val.flag ? "true" : "false");
		break;
	default:
		break;
	}
}

static void json_pair_print_json(struct json_pair *pair, int ind_lvl)
{
	print_ind(ind_lvl);
	printf("\"%s\": ", pair->key);
	switch (pair->type) {
	case JSON_VAL_STRING:
		printf("\"%s\"", pair->val.str);
		break;
	case JSON_VAL_LIST:
		json_list_print_json(pair->val.list, ind_lvl);
		break;
	case JSON_VAL_OBJECT:
		json_obj_print_json(pair->val.obj, ind_lvl);
		break;
	default:
		break;
	}
}
