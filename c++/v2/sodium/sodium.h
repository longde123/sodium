/**
 * Copyright (c) 2012-2015, Stephen Blackheath and Anthony Jones
 * Released under a BSD3 licence.
 *
 * C++ implementation courtesy of International Telematics Ltd.
 */
#ifndef _SODIUM2_SODIUM_H_
#define _SODIUM2_SODIUM_H_

#include <sodium/impl/common.h>
#include <sodium/impl/magic_ref.h>
#include <sodium/transaction.h>
#include <forward_list>
#include <stdexcept>

namespace SODIUM_NAMESPACE {
    namespace impl {
        template <class A>
        struct stream_impl {
            stream_impl() : node(node_t(0)) {}
            std::forward_list<impl::magic_ref<std::function<void()>>> finalizers;
            impl::magic_ref<node_t> node;
            std::forward_list<magic_ref<A>> firings;
        };
    }

    template <class A>
    class stream {
    protected:
        impl::magic_ref<impl::stream_impl<A>> impl;

    public:
        stream() : impl(impl::stream_impl<A>()) {}
        virtual ~stream() {}
        std::function<void()> listen(const std::function<void(const A&)>& action) const {
            return listen_(impl::node_t::null, [action] (const transaction&, const void* v) {
                action(*(const A*)v);
            });
        }
    protected:
        std::function<void()> listen_(const impl::magic_ref<impl::node_t>& target,
                const std::function<void(const transaction&, const void*)>& action) const {
            transaction trans;
            return listen(target, trans, action, false);
        }
        std::function<void()> listen(const impl::magic_ref<impl::node_t>& target,
                const transaction& trans,
                const std::function<void(const transaction&, const void*)>& action0,
                bool suppress_earlier_firings) const {
            impl::magic_ref<std::function<void(const transaction& trans, const void*)>> action(action0);
            transaction::listeners_lock.lock();
            if (link_to(impl->node, action, target))
                trans.impl->to_regen = true;
            transaction::listeners_lock.unlock();
            // reverse the firings list when we copy it, because we've stored it in reverse order.
            std::forward_list<impl::magic_ref<A>> firings;
            for (auto it = impl->firings.begin(); it != impl->firings.end(); ++it)
                firings.push_front(*it);
            if (!suppress_earlier_firings && firings.begin() != firings.end()) { 
                trans.prioritized(target, [firings, action] (const transaction& trans) {
                    for (auto it = firings.begin(); it != firings.end(); ++it) {
                        transaction::in_callback++;
                        try {
                            action.get()(trans, (const void*)&it->get());
                            transaction::in_callback--;
                        }
                        catch (...) {
                            transaction::in_callback--;
                            throw;
                        }
                    }
                });
            }
            auto impl(this->impl);
            return [impl, target] () {
                transaction::listeners_lock.lock();
                unlink_to(impl->node, target);
                transaction::listeners_lock.unlock();
            };
        }
    };

    template <class A>
    class stream_with_send : public stream<A> {
    public:
        stream_with_send() {}
        virtual ~stream_with_send() {}
    protected:
        void send(const transaction& trans, const A& a) const {
            auto impl(this->impl);
            if (impl->firings.begin() == impl->firings.end()) {
                trans.last([impl] () {
                    impl.unsafe_get().firings.clear();
                });
            }
            impl.unsafe_get().firings.push_front(impl::magic_ref<A>(a));

            transaction::listeners_lock.lock();
            std::vector<impl::node_t::target_t> listeners(impl->node->listeners);
            transaction::listeners_lock.unlock();
            for (auto it = listeners.begin(); it != listeners.end(); ++it) {
                auto action = it->action;
                trans.prioritized(it->node, [action, a] (const transaction& trans) {
                    transaction::in_callback++;
                    try {
                        action.get()(trans, (const void*)&a);
                        transaction::in_callback--;
                    }
                    catch (...) {
                        transaction::in_callback--;
                        throw;
                    }
                });
            }
        }
    };

