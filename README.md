# ObserverCpp

ObserverCpp is an implementation of the popular [Observer](https://en.wikipedia.org/wiki/Observer_pattern) design pattern in C++.

In this pattern, Subjects expose interface for Observers to register and get instant notification of change. 


## Usage

Subject events are modeled as trivially copyable data of a certain type. For instance,

    struct WindowGetsResized
    {
        int x,y;
    };

A Subject exposing events `A` and `B` owns a resource `_Subject<A,B>` and forwards the interface necessary for Observers of `A`, `B` or both to attach
and detach.

    class BenevolentSubject
    {
        private:
        std::unique_ptr<_Subject<A,B>> pSub;

        public:
        //custom subject interface forwarding to pSub

    };

Or it may just inherit the base mixin `Subject<A,B>` to expose the conventional interface.

    class BenevolentSubject
    : public Subject<A,B>   //holds _Subject<A,B> internally
    {
        ...
    };

Then the Subject needs to install the `Notify` calls in its own code as appropriate.

An Observer interested in event `A` and some other event `C` owns a resource `_Observer<A,C>` from which messages may come in once it has subscribed
to Subjects. 

    class InterestedObserver
    {
        private:
        std::unique_ptr<_Observer<A,C>> pObs;

        public:
        //possibly, nothing

    };

Then it needs to install on its resource what happens when event `A` or `C` come along.
In most cases, this is it:

    pObs->bindHandlerSubject1(A(),h);

Here `h` is any callable taking an `A` argument.
When a designated Subject is passed as an extra argument, the Observer will automatically detach from this Subject when destroyed.

If an Observer wants to track several Subjects on the same event, and automatically detach from all of them when destroyed, 
it needs to back against a map container to store these identities. It also makes it possible to define behaviour on a per-Subject basis.

    pObs->bindSubjectHandlers<default_map_container>(A());

Then, it might use

    pObs->Define(A(),subjectId-A-2,h);

to create or reset behaviour for `subjectId-A-2` on event `A`, or

    pObs->Remove(A(),subjectId-A-2);

to clean up the entry after successfully detached.

Indeed, Subject and Observer abstract identities may be used on all Subject or Observer methods if it is so wished not the disclose the address of
the respective resources.

For example, a `SubjectID<A>` object contains the address of a `_Subject` resource, which only `_Observer<A>` resources can read and use. 


## Installation

ObserverCpp is a header-only library. It requires C++17.


## Tests

The tests use [doctest](https://github.com/doctest/doctest), a header-only testing framework.


## Container Backends

ObserverCpp acts as a thin translation layer on top of abstract container interface. As such, container policies can be fully customized.

The default backend for Subjects is `std::unordered_set` and the default backend for multi-subject auto-detaching Observers is `std::unordered_map`.

A backend is either owned or unowned by the resource. When it is unowned, the resource works on an abstract view of the container, 
while the actual container remains managed by the resource host. When it is owned, the abstract view becomes an abstract *wrapper*, carrying the same
interface, yet holding the actual data as an opaque internal.

To give a taste of how easy it is to support custom backends, here is what defines the abstract set interface in use (this is the view version):

    template <typename T>
    struct AbstractSet
    {
        std::function<bool(T)> Append;
        std::function<bool(T)> Remove;
        using F = std::function<void(T)>;
        std::function<void(F)> Signal;
    };

The `Signal` method abstracts an iterator over all elements, taking in a visitor function.

Then, the support for the default `std::unordered_set` is just this:

    template <typename T>
    AbstractSet<T> abstract_set_view(std::unordered_set<T>& set)
    {
        auto append = [&set](T t){ return set.insert(t).second; };
        auto remove = [&set](T t){ return set.erase(t)==1; };
        using F = std::function<void(T)>;
        auto signal = [&set](F f){ for(auto t : set) f(t); };
        return {append,remove,signal};
    };

(The wrapper version uses this factory method too.)


## Related Work

An object-oriented implementation of the Observer pattern in C++ was presented by Gamma et al. in their book 
"Design Patterns: Elements of Reusable Object-Oriented Software".

[Signals and Slots](https://en.wikipedia.org/wiki/Signals_and_slots) are a popular primitive to implement Observer patterns in C++.
Signals are basically function containers. They go to the bare-bone abstraction of active notification: passing a piece of data along to many functions.

Our implementation stands in between. While the Observer role is mostly informal in Signals and Slots, it is restated here as a point of indirection
between subscription and behaviour. Then the idea of dynamic callbacks is completely moved into the Observer.


## Documentation

In wait of a full documentation, this [companion article]() on my blog should make the code easy to understand and use.


## Future Work

- formal documentation
- thread-safety
