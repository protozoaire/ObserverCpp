#ifndef OBSERVER_HPP
  #define OBSERVER_HPP 0
  #define OBSERVER_CTX
////////////////////////////


#include <cstddef>
#include <type_traits>
#include <memory>
#include <unordered_set>
#include <unordered_map>


//  An Implementation of the Observer Pattern;
//
//  USAGE:
//  
//  - One creates an events's set, e.g.
//      
//      struct Event0 { ... };
//      struct Event1 { ... };
//      ...
//
//      using events_t = SubjectEvents<Event0,Event1,...>;


namespace Observer {


template <typename ... Events>
struct SubjectEvents;

template <typename E, typename _SubjectEvents>
struct Event;

template <typename _SubjectEvents>
struct SubjectID;
//can be queried from a Subject;



//
//
//


template <typename E, typename ... List>
struct _Count;

template <typename E, typename H, typename ... Rs>
struct _Count<E,H,Rs...>
{
    static constexpr size_t value = _Count<E,Rs...>::value;
};

template <typename E, typename ... Rs>
struct _Count<E,E,Rs...>
{
    static constexpr size_t value = 1+_Count<E,Rs...>::value;
};

template <typename E>
struct _Count<E>
{
    static constexpr size_t value = 0;
};

template <typename ... >
struct _EachOnce
{
    static constexpr bool value = true;
};

template <typename H, typename ... Rs>
struct _EachOnce<H,Rs...>
{
    static constexpr bool value = (_Count<H,Rs...>::value == 0) && _EachOnce<Rs...>::value;
};

template <>
struct _EachOnce<>
{
    static constexpr bool value = true;
};

template <template <typename> class Pred, typename ... >
struct _ForEach;

template <template <typename> class Pred, typename H, typename ... Rs>
struct _ForEach<Pred,H,Rs...>
{
    static constexpr bool value = Pred<H>::value && _ForEach<Pred,Rs...>::value;
};

template <template <typename> class Pred>
struct _ForEach<Pred>
{
    static constexpr bool value = true;
};

template <typename T>
struct _IsDefined
{
    template <typename _T = T, size_t = sizeof(_T)>
    static std::true_type test(int);

    static std::false_type test(...);

    static constexpr bool value = decltype(test(0))::value;
};

template <typename ... Events>
struct SubjectEvents
{
    
    static constexpr bool AssertNoDuplicate() {
        //each event must appear once in the list;
        return _EachOnce<Events...>::value;
    }

    static constexpr bool AssertAllDefined() {
        //each event must be defined;
        return _ForEach<_IsDefined,Events...>::value;
    }

    SubjectEvents() {
        static_assert(AssertNoDuplicate(),
            "ERROR SubjectEvents<...>: any event can appear only once");
        static_assert(AssertAllDefined(),
            "ERROR SubjectEvents<...>: all events must be defined (not just declared)");
    }

};


//
//
//


template <typename E, typename _SubjectEvents>
struct IsSupportedEvent;

template <typename E, typename ... Events>
struct IsSupportedEvent< E, SubjectEvents<Events...> >
{
    static constexpr bool value = (_Count<E,Events...>::value > 0);
    //we need not optimize that because the template instance already exists anyway;
};

template <typename E, typename NotSubjectEvents>
struct IsSupportedEvent
{
    struct Wrong;
    static_assert(std::is_same<NotSubjectEvents,Wrong>::value,
        "USAGE: Observer::IsSupportedEvent<.,SubjectEvents<...>>");
};


//
//
//


template <typename E, typename ... Events>
struct Event< E, SubjectEvents<Events...> >
{
    static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
        "ERROR Observer::Event<E,SubjectEvents<...>>: 'E' is not in the SubjectEvents list");
};
template <typename E, typename NotSubjectEvents>
struct Event
{
    struct Wrong;
    static_assert(std::is_same<NotSubjectEvents,Wrong>::value,
        "USAGE: Observer::Event<.,SubjectEvents<...>>");
};


//
//
//


template <typename Event, typename _SubjectEvents>
struct EventIndex;

template <typename Event, typename hEvent, typename ... rEvents>
struct EventIndex< Event, SubjectEvents<hEvent,rEvents...> >
{ static constexpr size_t value = 1+EventIndex< Event,SubjectEvents<rEvents...> >::value; };

template <typename Event, typename ... rEvents>
struct EventIndex< Event, SubjectEvents<Event,rEvents...> >
{ static constexpr size_t value = 0; };


//
//
//


template <typename T>
struct StaticObject
{
    static constexpr bool AssertNoCopy() {
        return !std::is_copy_constructible<T>::value && !std::is_copy_assignable<T>::value;
    }
    
    static constexpr bool AssertNoMove() {
        return !std::is_move_constructible<T>::value && !std::is_move_assignable<T>::value;
    }
};


//
//
//


template
<
    typename _SubjectEvents,
    typename event_idx_t = size_t,
    template <typename> class observers_T = std::unordered_set,
    template <typename,typename> class event2observers_T = std::unordered_map,
    template <typename,typename> class observer2count_T = std::unordered_map
>
struct _Subject;

template <typename _SubjectEvents>
struct _Observer;

