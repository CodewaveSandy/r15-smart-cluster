#pragma once
#include "shared/application/MapStore.h"
#include "shared/application/NavigationStore.h"
#include "esp_heap_caps.h"
#include <new>
namespace cluster::board {
// Lifetime is the application lifetime. Failure leaves all ordinary instruments
// usable and map capability unavailable; never fall back to internal SRAM.
inline void reserveMaps(maplink::MapStore& store,std::uint32_t boot){
    void* allocations[4]{};const unsigned sizes[]={maplink::maxPackage,maplink::maxPackage,sizeof(maps::Scene),sizeof(maps::Scene)};
    bool ok=true;for(unsigned i=0;i<4;++i){allocations[i]=heap_caps_malloc(sizes[i],MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);if(!allocations[i]){ok=false;break;}}
    if(!ok){for(auto* p:allocations)heap_caps_free(p);store.initialize(boot,nullptr,nullptr,nullptr,nullptr);return;}
    auto* a=new(allocations[2]) maps::Scene;auto* b=new(allocations[3]) maps::Scene;
    store.initialize(boot,static_cast<std::uint8_t*>(allocations[0]),static_cast<std::uint8_t*>(allocations[1]),a,b);
}
inline void reserveNavigation(navlink::NavigationStore& nav,maplink::MapStore& maps,std::uint32_t boot){
    constexpr unsigned total=(navlink::cacheSlots+1)*navlink::maxObject+2*maplink::maxPackage+2*sizeof(maps::Scene);
    static_assert(total<=1024*1024,"Map/route arena must remain within the advertised 1 MiB ceiling");
    auto* raw=maps.ready()?static_cast<std::uint8_t*>(heap_caps_malloc((navlink::cacheSlots+1)*navlink::maxObject,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT)):nullptr;
    nav.initialize(boot,raw,maps.renderStorage(0),maps.renderStorage(1));
}
}
