#include "check.hpp"
#include "../src/auth_step.hpp"
#include "../src/deafen_intent.hpp"
#include "../src/frame.hpp"
#include "../src/level_edit.hpp"
#include "../src/level_key.hpp"
#include "../src/range.hpp"
#include "../src/run_tracker.hpp"
#include "../src/socket_paths.hpp"
#include <optional>
#include <string>

static constexpr RunConfig ON { .enabled = true, .start = 50.f, .end = 100.f, .practice = false };

static void runTracker() {
    { RunTracker t;
      check("below threshold does nothing", !t.frame(ON, 49.9f, false, false));
      check("threshold deafens", t.frame(ON, 50.f, false, false) == true);
      check("deafens once per run", !t.frame(ON, 51.f, false, false)); }
    { RunTracker t; t.frame(ON, 60.f, false, false);
      check("end undeafens", t.ended() == false);
      check("end undeafens once", !t.ended()); }
    { RunTracker t; check("end without deafen does nothing", !t.ended()); }
    { RunTracker t; t.frame(ON, 60.f, false, false); t.ended();
      check("frames after death do not deafen again", !t.frame(ON, 61.f, false, false));
      check("restart after death sends nothing", !t.restarted());
      check("next attempt deafens again", t.frame(ON, 60.f, false, false) == true); }
    { RunTracker t; t.frame(ON, 60.f, false, false);
      check("restart while deafened undeafens", t.restarted() == false); }
    { RunTracker t; auto c = ON; c.enabled = false;
      check("disabled does nothing", !t.frame(c, 60.f, false, false)); }
    { RunTracker t; check("platformer does nothing", !t.frame(ON, 60.f, false, true)); }
    { RunTracker t; check("100 percent does nothing", !t.frame(ON, 100.f, false, false)); }
    { RunTracker t; check("practice off does nothing", !t.frame(ON, 60.f, true, false)); }
    { RunTracker t; auto c = ON; c.practice = true;
      check("practice on deafens", t.frame(c, 60.f, true, false) == true); }
}

static constexpr RunConfig TO_80 { .enabled = true, .start = 50.f, .end = 80.f, .practice = false };

static void runRange() {
    { RunTracker t; t.frame(TO_80, 60.f, false, false);
      check("before the end stays deafened", !t.frame(TO_80, 79.9f, false, false));
      check("end undeafens mid-run", t.frame(TO_80, 80.f, false, false) == false);
      check("after the end stays quiet", !t.frame(TO_80, 81.f, false, false) && !t.ended());
      t.restarted();
      check("next attempt deafens after a range end", t.frame(TO_80, 55.f, false, false) == true); }
    { RunTracker t; check("starting past the end does nothing", !t.frame(TO_80, 85.f, false, false)); }
    { RunTracker t; t.frame(TO_80, 60.f, false, false);
      check("change that keeps us in range does nothing", !t.configChanged(ON, 60.f)); }
    { RunTracker t; t.frame(TO_80, 60.f, false, false); auto c = TO_80; c.start = 70.f;
      check("raising the start past us undeafens", t.configChanged(c, 60.f) == false);
      check("and deafens again at the new start", t.frame(c, 70.f, false, false) == true); }
    { RunTracker t; t.frame(TO_80, 60.f, false, false); auto c = TO_80; c.enabled = false;
      check("turning it off undeafens", t.configChanged(c, 60.f) == false); }
    { RunTracker t; t.frame(TO_80, 60.f, false, false); t.frame(TO_80, 80.f, false, false);
      check("extending the end sends nothing itself", !t.configChanged(ON, 81.f));
      check("extending the end deafens again", t.frame(ON, 81.f, false, false) == true); }
}

