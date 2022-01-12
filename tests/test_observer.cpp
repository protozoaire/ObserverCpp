#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../observer.hpp"


TEST_CASE("Testing _Count") {
    
    //_Count<typename E, typename ... List>
    struct E;
    CHECK(Observer::_Count<E>::value == 0);
    CHECK(Observer::_Count<E,E>::value == 1);
    CHECK(Observer::_Count<E,E,E>::value == 2);
    struct A;
    CHECK(Observer::_Count<E,A>::value == 0);
    CHECK(Observer::_Count<E,A,E>::value == 1);
    CHECK(Observer::_Count<E,E,A>::value == 1);
    CHECK(Observer::_Count<E,A,E,E>::value == 2);
    CHECK(Observer::_Count<E,A,E,A>::value == 1);
    CHECK(Observer::_Count<E,A,E,A,E>::value == 2);

}


TEST_CASE("Testing _EachOnce") {
    
    //_EachOnce<typename ...>
    CHECK(Observer::_EachOnce<>::value == true);
    struct A;
    CHECK(Observer::_EachOnce<A>::value == true);
    CHECK(Observer::_EachOnce<A,A>::value == false);
    CHECK(Observer::_EachOnce<A,A,A>::value == false);
    struct B;
    CHECK(Observer::_EachOnce<A,B>::value == true);
    CHECK(Observer::_EachOnce<A,B,A>::value == false);

}


TEST_CASE("Testing _ForEach") {

    //_ForEach<template <typename> class Pred, typename ... >
    CHECK(Observer::_ForEach<std::is_pointer>::value == true);
    struct A;
    CHECK(Observer::_ForEach<std::is_pointer,A>::value == false);
    CHECK(Observer::_ForEach<std::is_pointer,A*>::value == true);
    CHECK(Observer::_ForEach<std::is_pointer,A*,A>::value == false);
    CHECK(Observer::_ForEach<std::is_pointer,A*,A*>::value == true);

}


TEST_CASE("Testing _IsDefined") {

    //_IsDefined<typename T>
    struct A;
    CHECK(Observer::_IsDefined<A>::value == false);
    struct B {};
    CHECK(Observer::_IsDefined<B>::value == true);

}


TEST_CASE("Testing SubjectEvents") {

    //SubjectEvents<typename ... Events>
    
    struct A;
    CHECK(Observer::SubjectEvents<A,A>::AssertNoDuplicate() == false);
    struct B;
    CHECK(Observer::SubjectEvents<A,B>::AssertNoDuplicate() == true);
   
    CHECK(Observer::SubjectEvents<A>::AssertAllDefined() == false);
    struct C {};
    CHECK(Observer::SubjectEvents<C>::AssertAllDefined() == true);
    CHECK(Observer::SubjectEvents<C,A>::AssertAllDefined() == false);
    CHECK(Observer::SubjectEvents<C,C>::AssertAllDefined() == true);

}


TEST_CASE("Testing IsSupportedEvent") {

    //IsSupportedEvent<typename E, typename _SubjectEvents>
    struct A {};
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>; 
    struct C {};

    CHECK(Observer::IsSupportedEvent<A,events_t>::value == true);
    CHECK(Observer::IsSupportedEvent<B,events_t>::value == true);
    CHECK(Observer::IsSupportedEvent<C,events_t>::value == false);

}


TEST_CASE("Testing EventIndex") {

    //EventIndex<typename E, typename _SubjectEvents>
    struct A {};
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>; 
    
    CHECK(Observer::EventIndex<A,events_t>::value == 0);
    CHECK(Observer::EventIndex<B,events_t>::value == 1);

}


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


TEST_CASE("Testing _Subject / Create") {

    //_Subject<typename _SubjectEvents>

    struct A {};
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>;

    //static object
    CHECK(StaticObject<Observer::_Subject<events_t>>::AssertNoCopy());
    CHECK(StaticObject<Observer::_Subject<events_t>>::AssertNoMove());

}


TEST_CASE("Testing _Subject / Subscribe") {

    struct A {};
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>;
    
    using subject_t = Observer::_Subject<events_t>;
    using observer_t = Observer::_Observer<events_t>;
    
    subject_t sub;
    observer_t obs;
    //good event index succeeds:
    CHECK(sub.Attach(&obs,0) == true);
    CHECK(sub.Attach(&obs,1) == true);
    //bad event index fails
    CHECK(sub.Attach(&obs,2) == false);
    //know current registration of observer
    CHECK(sub.nEvents(&obs)==2);
    //already event does nothing;
    CHECK(sub.Attach(&obs,0) == true);
    CHECK(sub.nEvents(&obs) == 2);

    //multiple subscription
    observer_t obs2;
    //any bad event -> global fail
    CHECK(sub.Attach(&obs2,{0,2}) == false);
    CHECK(sub.nEvents(&obs2) == 0);
    //all good -> succeed
    CHECK(sub.Attach(&obs2,{1}) == true);
    CHECK(sub.nEvents(&obs2) == 1);
    //all good / redundant -> succeed
    CHECK(sub.Attach(&obs2,{1,1}) == true);
    CHECK(sub.nEvents(&obs2) == 1);

}


