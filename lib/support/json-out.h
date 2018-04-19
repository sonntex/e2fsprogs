/*
 * json-out.h -- JSON output
 *
 * Copyright (c) 2018 Virtuozzo International GmbH
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Public
 * License.
 * %End-Header%
 */

#ifndef H_JSON_OUT
#define H_JSON_OUT

enum json_val_type {
	JSON_VAL_STRING,
	JSON_VAL_OBJECT,
	JSON_VAL_LIST,
	JSON_VAL_FLAG
};

struct json_obj {
	struct json_pair *pair;
};

union json_val {
	char *str;
	struct json_obj *obj;
	struct json_list *list;
	char flag;
};

struct json_pair {
	char *key;
	enum json_val_type type;
	union json_val val;
	struct json_pair *next;
};

struct json_list {
	enum json_val_type type;
	struct json_list_node *node;
};

struct json_list_node {
	union json_val val;
	struct json_list_node *next;
};

struct json_obj *json_obj_create(void);
struct json_obj *json_obj_create_in_obj(struct json_obj *parent_obj, char *key);
void json_obj_add_str(struct json_obj *obj, const char *key, const char *str);
void json_obj_add_list(struct json_obj *obj, const char *key, struct json_list *list);
void json_obj_add_obj(struct json_obj *obj, const char *key, struct json_obj *new_obj);
void json_obj_add_flag(struct json_obj *obj, const char *key, char flag);
void json_obj_add_fmt_str(struct json_obj *obj, const char *key, size_t size, const char *fmt, ...);
void json_obj_add_fmt_buf_str(struct json_obj *obj, const char *key, char *buf, size_t size, const char *fmt, ...);
void json_obj_print_json(struct json_obj *obj, int ind_lvl);
void json_obj_delete_pair(struct json_obj *obj, char *key);
void json_obj_delete(struct json_obj *obj);

struct json_list *json_list_create(enum json_val_type type);
struct json_list *json_list_create_in_obj(struct json_obj *parent_obj, char *key, enum json_val_type type);
void json_list_add_str(struct json_list *list, const char *str);
void json_list_add_obj(struct json_list *list, struct json_obj *obj);
void json_list_print_json(struct json_list *list, int ind_lvl);
void json_list_delete(struct json_list *list);

#endif
