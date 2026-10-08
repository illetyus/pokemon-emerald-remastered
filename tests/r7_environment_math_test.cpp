#include "RemasterEnvironmentMath.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>

namespace
{
int Checks = 0;
void check(bool value, const char* message)
{
    ++Checks;
    if (!value) { std::fprintf(stderr, "R7 failed: %s\n", message); std::exit(1); }
}
bool close(double a, double b) { return std::abs(a - b) < 1e-10; }
}

int main()
{
    using namespace remaster::environment;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    check(hash_valid(std::string(64, 'a')), "valid hash");
    check(!hash_valid(std::string(64, 'G')), "invalid hex");
    check(!hash_valid(std::string(63, 'a')), "invalid length");
    check(budget_valid(6000, 3000, 1500, 2, 1024, 4000), "mobile boundaries");
    check(!budget_valid(6001, 3000, 1500, 2, 1024, 4000), "triangle ceiling");
    check(!budget_valid(100, 200, 50, 2, 1024, 4000), "non-descending LOD");
    check(!budget_valid(6000, 3000, 1500, 3, 1024, 4000), "material ceiling");
    check(!budget_valid(6000, 3000, 1500, 2, 2048, 4000), "texture ceiling");
    check(!budget_valid(6000, 3000, 1500, 2, 1024, nan), "finite culling");
    check(!budget_valid(6000, 3000, 1500, 2, 1024, 999), "near camera culling floor");
    check(can_admit(31, 255, 54000, 6000, true), "last chunk admission");
    check(!can_admit(32, 255, 54000, 6000, true), "component ceiling");
    check(can_admit(32, 255, 54000, 6000, false), "existing component allowed");
    check(!can_admit(32, 256, 54000, 6000, false), "instance ceiling");
    check(!can_admit(1, 1, 59999, 2, false), "triangle sum ceiling");
    check(!can_admit(-1, 1, 0, 1, true), "invalid chunk metadata");

    check(close(follow_alpha(8, 0), 0), "paused frame");
    check(close(follow_alpha(8, -1), 0), "negative frame");
    check(close(follow_alpha(nan, 1), 0), "nonfinite speed");
    check(close(follow_alpha(8, nan), 0), "nonfinite frame");
    check(close(follow_alpha(-1, 1), 0), "negative speed");
    double results[3]{};
    const int frames[] = {30, 60, 120};
    for (int index = 0; index < 3; ++index)
        for (int frame = 0; frame < frames[index]; ++frame)
            results[index] += (100 - results[index]) * follow_alpha(8, 1.0 / frames[index]);
    check(close(results[0], results[1]) && close(results[1], results[2]), "frame-rate-independent follow");
    check(close(results[0], 100 * (1 - std::exp(-8))), "one-second response");
    check(close(follow_alpha(8, 10000), 1), "long frame bounded");
    check(context(8) == Context::Indoor && context(9) == Context::Indoor, "indoor map types");
    check(context(4) == Context::Cave && context(5) == Context::Underwater, "cave/water map types");
    check(context(6) == Context::Outdoor && context(-1) == Context::Outdoor, "outdoor/fallback");
    check(framing(Context::Indoor).arm < framing(Context::Outdoor).arm, "interior framing");
    for (int id : {3, 5, 13}) check(weather(id) == Weather::Rain, "runtime rain");
    check(weather(7) == Weather::Dust && weather(9) == Weather::Fog, "runtime versus coordinate weather IDs");
    for (int id : {4, 15, 20, 21, 255, -1}) check(weather(id) == Weather::Unresolved, "no invented cycle simulation");
    check(close(hour(-1), 23) && close(hour(25), 1) && close(hour(nan), 12), "hour normalization");
    check(close(lighting(12, Context::Outdoor, Weather::Clear).solar, 1), "noon");
    check(close(lighting(0, Context::Outdoor, Weather::Clear).solar, 0), "midnight");
    check(close(lighting(12, Context::Outdoor, Weather::Rain).fog,
                lighting(0, Context::Outdoor, Weather::Rain).fog), "weather retained across hour change");
    check(close(lighting(12, Context::Indoor, Weather::Fog).fog, 0), "indoor fog suppressed");
    check(close(lighting(0, Context::Cave, Weather::Clear).ambient,
                lighting(12, Context::Cave, Weather::Clear).ambient), "cave ambient stable");

    check(occludes({0,0,10}, {10,0,0}, {4,-1,4}, {6,1,6}), "intervening roof");
    check(!occludes({0,0,10}, {10,0,0}, {4,2,4}, {6,3,6}), "off-axis roof");
    check(!occludes({0,0,10}, {10,0,0}, {12,-1,-4}, {14,1,-2}), "behind player");
    check(!occludes({0,0,10}, {10,0,0}, {9,-1,-1}, {11,1,1}), "player endpoint retained");
    check(!occludes({0,0,10}, {10,0,0}, {-1,-1,9}, {1,1,11}), "camera endpoint retained");
    check(!occludes({0,0,10}, {0,0,10}, {-1,-1,9}, {1,1,11}), "zero ray");
    check(!occludes({nan,0,10}, {10,0,0}, {4,-1,4}, {6,1,6}), "nonfinite ray");
    check(!occludes({0,0,10}, {10,0,0}, {6,1,6}, {4,-1,4}), "inverted bounds");
    check(occludes({10,0,0}, {0,0,10}, {4,-1,4}, {6,1,6}), "reverse ray");
    std::printf("R7 presentation math passed: %d checks\n", Checks);
    return 0;
}
