#pragma once
#include "stdafx.h"

template <typename T>
class ConcurrentQueue
{
public:
	ConcurrentQueue() = default;

	// ���� �ڷᱸ���� push �Ű������� �����ϰ� ������ش�.
	void push(const T& arg)
	{
		SAFE_LOCK(_mutex);	// lock_guard
		_queue.push(arg);

		_condition_event.notify_one();
	}

	void push(T&& arg)
	{
		SAFE_LOCK(_mutex);
		_queue.push(arg);

		_condition_event.notify_one();
	}

	// empty �� size �� �������� �ʰ� pop �� ��� ȣ�� (ȣ�� ��ÿ� �� ���� ���� ��� �ڵ忡���� ��� ���� �ٸ� �� �����Ƿ�)
	// Pop �õ�
	std::optional<T> TryPop()
	{
		SAFE_LOCK(_mutex);
		// ��������� ����
		if (_queue.empty())
			return std::nullopt;

		const T value = std::move(_queue.top());
		_queue.pop();

		return value;
	}

	// ���� ������ ���� ���� ���� ������ ���
	T WaitPop()
	{
		std::unique_lock<std::mutex> uniqueLock(_mutex);
		_condition_event.wait(uniqueLock, []() { return _queue.empty() == false; });		// ���� ���涧���� ���

		const T value = std::move(_queue.top());
		_queue.pop();

		return value;
	}

	// ���� ������ ����ϵ�, ���� �ð������� ��ٸ��ٰ� �� ������ return
	std::optional<T> WaitUntilPop(const std::chrono::milliseconds& ms)
	{
		std::unique_lock<std::mutex> uniqueLock(_mutex);
		_condition_event.wait_until(uniqueLock, std::chrono::system_clock::now() + ms, []() { return _queue.empty() == false; });

		if (_queue.empty())
			return std::nullopt;

		const T value = std::move(_queue.top());
		_queue.pop();

		return value;
	}

private:
	std::mutex				_mutex;
	std::queue<T>			_queue;
	std::condition_variable _condition_event;
};