    template <class A>
    struct stream_sink : stream_with_send<A> {
        stream_sink() {}
        virtual ~stream_sink() {}

        void send(const A& a) const {
            transaction trans;
            if (transaction::in_callback > 0)
                throw std::runtime_error("You are not allowed to use send() inside a Sodium callback");
            stream_with_send<A>::send(trans, a);
        }
	};

#if 0
    /**
     * Transform the event's value according to the supplied function.
     */
	public final <B> Stream<B> map(final Lambda1<A,B> f)
	{
	    final Stream<A> ev = this;
	    final StreamSink<B> out = new StreamSink<B>();
        Listener l = listen_(out.node, new TransactionHandler<A>() {
        	public void run(Transaction trans2, A a) {
	            out.send(trans2, f.apply(a));
	        }
        });
        return out.addCleanup(l);
	}

	/**
	 * Create a behavior with the specified initial value, that gets updated
     * by the values coming through the event. The 'current value' of the behavior
     * is notionally the value as it was 'at the start of the transaction'.
     * That is, state updates caused by event firings get processed at the end of
     * the transaction.
     */
	public final Cell<A> hold(final A initValue) {
		return Transaction.apply(new Lambda1<Transaction, Cell<A>>() {
			public Cell<A> apply(Transaction trans) {
			    return new Cell<A>(lastFiringOnly(trans), initValue);
			}
		});
	}

	final Cell<A> holdLazy(final Lambda0<A> initValue) {
		return Transaction.apply(new Lambda1<Transaction, Cell<A>>() {
			public Cell<A> apply(Transaction trans) {
			    return new LazyCell<A>(lastFiringOnly(trans), initValue);
			}
		});
	}

	/**
	 * Variant of snapshot that throws away the event's value and captures the behavior's.
	 */
	public final <B> Stream<B> snapshot(Cell<B> beh)
	{
	    return snapshot(beh, new Lambda2<A,B,B>() {
	    	public B apply(A a, B b) {
	    		return b;
	    	}
	    });
	}

	/**
	 * Sample the behavior at the time of the event firing. Note that the 'current value'
     * of the behavior that's sampled is the value as at the start of the transaction
     * before any state changes of the current transaction are applied through 'hold's.
     */
	public final <B,C> Stream<C> snapshot(final Cell<B> b, final Lambda2<A,B,C> f)
	{
	    final Stream<A> ev = this;
		final StreamSink<C> out = new StreamSink<C>();
        Listener l = listen_(out.node, new TransactionHandler<A>() {
        	public void run(Transaction trans2, A a) {
	            out.send(trans2, f.apply(a, b.sampleNoTrans()));
	        }
        });
        return out.addCleanup(l);
	}

    /**
     * Merge two streams of events of the same type.
     *
     * In the case where two event occurrences are simultaneous (i.e. both
     * within the same transaction), both will be delivered in the same
     * transaction. If the event firings are ordered for some reason, then
     * their ordering is retained. In many common cases the ordering will
     * be undefined.
     */
	public Stream<A> merge(final Stream<A> eb)
	{
	    return Stream.<A>merge(this, eb);
	}

    /**
     * Merge two streams of events of the same type.
     *
     * In the case where two event occurrences are simultaneous (i.e. both
     * within the same transaction), both will be delivered in the same
     * transaction. If the event firings are ordered for some reason, then
     * their ordering is retained. In many common cases the ordering will
     * be undefined.
     */
	private static <A> Stream<A> merge(final Stream<A> ea, final Stream<A> eb)
	{
	    final StreamSink<A> out = new StreamSink<A>();
        final Node left = new Node(0);
        final Node right = out.node;
        left.linkTo(null, right);
        TransactionHandler<A> h = new TransactionHandler<A>() {
        	public void run(Transaction trans, A a) {
	            out.send(trans, a);
	        }
        };
        Listener l1 = ea.listen_(left, h);
        Listener l2 = eb.listen_(right, h);
        return out.addCleanup(l1).addCleanup(l2).addCleanup(new Listener() {
            public void unlisten() {
                left.unlinkTo(right);
            }
        });
	}

