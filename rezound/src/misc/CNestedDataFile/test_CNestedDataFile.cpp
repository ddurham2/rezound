#include <stdio.h>

#include "CNestedDataFile.h"

int main()
{
	try
	{
		CNestedDataFile s("test.dat");

		s.createKey("foo.bar.bill.ester","15");
		s.createKey("foo.x",200.123);

		s.writeFile("test.dat");

	}
	catch(exception &e)
	{
		fprintf(stderr,"exception caught -- %s\n",e.what());
	}
	catch(...)
	{
		fprintf(stderr,"unknown exception caught\n");
	}

	return(0);
}

