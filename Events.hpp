#pragma once
#include <cstdint>
#include <functional>
#include <shared_mutex>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>
#include <algorithm>

namespace inventorysorter::events {
enum class Type : std::uint8_t { Frame, ScreenState };
enum class ScreenKind : std::uint8_t { Container };
enum class ScreenPhase : std::uint8_t { Opened, Closed };
struct FrameEvent { static constexpr Type type = Type::Frame; };
struct ScreenStateEvent { static constexpr Type type = Type::ScreenState; ScreenKind screen; ScreenPhase phase; void* controller; };
class Bus {
public:
 using Subscription=std::uint64_t; using Callback=std::function<void(void*)>;
 Subscription subscribeRaw(Type type, Callback cb) { if(!cb) return 0; std::unique_lock lock(mutex); auto id=next++; auto& v=listeners[type]; v.push_back({id,std::move(cb)}); return id; }
 template<class E, class F> Subscription subscribe(F&& f){ return subscribeRaw(E::type,[fn=std::forward<F>(f)](void* p) mutable { fn(*static_cast<E*>(p)); }); }
 template<class E> void publish(E& e){ publishRaw(E::type,&e); }
 void publishRaw(Type type,void* payload){ std::vector<Entry> s; { std::shared_lock lock(mutex); auto it=listeners.find(type); if(it==listeners.end()) return; s=it->second; } for(auto& e:s)e.cb(payload); }
private:
 struct Entry{Subscription id; Callback cb;}; struct Hash{size_t operator()(Type t)const noexcept{return static_cast<size_t>(t);}}; std::shared_mutex mutex; std::unordered_map<Type,std::vector<Entry>,Hash> listeners; Subscription next=1;
};
Bus& bus();
}
