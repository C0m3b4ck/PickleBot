// ---=== LANGUAGE (PL/EN) ===---
#include "lang.hpp"

static bool angielski = false;

bool english_mode() { return angielski; }
void set_english(bool en) { angielski = en; }

std::string tl(const std::string& pl, const std::string& en)
{
    return angielski ? en : pl;
}
