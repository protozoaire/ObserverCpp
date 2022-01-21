#ifndef _SCRATCH_OBSERVER
#  define _SCRATCH_OBSERVER
#  define _SCRATCH_OBSERVER_INCOMPLETE
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


using opaque_data_ptr_t = std::unique_ptr<void,std::function<void(void*)>>;

template <typename C>
auto make_opaque(C&& c)
{
    constexpr auto opaque_delete = [](void* ptr){ delete static_cast<C*>(ptr); };
    constexpr auto new_opaque = [](C&& c){ return static_cast<void*>(new C(std::move(c))); };
    return opaque_data_ptr_t(new_opaque(std::move(c)),opaque_delete);
}

template <typename AC>
struct _AbstractContainerData
{
    opaque_data_ptr_t pData;
    AC methods;

    template <typename C, typename ACV>
    explicit _AbstractContainerData(C&& cont_, ACV acv)
    : pData(make_opaque(std::move(cont_)))
    , methods(acv(*static_cast<C*>(pData.get())))
    {}

    explicit _AbstractContainerData(opaque_data_ptr_t&& pData_, AC methods_)
    : pData(std::move(pData_)),methods(methods_)
    {}

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


template <typename T>
struct AbstractSetData
: private _AbstractContainerData<AbstractSet<T>>
{
    bool Append(T t) { return this->methods.Append(t); }
    bool Remove(T t) { return this->methods.Remove(t); }
    using F = std::function<void(T)>;
    void Signal(F f) { this->methods.Signal(f); }

    private:
    using ASet_t = AbstractSet<T>;
    using impl_t = _AbstractContainerData<ASet_t>;

    public:
    using impl_t::_AbstractContainerData;

    template <typename C>
    explicit AbstractSetData(C&& set_)
    : impl_t(std::move(set_),&abstract_set_view<T>)
    {}

    AbstractSetData()
    : AbstractSetData(std::unordered_set<T>())
    {}

};


template <typename T>
AbstractSetData<T> abstract_set_data(std::unordered_set<T>&& set)
{ return AbstractSetData<T>(std::move(set)); }


template <typename T>
struct AbstractSetVariant
{
    bool Append(T t) { return std::visit([t](auto&& s){ return s.Append(t); },any); }
    bool Remove(T t) { return std::visit([t](auto&& s){ return s.Remove(t); },any); }
    using F = std::function<void(T)>;
    void Signal(F f) { std::visit([f](auto&& s){ s.Signal(f); },any); }

    private:
    using view_t = AbstractSet<T>;
    using data_t = AbstractSetData<T>;
    std::variant<view_t,data_t> any;

    public:
    explicit AbstractSetVariant(view_t v) : any(v) {}
    AbstractSetVariant& operator=(view_t v) { any = v; return *this; }
    
    explicit AbstractSetVariant(data_t&& d) : any(std::move(d)) {} 
    AbstractSetVariant& operator=(data_t&& d) { any = std::move(d); return *this; } 

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


template <typename ... Ts>
struct AbstractSetDataTuple
{
    private:
    using impl_t = std::tuple<AbstractSetData<Ts>...>;
    impl_t impl;

    public:
    AbstractSetDataTuple(AbstractSetData<Ts>&& ... sets)
    : impl(std::move(sets) ...)
    {}

    AbstractSetDataTuple()
    : AbstractSetDataTuple(AbstractSetData<Ts>() ...)
    {}

    template <typename T>
    AbstractSetDataTuple& Set(AbstractSetData<T>&& s)
    { std::get<AbstractSetData<T>>(impl) = std::move(s); return *this; }
    
