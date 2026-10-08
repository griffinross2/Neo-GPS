#include "filters.h"

#include <iostream>

FirstOrderPLL::FirstOrderPLL(double noise_bandwidth)
{
    m_w_0 = noise_bandwidth / 0.25;
}

double FirstOrderPLL::update(double input)
{
    // Update the accumulator and produce the output
    double code_error = m_w_0 * input;

    return code_error;
}

void FirstOrderPLL::set_bandwidth(double noise_bandwidth)
{
    m_w_0 = noise_bandwidth / 0.25;
}

SecondOrderPLL::SecondOrderPLL(double noise_bandwidth, double acc0)
{
    m_w_0 = noise_bandwidth / 0.53;
    m_w_0_2 = m_w_0 * m_w_0;
    m_a_2 = 1.414;
    m_acc = acc0;
}

double SecondOrderPLL::update(double input, double int_time)
{
    // Update the accumulator and produce the output
    double code_error;
    double new_acc = input * m_w_0_2 * int_time + m_acc;
    code_error = (new_acc + m_acc) * 0.5 + (m_a_2 * m_w_0 * input);
    m_acc = new_acc;

    return code_error;
}

void SecondOrderPLL::set_bandwidth(double noise_bandwidth)
{
    m_w_0 = noise_bandwidth / 0.53;
    m_w_0_2 = m_w_0 * m_w_0;
}

ThirdOrderPLL::ThirdOrderPLL(double noise_bandwidth, double acc0)
{
    m_w_0 = noise_bandwidth / 0.7845;
    m_w_0_2 = m_w_0 * m_w_0;
    m_w_0_3 = m_w_0_2 * m_w_0;
    m_a_3 = 1.1;
    m_b_3 = 2.4;
    m_acc1 = 0.0;
    m_acc2 = acc0;
}

double ThirdOrderPLL::update(double input, double int_time)
{
    // Update the accumulator and produce the output
    double code_error;
    double new_acc_1 = input * m_w_0_3 * int_time + m_acc1;
    double new_acc_2 = ((new_acc_1 + m_acc1) * 0.5 + (m_a_3 * m_w_0_2 * input)) * int_time + m_acc2;
    code_error = (new_acc_2 + m_acc2) * 0.5 + (m_b_3 * m_w_0 * input);
    m_acc1 = new_acc_1;
    m_acc2 = new_acc_2;

    return code_error;
}

void ThirdOrderPLL::set_bandwidth(double noise_bandwidth)
{
    m_w_0 = noise_bandwidth / 0.7845;
    m_w_0_2 = m_w_0 * m_w_0;
    m_w_0_3 = m_w_0_2 * m_w_0;
}

ThirdOrderFLLAssistedPLL::ThirdOrderFLLAssistedPLL(double noise_bandwidth_fll, double noise_bandwidth_pll, double acc0)
{
    m_w_0p = noise_bandwidth_pll / 0.7845;
    m_w_0_2p = m_w_0p * m_w_0p;
    m_w_0_3p = m_w_0_2p * m_w_0p;
    m_w_0f = noise_bandwidth_fll / 0.53;
    m_w_0_2f = m_w_0f * m_w_0f;
    m_a_3 = 1.1;
    m_b_3 = 2.4;
    m_a_2 = 1.414;
    m_acc1 = 0.0;
    m_acc2 = acc0;
}

double ThirdOrderFLLAssistedPLL::update(double fll_input, double pll_input, double int_time)
{
    // Update the accumulator and produce the output
    double code_error;
    double new_acc_1 = (pll_input * m_w_0_3p * int_time) + (fll_input * m_w_0_2f * int_time) + m_acc1;
    double new_acc_2 =
        (((new_acc_1 + m_acc1) * 0.5 + (m_a_3 * m_w_0_2p * pll_input) + (fll_input * m_a_2 * m_w_0f)) * int_time) +
        m_acc2;
    code_error = (new_acc_2 + m_acc2) * 0.5 + (m_b_3 * m_w_0p * pll_input);
    m_acc1 = new_acc_1;
    m_acc2 = new_acc_2;

    return code_error;
}

void ThirdOrderFLLAssistedPLL::set_bandwidth(double noise_bandwidth_fll, double noise_bandwidth_pll)
{
    m_w_0p = noise_bandwidth_pll / 0.7845;
    m_w_0_2p = m_w_0p * m_w_0p;
    m_w_0_3p = m_w_0_2p * m_w_0p;
    m_w_0f = noise_bandwidth_fll / 0.53;
    m_w_0_2f = m_w_0f * m_w_0f;
}

SecondOrderFLLAssistedPLL::SecondOrderFLLAssistedPLL(double noise_bandwidth_fll, double noise_bandwidth_pll,
                                                     double acc0)
{
    m_w_0p = noise_bandwidth_pll / 0.53;
    m_w_0_2p = m_w_0p * m_w_0p;
    m_w_0f = noise_bandwidth_fll / 0.25;
    m_a_2 = 1.414;
    m_acc1 = acc0;
}

double SecondOrderFLLAssistedPLL::update(double fll_input, double pll_input, double int_time)
{
    // Update the accumulator and produce the output
    double code_error;
    double new_acc_1 = (((fll_input * m_w_0f) + (pll_input * m_w_0_2p)) * int_time) + m_acc1;
    code_error = (new_acc_1 + m_acc1) * 0.5 + (m_a_2 * m_w_0p * pll_input);
    m_acc1 = new_acc_1;

    return code_error;
}

void SecondOrderFLLAssistedPLL::set_bandwidth(double noise_bandwidth_fll, double noise_bandwidth_pll)
{
    m_w_0p = noise_bandwidth_pll / 0.53;
    m_w_0_2p = m_w_0p * m_w_0p;
    m_w_0f = noise_bandwidth_fll / 0.25;
}