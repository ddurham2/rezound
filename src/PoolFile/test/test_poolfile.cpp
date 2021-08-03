#include "gtest/gtest.h"

#include <unistd.h>
#include <stdio.h>

#include "../TPoolFile.h"
#include "../TPoolAccesser.h"


TEST(PoolFile, basic) {
	// testing basic functionality

	TPoolFile <uint16_t, uint16_t> f(256, "testpool");
	unlink("test-init.pf");
	unlink("test.pf");
	f.openFile("test-init.pf");
	ASSERT_TRUE(f.isOpen());
	ASSERT_EQ(f.getFilename(), "test-init.pf");
	f.rename("test.pf");
	ASSERT_EQ(f.getFilename(), "test.pf");
	f.flushData();
	f.backupSAT();
	f.closeFile(true, false);

	f.openFile("test.pf");

	ASSERT_EQ(f.isExclusiveLocked(), false);
	ASSERT_EQ(f.getSharedLockCount(), 0);

	f.exclusiveLock();
	ASSERT_EQ(f.isExclusiveLocked(), true);
	ASSERT_EQ(f.getSharedLockCount(), 0);

	f.exclusiveUnlock();
	ASSERT_EQ(f.isExclusiveLocked(), false);
	ASSERT_EQ(f.getSharedLockCount(), 0);

	ASSERT_TRUE(f.exclusiveTrylock());
	ASSERT_EQ(f.isExclusiveLocked(), true);
	ASSERT_EQ(f.getSharedLockCount(), 0);

	f.exclusiveUnlock();
	ASSERT_EQ(f.isExclusiveLocked(), false);
	ASSERT_EQ(f.getSharedLockCount(), 0);

	f.sharedLock();
	ASSERT_FALSE(f.exclusiveTrylock());
	ASSERT_EQ(f.isExclusiveLocked(), false);
	ASSERT_EQ(f.getSharedLockCount(), 1);

	f.sharedLock();
	ASSERT_EQ(f.isExclusiveLocked(), false);
	ASSERT_EQ(f.getSharedLockCount(), 2);

	f.sharedUnlock();
	ASSERT_EQ(f.isExclusiveLocked(), false);
	ASSERT_EQ(f.getSharedLockCount(), 1);

	f.sharedUnlock();
	ASSERT_EQ(f.isExclusiveLocked(), false);
	ASSERT_EQ(f.getSharedLockCount(), 0);

	// no pools have been created
	char buf[4];
	EXPECT_THROW(f.getPoolAccesser<int>(31), std::exception);
	EXPECT_THROW(f.getPoolAccesser<int>("foo"), std::exception);
	EXPECT_THROW(const_cast<const decltype(f)&>(f).getPoolAccesser<int>(31), std::exception);
	EXPECT_THROW(const_cast<const decltype(f)&>(f).getPoolAccesser<int>("foo"), std::exception);
	EXPECT_THROW(f.readPoolRaw(31, buf, 4, 0), std::exception);
	EXPECT_THROW(f.readPoolRaw("foo", buf, 4, 0), std::exception);
	EXPECT_THROW(f.writePoolRaw(31, "blah", 4, 0), std::exception);
	EXPECT_THROW(f.writePoolRaw("foo", "blah", 4, 0), std::exception);
	EXPECT_THROW(f.getPoolSize(31), std::exception);
	EXPECT_THROW(f.getPoolSize("foo"), std::exception);
	EXPECT_THROW(f.removePool(31), std::exception);
	EXPECT_THROW(f.removePool("foo"), std::exception);
	f.clear();
	EXPECT_FALSE(f.containsPool("foo"));
	EXPECT_THROW(f.getPoolAlignment(31), std::exception);
	EXPECT_THROW(f.getPoolAlignment("foo"), std::exception);
	EXPECT_THROW(f.setPoolAlignment(31, 4), std::exception);
	EXPECT_THROW(f.setPoolAlignment("foo", 4), std::exception);
	EXPECT_THROW(f.swapPools(31, 32), std::exception);
	EXPECT_THROW(f.swapPools("foo", "bar"), std::exception);
	ASSERT_EQ(f.getPoolIndexCount(), 0);
	EXPECT_THROW(f.getPoolIdByName("foo"), std::exception);
	EXPECT_THROW(f.getPoolIdByIndex(0), std::exception);
	EXPECT_THROW(f.getPoolNameById(31), std::exception);
	ASSERT_FALSE(f.isValidPoolId(31));
	EXPECT_THROW(f.clearPool(31), std::exception);
	EXPECT_THROW(f.clearPool("foo"), std::exception);

	{
		{ auto a = f.createPool<uint32_t>("foo"); }
		EXPECT_THROW(f.createPool<uint32_t>("foo"), std::exception);

		EXPECT_NO_THROW(f.getPoolAccesser<uint32_t>("foo"));
		EXPECT_NO_THROW(const_cast<const decltype(f)&>(f).getPoolAccesser<int>("foo"));
		EXPECT_EQ(f.readPoolRaw("foo", buf, 0, 0), 0);
		EXPECT_EQ(f.writePoolRaw("foo", "blah", 4, 0), 0);
		EXPECT_EQ(f.getPoolSize("foo"), 0);
		EXPECT_TRUE(f.containsPool("foo"));
		EXPECT_EQ(f.getPoolAlignment("foo"), 4);
		EXPECT_NO_THROW(f.setPoolAlignment("foo", 2));
		EXPECT_THROW(f.swapPools("foo", "bar"), std::exception);
		ASSERT_EQ(f.getPoolIndexCount(), 1);
		EXPECT_NO_THROW(f.getPoolIdByName("foo"));
		EXPECT_NO_THROW(f.clearPool("foo"));

		f.removePool("foo");
		EXPECT_FALSE(f.containsPool("foo"));
		ASSERT_EQ(f.getPoolIndexCount(), 0);
	}

	{ // repeat, but just call clear()
		{ auto a = f.createPool<uint32_t>("foo"); }
		EXPECT_THROW(f.createPool<uint32_t>("foo"), std::exception);

		EXPECT_NO_THROW(f.getPoolAccesser<uint32_t>("foo"));
		EXPECT_NO_THROW(const_cast<const decltype(f)&>(f).getPoolAccesser<int>("foo"));
		EXPECT_EQ(f.readPoolRaw("foo", buf, 0, 0), 0);
		EXPECT_EQ(f.writePoolRaw("foo", "blah", 4, 0), 0);
		EXPECT_EQ(f.getPoolSize("foo"), 0);
		EXPECT_TRUE(f.containsPool("foo"));
		EXPECT_EQ(f.getPoolAlignment("foo"), 4);
		EXPECT_NO_THROW(f.setPoolAlignment("foo", 2));
		EXPECT_THROW(f.swapPools("foo", "bar"), std::exception);
		ASSERT_EQ(f.getPoolIndexCount(), 1);
		EXPECT_NO_THROW(f.getPoolIdByName("foo"));
		EXPECT_NO_THROW(f.clearPool("foo"));

		f.clear();
		EXPECT_FALSE(f.containsPool("foo"));
		ASSERT_EQ(f.getPoolIndexCount(), 0);
	}

	// TODO copyToFile
}

