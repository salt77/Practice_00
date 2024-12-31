#pragma once
#include "stdafx.h"

// Thread-Safe Stack
template <typename T>
class ConcurrentStack
{
public:
	ConcurrentStack() = default;

	// ���� �ڷᱸ���� push �Ű������� �����ϰ� ������ش�.
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

	// empty �� size �� �������� �ʰ� pop �� ��� ȣ�� (ȣ�� ��ÿ� �� ���� ���� ��� �ڵ忡���� ��� ���� �ٸ� �� �����Ƿ�)
	// Pop �õ�
	std::optional<T> TryPop()
	{
		SAFE_LOCK(_mutex);
		// ��������� ����
		if (_stack.empty())
			return std::nullopt;

		const T value = std::move(_stack.top());
		_stack.pop();

		return value;
	}

	// ���� ������ ���� ���� ���� ������ ���
	T WaitPop()
	{
		std::unique_lock<std::mutex> uniqueLock(_mutex);
		_condition_event.wait(uniqueLock, []() { return _stack.empty() == false; });		// ���� ���涧���� ���

		const T value = std::move(_stack.top());
		_stack.pop();

		return value;
	}

	// ���� ������ ����ϵ�, ���� �ð������� ��ٸ��ٰ� �� ������ return
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

