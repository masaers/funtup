#ifndef FUNTUP_HPP
#define FUNTUP_HPP
///
/// \file
///
/// \brief Provides the oportunity to bundle several functors into a
/// singel object.
///
/// \author Markus Saers
///
#include <tuple>
#include <type_traits>
#include <functional>

///
/// \namespace funtup
///
/// \brief This namespace provides the capability to group functors in
/// two ways: as pipes and as batteries.
///
/// A pipe chains the functors back to back so that a function
/// is called with the output of the previous function (the first
/// function is called with the provided arguments). The return value
/// of the <code>pipe</code> function is the return value of the last
/// funtion in the pipe.
///
/// A battery calles a number of functions with the same arguments,
/// and returns all the return values as a tuple. So if
/// <code>add</code> and <code>mul</code> are rolled into a battery
/// and called with <code>2</code> and <code>3</code>, the return
/// value would be a tuple containing <code>5</code> and
/// <code>6</code>. The order the arguments are passed into the
/// different functions is undefined, so passing non-constant
/// references as arguments to functions that modify them is risque.
///
namespace funtup {

  ///
  /// The type corresponding to a void return type.
  ///
  struct void_t {};

  ///
  /// \name Meta functions to represent and generate sequences of indices
  ///
  /// This is hugely useful when unpacking with <code>...</code>, as
  /// the sequence can be used to represent the range of indices.
  /// Thanks to Johannes Schaub who unlocked the workings of this in
  /// a stackoverflow answer [<a
  /// href="http://stackoverflow.com/questions/7858817/unpacking-a-tuple-to-call-a-matching-function-pointer">html<a>].
  ///
  /// \{
  // ------------------------------------------------------------------------ //
  ///
  /// \brief A sequence of indice.
  ///
  template<int...> struct seq {};
  ///
  /// \brief Iterative case for generating a sequence of indices.
  ///
  template<int N, int... S> struct gen_seq : gen_seq<N-1, N-1, S...> {};
  ///
  /// \brief Base-case for generating a sequence of indices.
  ///
  template<int... S> struct gen_seq<0, S...> { typedef seq<S...> type; };
  ///
  /// \brief Constructs a sequence if indices for a variadic pack of
  /// tempalte parameters.
  ///
  template<typename... T>
  inline auto make_seq() -> typename gen_seq<sizeof...(T)>::type {
    return typename gen_seq<sizeof...(T)>::type();
  }
  ///
  /// \brief Constructs a sequence of indices for a tuple.
  ///
  template<typename... T>
  inline auto make_seq(const std::tuple<T...>&) -> typename gen_seq<sizeof...(T)>::type {
    return typename gen_seq<sizeof...(T)>::type();
  }
  // ------------------------------------------------------------------------ //
  /// \}


  
  namespace funtup_helper {
    ///
    /// \brief Meta functions that deterins whether the result of
    /// applying a function to a list of parameters results in
    /// <code>void</code> or not.
    ///
    template<typename Func, typename... Args>
    struct returns_void
      : public std::is_void<typename std::result_of<Func(Args...)>::type>
    {};
    ///
    /// \brief Applies the other parameters to the first parameter and
    /// returns a storable result.
    ///
    /// This version applies to functions that would normally return
    /// <code>void</code>, but returns the storable type
    /// <code>void_t</code> to protect the caller agains receiving an
    /// unstorable result.
    ///
    template<typename Func, typename... Args>
    inline typename std::enable_if<returns_void<Func, Args...>::value, void_t>::type
    _apply_novoid(Func&& func, Args&&... args) {
      func(std::forward<Args>(args)...);
      return void_t();
    }
    ///
    /// \brief Applies the othe parameters to the first parameter and
    /// returns a storable result.
    ///
    /// This version just returns whatever the called function returns
    /// (since it is not void it is storable).
    ///
    template<typename Func, typename... Args>
    inline typename std::enable_if<! returns_void<Func, Args...>::value,
				   typename std::result_of<Func(Args...)>::type>::type
    _apply_novoid(Func&& func, Args&&... args) {
      return func(std::forward<Args>(args)...);
    }
    ///
    /// \brief Function that applies a tuple of functions to the other
    /// parameters and returns the results as a tuple.
    ///
    template<typename Funcs, typename... Args, int... I>
    inline auto
    _apply_tuple(Funcs&& funcs,
		 seq<I...> funcs_s,
		 Args&&... args) ->
    decltype(std::make_tuple(_apply_novoid(std::get<I>(funcs),
					   std::forward<Args>(args)...)...)) {
      return std::make_tuple(_apply_novoid(std::get<I>(funcs),
					   std::forward<Args>(args)...)...);
    }
    ///
    /// \name Piped function object
    ///
    /// Handles all the messiness of dealing with piped functions.
    ///
    /// \{
    // ---------------------------------------------------------------------- //
    ///
    /// Declaration.
    ///
    template<typename... Funcs> class pipe_t;
    ///
    /// Base case.
    ///
    template<typename Head>
    class pipe_t<Head> {
    public:
      inline pipe_t(Head&& head) : head_m(std::forward<Head>(head)) {}
      template<typename... Args>
      inline typename std::result_of<Head(Args&&...)>::type
      operator()(Args&&... args) const {
	return head_m(std::forward<Args>(args)...);
      }
    private:
      Head head_m;
    }; // pipe_t<Head>
    ///
    /// Inductive case.
    ///
    template<typename Head, typename... Tails>
    class pipe_t<Head, Tails...> : public pipe_t<Tails...> {
    public:
      inline pipe_t(Head&& head, Tails&&... tails)
	: pipe_t<Tails...>(std::forward<Tails>(tails)...)
	, head_m(std::forward<Head>(head))
      {}
      template<typename... Args>
      inline typename std::result_of<pipe_t<Tails...>(typename std::result_of<Head(Args&&...)>::type)>::type
      operator()(Args&&... args) const {
	return pipe_t<Tails...>::operator()(head_m(std::forward<Args>(args)...));
      }
    private:
      Head head_m;
    }; // pipe_t<Head, Tails...>
    // ---------------------------------------------------------------------- //
    /// \}


