#include "auth.hpp"
#include "discord_link.hpp"
#include <Geode/Geode.hpp>
#include <Geode/loader/GameEvent.hpp>

using namespace geode::prelude;

$on_mod(Loaded) {
    auth::load();
    link::start();
}

$on_game(Exiting) {
    link::stop();
}
