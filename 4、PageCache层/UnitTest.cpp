#include "ObjectPool.h"
#include "ConcurrentAlloc.h"


void Alloc1()
{
	for (size_t i = 0; i < 5; ++i)
	{
		void* ptr = ConcurrentAlloc(6);
	}
}

void Alloc2()
{
	for (size_t i = 0; i < 5; ++i)
	{
		void* ptr = ConcurrentAlloc(7);
	}
}

void TLSTest()
{
	std::thread t1(Alloc1);

	std::thread t2(Alloc2);

	t1.join();
	t2.join();
}


void TestConcurrentAlloc()
{
	void* p1 = ConcurrentAlloc(6);
	void* p2 = ConcurrentAlloc(2);
	void* p3 = ConcurrentAlloc(4);
	void* p4 = ConcurrentAlloc(8);
	void* p5 = ConcurrentAlloc(7);
}
int main()
{
	//TLSTest();
	TestConcurrentAlloc();

	return 0;
}