    ///
    /// \name Composed function object
    ///
    /// Handles all the messiness of dealing with composed functions.
    ///
    /// \{
    // ---------------------------------------------------------------------- //
    ///
    /// Declaration.
    ///
	template<typename... Funcs> class compose_t;
	///
	/// Base case
	///
	template<typename Head>
	class compose_t<Head> {
    public:
	  inline compose_t(Head&& head) : head_m(std::forward<Head>(head)) {}
	  template<typename... Args>
	  inline typename std::result_of<Head(Args&&...)>::type
	  operator()(Args&&... args) const {
	    return head_m(std::forward<Args>(args)...);
	  }
	private:
	  Head head_m;
	}; // compose_t<Head>
	///
	/// Inductive case
	///
	template<typename Head, typename... Tails>
	class compose_t<Head, Tails...> : public compose_t<Tails...> {
	public:
	  inline compose_t(Head&& head, Tails&&... tails)
	    : compose_t<Tails...>(std::forward<Tails>(tails)...)
	    , head_m(std::forward<Head>(head))
      {}
      template<typename... Args>
      inline typename std::result_of<Head(typename std::result_of<compose_t<Tails...>(Args&&...)>::type)>::type
      operator()(Args&&... args) const {
        return head_m(compose_t<Tails...>::operator()(std::forward<Args>(args)...));
      }
	private:
	  Head head_m;
	}; // compose_t<Head, Tails...>
    // ---------------------------------------------------------------------- //
    /// \}
	
	
    ///
    /// \name Automatic unpacking of function parameters.
    ///
    /// Whever a single tuple is passed in as parameter, it is
    /// automatically unpacked and the content is forwarded to the
    /// wrapped function as parameters. Any other configuration of
    /// parameters is forwarded to the wrapped function as is.
    ///
    /// \{
    // ---------------------------------------------------------------------- //
    ///
    /// A function that unpacks its second argument and calls its
    /// first argument with the unpacked parameter list. The third
    /// parameter is needed for unpacking the second.
    ///
    template<typename Func, typename Args, int... I>
    inline auto
    unpack_and_apply(/// The function to call
		     Func&& func,
		     /// A packed representation of the parameters to
		     /// call the function with
		     Args&& args,
		     /// An index sequence needed to unpack the
		     /// parameters
		     seq<I...> args_s) ->
    decltype(func(std::get<I>(args)...)) {
      return func(std::get<I>(args)...);
    }
    ///
    /// A wrapper for a function to provide the automatic unpacking of
    /// a single tuple into a parameter list.
    ///
    template<typename Func>
    class apply_unpack_t {
    public:
      inline apply_unpack_t(Func&& func)
	: func_m(std::forward<Func>(func))
      {}
      template<typename... Args>
      inline auto operator()(Args&&... args) const ->
      decltype(std::declval<Func>()(std::forward<Args>(args)...)) {
	return func_m(std::forward<Args>(args)...);
      }
      template<typename... Args>
      inline auto operator()(std::tuple<Args...>& args) const ->
      decltype(unpack_and_apply(std::declval<Func>(), args, make_seq(args))) {
	return unpack_and_apply(func_m, args, make_seq(args));
      }
      template<typename... Args>
      inline auto operator()(const std::tuple<Args...>& args) const ->
      decltype(unpack_and_apply(std::declval<Func>(), args, make_seq(args))) {
	return unpack_and_apply(func_m, args, make_seq(args));
      }
      template<typename... Args>
      inline auto operator()(std::tuple<Args...>&& args) const ->
      decltype(unpack_and_apply(std::declval<Func>(), args, make_seq(args))) {
	return unpack_and_apply(func_m, args, make_seq(args));
      }
    private:
      Func func_m;
    }; // apply_unpack_t
    // ---------------------------------------------------------------------- //
    /// \}
    
    
  } // namespace funtup_helper
  
  
  ///
  /// \brief Appplies the other parameters to the first parameter and
  /// returns the result.
  ///
  template<typename Func, typename... Args>
  inline auto
  apply(Func&& func, Args&&... args) ->
  decltype(func(std::forward<Args>(args)...)) {
    return func(std::forward<Args>(args)...);
  }
  
