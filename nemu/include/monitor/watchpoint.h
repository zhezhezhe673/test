#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;
	int jiu;
	int xin;
	char str[32];
} WP;
void wp_pool1();
void wp_pool2(char *);
void init_wp_pool();
WP* new_wp(char* );
void free_wp(WP* );
int  check_wp();
#endif
