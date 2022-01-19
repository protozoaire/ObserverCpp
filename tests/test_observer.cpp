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


TEST_CASE("Testing AbstractSet") {

    //AbstractSet<typename T>
    std::unordered_set<int> set;
    Observer::AbstractSet<int> Set;
    
    //Append
    auto _append = [&set](int i){ return set.insert(i).second; };
    Set.Append = _append;
    CHECK(Set.Append(3)==true);
    CHECK(set.count(3)==1);
    CHECK(set.size()==1);
    CHECK(Set.Append(3)==false);//already in

    //Remove
    set.insert(1);
    auto _remove = [&set](int i){ return set.erase(i)==1; };
    CHECK(set.count(1)==1);
    Set.Remove = _remove;
    CHECK(Set.Remove(1)==true);
    CHECK(set.count(1)==0);
    CHECK(Set.Remove(2)==false);//never added

    //Signal
    using F = std::function<void(int)>;
    auto _signal = [&set](F f){ for(auto t : set) f(t); };
    Set.Signal = _signal;
    int n = 0;
    auto f = [&n](int i){ n+=i; };
    Set.Signal(f);
    CHECK(n==3);
    CHECK(Set.Append(1)==true);
    Set.Signal(f);
    CHECK(n==7);
    CHECK(Set.Remove(1)==true);

    //constructing with abstract_set_view
    Set = Observer::abstract_set_view(set);
    CHECK(set.size()==1);//3 is still in
    CHECK(Set.Remove(2)==false);//not there
    CHECK(Set.Remove(3)==true);
    CHECK(Set.Append(1)==true);
    CHECK(set.count(1)>0);
    n = 2;
    auto g = [&n](int i){ n+=2*i; };
    Set.Signal(g);
    CHECK(n==4);

}


struct ASVs
{
    template <typename T>
    Observer::AbstractSet<T> operator()(std::unordered_set<T>&)
    {
        return {};
    }
};


TEST_CASE("Testing AbstractSetTuple") {

    //AbstractSetTuple<typename ... Ts>
    std::unordered_set<int> set_int;
    std::unordered_set<char> set_char;

    //ctor from already Abstract sets
    auto Set_int = Observer::abstract_set_view(set_int);
    auto Set_char = Observer::abstract_set_view(set_char);
    Observer::AbstractSetTuple<int,char> TSet(Set_int,Set_char);

    //ction using abstract_set_tuple_view
    //1. supported impls
    TSet = Observer::abstract_set_tuple_view(set_int,set_char);
    //2. tuple of supported impls
    std::tuple<std::unordered_set<int>, std::unordered_set<char>> tset;
    TSet = Observer::abstract_set_tuple_view(tset);
    //3. custom impls
    TSet = Observer::abstract_set_tuple_view<ASVs>(set_int,set_char);
    //4. tuple of custom impls
    TSet = Observer::abstract_set_tuple_view<ASVs>(tset);

    //Ops
    TSet.Set(Set_int).Set(Set_char);//chaining Set(.)
    auto Set_int2 = TSet.Get(int());//Get
    
}


TEST_CASE("Testing AbstractMap") {

    //AbstractMap<typename K, typename V>
    std::unordered_map<char,int> map;
    Observer::AbstractMap<char,int> Map;
    
    //Define
    auto _define = [&map](char c, int i) { map.insert_or_assign(c,i); };
    Map.Define = _define;
    Map.Define('a',1);
    CHECK(map.count('a')>0);
    CHECK(map.size()==1);
    Map.Define('a',2);
    CHECK(map.size()==1);
    CHECK(map['a']==2);
    
    //Remove
    map.insert_or_assign('b',3);
    auto _remove = [&map](char i) { return map.erase(i)==1; };
    CHECK(map.count('b')==1);
    Map.Remove = _remove;
    CHECK(Map.Remove('b')==true);
    CHECK(map.count('b')==0);
    CHECK(Map.Remove('b')==false);//already removed
    CHECK(Map.Remove('c')==false);//never defined

    //Signal
    using F = std::function<void(char,int)>;
    auto _signal = [&map](F f){ for(auto [k,v] : map) f(k,v); }; 
    Map.Signal = _signal;
    int n = 0;
    auto f = [&n](char, int i){ n += i; };
    Map.Signal(f);
    CHECK(map.size()==1);
    CHECK(map.count('a')>0);
    CHECK(n==2);
    Map.Define('b',5);
    Map.Signal(f);
    CHECK(n==9);

    //Lookup
    auto _lookup = [&map](char c){ return map.at(c); };
    Map.Lookup = _lookup;
    CHECK(Map.Lookup('a')==2);
    CHECK(Map.Lookup('b')==5);
    CHECK_THROWS(Map.Lookup('c'));//Never defined; map.at throws;

    //constructing from abstract_map_view
    Map = Observer::abstract_map_view(map);
    CHECK(map.size()==2);
    CHECK(Map.Remove('c')==false);//not there
    CHECK(Map.Remove('b')==true);
    CHECK(map.count('a')>0);//still here, with value 2
    Map.Define('c',3);//new entry
    n = 1;
    auto g = [&n](char, int i){ n+=2*i; };
    Map.Signal(g);
    CHECK(n==(1+2*2+2*3));

}


