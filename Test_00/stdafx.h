#pragma once

#include <mutex>
#include <stack>
#include <queue>
#include <thread>
#include <vector>
#include <iostream>
#include <windows.h>
#include <future>
#include <optional>

#define SAFE_LOCK(name) std::lock_guard<std::mutex> lock(name)

