#pragma once
#include "stdafx.h"

// Thread-Safe Stack
template <typename T>
class ConcurrentStack
{
public:
	ConcurrentStack() = default;

	// 기존 자료구조의 push 매개변수와 동일하게 만들어준다.
	void push(const T& arg)
	{
		SAFE_LOCK(_mutex);	// lock_guard
		_stack.push(arg);

		_condition_event.notify_one();
	}

	void push(T&& arg)
	{
		SAFE_LOCK(_mutex);
		_stack.push(arg);

		_condition_event.notify_one();
	}

	// empty 와 size 는 구현하지 않고 pop 에 묶어서 호출 (호출 당시와 그 다음 실제 사용 코드에서의 결과 값이 다를 수 있으므로)
	// Pop 시도
	std::optional<T> TryPop()
	{
		SAFE_LOCK(_mutex);
		// 비어있으면 실패
		if (_stack.empty())
			return std::nullopt;

		const T value = std::move(_stack.top());
		_stack.pop();

		return value;
	}

	// 조건 변수로 실제 값이 있을 때까지 대기
	T WaitPop()
	{
		std::unique_lock<std::mutex> uniqueLock(_mutex);
		_condition_event.wait(uniqueLock, []() { return _stack.empty() == false; });		// 값이 생길때까지 대기

		const T value = std::move(_stack.top());
		_stack.pop();

		return value;
	}

	// 조건 변수로 대기하되, 일정 시간까지만 기다리다가 값 없으면 return
	std::optional<T> WaitUntilPop(const std::chrono::milliseconds& ms)
	{
		std::unique_lock<std::mutex> uniqueLock(_mutex);
		_condition_event.wait_until(uniqueLock, std::chrono::system_clock::now() + ms, []() { return _stack.empty() == false; });

		if (_stack.empty())
			return std::nullopt;

		const T value = std::move(_stack.top());
		_stack.pop();

		return value;
	}

private:
	std::mutex				_mutex;
	std::stack<T>			_stack;
	std::condition_variable _condition_event;
};

