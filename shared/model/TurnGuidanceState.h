#pragma once
#include <array>
#include <cstdint>
namespace cluster::guidance {
constexpr std::uint32_t unknown=0xffffffffu;
constexpr std::uint64_t unknownTime=0xffffffffffffffffull;
enum class Source:std::uint8_t {Waiting,Guiding,Rerouting,Arrived,Unsupported,GpsLost,Paused};
enum class Maneuver:std::uint8_t {Continue,Left,Right,Straight,Arrive,KeepLeft,KeepRight,Uturn,Roundabout,Unknown=255};
enum class Quality:std::uint8_t {Inactive,Waiting,Fresh,LastUpdate,Pending,UnknownAge,Unsupported,LinkLost,Cleared};
enum class Reason:std::uint8_t {None,Stop,SourceAbsent,PermissionRevoked,Unsupported,LinkLost,ListenerLost};
enum class Error:std::uint8_t {None,Invalid,Context,Sequence,Closed,Clock,Revision,Busy,Rate};
enum Flag:std::uint16_t {Instruction=1,Road=2,Summary=4,Direction=8,NextDistance=16,RemainingDistance=32,RemainingTime=64,Exit=128,Truncated=256,AgeKnown=512};
struct Snapshot {
    Source source=Source::Waiting;Maneuver maneuver=Maneuver::Unknown;std::uint16_t flags=0;
    std::uint32_t next=unknown,remaining=unknown,seconds=unknown;std::uint8_t exit=0;
    std::uint64_t changed=unknownTime,dispatch=0;
    std::array<char,97> instruction{};std::array<char,33> road{};std::array<char,17> summary{};
};
struct TurnGuidanceState {
    Snapshot content;Quality quality=Quality::Inactive;Reason reason=Reason::None;
    std::uint32_t revision=0,ageMs=unknown;
    bool visible=false,action=false,clock=false,transport=false,ageKnown=false;
};
}
