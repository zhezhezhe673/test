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
	TK_hex = 265,
	fushu,
	zhizhen
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
	{"0[xX][A-Fa-f0-9]{1,8}", TK_hex},//后面是否需要'+'						   //16进制数字
	{"\\$[a-dA-D]|\\$[eE]?(ax|dx|cx|bx|bp|si|di|sp)", TK_reg}, //寄存器
	{"-",fushu},//负数
	{"*",zhizhen}//指针
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

int priority(int type)
{
	switch (type)
	{
	case NOTYPE:
		break;
	case '(':
		return 1;
		break;
	case ')':
		return 1;
		break;
	case '-':
		return 4;
		break;
	case '+':
		return 4;
		break;
	case '*':
		return 3;
		break;
	case '/':
		return 3;
		break;
	case EQ:
		return 7;
		break;
	case zhengshu:
		return -1;
		break;
	case NEQ:
		return 7;
		break;
	case yu:
		return 11;
		break;
	case huo:
		return 12;
		break;
	case fei:
		return 2;
		break;
	case TK_hex:
		return -1;
		break;
	case TK_reg:
		return -1;
	case fushu:
	    return 0;
	case zhizhen:
	    return 0;
	default:
		panic("please implement me");
	}
	return -2;
}

int find_dominant_operator(int p, int q)
{
	int op = -1;
	int op_p = 0;
	int i;
	int i_p = 0;
	int cnt = 0;
	for (i = p; i <= q; i++)
	{
		if (tokens[i].type == '(')
			cnt++;
		if (tokens[i].type == ')')
			cnt--;
		int ss = 0; //find op_priority
		for (; ss < NR_REGEX; ss++)
		{
			if (tokens[i].type == rules[ss].token_type)
			{
				i_p = priority(tokens[i].type);
				break;
			}
			if(tokens[i].type==zhizhen){
				i_p=priority(zhizhen);
			}
			if(tokens[i].type==fushu){
				i_p=priority(fushu);
			}
		}
		if (cnt == 0 && op_p <= i_p)
		{
			op = i;
			op_p = i_p;
		}
	}
	return op;
}
bool check_parentheses(int p, int q)
{
	int left = 0;
	int i;
	for (i = p; i <= q; i++)
	{
		if (tokens[i].type == '(')
			left++;
		if (tokens[i].type == ')')
			left--;
		if (left < 0)
			assert(0);
		if (left == 0 && i != q)
			return 0;
	}
	if (left)
		assert(0);
	return 1;
}

static int eval(int p, int q)
{
	int i = 0;
	if (p > q)
	{
		assert(0);
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
			int sl = 1, sw = 1, sb = 1;
			for (; i < 8 && sl != 0 && sw != 0 && sb; i++)
			{
				sl = strcmp(tokens[p].str + 1, regsl[i]);
				sw = strcmp(tokens[p].str + 1, regsw[i]);
				sb = strcmp(tokens[p].str + 1, regsb[i]);
			}
			if (!sl)
				i = reg_l(i);
			if (!sw)
				i = reg_w(i);
			if (!sb)
				i = reg_b(i);
		}
		return i; //...............
	}
	else if (check_parentheses(p, q) == true)
	{
		return eval(p + 1, q - 1);
	}
	else
	{
		if ((q - p == 1) && tokens[p].type == fushu)
			return 0 - eval(q, q);
		if (((q - p == 1) || (tokens[p + 1].type == '(' && tokens[q].type == ')')) && tokens[p].type == 261)
		{
			i = eval(p + 1, q);
			return !i;
		}
		if (tokens[p].type == zhizhen&&((q - p == 1)|| (tokens[p + 1].type == '(' && tokens[q].type == ')'))) 
		{
			return swaddr_read(eval(p + 1, q), 4);
		}//如果是指针的话；
		int po_do_op = find_dominant_operator(p, q); //dominat_operator的位置；
		int val1 = eval(p, po_do_op - 1);
		int val2 = eval(po_do_op + 1, q);
		switch (tokens[po_do_op].type)
		{
		case '+':
			return val1 + val2;
		case '-':
			return val1 - val2;
		case '*':
			return val1 * val2;
		case '/':
			return val1 / val2;
		case huo:
			return val1 || val2;
		case yu:
			return val1 && val2;
		case EQ:
			if (val1 == val2)
				return 1;
			else
				return 0;
		case NEQ:
			return val1 != val2;
		case fei:
			return !val2;
		case fushu:
		    return 0-val2;
		case zhizhen:
		    return swaddr_read(eval(p+1,q),4); 
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
	int i;
	for (i = 0; i < nr_token; i++)
	{
		if (tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type != TK_hex && tokens[i - 1].type != TK_reg && tokens[i - 1].type != zhengshu && tokens[i - 1].type != ')')))
			tokens[i].type = zhizhen;
		if (tokens[i].type == '-' && (i == 0 || (tokens[i - 1].type != TK_hex && tokens[i - 1].type != TK_reg&& tokens[i - 1].type != zhengshu && tokens[i - 1].type != ')')))
			tokens[i].type = fushu;
	}
	return eval(0,nr_token);
	/* TODO: Insert codes to evaluate the expression. */
	panic("please implement me");
	return 0;
}