    template <typename T>
    AbstractSetData<T> const& Get(T) const
    { return std::get<AbstractSetData<T>>(impl); }

};

template <typename ... Ts>
AbstractSetDataTuple<Ts...> abstract_set_tuple_data(std::unordered_set<Ts>&& ... sets)
{
    return { abstract_set_data(std::move(sets)) ... };
};

template <typename ... Cs>
auto abstract_set_tuple_data(std::tuple<Cs...>&& tset)
{
    return abstract_set_tuple_data(std::move(std::get<Cs>(tset))...);
}

template <typename ASDs, typename ... Cs>
AbstractSetDataTuple<typename Cs::value_type ...> abstract_set_tuple_data(Cs&& ... sets)
{
    return { ASDs()(std::move(sets)) ... };
};

template <typename ASDs, typename ... Cs>
auto abstract_set_tuple_data(std::tuple<Cs...>&& tset)
{
    return abstract_set_tuple_data<ASDs>(std::move(std::get<Cs>(tset))...);
}


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


template <typename K, typename V>
struct AbstractConstMapData
: private _AbstractContainerData<AbstractConstMap<K,V>>
{
    auto Lookup(K k){ return this->methods.Lookup(k); }
    using F = std::function<void(K,V)>;
    void Signal(F f) { this->methods.Signal(f); }

    private:
    using AMap_t = AbstractConstMap<K,V>;
    using impl_t = _AbstractContainerData<AMap_t>;
   
    template <typename,typename>
    friend struct AbstractMapData;

    public:
    using impl_t::_AbstractContainerData;

    template <typename C>
    explicit AbstractConstMapData(C&& map_)
    : impl_t(std::move(map_),&abstract_const_map_view<K,V>)
    {}
 
    AbstractConstMapData()
    : AbstractConstMapData(std::unordered_map<K,V>())
    {}

};


template <typename K, typename V>
AbstractConstMapData<K,V> abstract_const_map_data(std::unordered_map<K,V>&& map)
{ return AbstractConstMapData<K,V>(std::move(map)); }


template <typename K, typename V>
struct AbstractConstMapVariant
{
    auto Lookup(K k){ return std::visit([k](auto&& m){ return m.Lookup(k); },any); }
    using F = std::function<void(K,V)>;
    void Signal(F f){ std::visit([f](auto&& m){ m.Signal(f); },any); }

    private:
    using view_t = AbstractConstMap<K,V>;
    using data_t = AbstractConstMapData<K,V>;
    std::variant<view_t,data_t> any;

    public:
    explicit AbstractConstMapVariant(view_t v) : any(v) {}
    AbstractConstMapVariant& operator=(view_t v) { any = v; return *this; }
    
    explicit AbstractConstMapVariant(data_t&& d) : any(std::move(d)) {} 
    AbstractConstMapVariant& operator=(data_t&& d) { any = std::move(d); return *this; } 

};


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

template <typename K, typename V>
AbstractMap<K,V> MapEmbedConstMap(AbstractConstMap<K,V> m)
{
    static constexpr auto define_ignore = [](K,V){};
    static constexpr auto remove_ignore = [](K){ return false; };
    return { define_ignore, remove_ignore, m.Lookup, m.Signal };
}


template <typename K, typename V>
struct AbstractMapData
: private _AbstractContainerData<AbstractMap<K,V>>
{
    void Define(K k, V v){ this->methods.Define(k,v); }
    bool Remove(K k){ return this->methods.Remove(k); }
    auto Lookup(K k){ return this->methods.Lookup(k); }
    using F = std::function<void(K,V)>;
    void Signal(F f) { this->methods.Signal(f); }

    private:
    using AMap_t = AbstractMap<K,V>;
    using impl_t = _AbstractContainerData<AMap_t>;

    public:
    using impl_t::_AbstractContainerData;

    template <typename C>
    explicit AbstractMapData(C&& map_)
    : impl_t(std::move(map_),&abstract_map_view<K,V>)
    {}

    explicit AbstractMapData(AbstractConstMapData<K,V>&& cmap)
    : impl_t(std::move(cmap.pData),MapEmbedConstMap(cmap.methods))
    {}

    AbstractMapData()
    : AbstractMapData(std::unordered_map<K,V>())
    {}

};


template <typename K, typename V>
AbstractMapData<K,V> abstract_map_data(std::unordered_map<K,V>&& map)
{ return AbstractMapData<K,V>(std::move(map)); }


template <typename K, typename V>
struct AbstractMapVariant
{
    void Define(K k, V v){ std::visit([k,v](auto&& m){ m.Define(k,v); },any); }
    bool Remove(K k){ return std::visit([k](auto&& m){ return m.Remove(k); },any); }
    auto Lookup(K k){ return std::visit([k](auto&& m){ return m.Lookup(k); },any); }
    using F = std::function<void(K,V)>;
    void Signal(F f){ std::visit([f](auto&& m){ m.Signal(f); },any); }

    private:
    using view_t = AbstractMap<K,V>;
    using data_t = AbstractMapData<K,V>;
    std::variant<view_t,data_t> any;

    public:
    explicit AbstractMapVariant(view_t v) : any(v) {}
    AbstractMapVariant& operator=(view_t v) { any = v; return *this; }
    