TEST_CASE("Testing AbstractConstMap") {

    //AbstractConstMap<typename K, typename V>
    std::unordered_map<char,int> map;
    Observer::AbstractConstMap<char,int> Map;
    map['a'] = 0;
    map['b'] = 1;

    //Lookup
    auto _lookup = [&map](char c){ return map.at(c); };
    Map.Lookup = _lookup;
    CHECK(Map.Lookup('a')==0);
    CHECK(Map.Lookup('b')==1);
    CHECK_THROWS(Map.Lookup('c'));//Never defined; map.at throws;

    //constructing form abstract_lookup_view
    Map = Observer::abstract_const_map_view(map);
    CHECK(Map.Lookup('a')==0);//indeed
    Map.Lookup = [&map](char c){ return map.at(c)*2; };
    CHECK(Map.Lookup('b')==2);
 
    //Signal
    using F = std::function<void(char,int)>;
    auto _signal = [&map](F f){ for(auto [k,v] : map) f(k,v); }; 
    Map.Signal = _signal;
    int n = 0;
    auto f = [&n](char, int i){ n += i; };
    CHECK(map.size()==2);
    map['a'] = 1;
    map['b'] = 5;
    Map.Signal(f);
    CHECK(n==6);

    //constructing form abstract_lookup_view
    map.clear(); map['a'] = 3;
    Map = Observer::abstract_const_map_view(map);
    CHECK(Map.Lookup('a')==3);//indeed
    CHECK_THROWS(Map.Lookup('b'));

}


TEST_CASE("Testing SubjectId") {

    struct A { int value; };
    using _subject1_t = Observer::_Subject1<A>;
    using _observer1_t = Observer::_Observer1<A>;
    using handler_t = std::function<void(A)>;

    std::unordered_set<_observer1_t*> set;
    _subject1_t subA(Observer::abstract_set_view(set));
    auto subA_id = Observer::SubjectId<A>(&subA); 

    int n = 0;
    auto h = [&n](A a){ n+=a.value; };
    auto h2 = [&n](A a){ n+=2*a.value; };
    std::unordered_map<_subject1_t*,handler_t> subject_handlers;

    //1. no backend
    {
        _observer1_t obs;
        obs.bindHandlerSubject1(h,subA_id);
        obs.Subscribe(subA_id);
        n = 0;
        obs.Define(subA_id,h2);//no op, see below
        subA.Notify(A{1});
        CHECK(n==1);//still bound to h;
        obs.Unsubscribe(subA_id);
        obs.Remove(subA_id);
    }
    //2. backend
    {
        _observer1_t obs;
        auto Map = Observer::abstract_map_view(subject_handlers);
        obs.bindSubjectHandlers(Map);
        obs.Subscribe(subA_id);
        obs.Define(subA_id,h);
        obs.Unsubscribe(subA_id);
        obs.Remove(subA_id);
    }

}


template <typename T>
struct TestNoCopy
{
    static constexpr bool AssertNoCopy() {
        return !std::is_copy_constructible<T>::value && !std::is_copy_assignable<T>::value;
    }
    
    static constexpr bool AssertNoMove() {
        return !std::is_move_constructible<T>::value && !std::is_move_assignable<T>::value;
    }
};


TEST_CASE("Testing _Subject1[view] / Attach, Detach") {

    //_Subject1<typename E>
    struct A { int value; };
    using _subject1_t = Observer::_Subject1<A>;
    using _observer1_t = Observer::_Observer1<A>;

    //static object
    CHECK(TestNoCopy<_subject1_t>::AssertNoCopy());
    CHECK(TestNoCopy<_subject1_t>::AssertNoMove());
   
    //Attach
    std::unordered_set<_observer1_t*> set;
    _subject1_t subA(Observer::abstract_set_view(set));
    _observer1_t obs; 
    CHECK(set.size()==0);
    CHECK(subA.Attach(&obs)==true);
    CHECK(set.size()==1);
    _observer1_t obs2;
    CHECK(subA.Attach(&obs2)==true);
    CHECK(set.size()==2);
    CHECK(subA.Attach(&obs2)==false);//already attached fails
    CHECK(set.size()==2);
    
    //Detach
    CHECK(subA.Detach(&obs)==true);
    CHECK(set.size()==1);
    _observer1_t obs3;
    CHECK(subA.Detach(&obs3)==false);//never attached fails
    CHECK(set.size()==1);

}


