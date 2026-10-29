#pragma once
#include <array>
namespace wiring {
struct Input { int pin; bool activeHigh; };
// User-confirmed 7SEMI header 17 -> display GPIO_D IO38, 3.3 V UART.
inline constexpr int displayTx=17,displayRx=18;
// ESP GPIO numbers, not optocoupler IN/OUT channel numbers. -1 disables a signal.
inline constexpr Input left{4,true},right{5,true};
inline constexpr Input highBeam{7,true};
inline constexpr Input tripButton{14,true},infoButton{15,true};
// N, 1, 2, 3, 4, 5, 6. All enabled so opto channels can move without reflashing.
inline constexpr std::array<Input,7> gears{{{6,true},{8,true},{9,true},
    {10,true},{11,true},{12,true},{13,true}}};
constexpr bool valid() {
    std::array<Input,12> all{{tripButton,infoButton,left,right,highBeam,gears[0],gears[1],gears[2],gears[3],gears[4],gears[5],gears[6]}};
    for(unsigned i=0;i<all.size();++i) {
        if(all[i].pin==displayTx||all[i].pin==displayRx)return false;
        // Restrict this bench profile to proposed, non-reserved GPIO4..18.
        if(all[i].pin!=-1&&(all[i].pin<4||all[i].pin>18))return false;
        for(unsigned j=0;j<i;++j)if(all[i].pin>=0&&all[i].pin==all[j].pin)return false;
    }
    return true;
}
static_assert(valid(),"Duplicate or unsupported bench input GPIO");
}
