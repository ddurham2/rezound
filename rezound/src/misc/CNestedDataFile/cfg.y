%{

/*
 * Copyright (C) 2002 - David W. Durham
 *
 * This file is not part of any particular application.
 *
 * CNestedDataFile is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * CNestedDataFile is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

/*
 * As well as parsing CNestedDataFile data files, this is also used on the
 * side by string_to_anytype<vector> to parse a string and create a vector
 * of values from it.
 *
 * s2at means string_to_anytype used below.  Anything with this in front can be
 * ignored when maintaining this code for its primary use (parsing data files)
 *
 * Just adding this to be able to reuse this yacc file for string_to_anytype<vector>()
 * Since the generated yacc parser uses global variables, it's would cause a problem
 * if the string_to_anytype<vector>() function were called while it was parsing an actual
 * datafile... But, currently I see no reason for that to happen (unless I one day
 * implement array subscripting in the arithmetic expression stuff.)
 */
#include "../../../config/common.h"

#include <math.h>
#include <string.h>

#include <stack>
#include <stdexcept>

#include "CNestedDataFile.h"
#include "anytype.h"

struct RTokenPosition
{
	int first_line,last_line;
	int first_column,last_column;
	const char *text;
};
#define YYLTYPE RTokenPosition
#define YYLSP_NEEDED 1
#define YYERROR_VERBOSE

stack<string> scopeStack;

void cfg_error(int line,const char *msg);
void cfg_error(const YYLTYPE &pos,const char *msg);
void cfg_error(const char *msg);

int cfg_lex();

void cfg_init();
void cfg_deinit();

void cfg_includeFile(const char *filename);

void checkForDupMember(int line,const char *key);

static const string getCurrentScope();

int myyynerrs=0;

#define VERIFY_TYPE(ap,a)				\
	if(a->type!=CNestedDataFile::ktValue)		\
		cfg_error(ap,"invalid operand; value type expected");

#define BINARY_EXPR(r,ap,a,bp,b,o)	\
	VERIFY_TYPE(ap,a)		\
	VERIFY_TYPE(bp,b)		\
	r=a;				\
	double x1,x2;			\
	r->stringValue = strdup( anytype_to_string<double>(string_to_anytype<double>(r->stringValue,x1) o string_to_anytype<double>(b->stringValue,x2)).c_str() );	\
	delete b;


union cfg_parse_union
{
	char *				stringValue;
	CNestedDataFile::CVariant *	variant;
};
#define YYSTYPE union cfg_parse_union

%}

//%pure_parser -- would use, but flex isn't easily made reentrant.. having flex generate a C++ class is probably the way to do it, but it seems to much changing for what it's worth.. the mutex works for now

%expect 1

%token	<stringValue>	IDENT
%token	<stringValue>	LIT_NUMBER
%token	<stringValue>	LIT_STRING

%token			TRUE
%token			FALSE
%token			INCLUDE

%token			NE
%token			EQ
%token			GE
%token			LE
%token			OR
%token			AND


%type	<stringValue>	array_body;
%type	<stringValue>	array_body2;

/* numeric expression stuff */
%type	<variant>	primary_expr
%type	<variant>	unary_expr
%type	<variant>	multiplicative_expr
%type	<variant>	additive_expr
%type	<variant>	relational_expr
%type	<variant>	equality_expr
%type	<variant>	logical_and_expr
%type	<variant>	logical_or_expr
%type	<variant>	conditional_expr
%type	<variant>	expr

%type	<stringValue>	qualified_ident


/* stuff for s2at array parsing */
%type	<variant>	s2at_array_item
//%type			s2at_array_body
//%type			s2at_array_body2

%start start


%%


init
	: /* empty */ { cfg_init(); }
	;

start
	: init scope_body
	{
		cfg_deinit();

		return myyynerrs!=0;
	}

	| init '~' s2at_array_body
	{
		cfg_deinit();
		return myyynerrs!=0;
	}
	;


scope
	: IDENT '{'
	{ /* mid-rule action */

		checkForDupMember(@1.first_line,$1);

		scopeStack.push($1);

		free($1);

		/* now continue parsing for new scope's body */
	} scope_body '}' {
		scopeStack.pop();
	}
	;

scope_body
	: /* empty */
	| scope_body scope_body_item

	/* ERROR CASES */
	| error ';'
	| error
	;

/*
 * Arrays used to be set off by putting '[]' after the IDENT in
 * an assignment, but this is not the case anymore.  So, to let
 * it still be able to parse old config files I'm making it
 * optional for any assignment
 */