TEST_CASE("Testing _Subject1[data] / Attach, Detach") {

    //_Subject1<typename E>
    struct A { int value; };
    using _subject1_t = Observer::_Subject1<A>;
    using _observer1_t = Observer::_Observer1<A>;

    //static object
    CHECK(TestNoCopy<_subject1_t>::AssertNoCopy());
    CHECK(TestNoCopy<_subject1_t>::AssertNoMove());
   
    //Attach
    std::unordered_set<_observer1_t*> set;
    _subject1_t subA(Observer::AbstractSetData<_observer1_t*>(std::move(set)));
    _observer1_t obs; 
    CHECK(subA.Attach(&obs)==true);
    _observer1_t obs2;
    CHECK(subA.Attach(&obs2)==true);
    CHECK(subA.Attach(&obs2)==false);//already attached fails
    
    //Detach
    CHECK(subA.Detach(&obs)==true);
    _observer1_t obs3;
    CHECK(subA.Detach(&obs3)==false);//never attached fails

}


TEST_CASE("Testing _Observer1 / Notify") {

    //_Observer1<typename E>
    struct A { int value; };
    using _subject1_t = Observer::_Subject1<A>;
    using _observer1_t = Observer::_Observer1<A>;

    //static object
    CHECK(TestNoCopy<_observer1_t>::AssertNoCopy());
    CHECK(TestNoCopy<_observer1_t>::AssertNoMove());
    
    //observer onEvent
    _observer1_t obs;
    obs.onEvent(A{1});

    //observer bindHandler;
    int n = 0;
    auto h = [&n](A a){ n+=a.value; };
    auto h2 = [&n](A a){ n+=a.value*2; };
    obs.bindHandlerSubject1(h).bindHandlerSubject1(h2);//h2 should be in effect
    obs.onEvent(A{1});
    CHECK(n==2);
    obs.onEvent(A{2});
    CHECK(n==6);

    //through Notify
    std::unordered_set<_observer1_t*> set;
    _subject1_t subA(Observer::abstract_set_view(set));
    CHECK(subA.Attach(&obs)==true);
    subA.Notify(A{3});
    CHECK(n==12);
    obs.bindHandlerSubject1(h);
    subA.Notify(A{4});
    CHECK(n==16);

    //Notify to multiple observers
    _observer1_t obs2;
    obs2.bindHandlerSubject1(h2);
    CHECK(subA.Attach(&obs2)==true);
    n = 0;
    subA.Notify(A{5});
    CHECK(n==15);
    CHECK(subA.Detach(&obs2)==true);
    subA.Notify(A{5});
    CHECK(n==20); 

    //observer bindSubjectHandlers;
    std::unordered_set<_observer1_t*> set2;
    _subject1_t subA2(Observer::abstract_set_view(set2));
    CHECK(subA2.Attach(&obs)==true);
    n = 0;
    subA2.Notify(A{3});
    CHECK(n==3);//h is active
    using handler_t = std::function<void(A)>;
    std::unordered_map<_subject1_t*,handler_t> subject_handlers;
    subject_handlers[&subA] = h;
    subject_handlers[&subA2] = h2;
    obs.bindSubjectHandlers(Observer::abstract_map_view(subject_handlers));
    obs.onEvent(A{2},&subA);
    CHECK(n==5);
    obs.onEvent(A{3},&subA2);
    CHECK(n==11);
    subA2.Notify(A{4});
    CHECK(n==19);

}


