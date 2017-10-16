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
#ifndef __anytype_H__
#define __anytype_H__

#include "../../../config/common.h"

#include <mutex>
#include <sstream>
#include <string>
#include <vector> // for the vector implemenation to/from string
#include <cmath>
using namespace std;

// utilities just for s2at purposes
namespace s2at // s2at signifies string_to_anytype/anytype_to_string
{
	static const string remove_surrounding_quotes(const string &str);
	static const string escape_chars(string str);
	static const string unescape_chars(string str);
}


/*
 * These template functions are used to define many data-types to a
 * string representation and back again (using locale-less conversions!)
 * The string representation it converts to should agree with what
 * cfg_parse() is able to parse (i.e. strings have quotes around them
 * and vectors' values are '{...}')
 */


/*
 * To disambiguate some things since the return value is the only thing would
 * distinguish the template specialization to use when the compiler is inferring
 * the template specialation to use, I made the parameter list require the object
 * to put the return value into (and a reference to that gets returned)
 */

// this macro is used to remove set the locale setting on a iostream back to default (so it's consistant)
#define NO_LOCALE(stream) stream.imbue(std::locale::classic());

// or we could leave these unimplemented to get a linker error instead (that's what I have to do with gcc>=3.4
// 	/* the reason I haven't made string_to_anytype return a reference is because currently, it's probably not a big deal for most types.. and with string, we'd be making a copy into the ret parameter if I implemented it that way anyway.. perhaps I could remove the return type all together and always use the ret parameter to get back the data */
template<typename Type> STATIC_TPL const Type string_to_anytype(const string &str,Type &ret)       ;// { no_specialization_of_this_template_with_the_given_type; }
template<typename Type> STATIC_TPL const string anytype_to_string(const Type &any)                 ;// { no_specialization_of_this_template_with_the_given_type; }

// vector versions (prototyped BEFORE we include CNestedDataFile.h which calls them)
template<class Type> STATIC_TPL const vector<Type> &string_to_anytype(const string &str,vector<Type> &ret);
template<class Type> STATIC_TPL const string anytype_to_string(const vector<Type> &any);




// template specializations of string_to_anytype

template<> STATIC_TPL const string string_to_anytype<string>(const string &str,string &ret)                         { return ret=s2at::unescape_chars(s2at::remove_surrounding_quotes(str)); }

template<> STATIC_TPL const bool string_to_anytype<bool>(const string &str,bool &ret)                             { return s2at::remove_surrounding_quotes(str)=="true" ? ret=true : ret=false; }

template<> STATIC_TPL const char string_to_anytype<char>(const string &str,char &ret)                             { istringstream ss(s2at::remove_surrounding_quotes(str)); NO_LOCALE(ss) ret=0; ss >> ret; return ret; }
template<> STATIC_TPL const unsigned char string_to_anytype<unsigned char>(const string &str,unsigned char &ret)           { istringstream ss(s2at::remove_surrounding_quotes(str)); NO_LOCALE(ss) ret=0; ss >> ret; return ret; }

template<> STATIC_TPL const short string_to_anytype<short>(const string &str,short &ret)                           { istringstream ss(s2at::remove_surrounding_quotes(str)); NO_LOCALE(ss) ret=0; ss >> ret; return ret; }
template<> STATIC_TPL const unsigned short string_to_anytype<unsigned short>(const string &str,unsigned short &ret)         { istringstream ss(s2at::remove_surrounding_quotes(str)); NO_LOCALE(ss) ret=0; ss >> ret; return ret; }

template<> STATIC_TPL const int string_to_anytype<int>(const string &str,int &ret)                               { istringstream ss(s2at::remove_surrounding_quotes(str)); NO_LOCALE(ss) ret=0; ss >> ret; return ret; }
template<> STATIC_TPL const unsigned int string_to_anytype<unsigned int>(const string &str,unsigned int &ret)             { istringstream ss(s2at::remove_surrounding_quotes(str)); NO_LOCALE(ss) ret=0; ss >> ret; return ret; }

template<> STATIC_TPL const long string_to_anytype<long>(const string &str,long &ret)                             { istringstream ss(s2at::remove_surrounding_quotes(str)); NO_LOCALE(ss) ret=0; ss >> ret; return ret; }
template<> STATIC_TPL const unsigned long string_to_anytype<unsigned long>(const string &str,unsigned long &ret)           { istringstream ss(s2at::remove_surrounding_quotes(str)); NO_LOCALE(ss) ret=0; ss >> ret; return ret; }

