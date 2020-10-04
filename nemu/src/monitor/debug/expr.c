#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum
{
	NOTYPE = 256,
	EQ = 257,
	NEQ = 258,
	yu = 259,
	huo = 260,
	fei = 261,
	TK_reg = 262,
	zhengshu = 264,
	TK_hex = 265
	/* TODO: Add more token types */

};

static struct rule
{
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +", NOTYPE}, // spaces
	{"\\(", '('},
	{"\\)", ')'},
	{"\\*", '*'},			   //cheng
	{"/", '/'},				   //chu
	{"-", '-'},				   //jian
	{"\\+", '+'},			   // plus
	{"[0-9]{1,10}", zhengshu}, //zhengshu
	{"==", EQ},				   // equal
	{"!=", NEQ},
	{"&&", yu},
	{"\\|\\|", huo},
	{"!", fei},
	{"0[xX][A-Fa-f0-9]{1,8}", TK_hex},						   //16进制数字
	{"\\$[a-dA-D]|\\$[eE]?(ax|dx|cx|bx|bp|si|di|sp)", TK_reg}, //寄存器
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
	int i;
	char error_msg[128];
	int ret;

	for (i = 0; i < NR_REGEX; i++)
	{
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if (ret != 0)
		{
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token
{
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e)
{
	int position = 0;
	int i;
	regmatch_t pmatch;

	nr_token = 0;

	while (e[position] != '\0')
	{
		/* Try all rules one by one. */
		for (i = 0; i < NR_REGEX; i++)
		{
			if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
			{
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */
				switch (rules[i].token_type)
				{
				case EQ:
					tokens[nr_token++].type = 257;
					strcpy(tokens[nr_token].str, "==");
					break;
				case NOTYPE:
					break;
				case '(':
					tokens[nr_token++].type = 40;
					break;
				case ')':
					tokens[nr_token++].type = 41;
					break;
				case '*':
					tokens[nr_token++].type = 42;
					break;
				case '/':
					tokens[nr_token++].type = 47;
				case '+':
					tokens[nr_token++].type = 43;
					break;
				case '-':
					tokens[nr_token++].type = 45;
					break;

				case 258:
					tokens[nr_token++].type = 258;
					strcpy(tokens[nr_token].str, "!=");
					break;
				case 259:
					tokens[nr_token++].type = 259;
					strcpy(tokens[nr_token].str, "&&");
					break;
				case 260:
					tokens[nr_token++].type = 260;
					strcpy(tokens[nr_token].str, "||");
					break;
				case 261:
					tokens[nr_token++].type = 261;
					break;

				case 262:
					tokens[nr_token++].type = 262;
					strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
					break;
				case 263:
					tokens[nr_token++].type = 263;
					strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
					break;
				case 264:
					tokens[nr_token++].type = 264;
					strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
					break;
				case 265:
					tokens[nr_token++].type = 265;
					strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
					break;
				default:
					panic("please implement me");
				}

				break;
			}
		}

		if (i == NR_REGEX)
		{
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		} //。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。这里暂时不懂
	}

	return true;
}

static int find_dominant_operator(char *e)
{
	int e_chang = strlen(e);
	int jilushu[e_chang]; //记录e中每个符号的优先级顺序，初始化为0，代表是数字,左括号记为10，右括号记为11；
	char fuhao[e_chang + 1];	//记录e的每个符号，他们之间是一一对应的关系；
	int dengji[e_chang];	//记录符号的优先级顺序；
	int i;
	for(i=0;i<e_chang;i++){
		jilushu[i]=0;
		dengji[i]=0;
	}
	for (i = 0; i < e_chang; i++)
	{
		if (e[i] == '(')
		{
			jilushu[i] = 10;
			fuhao[i] = '(';
			dengji[i] = 99;
		}
		if (e[i] == ')')
		{
			jilushu[i] = 11;
			fuhao[i] = ')';
			dengji[i] = 100;
		}
		if (e[i] == '!' && e[i + 1] != '=')
		{
			jilushu[i] = 261;
			fuhao[i] = '!';
			dengji[i] = 261;
		}
		if (e[i] == '*')
		{
			jilushu[i] = 8;
			fuhao[i] = '*';
			dengji[i] = 8;
		}
		if (e[i] == '/')
		{
			jilushu[i] = 7;
			fuhao[i] = '/';
			dengji[i] = 8;
		}
		if (e[i] == '+')
		{
			jilushu[i] = 6;
			fuhao[i] = '+';
			dengji[i] = 7;
		}
		if (e[i] == '-')
		{
			jilushu[i] = 5;
			fuhao[i] = '-';
			dengji[i] = 7;
		}
		if (e[i] == '=' && e[i + 1] == '=')
		{
			jilushu[i] = 4;
			fuhao[i] = '=';
			dengji[i] = 257;
			++i;
			dengji[i] = 257; //对双运算符两个等级位设置成相同的；
		}
		if (e[i] == '!' && e[i + 1] == '=')
		{
			jilushu[i] = 3;
			fuhao[i] = '!';
			fuhao[i + 1] = '=';
			dengji[i] = 258;
			++i;
			dengji[i] = 258;
		}
		if (e[i] == '&')
		{
			jilushu[i] = 2;
			fuhao[i] = '&';
			dengji[i] = 259;
			++i;
			dengji[i] = 259;
		}
		if (e[i] == '|')
		{
			jilushu[i] = 1;
			fuhao[i] = '|';
			dengji[i] = 260;
			++i;
			dengji[i] = 260;
		}
	}
	int j;
	int min_po = 0;	 //运算符等级最小的位置；
	int po_zuo = -1; //第一个左括号的位置；
	int po_you = -1; //最后一个右括号的位置；
	for (j = 0; j < e_chang; j++)
	{
		if (dengji[j] == 99)
		{
			po_zuo = j;
			break;
		}
	}
	for (j = e_chang - 1; j > 0; j--)
	{
		if (dengji[j] == 100)
		{
			po_you = j;
			break;
		}
	}
	for (j = po_zuo; j <= po_you; j++)
	{
		dengji[j] = 98;
	}
	int er_or_yi = 1; //最终是二元运算符（包括!)（为1）还是一元运算符（为0）；
	for (j = 0; j < e_chang; j++)
	{
		if (dengji[min_po] >= dengji[j] && dengji[j] < 100 && dengji[j] != 0)
		{
			er_or_yi = 0;
			min_po = j;
		}
	}

	if (er_or_yi == 1)
	{

		for (j = 0; j < e_chang; j++)
		{
			if (dengji[min_po] <= dengji[j] && dengji[j] > 100)
			{
				min_po = j;
			}
		}
	}
	return min_po;
}
static bool check_parentheses(int p, int q, char *e)
{
	   if((e[p]!='(')||(e[q]!=')'))return false;
	   else {
		int e_chang = strlen(e);
		int zuokuo_shumu[e_chang]; //记录左括号的数目；
		int youkuo_shumu[e_chang]; //记录右括号的数目；
		int i;
		for(i=0;i<e_chang;i++){
			zuokuo_shumu[i]=0;
			youkuo_shumu[i]=0;
		}
		for (i = 0; i < e_chang; i++)
		{
			if (e[i] == '(')
				zuokuo_shumu[i]++;
			if (e[i] == ')')
				youkuo_shumu[i]++;
		}
		for (i = 0; i < e_chang; i++)
		{
			if (zuokuo_shumu[i] < youkuo_shumu[i])
				assert(0);
		}
		for (i = 0; i < e_chang - 1; i++)
		{
			if (zuokuo_shumu[i] == youkuo_shumu[i])
				return false;
		}
	   }
	   return true;
}
static int eval(int p, int q, char *e)
{   int i=0;
	if (p > q)
	{
		return 0;
	}
	else if (p == q)
	{
		if (tokens[p].type == 264) //10进制
		{
			sscanf(tokens[p].str, "%d", &i);
			return i;
		}
		else if (tokens[p].type == 265) //r 16进制
		{
			sscanf(tokens[p].str, "%x", &i);
			return i;
		}
		else if (tokens[p].type == 262)
		{
			int j = 0, sl = 1, sw = 1;
			for (; j < 8 && sl != 0 && sw != 0; j++)
			{
				sl = strcmp(tokens[p].str + 1, regsl[i]);
				sw = strcmp(tokens[p].str + 1, regsw[i]);
			}
			if (sl == 0)
			{
				i = cpu.gpr[j]._32;
				return i;
			}
			else if (sw == 0)
				return cpu.gpr[j]._16;
			else
			{
				if (strcmp(tokens[p].str, "$al") == 0)
					return reg_b(0);
				if (strcmp(tokens[p].str + 1, "cl") == 0)
					return reg_b(1);
				if (strcmp(tokens[p].str + 1, "dl") == 0)
					return reg_b(2);
				if (strcmp(tokens[p].str + 1, "bl") == 0)
					return reg_b(3);
				if (strcmp(tokens[p].str + 1, "ah") == 0)
					return reg_b(4);
				if (strcmp(tokens[p].str + 1, "ch") == 0)
					return reg_b(5);
				if (strcmp(tokens[p].str + 1, "dh") == 0)
					return reg_b(6);
				if (strcmp(tokens[p].str + 1, "bh") == 0)
					return reg_b(7);
			}
			if (j == 8)
				assert(0);
		}
		else
			assert(0); //...............
	}
	else if (check_parentheses(p,q,e) == true)
	{
		return eval(p + 1, q - 1,e);
	}
	else
	{   
		if((q-p==1)&&tokens[p].type=='-')
		   return 0-eval(q,q,e);
		if(((q-p==1)||(tokens[p+1].type=='('&&tokens[q].type==')'))&&tokens[p].type==261){
			i=eval(p+1,q,e);
            return !i;
		}
		if(((q-p==1)||(tokens[p+1].type=='('&&tokens[q].type==')'))&&tokens[p].type=='*'){
			return swaddr_read(eval(p+1,q,e),4);
		}
		char dest[q - p + 2];
		dest[q-p+1]='\0';
		strncpy(dest, e + p, q - p); //复制到数组中了，所以没有\0也没有关系；
		int val2=0;
		int val1=0;
		int po_do_op = find_dominant_operator(dest); //dominat_operator的位置；
		if (dest[po_do_op] == '=' || dest[po_do_op] == '&' || dest[po_do_op] == '|')
		{
			val1 = eval(p, po_do_op - 2, e);
			val2 = eval(po_do_op + 1, q, e);
		}
		switch (dest[po_do_op])
		{
		case '+':
			return val1 + val2;
		case '-':
			return val1 - val2;
		case '*':
			return val1 * val2;
		case '/':
			return val1 / val2;
		case '=':
			if (dest[po_do_op - 1] == '=')
			{
				if (val1 == val2)
					return 1;
				else
					return 0;
			}
			else
			{
				if (val1 != val2)
					return 1;
				else
					return 0;
			}
		case '&':
			if (val1 && val2)
				return 1;
			else
				return 0;
		case '|':
			if (val1 || val2)
				return 1;
		case '!':
			if (val2 != 0)
				return 0;
			else
				return 1;
		default:
			assert(0);
		}
	}
	return 0;
}
uint32_t expr(char *e, bool *success)
{
	if (!make_token(e))
	{
		*success = false;
		return 0;
	}
	else
	{
		int e_chan = strlen(e); //e的长度；
		int p = 0;
		int q = e_chan - 1;
		eval(p, q, e);
	}
	/* TODO: Insert codes to evaluate the expression. */
	panic("please implement me");
	return 0;
}