TEST(PoolFile, one_pool) {
	TPoolFile <uint16_t, uint16_t> f(256, "testpool");
	unlink("test.pf");
	f.openFile("test.pf");
	ASSERT_TRUE(f.isOpen());

	{
		TPoolAccesser<uint32_t, decltype(f)> a = f.createPool<uint32_t>("foo");
		EXPECT_THROW(a.insert(1, 1), std::exception); // can't insert out of bounds

		size_t size = 0;
		ASSERT_EQ(a.getSize(), size);

		// insert several elements right at the beginning of increasing size
		for (int c = 0; c < 10; ++c) {
			a.insert(0, c);
		}

		size += 0+1+2+3+4+5+6+7+8+9;
		ASSERT_EQ(a.getSize(), size);

		// insert several elements at the end of increasing size
		for (int c = 0; c < 10; ++c) {
			a.insert(a.getSize(), c);
		}

		size += 0+1+2+3+4+5+6+7+8+9;
		ASSERT_EQ(a.getSize(), size);

		a.insert(a.getSize()/2, 10);

		size += 10;
		ASSERT_EQ(a.getSize(), size);

		ASSERT_EQ(size, 100); // ensure we have 100 elements
		f.printSAT();

		for(int t = 0; t < 100; ++t) { a[t] = t; }

		// use direct index interface
		for(int t = 0; t < 100; ++t) { ASSERT_EQ(a[t], t); }

		// use stream interface
		for(int t = 0; t < 100; ++t) { uint32_t c=7; a.read(&c, 1); ASSERT_EQ(c, t); }
		a.seek(0);

		{
			// move elements 50-99 and insert at position 1 (not 0)
			a.moveData(1, a, 50, 49);
			
			size_t p = 0;
			ASSERT_EQ(a[p++], 0);
			for(int t = 50; t < 99; ++t) {
				ASSERT_EQ(a[p++], t);
			}
			for(int t = 1; t < 50; ++t) {
				ASSERT_EQ(a[p++], t);
			}
			ASSERT_EQ(a[p++], 99);
			ASSERT_EQ(p, 100);

			// undo the move
			a.moveData(50, a, 1, 49);
			for(int t = 0; t < 100; ++t) { ASSERT_EQ(a[t], t); }
		}
		
		{
			// copy elements 50-99 and to position 1 (not 0)
			auto b = a;
			a.copyData(1, b, 50, 49);
			//for(int t = 0; t < 100; ++t) { printf("%d \n", a[t]); }
			
			size_t p = 0;
			ASSERT_EQ(a[p++], 0);
			for(int t = 50; t < 99; ++t) {
				ASSERT_EQ(a[p++], t);
			}
			for(int t = 50; t < 100; ++t) {
				ASSERT_EQ(a[p++], t);
			}
			ASSERT_EQ(p, 100);
		}

		{
			const uint32_t buf[4] = {0, 1, 2, 3, };
			a.seek(100);
			a.write(buf, 4);
			size_t p = 100;
			for(int t = 0; t < 4; ++t) {
				ASSERT_EQ(a[p++], t);
			}
		}


		{
			a.zeroData(0, a.getSize());
			size_t p = 0;
			for(int t = 0; t < a.getSize(); ++t) {
				ASSERT_EQ(a[p++], 0);
			}
		}

	}
}

