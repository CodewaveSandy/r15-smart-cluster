#pragma once
#include "lvgl.h"
#include <array>
#include <cstdint>
extern "C" { LV_FONT_DECLARE(cluster_icons_32); LV_FONT_DECLARE(cluster_icons_30); }
namespace cluster::ui {
enum class Icon { Left, Right, HighBeam, TractionControl, Abs, Engine, Coolant, Message, Call, Hazard, Phone, Fuel, Appearance, Bluetooth, Wifi, Navigation };
struct IconDefinition { Icon kind; std::uint32_t codepoint; const char* glyph; };
inline constexpr std::array<IconDefinition,16> iconDefinitions{{
    {Icon::Left,0xF0731,u8"\U000F0731"},
    {Icon::Right,0xF0734,u8"\U000F0734"},
    {Icon::HighBeam,0xF0C4C,u8"\U000F0C4C"},
    {Icon::TractionControl,0xF0D64,u8"\U000F0D64"},
    {Icon::Abs,0xF0C47,u8"\U000F0C47"},
    {Icon::Engine,0xF01FA,u8"\U000F01FA"},
    {Icon::Coolant,0xF03C8,u8"\U000F03C8"},
    {Icon::Message,0xF0369,u8"\U000F0369"},
    {Icon::Call,0xF03F2,u8"\U000F03F2"},
    {Icon::Hazard,0xF0C89,u8"\U000F0C89"},
    {Icon::Phone,0xF011C,u8"\U000F011C"},
    {Icon::Fuel,0xF0298,u8"\U000F0298"},
    {Icon::Appearance,0xF03D8,u8"\U000F03D8"},
    {Icon::Bluetooth,0xF00AF,u8"\U000F00AF"},
    {Icon::Wifi,0xF05A9,u8"\U000F05A9"},
    {Icon::Navigation,0xF07D9,u8"\U000F07D9"}
}};
inline const lv_font_t* iconFont(Icon kind) { return kind==Icon::Abs?&cluster_icons_30:&cluster_icons_32; }
// Persistent 32px glyph widget; recolor/opacity without rebuilding or decoding assets.
lv_obj_t* createIcon(lv_obj_t* parent,int x,int y,Icon kind);
}
