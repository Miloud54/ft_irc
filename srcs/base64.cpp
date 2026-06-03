#include "base64.hpp"
#include <vector>

std::string base64_decode(const std::string &in) 
{
    std::string out;
    static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<int> T(256, -1);
    
    for (int i = 0; i < 64; i++)
        T[(unsigned char)chars[i]] = i;
    
    int val = 0;
    int valb = -8;

    for (size_t i = 0; i < in.size(); ++i)
    {
        unsigned char c = in[i];
        if (T[c] == -1)
            break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0)
        {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}