
#ifndef MEM_MONITOR_HPP
#define MEM_MONITOR_HPP

// mem monitor
// Copyright (c) 2014, Matthias Petri, All rights reserved.
// Modified by Roberto Konow for multi-platform

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3.0 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

#include <chrono>
#include <iostream>
#include <fstream>
#include <vector>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <stdexcept>

#include <memory_status.hpp>

#ifndef MEM_MON_MEM_LIMIT_MB
#define MEM_MON_MEM_LIMIT_MB 32
#endif

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::vector;

struct mem_stat {
    steady_clock::time_point timestamp;
    uint64_t pid;
    uint64_t VmPeak;
    uint64_t VmRSS;
    uint64_t event_id;
};

class mem_monitor
{
    private:
        steady_clock::time_point                                    m_start_timestamp;
        vector<mem_stat>                                                      m_stats;
        vector<std::string>                                             m_event_names;
        std::ofstream                                                            m_os;
        milliseconds                                                    m_granularity;
        std::condition_variable                                                  m_cv;
        std::mutex                                                            m_mutex;
        std::thread                                                          m_thread;
        std::atomic<bool>                               m_run = ATOMIC_VAR_INIT(true);
        std::atomic<uint64_t>                     m_cur_event_id = ATOMIC_VAR_INIT(0);
        uint64_t                             m_memory_limit_mb = MEM_MON_MEM_LIMIT_MB;
        bool                                                           m_write_header;

    private:
        void monitor() {
            while (m_run) {
                m_stats.emplace_back(get_current_stats());

                if (write_needed()) {
                    flush();
                }

                if (m_run) {
                    std::unique_lock<std::mutex> lk(m_mutex);
                    m_cv.wait_for(lk, m_granularity);
                }
            }
        }

        mem_stat get_current_stats() {
            mem_stat stat;
            stat.timestamp = steady_clock::now();
            stat.event_id = m_cur_event_id;
            stat.VmPeak = getPeakRSS();
            stat.VmRSS = getCurrentRSS();
            return stat;
        }

        bool write_needed() {
            if (m_memory_limit_mb * 1024 * 1024 < m_stats.size() * sizeof(mem_stat))
                return true;
            else
                return false;
        }

    public:
        void flush() {
            if (m_write_header) {
                m_os << "time_ms;pid;VmPeak;VmRSS;event\n";
                m_write_header = false;
            }

            for (const auto &s : m_stats) {
                m_os << duration_cast<milliseconds>(s.timestamp - m_start_timestamp).count() + 1 << ";" // round up!
                << s.pid    << ";"
                << s.VmPeak << ";"
                << s.VmRSS  << ";"
                << '"' << m_event_names[s.event_id] << '"' << "\n";
            }
            m_stats.clear();
        }
        // delete the constructors and operators we don't want
        mem_monitor() = delete;
        mem_monitor(const mem_monitor&) = delete;
        mem_monitor(mem_monitor&&) = delete;
        mem_monitor& operator=(const mem_monitor&) = delete;
        mem_monitor& operator=(mem_monitor&&) = delete;

        mem_monitor(const std::string& file_name,
                    milliseconds granularity = milliseconds(50))
            : m_os(file_name), m_granularity(granularity), m_write_header(true) {
            // some init stuff
            m_event_names.push_back(""); // default empty event
            m_start_timestamp = steady_clock::now();

            if (!m_os.is_open()) { // output stream open?
                throw std::ios_base::failure("memory monitor output file could not be opened.");
            }

            // spawn the thread
            m_thread = std::thread(&mem_monitor::monitor, this);
        }

        ~mem_monitor() {
            m_run = false;
            m_cv.notify_one(); // notify the sleeping thread
            m_thread.join();
            flush();
            m_os.close();
        }

        void event(const std::string& ev) {
            m_event_names.push_back(ev);
            m_cur_event_id++;
        }
};

#endif //MEM_MONITOR_HPP