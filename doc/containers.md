# ObserverCpp / doc / containers

Subjects use set containers, while Observers may optionally use map containers.

While the default implementations are `std::unordered_set` and `std::unordered_map`, ObserverCpp makes it easy to use different container strategies.

Indeed, ObserverCpp is defined in terms of abstract set and map operations, which one just needs to implement.

Ownership of the actual container is not enforced either. Views of unowned containers are enough to work. However owning the data
makes more sense in general and is the default.


## Abstract Views

ObserverCpp comes with the `AbstractSet`, `AbstractMap` and `AbstractConstMap` views. These views define the interface needed to implement
Subject/Observer operations. 

    template <typename T>
    struct AbstractSet
    {
        std::function<bool(T)> Append;
        std::function<bool(T)> Remove;
        using F = std::function<void(T)>;
        std::function<void(F)> Signal;
    };

`AbstractSet` has edit `Append`/`Remove` operations and one `Signal` internal iterator. The first two return a success flag (true is success). 
`Signal` takes in a function to be evaluated over all elements in the set.

    template <typename K, typename V>
    struct AbstractMap
    {
        std::function<void(K,V)> Define; 
        std::function<bool(K)> Remove;
        std::function<V(K)> Lookup;
        using F = std::function<void(K,V)>;
        std::function<void(F)> Signal;
    };

`AbstractMap` is quite similar. The edit operations are now `Define`/`Remove` where `Define` is a append-or-edit operation, always succeeding.
The `Lookup` operation returns the value associated with a particular key. The `Signal` operation is just as before, now iterating over key-value pairs.

    template <typename K, typename V>
    struct AbstractConstMap
    {
        std::function<V(K)> Lookup;
        using F = std::function<void(K,V)>;
        std::function<void(F)> Signal;
    };

`AbstractConstMap` slices the `AbstractMap` down the its read-only operations. It may be used instead of an `AbstractMap` should subject-management be
trivial or handled externally.

Abstract views can be generated for the default container implementations by using the following functions.

    template <typename T>
    AbstractSet<T> abstract_set_view(std::unordered_set<T>& set)

    template <typename K, typename V>
    AbstractMap<K,V> abstract_map_view(std::unordered_map<K,V>& map)

    template <typename K, typename V>
    AbstractConstMap<K,V> abstract_const_map_view(std::unordered_map<K,V> const& map)


Container backends are not always necessary for Observers and can be adjusted on a per-event basis.
However they are always necessary for Subjects. To accomodate the case when Subjects need to support many events while mostly reusing the same container
strategies (a single one?), a batch `AbstractSet` factory function is also provided. The result is a composite view that can be passed on to a Subject
interface.

    template <typename ... Ts>
    struct AbstractSetTuple;

As a user, it is not useful to know its interface although it is very simple. To batch-construct `AbstractSet`s using the default `abstract_set_view`
constructor, one uses any of the following.
(The second forwards to the first.)

    template <typename ... Ts>
    AbstractSetTuple<Ts...> abstract_set_tuple_view(std::unordered_set<Ts>& ... sets);

    template <typename ... Cs>
    auto abstract_set_tuple_view(std::tuple<Cs...>& tset);

To use other constructor functions, one needs to package them in a visitor struct, and pass them along as a template parameter. For instance,

    struct CustomCtors
    {
        template <typename T>
        AbstractSet<T> operator()(std::set<T>& set);

        template <typename T>
        AbstractSet<T> operator()(std::list<T>& set);
    };

    auto TSet = abstract_set_tuple_view<CustomCtors>(setA,setB,listC);
        
Note that custom container types need to expose the `value_type` typedef to use this facility.

## Abstract Container Data

Wrappers, instead of views, of containers are similarly constructed. Instead of taking an lvalue reference, they take an rvalue reference
because the container is moved in.

    template <typename T>
    AbstractSetData<T> abstract_set_data(std::unordered_set<T>&& set);
    
    template <typename K, typename V>
    AbstractMapData<K,V> abstract_map_data(std::unordered_map<K,V>&& map)

    template <typename K, typename V>
    AbstractConstMapData<K,V> abstract_const_map_data(std::unordered_map<K,V>&& map)

The above types store the container internally as an opaque type, while at the same time manufacturing a view bound to it. 
The default view generators are used, by default. 
If one wants to use different view generators, or add support for new
containers, one again passes the view generator as a template parameter; for instance,

    struct CustomViewCtor
    {
        template <typename T>
        AbstractSet<T> operator()(std::set<T>& set);
    };

    auto Set = abstract_set_data<CustomViewCtor>(std::set<T>());

The similar features exist for `abstract_map_data` and `abstract_const_map_data`.

Finally, batch-construction of `AbstractSetData` objects is likewise supported, by using `abstract_set_tuple_data`.
Here again, the default view generators are used unless view generator policies packaged in a struct are passed along as a template parameter.

