#include "tools.h"

#include "constants.h"

#include <numbers>
#include <cmath>

void bytes_to_number(void* dest, const uint8_t* buf, size_t dest_size, size_t src_size, bool is_signed)
{
    if ((dest_size < src_size) || (dest_size % 8 != 0))
    {
        return;
    }
    uint64_t result = 0;

    // Pack bytes
    for (size_t i = 0; i < src_size; i++)
    {
        result |= ((uint64_t)buf[src_size - i - 1] << i);
    }

    // Preserve sign if signed
    if (is_signed)
    {
        uint8_t sign_bit = (result >> (src_size - 1)) & 0x1;
        result |= sign_bit ? (0xFFFFFFFFFFFFFFFF << src_size) : 0;
    }

    // Copy result to dest
    memcpy(dest, &result, dest_size / 8);
}

void ecef_to_coords(double x, double y, double z, double& lat, double& lon, double& alt)
{
    const double p = std::sqrt(x * x + y * y);

    lon = 2.0 * std::atan2(y, x + p);
    lat = std::atan(z / (p * (1.0 - WGS84_E2)));
    alt = 0.0;

    for (int i = 0; i < 100; i++)
    {
        double N = WGS84_A / std::sqrt(1.0 - WGS84_E2 * std::sin(lat) * std::sin(lat));
        double alt_new = p / std::cos(lat) - N;
        lat = std::atan(z / (p * (1.0 - WGS84_E2 * N / (N + alt_new))));
        if (std::fabs(alt_new - alt) < 1e-3)
        {
            alt = alt_new;
            break;
        }
        alt = alt_new;
    }

    lat *= 180.0 / std::numbers::pi;
    lon *= 180.0 / std::numbers::pi;
}