template<> STATIC_TPL const long long string_to_anytype<long long>(const string &str,long long &ret)                   { istringstream ss(s2at::remove_surrounding_quotes(str)); NO_LOCALE(ss) ret=0; ss >> ret; return ret; }
template<> STATIC_TPL const unsigned long long string_to_anytype<unsigned long long>(const string &str,unsigned long long &ret) { istringstream ss(s2at::remove_surrounding_quotes(str)); NO_LOCALE(ss) ret=0; ss >> ret; return ret; }

template<> STATIC_TPL const float string_to_anytype<float>(const string &str,float &ret)                           { istringstream ss(s2at::remove_surrounding_quotes(str)); NO_LOCALE(ss) ret=0.0f; if(str=="inf") ret=INFINITY; else if(str=="-inf") ret=-INFINITY; else ss >> ret; return ret; }
template<> STATIC_TPL const double string_to_anytype<double>(const string &str,double &ret)                         { istringstream ss(s2at::remove_surrounding_quotes(str)); NO_LOCALE(ss) ret=0.0; if(str=="inf") ret=INFINITY; else if(str=="-inf") ret=-INFINITY; else ss >> ret; return ret; }
template<> STATIC_TPL const long double string_to_anytype<long double>(const string &str,long double &ret)               { istringstream ss(s2at::remove_surrounding_quotes(str)); NO_LOCALE(ss) ret=0.0; if(str=="inf") ret=INFINITY; else if(str=="-inf") ret=-INFINITY; else ss >> ret; return ret; }

// I really wished that I didn't have to explicitly use 'vector' in the definition; I'd have like to use any container with an iterator interface
#include <CNestedDataFile/CNestedDataFile.h>
extern int cfg_parse();
template<class Type> STATIC_TPL const vector<Type> &string_to_anytype(const string &str,vector<Type> &ret)
{
	// This function has to parse '{' ..., ... '}' where the ... can contain nested array
	// bodies, quoted strings, numbers, etc; but we only want the outermost list as vector,
	// the rest as a string

	std::unique_lock<std::mutex> l(CNestedDataFile::cfg_parse_mutex);
	ret.clear();
	
	CNestedDataFile::s2at_string="~"+str;           // the parser knows '~' sets off an s2at_array_body
	CNestedDataFile::s2at_return_value.clear();
	if(!cfg_parse())
	{       // successful
		Type temp;
		for(size_t t=0;t<CNestedDataFile::s2at_return_value.size();t++)
			ret.push_back(string_to_anytype(CNestedDataFile::s2at_return_value[t],temp));
	}
	return ret;
}




// template specializations of anytype_to_string

template<> STATIC_TPL const string anytype_to_string<string>(const string &any)             { return "\""+s2at::escape_chars(any)+"\""; }

template<> STATIC_TPL const string anytype_to_string<bool>(const bool &any)               { return any ? "true" : "false"; }

template<> STATIC_TPL const string anytype_to_string<char>(const char &any)               { ostringstream ss; NO_LOCALE(ss) ss << any; return ss.str(); }
template<> STATIC_TPL const string anytype_to_string<unsigned char>(const unsigned char &any)      { ostringstream ss; NO_LOCALE(ss) ss << any; return ss.str(); }

template<> STATIC_TPL const string anytype_to_string<short>(const short &any)              { ostringstream ss; NO_LOCALE(ss) ss << any; return ss.str(); }
template<> STATIC_TPL const string anytype_to_string<unsigned short>(const unsigned short &any)     { ostringstream ss; NO_LOCALE(ss) ss << any; return ss.str(); }

template<> STATIC_TPL const string anytype_to_string<int>(const int &any)                { ostringstream ss; NO_LOCALE(ss) ss << any; return ss.str(); }
template<> STATIC_TPL const string anytype_to_string<unsigned int>(const unsigned int &any)       { ostringstream ss; NO_LOCALE(ss) ss << any; return ss.str(); }

template<> STATIC_TPL const string anytype_to_string<long>(const long &any)               { ostringstream ss; NO_LOCALE(ss) ss << any; return ss.str(); }
template<> STATIC_TPL const string anytype_to_string<unsigned long>(const unsigned long &any)      { ostringstream ss; NO_LOCALE(ss) ss << any; return ss.str(); }

template<> STATIC_TPL const string anytype_to_string<long long>(const long long &any)          { ostringstream ss; NO_LOCALE(ss) ss << any; return ss.str(); }
template<> STATIC_TPL const string anytype_to_string<unsigned long long>(const unsigned long long &any) { ostringstream ss; NO_LOCALE(ss) ss << any; return ss.str(); }

