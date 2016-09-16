#ifndef _OBJ_LIST_H
#define _OBJ_LIST_H

enum ACTION {
	ACTION_NONE=-1,
	ACTION_SET,
	ACTION_GET,
	ACTION_MAX
};

struct _obj;
typedef int (*obj_op)(struct _obj * ,int argc,const char * argv[]);

typedef struct _obj{
	char * name;
	obj_op action[ACTION_MAX];
}obj_t,*pobj_t;



extern obj_t obj_list[];
#endif