static void range() {
    using enum RangeThumb;
    check("slider left edge is 0", rangeValue(-5.f, 200.f) == 0.f);
    check("slider right edge is 100", rangeValue(500.f, 200.f) == 100.f);
    check("slider snaps to whole percents", rangeValue(101.1f, 200.f) == 51.f);
    check("near start picks start", pickThumb(30.f, 20.f, 80.f) == Start);
    check("near end picks end", pickThumb(70.f, 20.f, 80.f) == End);
    check("together, left picks start", pickThumb(40.f, 50.f, 50.f) == Start);
    check("together, right picks end", pickThumb(60.f, 50.f, 50.f) == End);
    check("together at 100 picks start", pickThumb(100.f, 100.f, 100.f) == Start);
    check("together at 0 picks end", pickThumb(0.f, 0.f, 0.f) == End);
    Range r { 20.f, 80.f };
    check("start stops at the end", moveThumb(r, Start, 95.f) == Range(80.f, 80.f));
    check("end stops at the start", moveThumb(r, End, 5.f) == Range(20.f, 20.f));
    check("start moves freely below the end", moveThumb(r, Start, 33.f) == Range(33.f, 80.f));
}

static void deafenIntent() {
    { DeafenIntent i; i.want(true);
      check("already deafened sends nothing", !i.next(true));
      i.want(false);
      check("already deafened stays deafened", !i.next(true)); }
    { DeafenIntent i; i.want(true); i.want(false);
      check("die before reply sends nothing", !i.next(false)); }
    { DeafenIntent i; i.want(true);
      check("wants deafen", i.needsCheck());
      check("sends deafen", i.next(false) == true);
      i.applied(true);
      check("settled while deafened", !i.needsCheck());
      i.want(false);
      check("wants undeafen", i.needsCheck());
      check("sends undeafen", i.next(true) == false);
      i.applied(false);
      check("settled after undeafen", !i.needsCheck()); }
    { DeafenIntent i; i.want(true); i.next(false); i.want(false); i.applied(true);
      check("die between send and reply still undeafens", i.needsCheck() && i.next(true) == false); }
    { DeafenIntent i; i.want(true); i.next(false); i.applied(true); i.want(false);
      check("player undeafened themself: sends nothing", !i.next(false));
      check("player undeafened themself: settled", !i.needsCheck()); }
    { DeafenIntent i; i.want(true); i.next(true);
      check("player deafened before run: not asked again", !i.needsCheck()); }
    { DeafenIntent i; i.want(true); i.next(false); i.want(false);
      check("missed reply then death still undeafens", i.needsCheck() && i.next(true) == false); }
    { DeafenIntent i; i.want(true); i.next(false);
      check("missed reply is our deafen, not the player's", i.needsCheck() && !i.next(true));
      check("missed reply settles", !i.needsCheck());
      i.want(false);
      check("missed reply undeafens at the end", i.next(true) == false); }
    { DeafenIntent i; i.want(true); i.next(false);
      check("lost request is sent again", i.next(false) == true); }
    { DeafenIntent i; i.want(true); i.next(false); i.refused(); i.want(false);
      check("refused deafen is not ours", !i.needsCheck()); }
    { DeafenIntent i; i.want(true); i.next(false); i.applied(true); i.want(false); i.next(true); i.refused();
      check("refused undeafen is tried again", i.needsCheck() && i.next(true) == false); }
    { DeafenIntent i; i.want(true); i.next(false); i.applied(true); i.linkLost(); i.want(false);
      check("lost link never undeafens", !i.next(true)); }
}

static void framing() {
    using Frame = std::pair<uint32_t, std::string>;
    check("encodes little endian", encodeFrame(1, "{}") == std::string("\1\0\0\0\2\0\0\0{}", 10));
    { FrameReader r; r.feed(encodeFrame(1, "{\"a\":1}"));
      check("round trip", r.pop() == Frame(1, "{\"a\":1}"));
      check("nothing left", !r.pop()); }
    { FrameReader r; std::optional<Frame> got; bool early = false;
      auto bytes = encodeFrame(3, "hello");
      for (size_t i = 0; i < bytes.size(); ++i) {
          r.feed(std::string_view(bytes).substr(i, 1));
          got = r.pop();
          if (got && i + 1 < bytes.size()) early = true;
      }
      check("one byte at a time", !early && got == Frame(3, "hello")); }
    { FrameReader r; r.feed(encodeFrame(1, "x") + encodeFrame(2, "yy"));
      check("two frames: first", r.pop() == Frame(1, "x"));
      check("two frames: second", r.pop() == Frame(2, "yy")); }
    { FrameReader r; r.feed(std::string("\1\0\0\0\1\0\x10\0", 8));
      check("1048577 bytes is broken", !r.pop() && r.broken()); }
    { FrameReader r; r.feed(std::string("\1\0\0\0\0\0\x10\0", 8));
      check("1048576 bytes is fine", !r.pop() && !r.broken()); }
}