TEST_CASE("Testing _Observer1 / auto-Detach") {

//_Observer1<typename E>
    struct A { int value; };
    using _subject1_t = Observer::_Subject1<A>;
    using _observer1_t = Observer::_Observer1<A>;
    using handler_t = std::function<void(A)>;
    
    std::unordered_set<_observer1_t*> set;
    _subject1_t subA(Observer::abstract_set_view(set));

    int n = 0;
    auto h = [&n](A a){ n+=a.value; };
    std::unordered_map<_subject1_t*,handler_t> subject_handlers;
    subject_handlers[&subA] = h;
    

    //notify ObserverClose
    //1. no handler -> no Detach
    set.clear(); 
    {
        _observer1_t obs3;
        subA.Attach(&obs3);
    }
    CHECK(set.size()==1);
    //2. unique handler -> no Detach
    set.clear();
    n = 0;
    {
        _observer1_t obs3;
        subA.Attach(&obs3);
        obs3.bindHandlerSubject1(h);
        subA.Notify(A{4});
        CHECK(n==4);
    }
    CHECK(set.size()==1);
    //3. subject_handlers -> Detach
    set.clear();
    n = 0;
    {
        _observer1_t obs3;
        subA.Attach(&obs3);
        obs3.bindSubjectHandlers(Observer::abstract_map_view(subject_handlers));
        subA.Notify(A{4});
        CHECK(n==4);
    }
    CHECK(set.size()==0);
    //4. subject_handlers then unique handler -> no Detach
    set.clear();
    n = 0;
    {
        _observer1_t obs3;
        subA.Attach(&obs3);
        obs3.bindSubjectHandlers(Observer::abstract_map_view(subject_handlers));
        obs3.bindHandlerSubject1(h);
        subA.Notify(A{4});
        CHECK(n==4);
    }
    CHECK(set.size()==1);
    //4. subject_handlers then unique handler and unique Subject -> Detach
    set.clear();
    n = 0;
    {
        _observer1_t obs3;
        subA.Attach(&obs3);
        obs3.bindSubjectHandlers(Observer::abstract_map_view(subject_handlers));
        obs3.bindHandlerSubject1(h,Observer::SubjectId<A>(&subA));
        subA.Notify(A{4});
        CHECK(n==4);
    }
    CHECK(set.size()==0);

}


TEST_CASE("Testing _Subject1 / backend rebinding") {

    //_Subject1<typename E>
    struct A { int value; };
    using _subject1_t = Observer::_Subject1<A>;
    using _observer1_t = Observer::_Observer1<A>;

    //impl: view
    std::unordered_set<_observer1_t*> set;
    _subject1_t subA(Observer::abstract_set_view(set));
    _observer1_t obs; 
    CHECK(set.size()==0);
    CHECK(subA.Attach(&obs)==true);
    CHECK(set.size()==1);
    auto empty = Observer::AbstractSet<_observer1_t*>();
    subA.bindObserverSet(empty);
    CHECK_THROWS(subA.Attach(&obs));
    auto old_one = Observer::abstract_set_view(set);
    subA.bindObserverSet(empty).bindObserverSet(old_one);
    _observer1_t obs2;
    CHECK(subA.Attach(&obs2)==true);
    CHECK(subA.Attach(&obs)==false);//already here

    //impl: data
    _subject1_t subA2(Observer::AbstractSetData<_observer1_t*>(std::move(set)));
    CHECK(subA2.Attach(&obs)==false);//already here

}


TEST_CASE("Testing _Observer1 / subscribe, define, remove, unsubscribe") {

    struct A { int value; };
    using _subject1_t = Observer::_Subject1<A>;
    using _observer1_t = Observer::_Observer1<A>;
    using handler_t = std::function<void(A)>;

    std::unordered_set<_observer1_t*> set;
    _subject1_t subA(Observer::abstract_set_view(set));
    
    int n = 0;
    auto h = [&n](A a){ n+=a.value; };
    std::unordered_map<_subject1_t*,handler_t> subject_handlers;

    //1. no backend, no known pSub
    {
        _observer1_t obs;
        CHECK(obs.Subscribe(&subA)==true);
        CHECK(set.size()==1);
        obs.bindHandlerSubject1(h);
        obs.onEvent(A{1},&subA);
        CHECK(n==1);
        CHECK(obs.Unsubscribe(&subA)==true);
    }
    //2. no backend, pSub
    n = 0;
    set.clear();
    {
        _observer1_t obs;
        CHECK(obs.Subscribe(&subA)==true);
        CHECK(set.size()==1);
        obs.bindHandlerSubject1(h,&subA);
        obs.onEvent(A{1},&subA);
        CHECK(n==1);
        CHECK(obs.Unsubscribe(&subA)==true);
    }
    //3. with backend
    n = 0;
    set.clear();
    {
        _observer1_t obs;
        obs.bindSubjectHandlers(Observer::abstract_map_view(subject_handlers));
        CHECK(set.count(&obs)==0);
        CHECK(obs.Subscribe(&subA)==true);
        obs.Define(&subA,h);
        CHECK(subject_handlers.size()==1);
        obs.onEvent(A{1},&subA);
        CHECK(n==1);
        CHECK(obs.Subscribe(&subA)==false);
        CHECK(obs.Unsubscribe(&subA)==true);
        CHECK(obs.Remove(&subA)==true);
        CHECK(subject_handlers.size()==0);
        CHECK(set.size()==0);
        subA.Notify(A{1}); 
        CHECK(n==1);
    }

}


