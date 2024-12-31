#include "stdafx.h"

std::mutex m;
std::condition_variable cv;	// 조건 변수 : 이벤트와 유사하지만 커널 오브젝트가 아닌 유저 레벨 오브젝트

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

private:
	std::atomic_flag isLockedFlag = ATOMIC_FLAG_INIT;
};

SpinLock sl;
HANDLE handle;

int a = 0;

void Add()
{
	std::unique_lock<std::mutex> lock(m);

	for (int i = 0; i < 100'0000; ++i)
	{
		//sl.lock();
		++a;
		//sl.unlock();
	}

	//SetEvent(handle);
	cv.notify_one();
}

void Sub()
{
	// 이벤트가 시그널 상태가 아니면 아래 반복문을 실행하지 않고 여기서 기다린다. (bManualReset == FALSE : wait 종료 시 자동으로 논-시그널 상태로 변경함)
	// 현재 예제는 사실상 싱글 스레드 동작 방식인데, 큐 이벤트와 같은 곳에서 사용하는 것이 더 올바르다.
	//WaitForSingleObject(handle, INFINITE);

	for (int i = 0; i < 100'0000; ++i)
	{
		// 조건 변수는 항상 먼저 유니크 락을 건 상태에서 시작한다. (매개변수로 넘겨줌)
		// 두 번째 매개변수로 받은 함수가 true 이면 아래 코드를 실행하고 아니면 다음 notify 를 기다린다.
		// notify 를 받아서 깨어나는 것인데 더블 체크하는 이유 : notify 자체도 원자적으로 실행되지 않기 때문에 알림을 받고 깨어났는데 다른 스레드가 자원을 이미 활용했을 수가 있어서
		std::unique_lock<std::mutex> ul(m);
		cv.wait(ul, []() { return a > 0; });

		//sl.lock();
		--a;
		//sl.unlock();
	}
}

// std::future : 이름 그대로 미래에 결과물을 받아보기 위해서 사용하는 객체
// std::async 매개변수에 따라서 동작이 완전히 달라진다.
// 1. std::launch::defered : 지연 실행 (객체만 생성해두고 미래에 get 하는 시점에서 함수를 호출, 싱글 스레드처럼 동작)
// 2. std::launch::async : 임시 스레드를 생성해서 비동기 실행 (멀티 스레드 실행이지만, 별도로 스레드를 생성해서 관리해줄 필요가 없고 간단하게 사용하는 방법이다.)
unsigned int ProcessFuture()
{
	int sum = 0;
	for (int i = 0; i < 1000'0000; ++i)
	{
		// 대충 무거운 작업 실행
		sum += i;
	}

	return sum;
}

int main()
{
	// future 객체 생성
	std::future<unsigned int> future = std::async(std::launch::async, ProcessFuture);

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

	std::cout << future.get() << std::endl;

	return 0;
}