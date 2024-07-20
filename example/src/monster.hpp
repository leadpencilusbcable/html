#pragma once

#include <string>
#include <vector>

struct stats
{
    int str;
    int dex;
    int con;
    int _int;
    int wis;
    int cha;
};

struct trait
{
    std::string name;
    std::vector<std::string> text;
    std::vector<std::string> attacks;
};

struct monster
{
    std::string name;
    std::string size;
    std::string type;
    std::string alignment;
    int ac;
    int hp;
    struct stats stats;
    std::string cr;
    std::vector<trait> traits;
    std::vector<trait> actions;
};


bool m_name_comp(monster mon1, monster mon2)
{
    return mon1.name < mon2.name;
}

bool m_ac_comp(monster mon1, monster mon2)
{
    return mon1.ac < mon2.ac;
}

bool m_cr_comp(monster mon1, monster mon2)
{
    return mon1.cr < mon2.cr;
}

bool m_hp_comp(monster mon1, monster mon2)
{
    return mon1.hp < mon2.hp;
}

std::unordered_map<std::string, bool (*)(monster, monster)> query_param_compare_funcs_asc = {
    {"name", *m_name_comp},
    {"ac", *m_ac_comp},
    {"cr", *m_cr_comp},
    {"hp", *m_hp_comp},
};

bool m_name_comp_dsc(monster mon1, monster mon2)
{
    return mon1.name > mon2.name;
}

bool m_ac_comp_dsc(monster mon1, monster mon2)
{
    return mon1.ac > mon2.ac;
}

bool m_cr_comp_dsc(monster mon1, monster mon2)
{
    return mon1.cr > mon2.cr;
}

bool m_hp_comp_dsc(monster mon1, monster mon2)
{
    return mon1.hp > mon2.hp;
}

std::unordered_map<std::string, bool (*)(monster, monster)> query_param_compare_funcs_dsc = {
    { "name", *m_name_comp_dsc },
    { "ac", *m_ac_comp_dsc },
    { "cr", *m_cr_comp_dsc },
    { "hp", *m_hp_comp_dsc },
};