template <typename E>
using _pObs1 = Observer::_Observer1<E>*;

template <typename E>
using _set1 = std::unordered_set<_pObs1<E>>;

template <typename ... Events>
using _tset = std::tuple<_set1<Events> ... >;


TEST_CASE("Testing _Subject") {

    //_Subject<typename ... Events>
    struct A { int value; };
    struct B {};
    struct C {};
    using _subject_t = Observer::_Subject<A,B,C>;

    //ctor from an AbstractSet list
    _set1<A> _setA;
    _set1<B> _setB;
    _set1<C> _setC;
    auto SetA = Observer::abstract_set_view(_setA);
    auto SetB = Observer::abstract_set_view(_setB);
    auto SetC = Observer::abstract_set_view(_setC);
    _subject_t subABC_(SetA,SetB,SetC);
    
    //ctor from an AbstractSetTuple
    _tset<A,B,C> tset;
    _subject_t subABC(Observer::abstract_set_tuple_view(tset));

    //Attach some Observer
    struct D {};
    using _observer_t = Observer::_Observer<A,B,D>;
    _observer_t obs;
    CHECK(subABC.Attach(A(),&obs)==true);
    CHECK(subABC.Detach(B(),&obs)==false);//never attached
    CHECK(subABC.Detach(A(),&obs)==true);
    
    //backend rebinding
    CHECK(subABC.Attach(A(),&obs)==true);
    int n = 0;
    auto h = [&n](A a){ n+=a.value; };
    static_cast<Observer::_Observer1<A>*>(&obs)->bindHandlerSubject1(h);
    subABC.Notify(A{3});
    CHECK(n==3);
    _set1<A> _setA2;
    auto SetA2 = Observer::abstract_set_view(_setA2);
    subABC.bindObserverSet(SetA2);
    subABC.Notify(A{3});
    CHECK(n==3);

}


TEST_CASE("Testing _AbstractContainerData and derived") {

    using cont_t = std::unordered_set<int>;
    auto pOpaque = Observer::make_opaque(cont_t());
    static_cast<cont_t*>(pOpaque.get())->insert(3);
    auto Set = Observer::AbstractSetData<int>(cont_t());
    CHECK(Set.Append(4)==true);

    //with custom abstract_set_view object
    auto asv = [](cont_t& set){ return Observer::abstract_set_view(set); };
    auto Set2 = Observer::AbstractSetData<int>(cont_t(),asv);
    CHECK(Set2.Append(4)==true);

    //with custom abstract_set_view policy container
    struct ASV {
        auto operator()(cont_t& set) {
            return Observer::abstract_set_view(set);
        }
    };
    auto Set3 = Observer::AbstractSetData<int>(cont_t(),ASV());
    CHECK(Set3.Append(4)==true);

    //No copy, just move
    using Set_t = Observer::AbstractSetData<int>;
    CHECK(std::is_copy_constructible<Set_t>::value == false);
    CHECK(std::is_copy_assignable<Set_t>::value == false);
    CHECK(std::is_move_constructible<Set_t>::value == true);
    CHECK(std::is_move_assignable<Set_t>::value == true);
 
    //TODO ConstMap, Map 

}


TEST_CASE("Testing AbstractSetVariant") {

    using set_t = std::unordered_set<int>;
    set_t _set = set_t();

    using vSet_t = Observer::AbstractSet<int>;
    using dSet_t = Observer::AbstractSetData<int>;

    auto vSet = Observer::abstract_set_view(_set);
    auto dSet = Observer::AbstractSetData<int>(set_t());
    
    auto Set = std::variant<vSet_t,dSet_t>(vSet);
    CHECK(std::visit([](auto&& s){ return s.Append(4); },Set)==true);
    CHECK(_set.size()==1);
    Set = std::move(dSet);
    CHECK(std::visit([](auto&& s){ return s.Append(5); },Set)==true);
    CHECK(_set.size()==1);
    
    auto Set2 = Observer::AbstractSetVariant<int>(vSet);
    _set.clear();
    CHECK(Set2.Append(4)==true);
    CHECK(_set.size()==1);
    Set2 = vSet;
    CHECK(Set2.Append(5)==true);
    CHECK(_set.size()==2);
    Set2 = Observer::AbstractSetData<int>(set_t());
    CHECK(Set2.Append(5)==true);
    CHECK(_set.size()==2);

}