TEST_CASE("Testing _Subject / Unsubscribe") {

    struct A {};
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>;
    
    using subject_t = Observer::_Subject<events_t>;
    using observer_t = Observer::_Observer<events_t>;

    subject_t sub;
    observer_t obs;
    CHECK(sub.Attach(&obs,0) == true);
    CHECK(sub.nEvents(&obs) == 1);
    //bad event fails (count not changed) 
    CHECK(sub.Detach(&obs,2) == false);
    CHECK(sub.nEvents(&obs) == 1);
    //unsubscribed event succeeds (count not changed) 
    CHECK(sub.Detach(&obs,1) == true);
    CHECK(sub.nEvents(&obs) == 1);
    //subscribed event succeeds (count changed)
    CHECK(sub.Detach(&obs,0) == true);
    CHECK(sub.nEvents(&obs) == 0);
    //unregistered observer succeeds
    observer_t obs2;
    CHECK(sub.Detach(&obs2,0) == true);
    CHECK(sub.nEvents(&obs2) == 0);

    //multiple unsubscribe
    observer_t obs3;
    CHECK(sub.Attach(&obs3,{0,1}) == true);
    //any bad event -> global fail (count not changed)
    CHECK(sub.Detach(&obs3,{1,2}) == false);
    CHECK(sub.nEvents(&obs3) == 2);
    //all good -> succeed
    CHECK(sub.Detach(&obs3,{0}) == true);
    CHECK(sub.nEvents(&obs3) == 1);
    //all good / redundant -> succeed
    CHECK(sub.Detach(&obs3,{1,1}) == true);
    CHECK(sub.nEvents(&obs3) == 0);

}


TEST_CASE("Testing _Observer / Create") {

    //_Observer<typename _SubjectEvents>

    struct A {};
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>;

    //static object
    CHECK(StaticObject<Observer::_Observer<events_t>>::AssertNoCopy());
    CHECK(StaticObject<Observer::_Observer<events_t>>::AssertNoMove());

}


template <typename E, typename _SubjectEvents>
struct onEventExists
{
    using O = Observer::_Observer<_SubjectEvents>;
    
    template <typename T = E, void (O::*)(T) = &O::template onEvent<T>>
    static std::true_type test(int);

    static std::false_type test(...);

    static constexpr bool value = decltype(test(0))::value;
};


TEST_CASE("Testing _Observer / onEvent methods") {

    struct A {};
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>;
    struct C {};

    //methods exist
    CHECK(onEventExists<A,events_t>::value == true);
    CHECK(onEventExists<B,events_t>::value == true);
    CHECK(onEventExists<C,events_t>::value == false);

}


template <typename E, typename _SubjectEvents>
struct bindHandlerExists
{
    using O = Observer::_Observer<_SubjectEvents>;
    
    template <typename T = E, void (O::*)(std::function<void(T)>*) = &O::template bindHandler<T>>
    static std::true_type test(int);

    static std::false_type test(...);

    static constexpr bool value = decltype(test(0))::value;
};


TEST_CASE("Testing _Observer / handlers") {

    struct A { size_t value; };
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>;
    struct C {};

    //binding methods exist
    CHECK(bindHandlerExists<A,events_t>::value == true);
    CHECK(bindHandlerExists<B,events_t>::value == true);
    CHECK(bindHandlerExists<C,events_t>::value == false);

    using observer_t = Observer::_Observer<events_t>;
    
    //handler for event A
    size_t accA = 0;
    std::function<void(A)> handlerA = [&accA](A a){ accA += a.value; };
    observer_t obs;
    obs.bindHandler(&handlerA);
    CHECK(accA == 0);
    obs.onEvent(A{2});
    CHECK(accA == 2);
    obs.onEvent(A{3});
    CHECK(accA == 5);

    //redefine handlerA
    handlerA = [&accA](A a){ accA -= a.value; };
    obs.onEvent(A{2});
    CHECK(accA == 3);

    //add handler for B
    size_t nB = 0;
    std::function<void(B)> handlerB = [&nB](B){ ++nB; };
    obs.bindHandler(&handlerB);
    CHECK(nB == 0);
    obs.onEvent(B());
    CHECK(nB == 1);

    //handler for A still works
    obs.onEvent(A{2});
    CHECK(accA == 1);

    //handler and virtual calls
    struct H { virtual void event(A) {} };
    struct h : H {
        size_t value = 0;
        virtual void event(A a) override { value += a.value; }
    };
    auto pObjH = std::unique_ptr<H>(new h());
    handlerA = [&pObjH](A a){ pObjH->event(a); };
    obs.onEvent(A{2});
    CHECK(static_cast<h*>(pObjH.get())->value == 2);
    
}
