#pragma once
#include "shared/model/PhoneState.h"
namespace cluster {
enum class PhoneItem{Approve,Reject,Pair,Disconnect,Forget,ConfirmForget,Cancel,Back};
struct PhoneMenu{std::array<PhoneItem,3> items{{PhoneItem::Back,PhoneItem::Back,PhoneItem::Back}};unsigned count=1;};
inline PhoneMenu phoneMenu(const PhoneState& state,bool confirm){
    if(!state.available()||!valid(state))return {};
    if(state.forgetting)return {};
    if(state.pairingRequested&&!state.pairingApproved)return {{{PhoneItem::Reject,PhoneItem::Approve,PhoneItem::Back}},2};
    if(state.pairingRequested)return {{{PhoneItem::Reject,PhoneItem::Back,PhoneItem::Back}},2};
    if(confirm&&state.paired)return {{{PhoneItem::Cancel,PhoneItem::ConfirmForget,PhoneItem::Back}},2};
    if(state.connected)return {{{PhoneItem::Disconnect,PhoneItem::Forget,PhoneItem::Back}},3};
    if(state.paired)return {{{PhoneItem::Forget,PhoneItem::Back,PhoneItem::Back}},2};
    return {{{PhoneItem::Pair,PhoneItem::Back,PhoneItem::Back}},2};
}
inline const char* phoneItemTitle(PhoneItem item){switch(item){
    case PhoneItem::Approve:return "Allow pairing";
    case PhoneItem::Reject:return "Reject pairing";
    case PhoneItem::Pair:return "Pair phone";
    case PhoneItem::Disconnect:return "Disconnect";
    case PhoneItem::Forget:case PhoneItem::ConfirmForget:return "Forget phone";
    case PhoneItem::Cancel:return "Cancel";
    case PhoneItem::Back:return "Back";
}return "Back";}
}
