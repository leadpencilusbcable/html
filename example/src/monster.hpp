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