opt_old_array
	: /* empty */
	| '[' ']'
	;

scope_body_item
	: scope

	| IDENT opt_old_array '=' expr ';'
	{
		checkForDupMember(@1.first_line,$1);
		if($4->type==CNestedDataFile::ktValue)
			CNestedDataFile::parseTree->prvCreateKey(getCurrentScope()+$1,0,*$4,CNestedDataFile::parseTree->root);
		else
			yyerror(@3,"assigning an invalid typed value to an identifier");

		delete $4;
		free($1);
	}
	| INCLUDE LIT_STRING
	{
		cfg_includeFile($2);
		free($2);
	}
	| ';' /* allow stray ';'s */
	;

array_body
	: '{' '}'
	{
		$$=strdup("{}");
	}
	| '{' array_body2 '}'
	{
		$$=(char *)malloc((1+strlen($2)+1)+1);
		strcpy($$,"{");
		strcat($$,$2);
		strcat($$,"}");
		free($2);
	}
	;

array_body2
	: expr
	{
		$$=strdup($1->stringValue.c_str());
		delete $1;
	}

	| array_body2 ',' expr
	{
		$$=(char *)malloc((strlen($1)+1+$3->stringValue.size())+1);
		strcpy($$,$1);
		strcat($$,",");
		strcat($$,$3->stringValue.c_str());

		free($1);
		delete $3;
	}

	/* ERROR CASES */
	| error
	{
		$$=strdup("");
	}
	| array_body2 ',' error
	{
		$$=$1;
	}
	;




primary_expr
	/* literal values */
	: LIT_NUMBER
	{
		$$=new CNestedDataFile::CVariant($1);
		free($1);
	}
	| LIT_STRING
	{
		$$=new CNestedDataFile::CVariant(anytype_to_string<string>($1));
		free($1);
	}
	| TRUE
	{
		$$=new CNestedDataFile::CVariant("true");
	}
	| FALSE
	{
		$$=new CNestedDataFile::CVariant("false");
	}

	/* array body */
	| array_body
	{
		$$=new CNestedDataFile::CVariant($1);
		free($1);
	}

	/* symbol lookups */
	| qualified_ident
	{
		if(CNestedDataFile::parseTree==NULL)
		{
			$$=new CNestedDataFile::CVariant("");
		}
		else
		{
			CNestedDataFile::CVariant *value;
			if(!CNestedDataFile::parseTree->findVariantNode(value,$1,0,false,CNestedDataFile::parseTree->root))
			{
				cfg_error(@1,("symbol not found: '"+string($1)+"'").c_str());
				value=new CNestedDataFile::CVariant("");
			}

			switch(value->type)
			{
			case CNestedDataFile::ktValue:
				$$=new CNestedDataFile::CVariant(value->stringValue);
				break;
			case CNestedDataFile::ktScope:
				cfg_error(@1,("symbol resolves to a scope: '"+string($1)+"'").c_str());
				value=new CNestedDataFile::CVariant("");
				break;
			default:
				throw runtime_error("cfg_parse -- unhandled variant type");
			}
		}

		free($1);
	}

	/* parenthesis */
	| '(' expr ')'
	{
		$$=$2;
	}


	/* ERROR CASES */
	| '(' error ')'
	{
		yyclearin;
		$$=new CNestedDataFile::CVariant("");
	}
	;


unary_expr
	: primary_expr { $$=$1; }

	| '+' unary_expr
	{
		$$=$2;
		if($2->type!=CNestedDataFile::ktValue)
			cfg_error(@2,"invalid operand");
		else
		{
			double x;
			$$->stringValue=strdup(anytype_to_string<double>(+string_to_anytype<double>($$->stringValue,x)).c_str());
		}
	}
	| '-' unary_expr
	{
		$$=$2;
		if($2->type!=CNestedDataFile::ktValue)
			cfg_error(@2,"invalid operand");
		else
		{
			double x;
			$$->stringValue=strdup(anytype_to_string<double>(-string_to_anytype<double>($$->stringValue,x)).c_str());
		}
	}
	;