    explicit AbstractMapVariant(data_t&& d) : any(std::move(d)) {} 
    AbstractMapVariant& operator=(data_t&& d) { any = std::move(d); return *this; } 

};


template <typename K, typename V>
AbstractMapData<K,V> MapEmbedConstMap(AbstractConstMapData<K,V>&& m)
{ return AbstractMapData<K,V>(std::move(m)); }


//
//
//


template <typename E>
struct _Observer1;

template <typename E>
struct _Subject1;

template <typename ... Events>
struct _Observer;

template <typename ... Events>
struct _Subject;

template <typename E>
struct SubjectID
{
    SubjectID(_Subject1<E>* ptr_) : ptr(ptr_) {}
    
    private:

        _Subject1<E>* ptr;

        auto operator->() const { return ptr; }
        auto get() const { return ptr; }
        friend _Observer1<E>;

};

template <typename E>
struct ObserverID
{
    ObserverID(_Observer1<E>* ptr_) : ptr(ptr_) {}

    private:

        _Observer1<E>* ptr;

        auto operator->() const { return ptr; }
        auto get() const { return ptr; }
        friend _Subject1<E>;

};

template <typename E>
using pSub_T = _Subject1<E>*;

template <typename E>
using pObs_T = _Observer1<E>*;


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

        using observers_view_t = AbstractSet<pObs_t>;
        using observers_data_t = AbstractSetData<pObs_t>;
        using observers_t = AbstractSetVariant<pObs_t>;
        observers_t observers;
        
        using oId_t = ObserverID<E>; 
    
    public:

    explicit _Subject1(observers_view_t observers_)
    : NoCopy()
    , observers(observers_)
    {}

    explicit _Subject1(observers_data_t&& observers_ = observers_data_t())
    : NoCopy()
    , observers(std::move(observers_))
    {}

    bool Attach(oId_t oId) 
    { return observers.Append(oId.get()); }

    bool Detach(oId_t oId)
    { return observers.Remove(oId.get()); }

    void Notify(E e)
    { observers.Signal([e,this](pObs_t pObs){ pObs->onEvent(e,this); }); }
    /* Note: not const; Observers may respond by Detaching. */

    _Subject1& bindObserverSet(observers_view_t observers_)
    { observers = observers_; return *this; }

    _Subject1& bindObserverSet(observers_data_t&& observers_)
    { observers = std::move(observers_); return *this; }

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
        
        using subject_handlers_view_t = AbstractMap<pSub_t,handler_t>;
        using subject_handlers_data_t = AbstractMapData<pSub_t,handler_t>;
        using subject_handlers_t = AbstractMapVariant<pSub_t,handler_t>;
        using signal_func_t = std::function<void(pSub_t,handler_t)>;

        static constexpr auto define_ignore = [](pSub_t,handler_t){};
        static constexpr auto remove_ignore = [](pSub_t){ return false; };
        static constexpr auto lookup_ignore = [](pSub_t){ return [](E){}; };
        static constexpr auto signal_ignore = [](signal_func_t){};

        static constexpr subject_handlers_view_t allset_ignore()
        { return { define_ignore, remove_ignore, lookup_ignore, signal_ignore }; }

        subject_handlers_t subject_handlers; 
       
        using sId_t = SubjectID<E>; 
        
        using subject_handlers_const_view_t = AbstractConstMap<pSub_t,handler_t>;
        using subject_handlers_const_data_t = AbstractConstMapData<pSub_t,handler_t>;

    public:

    _Observer1()
    : subject_handlers(allset_ignore())
    {}

    void onEvent(E e, pSub_t pSub = nullptr)
    { subject_handlers.Lookup(pSub)(e); }

    _Observer1& bindSubjectHandlers(subject_handlers_view_t subject_handlers_)
    { subject_handlers = subject_handlers_; return *this; }
    
    _Observer1& bindSubjectHandlers(subject_handlers_const_view_t subject_handlers_)
    { subject_handlers = MapEmbedConstMap(subject_handlers_); return *this; }

    _Observer1& bindSubjectHandlers(subject_handlers_data_t&& subject_handlers_)
    { subject_handlers = std::move(subject_handlers_); return *this; }
    
    _Observer1& bindSubjectHandlers(subject_handlers_const_data_t&& subject_handlers_)
    { subject_handlers = MapEmbedConstMap(std::move(subject_handlers_)); return *this; }

    _Observer1& bindSubjectHandlers()
    { return bindSubjectHandlers(subject_handlers_data_t()); }

