#pragma once

// Reference for PLLs: Understanding GPS/GNSS Principles and Applications by Elliott D. Kaplan (3rd Edition)

class PLL
{
public:
    PLL() {}
    virtual double update(double input) { return 0; }
    virtual double update(double input, double int_time) { return 0; }
    virtual double update(double fll_input, double pll_input, double int_time) { return 0; }
    virtual void set_bandwidth(double noise_bandwidth) {}
    virtual void set_bandwidth(double noise_bandwidth_fll, double noise_bandwidth_pll) {}
};

class FirstOrderPLL : public PLL
{
public:
    FirstOrderPLL() = default;
    FirstOrderPLL(double noise_bandwidth);

    double update(double input);
    void set_bandwidth(double noise_bandwidth);

private:
    double m_w_0;  // Natural frequency
};

class SecondOrderPLL : public PLL
{
public:
    SecondOrderPLL() = default;
    SecondOrderPLL(double noise_bandwidth, double acc0 = 0.0);

    double update(double input, double int_time);
    void set_bandwidth(double noise_bandwidth);
    void reset(double acc0 = 0.0) { m_acc = acc0; }

private:
    double m_w_0;    // Natural frequency
    double m_w_0_2;  // Squared natural frequency
    double m_a_2;    // Coefficient
    double m_acc;    // Accumulator
};

class ThirdOrderPLL : public PLL
{
public:
    ThirdOrderPLL() = default;
    ThirdOrderPLL(double noise_bandwidth, double acc0 = 0.0);

    double update(double input, double int_time);
    void set_bandwidth(double noise_bandwidth);
    void reset(double acc0 = 0.0)
    {
        m_acc1 = 0.0;
        m_acc2 = acc0;
    }

private:
    double m_w_0;    // Natural frequency
    double m_w_0_2;  // Squared natural frequency
    double m_w_0_3;  // Cubed natural frequency
    double m_a_3;    // Coefficient
    double m_b_3;    // Coefficient
    double m_acc1;   // First Accumulator
    double m_acc2;   // Second Accumulator
};

class ThirdOrderFLLAssistedPLL : public PLL
{
public:
    ThirdOrderFLLAssistedPLL() = default;
    ThirdOrderFLLAssistedPLL(double noise_bandwidth_fll, double noise_bandwidth_pll, double acc0 = 0.0);

    double update(double fll_input, double pll_input, double int_time);
    void set_bandwidth(double noise_bandwidth_fll, double noise_bandwidth_pll);
    void reset(double acc0 = 0.0)
    {
        m_acc1 = 0.0;
        m_acc2 = acc0;
    }

private:
    double m_w_0p;    // PLL Natural frequency
    double m_w_0_2p;  // PLL Squared natural frequency
    double m_w_0_3p;  // PLL Cubed natural frequency
    double m_w_0f;    // FLL Natural frequency
    double m_w_0_2f;  // FLL Squared natural frequency
    double m_a_3;     // Coefficient
    double m_b_3;     // Coefficient
    double m_a_2;     // Coefficient
    double m_acc1;    // First Accumulator
    double m_acc2;    // Second Accumulator
};

class SecondOrderFLLAssistedPLL : public PLL
{
public:
    SecondOrderFLLAssistedPLL() = default;
    SecondOrderFLLAssistedPLL(double noise_bandwidth_fll, double noise_bandwidth_pll, double acc0 = 0.0);

    double update(double fll_input, double pll_input, double int_time);
    void set_bandwidth(double noise_bandwidth_fll, double noise_bandwidth_pll);
    void reset(double acc0 = 0.0) { m_acc1 = acc0; }

private:
    double m_w_0p;    // PLL Natural frequency
    double m_w_0_2p;  // PLL Squared natural frequency
    double m_w_0f;    // FLL Natural frequency
    double m_a_2;     // Coefficient
    double m_acc1;    // First Accumulator
};