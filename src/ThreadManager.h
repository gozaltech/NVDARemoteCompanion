#pragma once
#include <thread>
#include <atomic>
#include <vector>
#include <functional>
#include <memory>
#include "Debug.h"

namespace ThreadManager {
    class WorkerThread {
    private:
        std::thread m_thread;
        std::atomic<bool> m_shouldStop{false};
        std::string m_name;
        
    public:
        explicit WorkerThread(const std::string& name) : m_name(name) {}
        
        ~WorkerThread() {
            Stop();
        }
        
        template<typename Func>
        void Start(Func&& func) {
            if (!m_thread.joinable()) {
                m_shouldStop = false;
                m_thread = std::thread([this, func = std::forward<Func>(func)]() {
                    DEBUG_VERBOSE_F("THREAD", "Starting worker thread: {}", m_name);
                    try {
                        func(m_shouldStop);
                    } catch (const std::exception& e) {
                        DEBUG_ERROR_F("THREAD", "Exception in worker thread {}: {}", m_name, e.what());
                    }
                    DEBUG_VERBOSE_F("THREAD", "Worker thread {} ended", m_name);
                });
            }
        }
        
        void Stop() {
            if (m_thread.joinable()) {
                DEBUG_VERBOSE_F("THREAD", "Stopping worker thread: {}", m_name);
                m_shouldStop = true;
                m_thread.join();
                DEBUG_VERBOSE_F("THREAD", "Worker thread {} stopped", m_name);
            }
        }
        
        bool IsRunning() const { 
            return m_thread.joinable() && !m_shouldStop; 
        }
    };
    
    class ThreadPool {
    private:
        std::vector<std::unique_ptr<WorkerThread>> m_threads;
        
    public:
        ~ThreadPool() {
            StopAll();
        }
        
        template<typename Func>
        void AddWorker(const std::string& name, Func&& func) {
            auto worker = std::make_unique<WorkerThread>(name);
            worker->Start(std::forward<Func>(func));
            m_threads.push_back(std::move(worker));
        }
        
        void StopAll() {
            DEBUG_VERBOSE_F("THREAD", "Stopping {} worker threads", m_threads.size());
            for (auto& thread : m_threads) {
                thread->Stop();
            }
            m_threads.clear();
        }
        
        size_t Size() const { return m_threads.size(); }
        
        bool AllRunning() const {
            return std::all_of(m_threads.begin(), m_threads.end(),
                             [](const auto& thread) { return thread->IsRunning(); });
        }
    };
}