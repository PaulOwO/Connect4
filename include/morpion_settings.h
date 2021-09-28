#pragma once
#include <SFML/System/Vector2.hpp>

namespace morpion
{
constexpr int maxClientNmb = 2;
constexpr unsigned short serverPortNumber = 65000;
enum class MorpionPhase
{
    CONNECTION,
    GAME,
    END
};

using PlayerNumber = unsigned char;
struct Move
{
    int position;
    PlayerNumber playerNumber;
};
}
