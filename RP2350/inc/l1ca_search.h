#pragma once

#include <cstdint>

class L1CASearch
{
public:
    L1CASearch(unsigned int search_unit);
    void start(unsigned int channel, unsigned int sv);
    bool is_finished();
    void result(unsigned int& acc, float& doppler, float& code_phase, uint64_t& start_time_us);

private:
    bool m_running = false;
    unsigned int m_search_unit;
    unsigned int m_channel;
    uint64_t m_start_time;
};