template <typename ... Events, typename event_idx_t, template <typename> class observers_T,
         template <typename,typename> class event2observers_T, 
         template <typename,typename> class observer2count_T>
struct _Subject
< 
    SubjectEvents<Events...>,
    event_idx_t,
    observers_T,
    event2observers_T,
    observer2count_T
>
{
    
    _Subject(_Subject const&) = delete;
    _Subject& operator=(_Subject const&) = delete;

    _Subject(_Subject&&) = delete;
    _Subject& operator=(_Subject&&) = delete;
   
    private:

        using events_t = SubjectEvents<Events...>;

        using pObs_t = _Observer<events_t>*;
      
        using observers_t = observers_T<pObs_t>;
        using event2observers_t = event2observers_T<event_idx_t,observers_t>;
        event2observers_t event2observers;

        bool event_add_observer(event_idx_t eventIdx_, pObs_t pObs_)
        {
            auto const it = event2observers.find(eventIdx_);
            if(it == event2observers.cend()) {
                event2observers.insert(std::make_pair(eventIdx_,observers_t{ pObs_ }));
                return true;
            }
            else {
                auto const jt = it->second.find(pObs_);
                if(jt == it->second.cend()) {
                    it->second.insert(pObs_);
                    return true;
                }
                else {
                    //already subscribed
                    return false;
                }
            }
        }

        bool event_del_observer(event_idx_t eventIdx_, pObs_t pObs_)
        {
            auto const it = event2observers.find(eventIdx_);
            if(it == event2observers.cend()) {
                //not subscribed by anybody
                return false;
            }
            else {
                auto const jt = it->second.find(pObs_);
                if(jt == it->second.cend()) {
                    //not subscribed
                    return false;
                }
                else {
                    it->second.erase(jt);
                    return true;
                }
            }
        }

        using observer2count_t = observer2count_T<pObs_t,size_t>;
        observer2count_t observer2count;
        
        void incr_observer_count(pObs_t pObs_, size_t n)
        {
            auto it = observer2count.find(pObs_);
            if(it == observer2count.end()) { 
                observer2count.insert(std::make_pair(pObs_,n));
            }
            else it->second += n;
        }
 
        void decr_observer_count(pObs_t pObs_, size_t n)
        {
            auto it = observer2count.find(pObs_);
            if(it == observer2count.end()) { /* nothing to do */ }
            else {
                if(it->second <= n) {
                    /* clean up observer */
                    observer2count.erase(it); 
                }
                else it->second -= n;
            }
        }
       
        size_t read_observer_count(pObs_t pObs_) const
        {
            auto const it = observer2count.find(pObs_);
            if(it==observer2count.cend()) return 0;
            else return it->second;
        }

        //

        void _Attach(pObs_t pObs_, event_idx_t eventIdx_)
        {
            if(event_add_observer(eventIdx_,pObs_)) incr_observer_count(pObs_,1);
            else { /* already subscribed -> do nothing */ }
        }

        void _Detach(pObs_t pObs_, event_idx_t eventIdx_)
        {
            if(event_del_observer(eventIdx_,pObs_)) decr_observer_count(pObs_,1);
            else { /* not subscribed -> do nothing */ }
        }

    public:

    _Subject() = default;

    bool Attach(pObs_t pObs_, event_idx_t eventIdx_)
    {
        if(eventIdx_ >= sizeof...(Events)) return false;
        _Attach(pObs_,eventIdx_); 
        return true;
    }

    bool Detach(pObs_t pObs_, event_idx_t eventIdx_)
    {
        if(eventIdx_ >= sizeof...(Events)) return false;
        _Detach(pObs_,eventIdx_);
        return true;
    }

    bool Attach(pObs_t pObs_, std::initializer_list<event_idx_t> eventIdxList_)
    {
        for(auto const eventIdx_ : eventIdxList_) { if(eventIdx_ >= sizeof...(Events)) return false; }
        for(auto const eventIdx_ : eventIdxList_) { _Attach(pObs_,eventIdx_); }
        return true;
    }

    bool Detach(pObs_t pObs_, std::initializer_list<event_idx_t> eventIdxList_)
    {
        for(auto const eventIdx_ : eventIdxList_) { if(eventIdx_ >= sizeof...(Events)) return false; }
        for(auto const eventIdx_ : eventIdxList_) { _Detach(pObs_,eventIdx_); }
        return true;
    }

    size_t nEvents(pObs_t pObs_) const
    {
        return read_observer_count(pObs_);
    }

};


template <typename _SubjectEvents>
struct _Observer
{

    _Observer(_Observer const&) = delete;
    _Observer& operator=(_Observer const&) = delete;

    _Observer(_Observer&&) = delete;
    _Observer& operator=(_Observer&&) = delete;
 
    
    private:

        using events_t = SubjectEvents<Events...>;

    
    public:

    _Observer() = default;

};


}//close Observer


////////////////////////////
#undef OBSERVER_CTX
#undef OBSERVER_HPP
  #define OBSERVER_HPP 1
#else
  #if OBSERVER_HPP == 0
  #error incomplete include of "observer.hpp"
  #endif
#endif
