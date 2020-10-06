#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32
static WP wp_pool[NR_WP];
static WP *head, *free_;
void wp_pool1(){
printf("Num\t\tOldValue\t\tNewValue\tWhat\n");
	int i;
	for( i=0;i<32;i++){
    printf("%4d\t\t0x%x\t\t\t0x%x\t\t%s\n",wp_pool[i].NO,wp_pool[i].jiu,wp_pool[i].xin,wp_pool[i].str);		
}}
void wp_pool2(char *arg)
{
	int i;
	for( i=0;i<32;i++){
     if (strcmp(wp_pool[i].str,arg)==0)
	 {
		 free_wp(wp_pool+i);
	 }	 	
	}
}
void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}
/* TODO: Implement the functionality of watchpoint */
WP* new_wp(char* strr){
	WP* q=NULL;
	WP* r=NULL;
	r=free_;
	if(r==NULL){assert(0);}
	else{free_=free_->next;
	strcpy(r->str,strr);    //1.
    q=head;
	if(q==NULL){
		head=r;
		head->next=NULL;
		bool success;
		r->jiu=expr(strr,&success);
		return r;
	}
	else{while(q->next!=NULL){
		q=q->next;
	}
		q->next=r;
		r->next=NULL;
		bool success;
		r->jiu=expr(strr,&success);//r是地址所以用箭头，看前面的
		return r;
	}}
};

void free_wp(WP* wp){
	WP* p;
	WP* q;
	WP *r;
	q=head;
	if(strcmp(q->str,wp->str)==0){
        p=free_;//未来改bug:free_是空的情况下我没有考虑；
		while(p->next!=NULL)
		    p=p->next;
        p->next=head;
		head=head->next;
		q->next=NULL;
	}
	else{
		while(strcmp(q->next->str,wp->str)!=0){
			q=q->next;
		}
		r=q->next;
        q->next=r->next;
		r->next=NULL;
        p=free_;
		while(p->next!=NULL)
		    p=p->next;
        p->next=r;
	}
}

int  check_wp(){
WP *p;
bool success;
p=head;
if(p==NULL)return 0;
while(p){
	p->xin=expr(p->str,&success);
	if(p->xin!=p->jiu)return -1;
	p=p->next;
}	
return 0;
}