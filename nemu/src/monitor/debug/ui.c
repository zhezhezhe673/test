#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args){
char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		cpu_exec(1);
		}
	else {
    sscanf(arg,"%d",&i);
    if(i>=1)
    { 
		cpu_exec(i);
	}
    else {if(i==-1) cpu_exec(-1);
    else  printf("Parameter error\n");}
    }
	return 0;
}

static int cmd_info_r(char *args)  
{  
    char *arg=strtok(NULL," ");  
    if(strcmp(arg,"r")==0)  {  
         int i;
        for(i=0;i<8;i++) {  
            printf("%s %x %d\n",regsl[i],cpu.gpr[i]._32,cpu.gpr[i]._32);  
        }  
    }  
	else if(strcmp(arg,"w")==0) {
	wp_pool1();
	}
    return 0;  
}  

static int cmd_saomiao(char *args)
{
	char *arg=strtok(NULL," ");  
	char *argg=strtok(NULL," ");
    swaddr_t zanshi;
	int j;
	sscanf(arg,"%d",&j);
	sscanf(argg,"%x",&zanshi);
	printf("0x%x:",zanshi);
	int iii;
	for(iii=0;iii<j;iii++){
		printf("%08x",swaddr_read(zanshi,4)&0xffffffff);
		printf(" ");
		zanshi+=4;
	}
	printf("\n");
	return 0;
}

static int cmd_p(char *args)
{
	char *arg=strtok(NULL," "); 
	bool success;
	success=true;
	int j;
	j=expr(arg,&success);
	printf("%d\n",j);
	return 0;
};

static int cmd_d(char *args){
wp_pool2();
return 0;
}

static int cmd_w(char *args){
	char *arg=strtok(NULL," ");
	printf("i am sb1");
	WP* l=new_wp(arg);
	printf("Set a watchpoint at %s\n",l->str);
	return 0;
}
static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
    { "si","Single step execution N instructions then pause",cmd_si },
    { "info","Print the values of all registers",cmd_info_r},
    {"x", "sao miao nei cun",cmd_saomiao}, 
	{"w","设置间断点",cmd_w},
	{"p","计算表达式",cmd_p},
	{"d","删除监视点",cmd_d}  
	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
