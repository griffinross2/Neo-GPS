#include "l1ca_search.h"

#include "gps_if.h"

#include "pico/time.h"

L1CASearch::L1CASearch(unsigned int search_unit) : m_search_unit(search_unit) {}

void L1CASearch::start(unsigned int channel, unsigned int sv)
{
    m_channel = channel;
    m_running = true;

    gps_if_clear_channel(channel);
    gps_if_set_channel_sv(channel, sv);
    gps_if_start_search(m_search_unit, channel, sv);
    m_start_time = time_us_64();
}

bool L1CASearch::is_finished()
{
    bool finished = gps_if_search_done(m_search_unit) && m_running;
    if (finished)
    {
        m_running = false;
    }
    return finished;
}

void L1CASearch::result(unsigned int& acc, float& doppler, float& code_phase, uint64_t& start_time_us)
{
    acc = gps_if_search_accumulator(m_search_unit);
    gps_if_search_code_doppler(m_search_unit, code_phase, doppler);
    start_time_us = m_start_time;
}