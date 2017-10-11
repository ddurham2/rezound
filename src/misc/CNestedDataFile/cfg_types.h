#ifndef cfg_types_H__
#define cfg_types_H__
#pragma once

#include "CNestedDataFile.h"


struct RTokenPosition
{
	int first_line,last_line;
	int first_column,last_column;
	const char *text;
};
#define CFG_LTYPE RTokenPosition


union cfg_parse_union
{
	char *				stringValue;
	CNestedDataFile::CVariant *	variant;
};
#define CFG_STYPE union cfg_parse_union

#endif // cfg_stype_H__