// I've picked a rather arbitrary way of formatting floats one way or another depending on how big it is.. I wish there were a way to output the ascii in such a way as to preserve all the information in the float (without printing the hex of it or something like that)
#include <istring>
#include <math.h> // for isnan which I hope is there (maybe fix in common.h if it's not
template<> STATIC_TPL const string anytype_to_string<float>(const float &any)              { if(std::isnan(any)) return "0"; else if(std::isinf(any) && any > 0) return "inf"; else if(std::isinf(any) && any < 0) return "-inf"; else { ostringstream ss; NO_LOCALE(ss) if(any>999999.0) {ss.setf(ios::scientific); ss.width(0); ss.precision(12); ss.fill(' '); } else {ss.setf(ios::fixed); ss.precision(6); ss.fill(' '); } ss << any; return istring(ss.str()).trim(); } }
template<> STATIC_TPL const string anytype_to_string<double>(const double &any)             { if(std::isnan(any)) return "0"; else if(std::isinf(any) && any > 0) return "inf"; else if(std::isinf(any) && any < 0) return "-inf"; else { ostringstream ss; NO_LOCALE(ss) if(any>999999.0) {ss.setf(ios::scientific); ss.width(0); ss.precision(12); ss.fill(' '); } else {ss.setf(ios::fixed); ss.precision(6); ss.fill(' '); } ss << any; return istring(ss.str()).trim(); } }
template<> STATIC_TPL const string anytype_to_string<long double>(const long double &any)        { if(std::isnan(any)) return "0"; else if(std::isinf(any) && any > 0) return "inf"; else if(std::isinf(any) && any < 0) return "-inf"; else { ostringstream ss; NO_LOCALE(ss) if(any>999999.0) {ss.setf(ios::scientific); ss.width(0); ss.precision(12); ss.fill(' '); } else {ss.setf(ios::fixed); ss.precision(6); ss.fill(' '); } ss << any; return istring(ss.str()).trim(); } }


// I really wished that I didn't have to explicitly use 'vector' in the definition, I'd have like to use any container with an iterator interface
template<class Type> STATIC_TPL const string anytype_to_string(const vector<Type> &any)
{
	string s;
	size_t l=any.size();
	s="{";
	for(size_t t=0;t<l;t++)
	{
		// leaving type in case it's not able to deduce aruments and chooses the default template implemenation
		// if I knew how to constrain the original definition of the template, I would make it fully constrained
		s+=anytype_to_string<Type>(any[t]);
		if(t!=(l-1))
				s+=",";
	}
	return s+"}";
}


// ----------------------------------------------------------------------------

namespace s2at // s2at signifies string_to_anytype/anytype_to_string
{
	using namespace std;

	/*
	 * Removes quotes around string if they exist
	 *
	 * I call this in all the numeric->string specializations in case the user calls
	 * getValue<numeric_type>() for a key that was created or parsed as a string (which
	 * would cause quotes to be put around it internally)
	 *
	 * And I call it in the string specialization just to do the work it would always do anyway
	 */
	STATIC_TPL const string remove_surrounding_quotes(const string &str)
	{
		return (str.size()>=2 && str[0]=='"' && str[str.size()-1]=='"') ? str.substr(1,str.size()-2) : str;
	}

	/* put '\' infront of '"' and '\' and convert linefeeds into '\n' */
	STATIC_TPL const string escape_chars(string str)
	{
		size_t len=str.length();
		for(size_t t=0;t<len;t++)
		{
			if(str[t]=='\\' || str[t]=='"')
			{
				str.insert(t,"\\");
				len++;
				t++;
			}
			else if(str[t]=='\n')
			{
				str[t]='n';
				str.insert(t,"\\");
				len++;
				t++;
			}
		}
		return str;
	}

	/* This function does the inverse of escape_chars */
	STATIC_TPL const string unescape_chars(string str)
	{
		size_t len=str.length();
		for(size_t t=0;t<len;t++)
		{
			if(str[t]=='\\' && t<len-1)
			{
				if(str[t+1]=='\\' || str[t+1]=='"')
				{
					str.erase(t,1);
					len--;
					t--;
				}
				else if(str[t+1]=='n')
				{
					str.erase(t,1);
					str[t]='\n';
					len--;
					t--;
				}
			}
		}
		return str;
	}



} // namespace s2at

#endif