  ///
  /// \brief Applies a tuple of functions to the other parameters, and
  /// returns a corresponding tuple of results.
  ///
  /// This particular implementation calles the functions in the order
  /// they are in in the tuple.
  ///
  template<typename Funcs, typename... Args>
  inline auto
  apply_tuple(Funcs&& funcs, Args&&... args) ->
  decltype(funtup_helper::_apply_tuple(std::forward<Funcs>(funcs),
				       make_seq(funcs),
				       std::forward<Args>(args)...)) {
    return funtup_helper::_apply_tuple(std::forward<Funcs>(funcs),
				       make_seq(funcs),
				       std::forward<Args>(args)...);
  }
  
  ///
  /// \brief Pipes a series of functors into one functor.
  ///
  /// <code>fgh = pipe(f, g, h)</code> results in the function
  /// <code>fgh</code>, which equivalent to
  /// <code>h(g(f(x)))</code>.
  ///
  /*!\code
    struct add3 { int operator()(int a) const { return a + 3; } };
    struct mul3 { int operator()(int a) const { return a * 3; } };
    auto c1 = pipe(add3(), mul3());
    auto c2 = pipe(mul3(), add3());
    std::cout << c1(2) << std::endl; // prints 15
    std::cout << c2(2) << std::endl; // prints 9
    \endcode*/
  template<typename... Funcs>
  inline funtup_helper::pipe_t<Funcs...>
  pipe(Funcs&&... funcs) {
    return funtup_helper::pipe_t<Funcs...>(std::forward<Funcs>(funcs)...);
  }

  template<typename... Funcs>
  inline funtup_helper::compose_t<Funcs...>
  compose(Funcs&&... funcs) {
    return funtup_helper::compose_t<Funcs...>(std::forward<Funcs>(funcs)...);
  }
  
  
  ///
  /// \brief Transforms a function so that it automatically unpacks a
  /// singel tuple into a list of parameters.
  ///
  /// This is very useful when a function that returns a tuple needs
  /// to be piped to a function taking multiple arguments.
  ///
  /*!\code
    struct add { int operator()(int a, int b) const { return a + b; } };
    std::tuple<int, int> divint(int a, int b) {
      return std::make_tuple(a / b, a % b);
    }
    auto c3 = pipe(&divint, auto_unpack(add()));
    assert(c3(5, 2) == 3);
    \endcode*/
  template<typename Func>
  inline funtup_helper::apply_unpack_t<Func>
  auto_unpack(Func&& func) {
    return funtup_helper::apply_unpack_t<Func>(std::forward<Func>(func));
  }
  
  namespace funtup_helper {
    ///
    /// A wrapper to group several functors into a single object so
    /// that they can all be called with the same parameters.
    ///
    template<typename... Funcs>
    struct battery_t : public std::tuple<Funcs...> {
      inline battery_t(Funcs&&... funcs)
	: std::tuple<Funcs...>(std::forward<Funcs>(funcs)...)
      {}
      template<typename... Args>
      inline auto
      operator()(Args&&... args) const ->
      decltype(apply_tuple(std::declval<battery_t>(), std::forward<Args>(args)...)) {
	return apply_tuple(*this, std::forward<Args>(args)...);
      }
    };
  } // namespace funtup_helper
  
  ///
  /// Builds a functor from several functors, which will all be
  /// applied to the arguments passed to the built functor and return
  /// the resulting tuple of return values.
  ///
  /*!\code
    struct add { int operator()(int a, int b) const { return a + b; } };
    struct mul { int operator()(int a, int b) const { return a * b; } };
    auto b = battery(add(), mul());
    std::tuple<int, int> r = b(3, 4);
    std::cout << std::get<0>(r) << std::endl; // prints 7
    std::cout << std::get<1>(r) << std::endl; // prints 12
  \endcode*/
  template<typename... Funcs>
  inline funtup_helper::battery_t<Funcs...>
  battery(Funcs&&... funcs) {
    return funtup_helper::battery_t<Funcs...>(std::forward<Funcs>(funcs)...);
  }

  ///
  /// A function that makes a copy of whatever is passed in.
  ///
  /// This is useful when you want a battery or pipe to assume
  /// ownership of a function.
  ///
  template<typename T>
  typename std::decay<T>::type clone(T&& x) {
    return std::decay<T>::type(std::forward<T>(x));
  }

  
} // namespace funtup
#endif

