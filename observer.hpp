#ifndef OBSERVER
#  define OBSERVER
#  define OBSERVER_INCOMPLETE
///////////////////////////////


#include <cstddef>
#include <type_traits>
#include <memory>
#include <variant>
#include <unordered_set>
#include <unordered_map>


namespace Observer {


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

template <typename E, typename _SubjectEvents>
struct Event;

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


struct NoCopy
{
    NoCopy(NoCopy const&) = delete;
    NoCopy& operator=(NoCopy const&) = delete;

    NoCopy(NoCopy&&) = delete;
    NoCopy& operator=(NoCopy&&) = delete;

    protected:
    NoCopy() = default;
};


//
//
//


template <typename T>
struct AbstractSet
{
    std::function<bool(T)> Append;
    std::function<bool(T)> Remove;
    using F = std::function<void(T)>;
    std::function<void(F)> Signal;
};


template <typename T>
AbstractSet<T> abstract_set_view(std::unordered_set<T>& set)
{
    auto append = [&set](T t){ return set.insert(t).second; };
    auto remove = [&set](T t){ return set.erase(t)==1; };
    using F = std::function<void(T)>;
    auto signal = [&set](F f){ for(auto t : set) f(t); };
    return {append,remove,signal};
};
 

//
//
//


template <typename ... Ts>
struct AbstractSetTuple
{
    private:
    using impl_t = std::tuple<AbstractSet<Ts>...>;
    impl_t impl;

    public:
    AbstractSetTuple() = default;

    AbstractSetTuple(AbstractSet<Ts> ... sets)
    : impl(sets ...)
    {}

    template <typename T>
    AbstractSetTuple& Set(AbstractSet<T> s)
    { std::get<AbstractSet<T>>(impl) = s; return *this; }

    template <typename T>
    AbstractSet<T> Get(T) const
    { return std::get<AbstractSet<T>>(impl); }

};

template <typename ... Ts>
AbstractSetTuple<Ts...> abstract_set_tuple_view(std::unordered_set<Ts>& ... sets)
{
    return { abstract_set_view(sets) ... };
};

template <typename ... Cs>
auto abstract_set_tuple_view(std::tuple<Cs...>& tset)
{
    return abstract_set_tuple_view(std::get<Cs>(tset)...);
}

template <typename ASVs, typename ... Cs>
AbstractSetTuple<typename Cs::value_type ...> abstract_set_tuple_view(Cs& ... sets)
{
    return { ASVs()(sets) ... };
};

template <typename ASVs, typename ... Cs>
auto abstract_set_tuple_view(std::tuple<Cs...>& tset)
{
    return abstract_set_tuple_view<ASVs>(std::get<Cs>(tset)...);
}


//
//
//


template <typename K, typename V>
struct AbstractMap
{
    std::function<void(K,V)> Define;
    std::function<bool(K)> Remove;
    std::function<V(K)> Lookup;
    using F = std::function<void(K,V)>;
    std::function<void(F)> Signal;
};


template <typename K, typename V>
AbstractMap<K,V> abstract_map_view(std::unordered_map<K,V>& map)
{
    auto define = [&map](K k, V v){ map.insert_or_assign(k,v); };
    auto remove = [&map](K k){ return map.erase(k)==1; };
    auto lookup = [&map](K k){ return map.at(k); };
    using F = std::function<void(K,V)>; 
    auto signal = [&map](F f){ for(auto [k,v] : map) f(k,v); }; 
    return {define,remove,lookup,signal};
};


//
//
//


template <typename K, typename V>
struct AbstractConstMap
{
    std::function<V(K)> Lookup;
    using F = std::function<void(K,V)>;
    std::function<void(F)> Signal;
};


template <typename K, typename V>
AbstractConstMap<K,V> abstract_const_map_view(std::unordered_map<K,V> const& map)
{
    auto lookup = [&map](K k){ return map.at(k); };
    using F = std::function<void(K,V)>; 
    auto signal = [&map](F f){ for(auto [k,v] : map) f(k,v); }; 
    return {lookup,signal};
};


//
//
//

template <typename E>
struct _Observer1;

template <typename E>
struct _Subject1;

template <bool as_base>
struct AsBase;

template <>
struct AsBase<true>
{
    protected:
    ~AsBase() = default;
};

template <typename T, typename friend_t = T>
struct Id
{
    explicit Id(T* const secret_ptr_, void* const pOwner_ = nullptr)
    : pOwner(pOwner_ ? pOwner_ : secret_ptr_),secret_ptr(secret_ptr_)
    {}

    void* value() const
    { return pOwner; }

    private:
        
        void* const pOwner;
 