static void paths() {
    auto p = socketPaths("/run/user/1000");
    check("20 socket paths", p.size() == 20);
    check("first socket path", !p.empty() && p.front() == "/run/user/1000/discord-ipc-0");
    check("flatpak socket path", p.size() == 20 && p.back() == "/run/user/1000/app/com.discordapp.Discord/discord-ipc-9");
    check("too long gives none", socketPaths("/" + std::string(119, 'a')).empty());
    check("Wine's name for the runtime dir wins", runtimeDir("/run/user/1", "/other") == "/run/user/1");
    check("plain runtime dir is the fallback", runtimeDir(nullptr, "/other") == "/other");
    check("empty Wine value uses the fallback", runtimeDir("", "/other") == "/other");
    check("no runtime dir at all", runtimeDir(nullptr, nullptr).empty());
    check("no runtime dir gives none", socketPaths("").empty());
}

static void authStep() {
    using enum AuthStep;
    check("saved login is used", nextAuthStep(true, true, false, false) == UseToken);
    check("rejected login is refreshed", nextAuthStep(true, true, true, false) == Refresh);
    check("failed refresh needs setup", nextAuthStep(true, true, true, true) == NeedSetup);
    check("rejected login without refresh needs setup", nextAuthStep(true, false, true, false) == NeedSetup);
    check("refresh token alone is used", nextAuthStep(false, true, false, false) == Refresh);
    check("nothing saved needs setup", nextAuthStep(false, false, false, false) == NeedSetup);
}

static void levels() {
    check("main level key", levelKey(LevelKind::Main, 1, "Stereo", 0) == "level-main-1");
    check("online level key", levelKey(LevelKind::Online, 42, "x", 0) == "level-online-42");
    check("daily level key", levelKey(LevelKind::Daily, 42, "x", 0) == "level-daily-42");
    check("gauntlet level key", levelKey(LevelKind::Gauntlet, 42, "x", 0) == "level-gauntlet-42");
    check("created level key ignores the id", levelKey(LevelKind::Created, 7, "My Level", 2) == levelKey(LevelKind::Created, 8, "My Level", 2));
    check("created levels differ by name", levelKey(LevelKind::Created, 0, "a", 0) != levelKey(LevelKind::Created, 0, "b", 0));
    check("created levels differ by revision", levelKey(LevelKind::Created, 0, "a", 0) != levelKey(LevelKind::Created, 0, "a", 1));
}

static void levelEdits() {
    auto none = std::nullopt;
    check("untouched popup saves nothing", !levelEdit(ON, true, none, none));
    auto off = levelEdit(ON, false, none, none);
    check("toggle is saved and keeps the range", off && !off->enabled && off->start == 50.f && off->end == 100.f);
    auto start = levelEdit(ON, true, 75.f, none);
    check("start is saved", start && start->enabled && start->start == 75.f && start->end == 100.f);
    auto end = levelEdit(ON, true, none, 90.f);
    check("end is saved", end && end->start == 50.f && end->end == 90.f);
    auto wild = levelEdit(ON, true, -5.f, 250.f);
    check("range is clamped", wild && wild->start == 0.f && wild->end == 100.f);
    auto swapped = levelEdit(ON, true, 90.f, 30.f);
    check("backwards range is swapped", swapped && swapped->start == 30.f && swapped->end == 90.f);
}

int main() {
    levels();
    levelEdits();
    runRange();
    range();
    authStep();
    framing();
    paths();
    runTracker();
    deafenIntent();
    std::printf("%d failed\n", g_fails);
    return g_fails != 0;
}
