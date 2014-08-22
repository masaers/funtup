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
  template<typename... _T>
  inline auto make_seq() -> typename gen_seq<sizeof...(_T)>::type {
    return typename gen_seq<sizeof...(_T)>::type();
  }
  ///
  /// \brief Constructs a sequence of indices for a tuple.
  ///
  template<typename... _T>
  inline auto make_seq(const std::tuple<_T...>&) -> typename gen_seq<sizeof...(_T)>::type {
    return typename gen_seq<sizeof...(_T)>::type();
  }
  // ------------------------------------------------------------------------ //
  /// \}


  
  namespace funtup_helper {
    ///
    /// \brief Meta functions that deterins whether the result of
    /// applying a function to a list of parameters results in
    /// <code>void</code> or not.
    ///
    template<typename func_T, typename... args_T>
    struct returns_void
      : public std::is_void<typename std::result_of<func_T(args_T...)>::type>
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
    template<typename func_T, typename... args_T>
    inline typename std::enable_if<returns_void<func_T, args_T...>::value,
				   void_t>::type
    _apply_novoid(func_T&& func, args_T&&... args) {
      func(std::forward<args_T>(args)...);
      return void_t();
    }
    ///
    /// \brief Applies the othe parameters to the first parameter and
    /// returns a storable result.
    ///
    /// This version just returns whatever the called function returns
    /// (since it is not void it is storable).
    ///
    template<typename func_T, typename... args_T>
    inline typename std::enable_if<! returns_void<func_T, args_T...>::value,
				   typename std::result_of<func_T(args_T...)>::type>::type
    _apply_novoid(func_T&& func, args_T&&... args) {
      return func(std::forward<args_T>(args)...);
    }
    ///
    /// \brief Function that applies a tuple of functions to the other
    /// parameters and returns the results as a tuple.
    ///
    template<typename funcs_T, typename... args_T, int... funcs_S>
    inline auto
    _apply_tuple(funcs_T&& funcs,
		 seq<funcs_S...> funcs_s,
		 args_T&&... args) ->
    decltype(std::make_tuple(_apply_novoid(std::get<funcs_S>(funcs),
					   std::forward<args_T>(args)...)...)) {
      return std::make_tuple(_apply_novoid(std::get<funcs_S>(funcs),
					   std::forward<args_T>(args)...)...);
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
    template<typename... funcs_T> class pipe_t;
    ///
    /// Base case.
    ///
    template<typename head_T>
    class pipe_t<head_T> {
    public:
      typedef head_T head_type;
      inline pipe_t() : head_m() {}
      inline pipe_t(head_T&& head) : head_m(std::forward<head_T>(head)) {}
      inline pipe_t(const pipe_t& x) : head_m(x.head_m) {}
      inline pipe_t(pipe_t&& x) : head_m(std::move(x.head_m)) {}
      inline pipe_t& operator=(pipe_t x) {
	head_m = std::move(x.head_m);
	return *this;
      }
      template<typename... args_T>
      inline typename std::result_of<head_T(args_T&&...)>::type
      operator()(args_T&&... args) const {
	return head_m(std::forward<args_T>(args)...);
      }
    private:
      head_type head_m;
    };
    ///
    /// Inductive case.
    ///
    template<typename head_T, typename... tails_T>
    class pipe_t<head_T, tails_T...> : public pipe_t<tails_T...> {
    public:
      typedef head_T head_type;
      inline pipe_t() : pipe_t<tails_T...>(), head_m() {}
      inline pipe_t(head_T&& head, tails_T&&... tails)
	: pipe_t<tails_T...>(std::forward<tails_T>(tails)...)
	, head_m(std::forward<head_T>(head))
      {}
      inline pipe_t(const pipe_t& x)
	: pipe_t<tails_T...>(x)
	, head_m(x.head_m)
      {}
      inline pipe_t(pipe_t&& x)
	: pipe_t<tails_T...>(x)
	, head_m(std::move(x.head_m))
      {}
      inline pipe_t& operator=(pipe_t x) {
	head_m = std::move(x.head_m);
	pipe_t<tails_T...>::operator=(std::move(x));
	return *this;
      }
      template<typename... args_T>
      inline typename std::result_of<pipe_t<tails_T...>(typename std::result_of<head_type(args_T&&...)>::type)>::type
      operator()(args_T&&... args) const {
	return pipe_t<tails_T...>::operator()(head_m(std::forward<args_T>(args)...));
      }
    private:
      head_type head_m;
    };
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
    template<typename func_T, typename args_T, int... args_S>
    inline auto
    unpack_and_apply(/// The function to call
		     const func_T& func,
		     /// A packed representation of the parameters to
		     /// call the function with
		     args_T&& args,
		     /// An index sequence needed to unpack the
		     /// parameters
		     seq<args_S...> args_s) ->
    decltype(func(std::get<args_S>(args)...)) {
      return func(std::get<args_S>(args)...);
    }
    ///
    /// A wrapper for a function to provide the automatic unpacking of
    /// a single tuple into a parameter list.
    ///
    template<typename func_T>
    class apply_unpack_t {
      //      typedef typename std::decayremove_cv<func_T>::type func_type;
      typedef func_T func_type;
    public:
      inline apply_unpack_t(func_T&& func)
	: func_m(std::forward<func_T>(func))
      {}
      template<typename... args_T>
      inline auto operator()(args_T&&... args) const ->
      decltype(std::declval<func_type>()(std::forward<args_T>(args)...)) {
	return func_m(std::forward<args_T>(args)...);
      }
      template<typename... args_T>
      inline auto operator()(std::tuple<args_T...>& args) const ->
      decltype(unpack_and_apply(std::declval<func_type>(), args, make_seq(args))) {
	return unpack_and_apply(func_m, args, make_seq(args));
      }
      template<typename... args_T>
      inline auto operator()(const std::tuple<args_T...>& args) const ->
      decltype(unpack_and_apply(std::declval<func_type>(), args, make_seq(args))) {
	return unpack_and_apply(func_m, args, make_seq(args));
      }
      template<typename... args_T>
      inline auto operator()(std::tuple<args_T...>&& args) const ->
      decltype(unpack_and_apply(std::declval<func_type>(), args, make_seq(args))) {
	return unpack_and_apply(func_m, args, make_seq(args));
      }
    private:
      func_type func_m;
    };
    // ---------------------------------------------------------------------- //
    /// \}
    
  } // namespace funtup_helper
  
  
  ///
  /// \brief Appplies the other parameters to the first parameter and
  /// returns the result.
  ///
  template<typename func_T, typename... args_T>
  inline auto
  apply(func_T&& func, args_T&&... args) ->
  decltype(func(std::forward<args_T>(args)...)) {
    return func(std::forward<args_T>(args)...);
  }
  
  ///
  /// \brief Applies a tuple of functions to the other parameters, and
  /// returns a corresponding tuple of results.
  ///
  /// This particular implementation calles the functions in the order
  /// they are in in the tuple.
  ///
  template<typename funcs_T, typename... args_T>
  inline auto
  apply_tuple(funcs_T&& funcs, args_T&&... args) ->
  decltype(funtup_helper::_apply_tuple(std::forward<funcs_T>(funcs),
				       make_seq(funcs),
				       std::forward<args_T>(args)...)) {
    return funtup_helper::_apply_tuple(std::forward<funcs_T>(funcs),
				       make_seq(funcs),
				       std::forward<args_T>(args)...);
  }
  
  ///
  /// \brief Pipes a series of functors into one functor.
  ///
  /// <code>fgh = pipe(f, g, h)</code> results in the function
  /// <code>fgh</code>, which equivalent to
  /// <code>h(g(f(x)))</code>. The resulting function object will
  /// assume ownership of (copies of) all functions being passed in.
  ///
  /*!\code
    struct add3 { int operator()(int a) const { return a + 3; } };
    struct mul3 { int operator()(int a) const { return a * 3; } };
    auto c1 = pipe(add3(), mul3());
    auto c2 = pipe(mul3(), add3());
    std::cout << c1(2) << std::endl; // prints 15
    std::cout << c2(2) << std::endl; // prints 9
    \endcode*/
  template<typename... funcs_T>
  inline funtup_helper::pipe_t<funcs_T...>
  pipe(funcs_T&&... funcs) {
    return funtup_helper::pipe_t<funcs_T...>(std::forward<funcs_T>(funcs)...);
  }

  ///
  /// \brief Transforms a function so that it automatically unpacks a
  /// singel tuple into a list of parameters.
  ///
  /// This is very useful when a function that returns a tuple needs
  /// to be pipedto a function taking multiple arguments.
  ///
  /*!\code
    struct add { int operator()(int a, int b) const { return a + b; } };
    std::tuple<int, int> divint(int a, int b) {
      return std::make_tuple(a / b, a % b);
    }
    auto c3 = pipe(&divint, auto_unpack(add()));
    assert(c3(5, 2) == 3);
    \endcode*/
  template<typename func_T>
  inline funtup_helper::apply_unpack_t<func_T>
  auto_unpack(func_T&& func) {
    return funtup_helper::apply_unpack_t<func_T>(std::forward<func_T>(func));
  }
  
  namespace funtup_helper {
    ///
    /// A wrapper to group several functors into a single object so
    /// that they can all be called with the same parameters.
    ///
    template<typename... funcs_T>
    struct battery_t : public std::tuple<funcs_T...> {
      inline battery_t(funcs_T&&... funcs)
	: std::tuple<funcs_T...>(std::forward<funcs_T>(funcs)...)
      {}
      template<typename... args_T>
      inline auto
      operator()(args_T&&... args) ->
      decltype(apply_tuple(std::declval<battery_t>(), std::forward<args_T>(args)...)) {
	return apply_tuple(*this, std::forward<args_T>(args)...);
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
  template<typename... funcs_T>
  inline funtup_helper::battery_t<funcs_T...>
  battery(funcs_T&&... funcs) {
    return funtup_helper::battery_t<funcs_T...>(std::forward<funcs_T>(funcs)...);
  }

  ///
  /// A function that makes a copy of whatever is passed in.
  ///
  /// This is useful when you want a battery or pipe to assume
  /// ownership of a function.
  ///
  template<typename T>
  typename std::decay<T>::type make_copy(T&& x) {
    return std::decay<T>::type(std::forward<T>(x));
  }

  
} // namespace funtup
#endif

