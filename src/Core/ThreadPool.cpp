#include "ThreadPool.h"
#include <chrono>

#ifdef __EMSCRIPTEN__

ThreadPool::ThreadPool(unsigned int) {}
ThreadPool::~ThreadPool() = default;

void ThreadPool::enqueue(std::function<void()> job)
{
    m_jobs.push(std::move(job));
}

size_t ThreadPool::pending() const
{
    return m_jobs.size();
}

void ThreadPool::runPending(double budgetMilliseconds)
{
    const auto start = std::chrono::steady_clock::now();
    while (!m_jobs.empty())
    {
        std::function<void()> job = std::move(m_jobs.front());
        m_jobs.pop();
        job();

        const double elapsed = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= budgetMilliseconds) break;
    }
}

#else

ThreadPool::ThreadPool(unsigned int threadCount)
{
    if (threadCount == 0) threadCount = 1;
    for (unsigned int i = 0; i < threadCount; ++i)
    {
        m_workers.emplace_back([this]() {
            for (;;)
            {
                std::function<void()> job;
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_cv.wait(lock, [this]() { return m_stopping.load() || !m_jobs.empty(); });
                    if (m_stopping.load() && m_jobs.empty()) return;
                    job = std::move(m_jobs.front());
                    m_jobs.pop();
                }
                job();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    m_stopping.store(true);
    m_cv.notify_all();
    for (auto& worker : m_workers)
        if (worker.joinable()) worker.join();
}

void ThreadPool::enqueue(std::function<void()> job)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_jobs.push(std::move(job));
    }
    m_cv.notify_one();
}

size_t ThreadPool::pending() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_jobs.size();
}

void ThreadPool::runPending(double) {}

#endif