    _Observer1& bindHandlerSubject1(handler_t handler = [](E){}, sId_t sId = nullptr)
    {
        pSub_t pSub = sId.get();
        auto unique_handler = [handler](pSub_t){ return handler; };
        auto detach_unique = [pSub,this](signal_func_t){ pSub->Detach(this); };
        //prepare new view
        subject_handlers_view_t view = allset_ignore();
        view.Lookup = unique_handler;
        if(pSub) view.Signal = detach_unique;
        //commit view
        subject_handlers = view;
        return *this;
    }
  
    ~_Observer1()
    { subject_handlers.Signal([this](pSub_t pSub,handler_t){ pSub->Detach(this); }); }
    //only works when a subject_handlers was bound;

    bool Subscribe(sId_t sId)
    { return sId->Attach(this); }

    bool Unsubscribe(sId_t sId)
    { return sId->Detach(this); }
    
    void Define(sId_t sId, handler_t handler = [](E){})
    { subject_handlers.Define(sId.get(),handler); }
    
    bool Remove(sId_t sId)
    { return subject_handlers.Remove(sId.get()); }

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
        using observers_view_T = AbstractSet<pObs_T<E>>;
    
        template <typename E>
        using observers_data_T = AbstractSetData<pObs_T<E>>;

        template <typename E>
        using observers_T = AbstractSetVariant<pObs_T<E>>;
        
        template <typename E>
        auto _this() { return static_cast<_Subject1<E>*>(this); }


    public:

    explicit _Subject(observers_view_T<Events> ... Sets)
    : _Subject1<Events>(Sets) ...
    {}

    explicit _Subject(observers_data_T<Events>&& ... Sets)
    : _Subject1<Events>(std::move(Sets)) ...
    {}
  
    explicit _Subject(AbstractSetTuple<pObs_T<Events>...> TSet)
    : _Subject1<Events>(TSet.Get(pObs_T<Events>())) ... 
    {}
    
    explicit _Subject(AbstractSetDataTuple<pObs_T<Events>...>&& TSet)
    : _Subject1<Events>(std::move(TSet.Get(pObs_T<Events>()))) ... 
    {}

    _Subject()
    : _Subject(observers_data_T<Events>() ...)
    {}

    template <typename E>
    bool Attach(E, ObserverID<E> oId) 
    { 
        static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
            "ERROR Observer::_Subject<...>::Attach<E>: event E does not belong to subject events");
        return _this<E>()->Attach(oId);
    }

