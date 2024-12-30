#include <thread>
#include <vector>
#include <mutex>

std::mutex m;
std::vector<int> v;

void Emplace()
{
	for (int i = 0; i < 100'0000; ++i)
	{
		// 1. 락, 언락은 재귀적으로 할 수 없음
		//m.lock();
		//m.lock();

		// 2. 락 가드, 유니크 락을 통해 뮤텍스의 생명주기를 쉽게 관리 가능
		//std::lock_guard<std::mutex> lockGuard(m);
		//std::unique_lock<std::mutex> uniqueLock(m);

		// 3. 유니크 락 생성자 매개변수에 defer_lock 을 넣어줌으로써 Lock/Unlock 시점을 제어 가능
		//std::unique_lock<std::mutex> uniqueLock(m, std::defer_lock);
		//uniqueLock.lock();

		// 3-2. try_to_lock : 이미 잠겨있다면 바로 해제하여 반환 (내부에서 try_lock 호출)
		//std::unique_lock<std::mutex> uniqueLock(m, std::try_to_lock);

		// 3-3. adopt_lock : 이미 잠겨있는 뮤텍스 락을 인계받음
		m.lock();
		std::unique_lock<std::mutex> uniqueLock(m, std::adopt_lock);

		//std::unique_lock<std::mutex> uniqueLock(m, std::defer_lock);

		v.emplace_back(i);
		
		//m.unlock();
	}
}

int main()
{
	std::thread t1(Emplace);
	std::thread t2(Emplace);

	if (t1.joinable())
		t1.join();

	if (t2.joinable())
		t2.join();

	return 0;
}