	/**
	 * Push this event occurrence onto a new transaction. Same as split() but works
     * on a single value.
	 */
	public final Stream<A> defer()
	{
	    final StreamSink<A> out = new StreamSink<A>();
	    Listener l1 = listen_(out.node, new TransactionHandler<A>() {
	        public void run(Transaction trans, final A a) {
	            trans.post(new Runnable() {
                    public void run() {
                        Transaction trans = new Transaction();
                        try {
                            out.send(trans, a);
                        } finally {
                            trans.close();
                        }
                    }
	            });
	        }
	    });
	    return out.addCleanup(l1);
	}

	/**
	 * Push each event occurrence in the list onto a new transaction.
	 */
    public static final <A, C extends Collection<A>> Stream<A> split(Stream<C> s)
    {
	    final StreamSink<A> out = new StreamSink<A>();
	    Listener l1 = s.listen_(out.node, new TransactionHandler<C>() {
	        public void run(Transaction trans, final C as) {
	            trans.post(new Runnable() {
                    public void run() {
                        for (A a : as) {
                            Transaction trans = new Transaction();
                            try {
                                out.send(trans, a);
                            } finally {
                                trans.close();
                            }
                        }
                    }
	            });
	        }
	    });
	    return out.addCleanup(l1);
    }

    /**
     * If there's more than one firing in a single transaction, combine them into
     * one using the specified combining function.
     *
     * If the event firings are ordered, then the first will appear at the left
     * input of the combining function. In most common cases it's best not to
     * make any assumptions about the ordering, and the combining function would
     * ideally be commutative.
     */
	public final Stream<A> coalesce(final Lambda2<A,A,A> f)
	{
	    return Transaction.apply(new Lambda1<Transaction, Stream<A>>() {
	    	public Stream<A> apply(Transaction trans) {
	    		return coalesce(trans, f);
	    	}
	    });
	}

	final Stream<A> coalesce(Transaction trans1, final Lambda2<A,A,A> f)
	{
	    final Stream<A> ev = this;
	    final StreamSink<A> out = new StreamSink<A>();
        TransactionHandler<A> h = new CoalesceHandler<A>(f, out);
        Listener l = listen(out.node, trans1, h, false);
        return out.addCleanup(l);
    }

    /**
     * Clean up the output by discarding any firing other than the last one. 
     */
    final Stream<A> lastFiringOnly(Transaction trans)
    {
        return coalesce(trans, new Lambda2<A,A,A>() {
        	public A apply(A first, A second) { return second; }
        });
    }

    /**
     * Merge two streams of events of the same type, combining simultaneous
     * event occurrences.
     *
     * In the case where multiple event occurrences are simultaneous (i.e. all
     * within the same transaction), they are combined using the same logic as
     * 'coalesce'.
     */
    public Stream<A> merge(Stream<A> eb, Lambda2<A,A,A> f)
    {
        return merge(eb).coalesce(f);
    }

    /**
     * Only keep event occurrences for which the predicate returns true.
     */
    public final Stream<A> filter(final Lambda1<A,Boolean> f)
    {
        final Stream<A> ev = this;
        final StreamSink<A> out = new StreamSink<A>();
        Listener l = listen_(out.node, new TransactionHandler<A>() {
        	public void run(Transaction trans2, A a) {
	            if (f.apply(a)) out.send(trans2, a);
	        }
        });
        return out.addCleanup(l);
    }

    /**
     * Filter out any event occurrences whose value is a Java null pointer.
     */
    public final Stream<A> filterNotNull()
    {
        return filter(new Lambda1<A,Boolean>() {
        	public Boolean apply(A a) { return a != null; }
        });
    }