multiplicative_expr
	: unary_expr { $$=$1; }
	| multiplicative_expr '*' unary_expr { BINARY_EXPR($$,@1,$1,@3,$3,*) }
	| multiplicative_expr '/' unary_expr { BINARY_EXPR($$,@1,$1,@3,$3,/) }
	| multiplicative_expr '%' unary_expr { double x1,x2; VERIFY_TYPE(@1,$1) VERIFY_TYPE(@3,$3) $$=$1; $$->stringValue= strdup( anytype_to_string<double>(fmod(string_to_anytype<double>($$->stringValue,x1),string_to_anytype<double>($3->stringValue,x2))).c_str() ); delete $3; }
	;


additive_expr
	: multiplicative_expr { $$=$1; }
	| additive_expr '+' multiplicative_expr { BINARY_EXPR($$,@1,$1,@3,$3,+) }
	| additive_expr '-' multiplicative_expr { BINARY_EXPR($$,@1,$1,@3,$3,-) }
	;


relational_expr
	: additive_expr { $$=$1; }
	| relational_expr LE additive_expr { BINARY_EXPR($$,@1,$1,@3,$3,<=) }
	| relational_expr GE additive_expr { BINARY_EXPR($$,@1,$1,@3,$3,>=) }
	| relational_expr '<' additive_expr { BINARY_EXPR($$,@1,$1,@3,$3,<) }
	| relational_expr '>' additive_expr { BINARY_EXPR($$,@1,$1,@3,$3,>) }
	;

equality_expr
	: relational_expr { $$=$1; }
	| equality_expr EQ relational_expr { BINARY_EXPR($$,@1,$1,@3,$3,==) }
	| equality_expr NE relational_expr { BINARY_EXPR($$,@1,$1,@3,$3,!=) }
	;

logical_and_expr
	: equality_expr { $$=$1; }
	| logical_and_expr AND equality_expr { BINARY_EXPR($$,@1,$1,@3,$3,&&) }
	;

logical_or_expr
	: logical_and_expr { $$=$1; }
	| logical_or_expr OR logical_and_expr { BINARY_EXPR($$,@1,$1,@3,$3,||) }
	;


conditional_expr
	: logical_or_expr { $$=$1; }
	| logical_or_expr '?' expr ':' conditional_expr { double x; VERIFY_TYPE(@1,$1) $$=(string_to_anytype<double>($1->stringValue,x) ? ((delete $5),$3) : ((delete $3),$5)); delete $1; }
	;


expr
	: conditional_expr { $$=$1; }
	;



qualified_ident
	: IDENT
	{
		$$=$1;
	}
	| qualified_ident '|' IDENT
	{
		$$=(char *)realloc($$,strlen($$)+1+strlen($3)+1);
		strcat($$,CNestedDataFile::delim.c_str());
		strcat($$,$3);

		free($3);
	}
	;



/* s2at stuff below */

s2at_array_body
	: '{' '}'			{}
	| '{' s2at_array_body2 '}'	{}
	| error				{ YYABORT; }
	;

s2at_array_body2
	: s2at_array_item
	{
		CNestedDataFile::s2at_return_value.push_back($1->stringValue);
		delete $1;
	}
	| s2at_array_body2 ',' s2at_array_item
	{
		CNestedDataFile::s2at_return_value.push_back($3->stringValue);
		delete $3;
	}
	;

s2at_array_item
	// allow nested arrays, but nested ones should just be parsed as strings and not turned into vectors
	: expr	{ $$=$1; }
	| error	{ YYABORT; }
	;


%%

#include "cfg.lex.c"

void cfg_error(int line,const char *msg)
{
	myyynerrs++;

	if(CNestedDataFile::s2at_string.size()>0)
		// suppress error messages when in s2at mode
		return;

	fprintf(stderr,"%s:%d: %s\n",currentFilename.c_str(),line,msg);

	if(myyynerrs>25)
		throw runtime_error("cfg_error -- more than 25 errors; bailing");
}

void cfg_error(const RTokenPosition &pos,const char *msg)
{
	cfg_error(pos.first_line,msg);
}

void cfg_error(const char *msg)
{
	cfg_error(yylloc.first_line,msg);
}

void checkForDupMember(int line,const char *key)
{
	if(CNestedDataFile::parseTree->keyExists(getCurrentScope()+key))
		cfg_error(line,("duplicate scope name: "+(getCurrentScope()+string(key))).c_str());
}

static const string getCurrentScope()
{
	stack<string> copy=scopeStack;
	string s;
	while(!copy.empty())
	{
		s=copy.top()+s;
		s=CNestedDataFile::delim+s;
		copy.pop();
	}

	s+=CNestedDataFile::delim;

	s.erase(0,1);

	return s;
}




