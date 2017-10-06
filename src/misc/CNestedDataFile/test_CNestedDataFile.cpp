#include <iostream>

#include "CNestedDataFile.h"

class foo
{
	public:
		foo() { }
};


int main()
{
	try
	{
		CNestedDataFile s("test.dat");

		setlocale(LC_ALL,"");

		// test reading int on top level
		cout << "foo_int: " << s.getValue<int>("foo_int") << endl;

		// test reading float on top level
		printf("foo_float1: %25.25f\n",s.getValue<double>("foo_float1"));
		printf("foo_float2: %25.25f\n",s.getValue<double>("foo_float2"));

		// test reading string on top level
		cout << "foo_string: '" << s.getValue<string>("foo_string") << "'" << endl;

		// test reading bool on top level
		cout << "foo_bool: " << s.getValue<bool>("foo_bool1") << endl;
		cout << "foo_bool: " << s.getValue<bool>("foo_bool2") << endl;

		// should cause a somewhat informative error message
		//s.getValue<foo>("foo");
		//s.setValue<foo>("foo",foo());

		// test reading vector<vector<string> > on top level
		{
			{
				const vector<vector<string> > v=s.getValue<vector<vector<string> > >("foo_vector");
				for(size_t x=0;x<v.size();x++)
					for(size_t y=0;y<v[x].size();y++)
						cout << "foo_vector[" << x << "][" << y << "]: " << v[x][y] << endl;
			}

			{
				const vector<string> v=s.getValue<vector<string> >("foo_vector2");
				cout << "should be zero: " << v.size() << endl;
			}

			// read vector as string
			cout << "vector value as string: '" << s.getValue<string>("foo_vector") << "'" << endl;
		}

		
		// test reading string from deep scopes
		cout << "aaa.b.ccc: '" << s.getValue<string>("aaa"DOT"b"DOT"ccc"DOT"bar_string") << "'" << endl;


		// test writing int to deep scopes
		cout << "writing d.eee.f.baz_int" << endl;
		s.setValue<int>("d"DOT"eee"DOT"f"DOT"baz_int",27);

		// test writing float to deep scopes
		cout << "writing d.eee.f.baz_float1" << endl;
		s.setValue<double>("d"DOT"eee"DOT"f"DOT"baz_float1",12.12345678912345678);
		cout << "writing d.eee.f.baz_float2" << endl;
		s.setValue<double>("d"DOT"eee"DOT"f"DOT"baz_float2",10000000.12345);

		// test writing string to deep scopes
		cout << "writing d.eee.f.baz_string" << endl;
		s.setValue<string>("d"DOT"eee"DOT"f"DOT"baz_string","\"qwer\"asdf\\zxcv\"");

		// test writing bool to deep scopes
		cout << "writing d.eee.f.baz_bool" << endl;
		s.setValue<bool>("d"DOT"eee"DOT"f"DOT"baz_bool1",true);
		s.setValue<bool>("d"DOT"eee"DOT"f"DOT"baz_bool2",false);

		// test writing vector<vector<string> > to deep scopes
		cout << "writing d.eee.f.baz_vector" << endl;
		{
			vector<vector<string> > v;
			vector<string> v1,v2;

			v1.push_back("qwer");
			v1.push_back("asdf");
			v1.push_back("zxcv");

			v2.push_back("uiop");
			v2.push_back("hjkl");
			v2.push_back("vbnm");

			v.push_back(v1);
			v.push_back(v2);

			s.setValue<vector<vector<string> > >("d"DOT"eee"DOT"f"DOT"baz_vector",v);
		}


		// test getting child names
		{
			const vector<string> v=s.getChildKeys("aaa");
			for(size_t t=0;t<v.size();t++)
				cout << "aaa." << v[t] << endl;
		}

		s.writeFile("test.dat2");

	}
	catch(exception &e)
	{
		fprintf(stderr,"%s caught -- %s\n",typeid(e).name(),e.what());
		return 1;
	}
	catch(...)
	{
		fprintf(stderr,"unknown exception caught\n");
		return 1;
	}

	return 0;
}