    template <typename E>
    bool Detach(E, ObserverID<E> oId) 
    { 
        static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
            "ERROR Observer::_Subject<...>::Detach<E>: event E does not belong to subject events");
        return _this<E>()->Detach(oId);
    }
    
    template <typename E>
    void Notify(E e)
    {
        static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
            "ERROR Observer::_Subject<...>::Notify<E>: event E does not belong to subject events");
        return _this<E>()->Notify(e);
    }
    
    template <typename E>
    _Subject& bindObserverSet(E, observers_view_T<E> observers_)
    {
        static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
            "ERROR Observer::_Subject<...>::bindObserverSet<E>: event E does not belong to subject events");
        _this<E>()->bindObserverSet(observers_); return *this;
    }

    template <typename E>
    _Subject& bindObserverSet(E, observers_data_T<E>&& observers_)
    {
        static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
            "ERROR Observer::_Subject<...>::bindObserverSet<E>: event E does not belong to subject events");
        _this<E>()->bindObserverSet(std::move(observers_)); return *this;
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

        template <typename E>
        auto _this() { return static_cast<_Observer1<E>*>(this); }
    
        template <typename E>
        using handler_T = std::function<void(E)>;

        template <typename E>
        using subject_handlers_view_T = AbstractMap<pSub_T<E>,handler_T<E>>;
        
        template <typename E>
        using subject_handlers_const_view_T = AbstractConstMap<pSub_T<E>,handler_T<E>>;

        template <typename E>
        using subject_handlers_data_T = AbstractMapData<pSub_T<E>,handler_T<E>>;
        
        template <typename E>
        using subject_handlers_const_data_T = AbstractConstMapData<pSub_T<E>,handler_T<E>>;

    public:

    template <typename E>
    _Observer& bindSubjectHandlers(E, subject_handlers_view_T<E> subject_handlers_)
    {
        static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
            "ERROR Observer::_Observer<...>::bindSubjectHandlers<E>(view): event E does not belong to observer events");
        _this<E>()->bindSubjectHandlers(subject_handlers_); return *this;
    }
    
    template <typename E>
    _Observer& bindSubjectHandlers(E, subject_handlers_const_view_T<E> subject_handlers_)
    {
        static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
            "ERROR Observer::_Observer<...>::bindSubjectHandlers<E>(const_view): event E does not belong to observer events");
        _this<E>()->bindSubjectHandlers(subject_handlers_); return *this;
    }
    
    template <typename E>
    _Observer& bindSubjectHandlers(E, subject_handlers_data_T<E>&& subject_handlers_)
    {
        static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
            "ERROR Observer::_Observer<...>::bindSubjectHandlers<E>(data): event E does not belong to observer events");
        _this<E>()->bindSubjectHandlers(std::move(subject_handlers_)); return *this;
    }

    template <typename E>
    _Observer& bindSubjectHandlers(E, subject_handlers_const_data_T<E>&& subject_handlers_)
    {
        static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
            "ERROR Observer::_Observer<...>::bindSubjectHandlers<E>(const_data): event E does not belong to observer events");
        _this<E>()->bindSubjectHandlers(std::move(subject_handlers_)); return *this;
    }
   
    template <typename E>
    _Observer& bindSubjectHandlers(E e)
    { return bindSubjectHandlers(e, subject_handlers_data_T<E>()); }

    template <typename E, typename H>
    _Observer& bindHandlerSubject1(E, H handler = [](E){}, SubjectID<E> sId = nullptr)
    {
        static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
            "ERROR Observer::_Observer<...>::bindHandlerSubject1<E>: event E does not belong to observer events");
        _this<E>()->bindHandlerSubject1(handler,sId); return *this;
    }
 
    template <typename E>
    bool Subscribe(E, SubjectID<E> sId)
    { 
        static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
            "ERROR Observer::_Observer<...>::Subscribe<E>: event E does not belong to observer events");
        return _this<E>()->Subscribe(sId);
    }

    template <typename E>
    bool Unsubscribe(E, SubjectID<E> sId)
    {
        static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
            "ERROR Observer::_Observer<...>::Unsubscribe<E>: event E does not belong to observer events");
        return _this<E>()->Unsubscribe(sId);
    }
     
    template <typename E, typename H>
    void Define(E, SubjectID<E> sId, H handler = [](E){})
    { 
        static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
            "ERROR Observer::_Observer<...>::Define<E>: event E does not belong to observer events");
        return _this<E>()->Define(sId,handler);
    }
    
    template <typename E>
    bool Remove(E, SubjectID<E> sId)
    {
        static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
            "ERROR Observer::_Observer<...>::Remove<E>: event E does not belong to observer events");
        return _this<E>()->Remove(sId);
    }

}; 


//
//
//


template <typename ... Events>
struct SubjectConnect
{

    template <typename E>
    SubjectID<E> subject_id(E)
    { return pSub; }

    template <typename E>
    bool Attach(ObserverID<E> oID)
    { return pSub->Attach(E(),oID); }

    template <typename E>
    bool Detach(ObserverID<E> oID)
    { return pSub->Detach(E(),oID); }

    private:
        
        _Subject<Events...>* pSub = nullptr;

    protected:
   
    explicit SubjectConnect(_Subject<Events...>* pSub_ = nullptr)
    : pSub(pSub_)
    {}

    SubjectConnect& reset(_Subject<Events...>* pSub_)
    { pSub = pSub_; return *this; }

    ~SubjectConnect() = default;

};


template <typename ... Events>
struct ObserverConnect
{
    
    template <typename E>
    ObserverID<E> observer_id(E)
    { return pObs; }

    template <typename E>
    bool Subscribe(SubjectID<E> sID)
    { return pObs->Subscribe(E(),sID); }

    template <typename E>
    bool Unsubscribe(SubjectID<E> sID)
    { return pObs->Unsubscribe(E(),sID); }

    private:
        
        _Observer<Events...>* pObs = nullptr;

    protected:
   
    explicit ObserverConnect(_Observer<Events...>* pObs_ = nullptr)
    : pObs(pObs_)
    {}

    ObserverConnect& reset(_Observer<Events...>* pObs_)
    { pObs = pObs_; return *this; }

    ~ObserverConnect() = default;

};


}//close Observer


///////////////////////////////
#  undef _SCRATCH_OBSERVER_INCOMPLETE
#else
#  ifdef _SCRATCH_OBSERVER_INCOMPLETE
#    error circular inclusion of observer.hpp
#  endif
#endif
