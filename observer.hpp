#ifndef OBSERVER_HPP
  #define OBSERVER_HPP 0
  #define OBSERVER_CTX
////////////////////////////


#include <cstddef>
#include <type_traits>
#include <memory>


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
        "ERROR: Observer::Event<E,SubjectEvents<...>>: 'E' is not in the SubjectEvents list");
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