        friend friend_t;  
        T* const secret_ptr;
        T* operator->() const { return secret_ptr; }
        T* get() const { return secret_ptr; }

};

template <typename T, typename friend_t>
bool operator==(Id<T,friend_t> lhs, Id<T,friend_t> rhs)
{ return lhs.value() == rhs.value(); }

template <typename T, typename friend_t>
bool operator!=(Id<T,friend_t> lhs, Id<T,friend_t> rhs)
{ return !(lhs==rhs); }

template <typename E>
struct SubjectId
: private Id<_Subject1<E>,_Observer1<E>>
{
    private:
    using Id_t = Id<_Subject1<E>,_Observer1<E>>;
    
    public:
    using Id_t::Id;

    void* value() const
    { return Id_t::value(); }

};



template <typename E>
using pSub_T = _Subject1<E>*;

template <typename E>
using _pSub_T = _Subject1<E>*;

template <typename E>
using pObs_T = _Observer1<E>*;

template <typename E>
using _pObs_T = _Observer1<E>*;


//
//
//


template <typename E>
struct _Subject1
: private NoCopy
{
    private:

        static_assert(SubjectEvents<E>::AssertAllDefined(),
            "ERROR Observer::_Subject1<.>: events must be defined (not just declared)");
       
        using pObs_t = pObs_T<E>;
        using observers_t = AbstractSet<pObs_t>;
        observers_t observers;
        
    public:

    explicit _Subject1(observers_t observers_)
    : NoCopy()
    , observers(observers_)
    {}

    bool Attach(pObs_t pObs) 
    { return observers.Append(pObs); }

    bool Detach(pObs_t pObs)
    { return observers.Remove(pObs); }

    void Notify(E e)
    { observers.Signal([e,this](pObs_t pObs){ pObs->onEvent(e,this); }); }
    /* Note: not const; Observers may respond by Detaching. */

    _Subject1& bindObserverSet(observers_t observers_)
    { observers = observers_; return *this; }

};  

template <typename E>
struct _Observer1
: private NoCopy
{
    private:

        static_assert(SubjectEvents<E>::AssertAllDefined(),
            "ERROR Observer::_Observer1<.>: events must be defined (not just declared)");

        using pSub_t = pSub_T<E>;
        using handler_t = std::function<void(E)>;
        using subject_handlers_t = AbstractMap<pSub_t,handler_t>;
        using signal_func_t = std::function<void(pSub_t,handler_t)>;

        static constexpr auto define_ignore = [](pSub_t,handler_t){};
        static constexpr auto remove_ignore = [](pSub_t){ return true; };
        static constexpr auto lookup_ignore = [](pSub_t){ return [](E){}; };
        static constexpr auto signal_ignore = [](signal_func_t){};

        static constexpr subject_handlers_t allset_ignore()
        { return { define_ignore, remove_ignore, lookup_ignore, signal_ignore }; }

        subject_handlers_t subject_handlers = allset_ignore(); 
        
    public:

    _Observer1() = default;

    void onEvent(E e, pSub_t pSub = nullptr)
    { subject_handlers.Lookup(pSub)(e); }

    _Observer1& bindSubjectHandlers(subject_handlers_t subject_handlers_)
    { subject_handlers = subject_handlers_; return *this; }

    _Observer1& bindHandlerSubject1(handler_t handler = [](E){}, pSub_t pSub = nullptr)
    {
        subject_handlers = allset_ignore();
        subject_handlers.Lookup = [handler](pSub_t){ return handler; };
        if(pSub) {
            auto detach_unique = [pSub,this](signal_func_t){ pSub->Detach(this); };
            subject_handlers.Signal = detach_unique;
        }
        return *this; 
    }
    
    ~_Observer1()
    { subject_handlers.Signal([this](pSub_t pSub,handler_t){ pSub->Detach(this); }); }
    //only works when a subject_handlers was bound;

    bool Subscribe(pSub_t pSub)
    { return pSub->Attach(this); }

    void Define(pSub_t pSub, handler_t handler = [](E){})
    { subject_handlers.Define(pSub,handler); }

    bool Unsubscribe(pSub_t pSub)
    { return pSub->Detach(this) && subject_handlers.Remove(pSub); }

};


//
//
//


template <typename ... Events>
struct _Observer;


template <typename ... Events>
struct _Subject
: _Subject1<Events> ...
{
    private:

        using events_check_t = SubjectEvents<Events...>;
        static_assert(events_check_t::AssertNoDuplicate(),
            "ERROR Observer::_Subject<...>: any event can appear only once");
        
        template <typename E>
        using observers_T = AbstractSet<pObs_T<E>>;
    
    public:

    explicit _Subject(observers_T<Events> ... Sets)
    : _Subject1<Events>(Sets) ...
    {}

    explicit _Subject(AbstractSetTuple<pObs_T<Events>...> TSet)
    : _Subject1<Events>(TSet.Get(pObs_T<Events>())) ... 
    {}

    template <typename E, typename ... Evs>
    bool Attach(E, _Observer<Evs...>* pObs) 
    { 
        static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
            "ERROR Observer::_Subject<...>::Attach<E>: event E does not belong to subject events");
        static_assert(IsSupportedEvent<E,SubjectEvents<Evs...>>::value,
            "ERROR Observer::_Subject<...>::Attach<E>(pObs): event E does not belong to pObs events");
        return static_cast<_Subject1<E>*>(this)->Attach(pObs);
    }

    template <typename E, typename ... Evs>
    bool Detach(E, _Observer<Evs...>* pObs) 
    { 
        static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
            "ERROR Observer::_Subject<...>::Detach<E>: event E does not belong to subject events");
        static_assert(IsSupportedEvent<E,SubjectEvents<Evs...>>::value,
            "ERROR Observer::_Subject<...>::Detach<E>(pObs): event E does not belong to pObs events");
        return static_cast<_Subject1<E>*>(this)->Detach(pObs);
    }

};

template <typename ... Events>
struct _Observer
: _Observer1<Events> ...
{
    private:

        using events_check_t = SubjectEvents<Events...>;
        static_assert(events_check_t::AssertNoDuplicate(),
            "ERROR Observer::_Observer<...>: any event can appear only once");

}; 


}//close Observer


template <typename _1, typename _2>
struct std::hash< Observer::Id<_1,_2> >
{
    size_t operator()(Observer::Id<_1,_2> const& id) const
    { return id.value(); }

};


///////////////////////////////
#  undef OBSERVER_INCOMPLETE
#else
#  ifdef OBSERVER_INCOMPLETE
#    error circular inclusion of observer.hpp
#  endif
#endif
