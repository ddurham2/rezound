#define TESTING_TPOOLFILE

#include "TPoolFile.h"
#include "TPoolFile.cpp"

#include "TPoolAccesser.h"

#include <stdlib.h>
#include <unistd.h>

			struct foo
			{
				bool dirty;
				int a;
				int b;
			};

int main()
{

	try
	{
		
		typedef TPoolFile<unsigned,unsigned> poolfile_t;

		// test the allocator
		{
			unlink("/tmp/test.pf");
			poolfile_t poolfile;
			poolfile.openFile("/tmp/test.pf");

			poolfile_t::CPhysicalAddressSpaceManager &a=poolfile.pasm;

			int a1=a.alloc(10); // testing alloc case 1
			a.verify();
			int a2=a.alloc(10);
			int a3=a.alloc(10);
			int a4=a.alloc(10);
			int a5=a.alloc(10);
			int a6=a.alloc(10);
			a.verify();



			a.free(a4);	// testing free case 1
			a.verify();

			a.free(a3);	// testing free case 2
			a.verify();

			a.free(a5);	// testing free case 3
			a.verify();

			a.free(a1);
			a.free(a2);	// testing free case 4
			a.verify();


			a1=a.alloc(10);	// testing alloc case 2
			a.verify();

			a2=a.alloc(40);	// testing alloc case 3
			a.verify();

			a3=a.alloc(20);	// testing alloc case 4
			a.verify();

			a4=a.split_block(a3,10); // testing split_block
			a.verify();


			a2=a.partial_free(a2,a2+10,30); // testing partial_free case 1
			a.verify();

			a.partial_free(a2,a2+10,20); // testing partial_free case 2
			a.verify();

			a1=a.partial_free(a1,a1+5,5); // testing partial_free case 4
			a.verify();

			a1=a.partial_free(a1,a1+1,4); // testing partial_free case 3
			a.verify();


			a3=a.partial_free(a3,a3,5); // testing partial_free case 5
			a.verify();

			a3=a.partial_free(a3,a3,3); // testing partial_free case 6
			a.verify();

			a.print();


			a.set_file_size(100);



			int a7=a.alloc(100);
			a.free(a7);
			a7=a.alloc(1000);
			int a8=a.alloc(1000); // testing alloc case 1.5
			a.verify();



			// testing buildFromSAT()
			vector<vector<poolfile_t::RLogicalBlock> > SAT;
			{
				poolfile_t::RLogicalBlock b;
				vector<poolfile_t::RLogicalBlock> s;

				b.logicalStart=3;
				b.physicalStart=3;
				b.size=7;
				s.push_back(b);

				b.logicalStart=10;
				b.physicalStart=20;
				b.size=10;
				s.push_back(b);

				b.logicalStart=20;
				b.physicalStart=40;
				b.size=10;
				s.push_back(b);

				b.logicalStart=30;
				b.physicalStart=60;
				b.size=10;
				s.push_back(b);

				SAT.push_back(s);
			}
			{
				poolfile_t::RLogicalBlock b;
				vector<poolfile_t::RLogicalBlock> s;

				b.logicalStart=0;
				b.physicalStart=10;
				b.size=10;
				s.push_back(b);

				b.logicalStart=10;
				b.physicalStart=30;
				b.size=10;
				s.push_back(b);

				b.logicalStart=20;
				b.physicalStart=50;
				b.size=10;
				s.push_back(b);

				b.logicalStart=30;
				b.physicalStart=70;
				b.size=10;
				s.push_back(b);

				SAT.push_back(s);
			}

			a.buildFromSAT(SAT);
			a.verify();
			a.print();

		}

		// test insertSpace cases
		{
			unlink("/tmp/test.pf");
			poolfile_t poolfile(10000);
			poolfile.openFile("/tmp/test.pf");

			poolfile.createPool<char>("test");

			// testing case 4/6
			poolfile.insertSpace(0,0,100);
			poolfile.verifyAllBlockInfo(false);

			// testing case 3/6 and case 6/6
			poolfile.insertSpace(0,100,100);
			poolfile.verifyAllBlockInfo(false);

			// testing case 5/6
			poolfile.insertSpace(0,200,30000);
			poolfile.verifyAllBlockInfo(false);


			// testing case 1/6
			poolfile.insertSpace(0,100,100);
			poolfile.verifyAllBlockInfo(false);

			// testing case 2/6
			poolfile.insertSpace(0,100,1000);
			poolfile.verifyAllBlockInfo(false);

			poolfile.printSAT();
		}


		// test removeSpace cases
		{
			unlink("/tmp/test.pf");
			poolfile_t poolfile(10000);
			poolfile.openFile("/tmp/test.pf");

			poolfile.createPool<char>("test");

			poolfile.insertSpace(0,0,40000);
			poolfile.verifyAllBlockInfo(false);

			// testing case 1/4
			poolfile.removeSpace(0,10000,10000);
			poolfile.verifyAllBlockInfo(false);

			// testing case 2/4
			poolfile.removeSpace(0,10000,1000);
			poolfile.verifyAllBlockInfo(false);

			// testing case 3/4
			poolfile.removeSpace(0,10000+8000,1000);
			poolfile.verifyAllBlockInfo(false);

			// testing case 4/4
			poolfile.removeSpace(0,10000+2000,1000);
			poolfile.verifyAllBlockInfo(false);

			// testing cases 1-3 
			poolfile.removeSpace(0,0,poolfile.getPoolSize(0));
			poolfile.verifyAllBlockInfo(false);

			poolfile.printSAT();
		}

		// test moveData cases
		{
			unlink("/tmp/test.pf");
			poolfile_t poolfile(10000);
			poolfile.openFile("/tmp/test.pf");

			poolfile.createPool<char>("test");
			poolfile.createPool<char>("test2");

			poolfile.insertSpace(0,0,50000);
			poolfile.insertSpace(1,0,50000);
			poolfile.verifyAllBlockInfo(false);


			// testing case 2/6 and 3/6
			poolfile.moveData(1,10000,0,20000,10000);
			poolfile.verifyAllBlockInfo(false);

			// testing case 2.5/6 and 4/6
			poolfile.moveData(1,15000,0,20000,5000);
			poolfile.verifyAllBlockInfo(false);

			// testing case 5/6
			poolfile.moveData(1,10000,0,15000,5000);
			poolfile.verifyAllBlockInfo(false);

			// testing case 6/6
			poolfile.moveData(1,10000,0,11000,1000);
			poolfile.verifyAllBlockInfo(false);

			// ---------------------------------------------
			poolfile.clear();

			poolfile.createPool<char>("test");
			poolfile.createPool<char>("test2");

			poolfile.insertSpace(0,0,50000);
			poolfile.verifyAllBlockInfo(false);


			// testing case 1/6
			poolfile.moveData(0,10000,0,20000,15000);
			poolfile.verifyAllBlockInfo(false);

			poolfile.moveData(0,20000,0,10000,15000);
			poolfile.verifyAllBlockInfo(false);

			poolfile.printSAT();
		}

		// testing that moveData back and forth joins split blocks back together
		//   it would be necessary to visually look at the two SATs before and after
		{
			unlink("/tmp/test.pf");
			poolfile_t poolfile(10000);
			poolfile.openFile("/tmp/test.pf");

			poolfile.createPool<char>("test");
			poolfile.createPool<char>("test2");

			poolfile.insertSpace(0,0,100000);
			poolfile.verifyAllBlockInfo(false);

			poolfile.printSAT();
			poolfile.moveData(1,0,0,25000,50000);
			poolfile.moveData(0,25000,1,0,50000);



			poolfile.printSAT();
		}

		// test defrag
		{
			printf("-------------------------------------------------\n");
			unlink("/tmp/test.pf");
			poolfile_t poolfile(17);
			poolfile.openFile("/tmp/test.pf");

			typedef foo element_t;

			poolfile.createPool<element_t>("test1");
			poolfile.createPool<element_t>("test2");
			//poolfile.createPool<element_t>("test3");

			for(int x=0;x<poolfile.getPoolIndexCount();x++)
			{
				if(x!=1)
				{
					TPoolAccesser<element_t,poolfile_t> a=poolfile.getPoolAccesser<element_t>(x);
					a.insert(0,100);
				}
			}

			for(int x=0;x<poolfile.getPoolIndexCount();x++)
			{
				TPoolAccesser<element_t,poolfile_t> a=poolfile.getPoolAccesser<element_t>(x);
				for(int t=0;t<a.getSize();t++)
					a[t].a=t;
			}

			int x1;
			{
				TPoolAccesser<element_t,poolfile_t> a0=poolfile.getPoolAccesser<element_t>(0);
				TPoolAccesser<element_t,poolfile_t> a1=poolfile.getPoolAccesser<element_t>(1);

				a1.moveData(0,a0,x1=(a0.getSize()/4),a0.getSize()/2);
			}
				

			poolfile.printSAT();

			poolfile.defrag();
			poolfile.verifyAllBlockInfo(false);

			{
				TPoolAccesser<element_t,poolfile_t> a0=poolfile.getPoolAccesser<element_t>(0);
				TPoolAccesser<element_t,poolfile_t> a1=poolfile.getPoolAccesser<element_t>(1);

				a0.moveData(x1,a1,0,a1.getSize());
			}
			//poolfile.defrag();

			for(int x=0;x<poolfile.getPoolIndexCount();x++)
			{
				TPoolAccesser<element_t,poolfile_t> a=poolfile.getPoolAccesser<element_t>(x);
				for(int t=0;t<a.getSize();t++)
				{
					if(a[t].a!=t)
					{
						printf("DATA INCONSISTANCY\n");
						exit(0);
					}
				}
			}

			poolfile.printSAT();
		}
#if 0
		{
			printf("-------------------------------------------------\n");
			unlink("/tmp/test.pf");
			poolfile_t poolfile(32768);
			poolfile.openFile("/tmp/test.pf");

			typedef foo element_t;

			poolfile.createPool<element_t>("test1");
			poolfile.createPool<element_t>("test2");
			//poolfile.createPool<element_t>("test3");
			//poolfile.createPool<element_t>("test4");

			for(int x=0;x<poolfile.getPoolIndexCount();x++)
			{
				TPoolAccesser<element_t,poolfile_t> a=poolfile.getPoolAccesser<element_t>(x);
				//poolfile.insertSpace(x,0,100);
				a.insert(0,1);
			}

			for(int t=0;t<1;t++)
			{
				int pool=rand()%poolfile.getPoolIndexCount();
				TPoolAccesser<element_t,poolfile_t> a=poolfile.getPoolAccesser<element_t>(pool);
				a.insert(rand()%poolfile.getPoolSize(pool)/sizeof(element_t),rand()%100+1);
			}

			TPoolAccesser<element_t, poolfile_t> *accessers[256];
			for(int x=0;x<poolfile.getPoolIndexCount();x++)
				accessers[x]=new TPoolAccesser<element_t,poolfile_t>(poolfile.getPoolAccesser<element_t>(x));

			for(int x=0;x<poolfile.getPoolIndexCount();x++)
			{
				for(int t=0;t<accessers[x]->getSize();t++)
					(*(accessers[x]))[t].a=t;
			}

			int x1;
			{
				TPoolAccesser<element_t,poolfile_t> a0=poolfile.getPoolAccesser<element_t>(0);
				TPoolAccesser<element_t,poolfile_t> a4=poolfile.getPoolAccesser<element_t>(1);

				a4.moveData(0,a0,x1=(a0.getSize()/4),a0.getSize()/2);
			}
				

			poolfile.printSAT();

			poolfile.defrag();
			poolfile.verifyAllBlockInfo(false);

			{
				TPoolAccesser<element_t,poolfile_t> a0=poolfile.getPoolAccesser<element_t>(0);
				TPoolAccesser<element_t,poolfile_t> a4=poolfile.getPoolAccesser<element_t>(1);

				a0.moveData(x1,a4,0,a4.getSize());
			}
			//poolfile.defrag();

			for(int x=0;x<poolfile.getPoolIndexCount();x++)
			{
				TPoolAccesser<element_t,poolfile_t> a=poolfile.getPoolAccesser<element_t>(x);
				for(int t=0;t<a.getSize();t++)
				{
					if((*(accessers[x]))[t].a!=t)
					{
						printf("DATA INCONSISTANCY\n");
						exit(0);
					}
				}
			}

			for(int x=0;x<poolfile.getPoolIndexCount();x++)
				delete accessers[x];

			poolfile.printSAT();
		}
#endif
	}
	catch(exception &e)
	{
		printf("exception: %s\n",e.what());

	}

	return 0;
}
