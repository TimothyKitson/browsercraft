#pragma once
#include <queue>
#include <functional>

#ifndef __EMSCRIPTEN__
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#endif

// Runs chunk generation and meshing off the critical path.
//
// Native builds use real worker threads. The browser build is
// single-threaded on purpose: pthreads in WebAssembly need
// SharedArrayBuffer, which requires COOP/COEP headers that most static
// hosts (GitHub Pages, itch.io) don't send. Instead the jobs queue up and
// runPending() works through them with a per-frame time budget, which
// keeps the frame rate smooth even though the work is serialised.
class ThreadPool
{
public:
    explicit ThreadPool(unsigned int threadCount);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void enqueue(std::function<void()> job);
    size_t pending() const;

    // Single-threaded builds only: run queued jobs until the budget runs
    // out. No-op when worker threads are doing the work.
    void runPending(double budgetMilliseconds);

private:
#ifdef __EMSCRIPTEN__
    std::queue<std::function<void()>> m_jobs;
#else
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_jobs;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_stopping{ false };
#endif
};