    /**
     * Filter the empty values out, and strip the Optional wrapper from the present ones.
     */
    public static final <A> Stream<A> filterOptional(final Stream<Optional<A>> ev)
    {
        final StreamSink<A> out = new StreamSink<A>();
        Listener l = ev.listen_(out.node, new TransactionHandler<Optional<A>>() {
        	public void run(Transaction trans2, Optional<A> oa) {
	            if (oa.isPresent()) out.send(trans2, oa.get());
	        }
        });
        return out.addCleanup(l);
    }

    /**
     * Let event occurrences through only when the behavior's value is True.
     * Note that the behavior's value is as it was at the start of the transaction,
     * that is, no state changes from the current transaction are taken into account.
     */
    public final Stream<A> gate(Cell<Boolean> bPred)
    {
        return snapshot(bPred, new Lambda2<A,Boolean,A>() {
        	public A apply(A a, Boolean pred) { return pred ? a : null; }
        }).filterNotNull();
    }

    /**
     * Transform an event with a generalized state loop (a mealy machine). The function
     * is passed the input and the old state and returns the new state and output value.
     */
    public final <B,S> Stream<B> collect(final S initState, final Lambda2<A, S, Tuple2<B, S>> f)
    {
        return Transaction.<Stream<B>>run(() -> {
            final Stream<A> ea = this;
            StreamLoop<S> es = new StreamLoop<S>();
            Cell<S> s = es.hold(initState);
            Stream<Tuple2<B,S>> ebs = ea.snapshot(s, f);
            Stream<B> eb = ebs.map(new Lambda1<Tuple2<B,S>,B>() {
                public B apply(Tuple2<B,S> bs) { return bs.a; }
            });
            Stream<S> es_out = ebs.map(new Lambda1<Tuple2<B,S>,S>() {
                public S apply(Tuple2<B,S> bs) { return bs.b; }
            });
            es.loop(es_out);
            return eb;
        });
    }

    /**
     * Accumulate on input event, outputting the new state each time.
     */
    public final <S> Cell<S> accum(final S initState, final Lambda2<A, S, S> f)
    {
        return Transaction.<Cell<S>>run(() -> {
            final Stream<A> ea = this;
            StreamLoop<S> es = new StreamLoop<S>();
            Cell<S> s = es.hold(initState);
            Stream<S> es_out = ea.snapshot(s, f);
            es.loop(es_out);
            return es_out.hold(initState);
        });
    }

    /**
     * Throw away all event occurrences except for the first one.
     */
    public final Stream<A> once()
    {
        // This is a bit long-winded but it's efficient because it deregisters
        // the listener.
        final Stream<A> ev = this;
        final Listener[] la = new Listener[1];
        final StreamSink<A> out = new StreamSink<A>();
        la[0] = ev.listen_(out.node, new TransactionHandler<A>() {
        	public void run(Transaction trans, A a) {
	            if (la[0] != null) {
                    out.send(trans, a);
	                la[0].unlisten();
	                la[0] = null;
	            }
	        }
        });
        return out.addCleanup(la[0]);
    }

    Stream<A> addCleanup(Listener cleanup)
    {
        finalizers.add(cleanup);
        return this;
    }

	@Override
	protected void finalize() throws Throwable {
		for (Listener l : finalizers)
			l.unlisten();
	}
#endif

#if defined(SODIUM_OLD_NAME_COMPATIBILITY)
    template <class A>
    struct event : stream<A> {
        event() {}
        event(const stream<A>& other) : stream<A>(other) {}
        virtual ~event() {}
    };
    template <class A>
    struct event_sink : stream_sink<A> {
        event_sink() {}
        event_sink(const stream_sink<A>& other) : stream_sink<A>(other) {}
        virtual ~event_sink() {}
    };
#endif

}  // end namespace SODIUM_NAMESPACE

#endif

