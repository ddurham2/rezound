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

#include <sstream>
#include <string>
#include <vector> // for the vector implemenation to/from string
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
 * string representation and back again (using locale-less conversions)
 * The string representation is converts to should agree with what
 * cfg_parse() is able to parse (i.e. strings have quotes around them
 * and vectors' values are '{...}')
 *
 * I would expect using ostringstream and istringstream as is would look at the
 * current locale and alter their behaviors based on that, but I guess that's
 * not the case since setlocale() is a C thing and stream::imbue() is the C++ way
 *
 * AFAICT the locale for a C++ stream object has to be explicitly set for on each
 * stream.
 */


/*
 * To disambiguate some things since the return value is the only thing would
 * distinguish the template specialization to use when the compiler is inferring
 * the template specialation to use, I made the parameter list require the object
 * to put the return value into (and a reference to that gets returned)
 */

// or we could leave these unimplemented to get a linker error instead
template<typename Type> static const Type string_to_anytype(const string &str,Type &ret)        { no_specialization_of_this_template_with_the_given_type; }
template<typename Type> static const string anytype_to_string(const Type &any)                  { no_specialization_of_this_template_with_the_given_type; }




// template specializations of string_to_anytype

template<> static const string string_to_anytype(const string &str,string &ret)                         { return s2at::unescape_chars(s2at::remove_surrounding_quotes(str)); }

template<> static const bool string_to_anytype(const string &str,bool &ret)                             { return s2at::remove_surrounding_quotes(str)=="true" ? ret=true : ret=false; }

template<> static const char string_to_anytype(const string &str,char &ret)                             { istringstream ss(s2at::remove_surrounding_quotes(str)); ret=0; ss >> ret; return ret; }
template<> static const unsigned char string_to_anytype(const string &str,unsigned char &ret)           { istringstream ss(s2at::remove_surrounding_quotes(str)); ret=0; ss >> ret; return ret; }

template<> static const short string_to_anytype(const string &str,short &ret)                           { istringstream ss(s2at::remove_surrounding_quotes(str)); ret=0; ss >> ret; return ret; }
template<> static const unsigned short string_to_anytype(const string &str,unsigned short &ret)         { istringstream ss(s2at::remove_surrounding_quotes(str)); ret=0; ss >> ret; return ret; }

template<> static const int string_to_anytype(const string &str,int &ret)                               { istringstream ss(s2at::remove_surrounding_quotes(str)); ret=0; ss >> ret; return ret; }
template<> static const unsigned int string_to_anytype(const string &str,unsigned int &ret)             { istringstream ss(s2at::remove_surrounding_quotes(str)); ret=0; ss >> ret; return ret; }

template<> static const long string_to_anytype(const string &str,long &ret)                             { istringstream ss(s2at::remove_surrounding_quotes(str)); ret=0; ss >> ret; return ret; }
template<> static const unsigned long string_to_anytype(const string &str,unsigned long &ret)           { istringstream ss(s2at::remove_surrounding_quotes(str)); ret=0; ss >> ret; return ret; }

template<> static const long long string_to_anytype(const string &str,long long &ret)                   { istringstream ss(s2at::remove_surrounding_quotes(str)); ret=0; ss >> ret; return ret; }
template<> static const unsigned long long string_to_anytype(const string &str,unsigned long long &ret) { istringstream ss(s2at::remove_surrounding_quotes(str)); ret=0; ss >> ret; return ret; }

template<> static const float string_to_anytype(const string &str,float &ret)                           { istringstream ss(s2at::remove_surrounding_quotes(str)); ret=0; ss >> ret; return ret; }
template<> static const double string_to_anytype(const string &str,double &ret)                         { istringstream ss(s2at::remove_surrounding_quotes(str)); ret=0; ss >> ret; return ret; }
template<> static const long double string_to_anytype(const string &str,long double &ret)               { istringstream ss(s2at::remove_surrounding_quotes(str)); ret=0; ss >> ret; return ret; }

// I really wished that I didn't have to explicitly use 'vector' in the definition; I'd have like to use any container with an iterator interface
template<class Type> static const vector<Type> &string_to_anytype(const string &str,vector<Type> &ret)
{
	// This function has to parse '{' ..., ... '}' where the ... can contain nested array
	// bodies, quoted strings, numbers, etc; but we only want the outermost list as vector,
	// the rest as a string

	CMutexLocker l(CNestedDataFile::cfg_parse_mutex);
	ret.clear();
	
	CNestedDataFile::s2at_string="~"+str;           // the parse knows '~' sets off an s2at_array_body
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

template<> static const string anytype_to_string(const string &any)             { return "\""+s2at::escape_chars(any)+"\""; }

template<> static const string anytype_to_string(const bool &any)               { return any ? "true" : "false"; }

template<> static const string anytype_to_string(const char &any)               { ostringstream ss; ss << any; return ss.str(); }
template<> static const string anytype_to_string(const unsigned char &any)      { ostringstream ss; ss << any; return ss.str(); }

template<> static const string anytype_to_string(const short &any)              { ostringstream ss; ss << any; return ss.str(); }
template<> static const string anytype_to_string(const unsigned short &any)     { ostringstream ss; ss << any; return ss.str(); }

template<> static const string anytype_to_string(const int &any)                { ostringstream ss; ss << any; return ss.str(); }
template<> static const string anytype_to_string(const unsigned int &any)       { ostringstream ss; ss << any; return ss.str(); }

template<> static const string anytype_to_string(const long &any)               { ostringstream ss; ss << any; return ss.str(); }
template<> static const string anytype_to_string(const unsigned long &any)      { ostringstream ss; ss << any; return ss.str(); }

template<> static const string anytype_to_string(const long long &any)          { ostringstream ss; ss << any; return ss.str(); }
template<> static const string anytype_to_string(const unsigned long long &any) { ostringstream ss; ss << any; return ss.str(); }

// I've picked a rather arbitrary way of formatting floats one way or another depending on how big it is.. I wish there were a way to output the ascii in such a way as to preserve all the information in the float (without printing the hex of it or something like that)
#include <istring>
template<> static const string anytype_to_string(const float &any)              { ostringstream ss; if(any>999999.0) {ss.setf(ios::scientific); ss.width(0); ss.precision(12); ss.fill(' '); } else {ss.setf(ios::fixed); ss.precision(6); ss.fill(' '); } ss << any; return istring(ss.str()).trim(); }
template<> static const string anytype_to_string(const double &any)             { ostringstream ss; if(any>999999.0) {ss.setf(ios::scientific); ss.width(0); ss.precision(12); ss.fill(' '); } else {ss.setf(ios::fixed); ss.precision(6); ss.fill(' '); } ss << any; return istring(ss.str()).trim(); }
template<> static const string anytype_to_string(const long double &any)        { ostringstream ss; if(any>999999.0) {ss.setf(ios::scientific); ss.width(0); ss.precision(12); ss.fill(' '); } else {ss.setf(ios::fixed); ss.precision(6); ss.fill(' '); } ss << any; return istring(ss.str()).trim(); }


// I really wished that I didn't have to explicitly use 'vector' in the definition, I'd have like to use any container with an iterator interface
template<class Type> static const string anytype_to_string(const vector<Type> &any)
{
	string s;
	size_t l=any.size();
	s="{";
	for(size_t t=0;t<l;t++)
	{
		// leaving type in case it's not able to deduce aruments and chooses the default template implemenation
		// if I knew how to constrain the original definition of the template, I would make it fully constrained
		s+=anytype_to_string(any[t]);
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
	static const string remove_surrounding_quotes(const string &str)
	{
		return (str.size()>=2 && str[0]=='"' && str[str.size()-1]=='"') ? str.substr(1,str.size()-2) : str;
	}

	/* put '\' infront of '"' and '\' and convert linefeeds into '\n' */
	static const string escape_chars(string str)
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
	static const string unescape_chars(string str)
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

