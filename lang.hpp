// ---=== LANGUAGE (PL/EN) ===---
// Bilingual support modelled on the Decoder-Malfunction-Simulator's
// `tl(pl, en)` + `--en`/`--english` pattern.
#ifndef PICKLEBOT_LANG_HPP
#define PICKLEBOT_LANG_HPP

#include <string>

// returns the English string when English mode is on, otherwise the Polish one
std::string tl(const std::string& pl, const std::string& en);

bool english_mode();
void set_english(bool en);

#endif // PICKLEBOT_LANG_HPP
