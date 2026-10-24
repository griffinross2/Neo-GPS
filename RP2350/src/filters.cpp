#include "filters.h"

#include "pico/stdlib.h"

FirstOrderPLL::FirstOrderPLL(float noise_bandwidth)
{
    m_w_0 = noise_bandwidth / 0.25;
}

float __not_in_flash_func(FirstOrderPLL::update)(float input)
{
    // Update the accumulator and produce the output
    float code_error = m_w_0 * input;

    return code_error;
}

void __not_in_flash_func(FirstOrderPLL::set_bandwidth)(float noise_bandwidth)
{
    m_w_0 = noise_bandwidth / 0.25;
}

SecondOrderPLL::SecondOrderPLL(float noise_bandwidth, float acc0)
{
    m_w_0 = noise_bandwidth / 0.53;
    m_w_0_2 = m_w_0 * m_w_0;
    m_a_2 = 1.414;
    m_acc = acc0;
}

float __not_in_flash_func(SecondOrderPLL::update)(float input, float int_time)
{
    // Update the accumulator and produce the output
    float code_error;
    float new_acc = input * m_w_0_2 * int_time + m_acc;
    code_error = (new_acc + m_acc) * 0.5 + (m_a_2 * m_w_0 * input);
    m_acc = new_acc;

    return code_error;
}

void __not_in_flash_func(SecondOrderPLL::set_bandwidth)(float noise_bandwidth)
{
    m_w_0 = noise_bandwidth / 0.53;
    m_w_0_2 = m_w_0 * m_w_0;
}

ThirdOrderPLL::ThirdOrderPLL(float noise_bandwidth, float acc0)
{
    m_w_0 = noise_bandwidth / 0.7845;
    m_w_0_2 = m_w_0 * m_w_0;
    m_w_0_3 = m_w_0_2 * m_w_0;
    m_a_3 = 1.1;
    m_b_3 = 2.4;
    m_acc1 = 0.0;
    m_acc2 = acc0;
}

float __not_in_flash_func(ThirdOrderPLL::update)(float input, float int_time)
{
    // Update the accumulator and produce the output
    float code_error;
    float new_acc_1 = input * m_w_0_3 * int_time + m_acc1;
    float new_acc_2 = ((new_acc_1 + m_acc1) * 0.5 + (m_a_3 * m_w_0_2 * input)) * int_time + m_acc2;
    code_error = (new_acc_2 + m_acc2) * 0.5 + (m_b_3 * m_w_0 * input);
    m_acc1 = new_acc_1;
    m_acc2 = new_acc_2;

    return code_error;
}

void __not_in_flash_func(ThirdOrderPLL::set_bandwidth)(float noise_bandwidth)
{
    m_w_0 = noise_bandwidth / 0.7845;
    m_w_0_2 = m_w_0 * m_w_0;
    m_w_0_3 = m_w_0_2 * m_w_0;
}

ThirdOrderFLLAssistedPLL::ThirdOrderFLLAssistedPLL(float noise_bandwidth_fll, float noise_bandwidth_pll, float acc0)
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

float __not_in_flash_func(ThirdOrderFLLAssistedPLL::update)(float fll_input, float pll_input, float int_time)
{
    // Update the accumulator and produce the output
    float code_error;
    float new_acc_1 = (pll_input * m_w_0_3p * int_time) + (fll_input * m_w_0_2f * int_time) + m_acc1;
    float new_acc_2 =
        (((new_acc_1 + m_acc1) * 0.5 + (m_a_3 * m_w_0_2p * pll_input) + (fll_input * m_a_2 * m_w_0f)) * int_time) +
        m_acc2;
    code_error = (new_acc_2 + m_acc2) * 0.5 + (m_b_3 * m_w_0p * pll_input);
    m_acc1 = new_acc_1;
    m_acc2 = new_acc_2;

    return code_error;
}

void __not_in_flash_func(ThirdOrderFLLAssistedPLL::set_bandwidth)(float noise_bandwidth_fll, float noise_bandwidth_pll)
{
    m_w_0p = noise_bandwidth_pll / 0.7845;
    m_w_0_2p = m_w_0p * m_w_0p;
    m_w_0_3p = m_w_0_2p * m_w_0p;
    m_w_0f = noise_bandwidth_fll / 0.53;
    m_w_0_2f = m_w_0f * m_w_0f;
}

SecondOrderFLLAssistedPLL::SecondOrderFLLAssistedPLL(float noise_bandwidth_fll, float noise_bandwidth_pll, float acc0)
{
    m_w_0p = noise_bandwidth_pll / 0.53;
    m_w_0_2p = m_w_0p * m_w_0p;
    m_w_0f = noise_bandwidth_fll / 0.25;
    m_a_2 = 1.414;
    m_acc1 = acc0;
}

float __not_in_flash_func(SecondOrderFLLAssistedPLL::update)(float fll_input, float pll_input, float int_time)
{
    // Update the accumulator and produce the output
    float code_error;
    float new_acc_1 = (((fll_input * m_w_0f) + (pll_input * m_w_0_2p)) * int_time) + m_acc1;
    code_error = (new_acc_1 + m_acc1) * 0.5 + (m_a_2 * m_w_0p * pll_input);
    m_acc1 = new_acc_1;

    return code_error;
}

void __not_in_flash_func(SecondOrderFLLAssistedPLL::set_bandwidth)(float noise_bandwidth_fll, float noise_bandwidth_pll)
{
    m_w_0p = noise_bandwidth_pll / 0.53;
    m_w_0_2p = m_w_0p * m_w_0p;
    m_w_0f = noise_bandwidth_fll / 0.25;
}