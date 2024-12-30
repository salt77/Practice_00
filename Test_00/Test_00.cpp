#include <thread>
#include <vector>
#include <mutex>
#include <iostream>
#include <windows.h>

std::mutex m;

/* 데드락 ? */
// 서로 다른 스레드들 간에서 서로 락을 하고 있는 상황
// 이 때 락/언락의 순서가 보장되지 않으면 서로의 락이 풀리길 기다리게 되는 경우가 발생하면서 작업이 더 이상 진행되지 않음
// 락의 순서를 신경쓰며 작업이 필요

/* 락의 종류와 방법 */
// 1. 스핀락 : 얻으려는 자원이 락 상태이면 락이 종료될 때까지 대기하다가 진입 (가장 기본적인 락)
// 비효율적이게 보이지만, 짧은 시간만 대기할 것으로 예상되는 자원에 사용하면 좋음 (컨텍스트 스위칭 비용을 낭비하지 않는다.)
// 2. 슬립 : 스핀락에서 약간 더 나아간 개념으로 얻으려는 자원이 락 상태이면 잠깐 휴식을 가지고 다시 자원을 얻으려고 시도한다.
// 3. 커널 Event 활용하는 방법 : 언락 상태가 되면 커널단의 이벤트를 활용해서 대기하는 스레드들에게 알려 실행
// 대기 시간이 너무 길다면 이벤트를 통해 필요한 상황에서만 작업하도록 할 수 있다. (스핀락처럼 매번 체크를 하지 않기 때문에 CPU 활용 성능이 높음)

// 스핀락 + 슬립
class SpinLock
{
public:
	void lock()
	{
		while (isLockedFlag.test_and_set())
		{
			// 100ms 만큼 쉰다.
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			
			// 활동하는 다른 스레드에게 작업권을 넘긴다. (언제든지 되돌아올 수 있음)
			//std::this_thread::yield();
		}
	}

	void unlock()
	{
		isLockedFlag.clear();
	}

	std::atomic_flag isLockedFlag = ATOMIC_FLAG_INIT;
};

SpinLock sl;
HANDLE handle;

int a = 0;

void Add()
{
	for (int i = 0; i < 100'0000; ++i)
	{
		//sl.lock();
		++a;
		//sl.unlock();
	}

	SetEvent(handle);
}

void Sub()
{
	// 이벤트가 시그널 상태가 아니면 아래 반복문을 실행하지 않고 여기서 기다린다. (bManualReset == FALSE : wait 종료 시 자동으로 논-시그널 상태로 변경함)
	// 현재 예제는 사실상 싱글 스레드 동작 방식인데, 큐 이벤트와 같은 곳에서 사용하는 것이 더 올바르다.
	WaitForSingleObject(handle, INFINITE);

	for (int i = 0; i < 100'0000; ++i)
	{
		//sl.lock();
		--a;
		//sl.unlock();
	}
}

int main()
{
	// 커널 이벤트 생성
	handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	std::thread t0(Add);
	std::thread t1(Sub);

	t0.join();
	t1.join();

	std::cout << a << std::endl;

	if (handle)
	{
		CloseHandle(handle);
	}

	return 0;
}