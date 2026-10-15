#pragma once

// Reference for PLLs: Understanding GPS/GNSS Principles and Applications by Elliott D. Kaplan (3rd Edition)

class PLL
{
public:
    PLL() {}
    virtual float update(float input) { return 0; }
    virtual float update(float input, float int_time) { return 0; }
    virtual float update(float fll_input, float pll_input, float int_time) { return 0; }
    virtual void set_bandwidth(float noise_bandwidth) {}
    virtual void set_bandwidth(float noise_bandwidth_fll, float noise_bandwidth_pll) {}
};

class FirstOrderPLL : public PLL
{
public:
    FirstOrderPLL() = default;
    FirstOrderPLL(float noise_bandwidth);

    float update(float input);
    void set_bandwidth(float noise_bandwidth);

private:
    float m_w_0;  // Natural frequency
};

class SecondOrderPLL : public PLL
{
public:
    SecondOrderPLL() = default;
    SecondOrderPLL(float noise_bandwidth, float acc0 = 0.0);

    float update(float input, float int_time);
    void set_bandwidth(float noise_bandwidth);
    void reset(float acc0 = 0.0) { m_acc = acc0; }

private:
    float m_w_0;    // Natural frequency
    float m_w_0_2;  // Squared natural frequency
    float m_a_2;    // Coefficient
    float m_acc;    // Accumulator
};

class ThirdOrderPLL : public PLL
{
public:
    ThirdOrderPLL() = default;
    ThirdOrderPLL(float noise_bandwidth, float acc0 = 0.0);

    float update(float input, float int_time);
    void set_bandwidth(float noise_bandwidth);
    void reset(float acc0 = 0.0)
    {
        m_acc1 = 0.0;
        m_acc2 = acc0;
    }

private:
    float m_w_0;    // Natural frequency
    float m_w_0_2;  // Squared natural frequency
    float m_w_0_3;  // Cubed natural frequency
    float m_a_3;    // Coefficient
    float m_b_3;    // Coefficient
    float m_acc1;   // First Accumulator
    float m_acc2;   // Second Accumulator
};

class ThirdOrderFLLAssistedPLL : public PLL
{
public:
    ThirdOrderFLLAssistedPLL() = default;
    ThirdOrderFLLAssistedPLL(float noise_bandwidth_fll, float noise_bandwidth_pll, float acc0 = 0.0);

    float update(float fll_input, float pll_input, float int_time);
    void set_bandwidth(float noise_bandwidth_fll, float noise_bandwidth_pll);
    void reset(float acc0 = 0.0)
    {
        m_acc1 = 0.0;
        m_acc2 = acc0;
    }

private:
    float m_w_0p;    // PLL Natural frequency
    float m_w_0_2p;  // PLL Squared natural frequency
    float m_w_0_3p;  // PLL Cubed natural frequency
    float m_w_0f;    // FLL Natural frequency
    float m_w_0_2f;  // FLL Squared natural frequency
    float m_a_3;     // Coefficient
    float m_b_3;     // Coefficient
    float m_a_2;     // Coefficient
    float m_acc1;    // First Accumulator
    float m_acc2;    // Second Accumulator
};

class SecondOrderFLLAssistedPLL : public PLL
{
public:
    SecondOrderFLLAssistedPLL() = default;
    SecondOrderFLLAssistedPLL(float noise_bandwidth_fll, float noise_bandwidth_pll, float acc0 = 0.0);

    float update(float fll_input, float pll_input, float int_time);
    void set_bandwidth(float noise_bandwidth_fll, float noise_bandwidth_pll);
    void reset(float acc0 = 0.0) { m_acc1 = acc0; }

private:
    float m_w_0p;    // PLL Natural frequency
    float m_w_0_2p;  // PLL Squared natural frequency
    float m_w_0f;    // FLL Natural frequency
    float m_a_2;     // Coefficient
    float m_acc1;    // First Accumulator
};