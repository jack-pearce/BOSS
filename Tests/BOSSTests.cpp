#include <string_view>
#define CATCH_CONFIG_RUNNER
#include "../Source/BOSS.hpp"
#include "../Source/BootstrapEngine.hpp"
#include "../Source/ExpressionUtilities.hpp"
#include "../Source/Serialization.hpp"
#include <array>
#include <catch2/catch.hpp>
#include <numeric>
#include <variant>
using boss::Expression;
using std::string;
using std::literals::string_literals::operator""s; // NOLINT(misc-unused-using-decls) clang-tidy bug
using boss::utilities::operator""_;                // NOLINT(misc-unused-using-decls) clang-tidy bug
using Catch::Generators::random;
using Catch::Generators::take;
using Catch::Generators::values;
using std::vector;
using namespace Catch::Matchers;
using boss::expressions::CloneReason;
using boss::expressions::ComplexExpression;
using boss::expressions::generic::get;
using boss::expressions::generic::get_if;
using boss::expressions::generic::holds_alternative;
namespace boss {
using boss::expressions::atoms::Span;
};
using std::int32_t;
using std::int64_t;
using std::int8_t;

namespace {
std::vector<string> librariesToTest{}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
}
// NOLINTBEGIN(readability-magic-numbers)
// NOLINTBEGIN(bugprone-exception-escape)
// NOLINTBEGIN(readability-function-cognitive-complexity)
// TODO: @Hubert, can you reduce the complexity of the tests, please?
TEST_CASE("Subspans work correctly", "[spans]") {
  auto input = boss::Span<int64_t>{std::vector<int64_t>{1, 2, 4, 3}};
  auto subrange = std::move(input).subspan(1, 3);
  CHECK(subrange.size() == 3);
  CHECK(subrange[0] == 2);
  CHECK(subrange[1] == 4);
  CHECK(subrange[2] == 3);
  auto subrange2 = boss::Span<int64_t>{std::vector<int64_t>{1, 2, 3, 2}}.subspan(2);
  CHECK(subrange2[0] == 3);
  CHECK(subrange2[1] == 2);
}

TEST_CASE("Expressions", "[expressions]") {
  using SpanArguments = boss::expressions::ExpressionSpanArguments;
  using SpanArgument = boss::expressions::ExpressionSpanArgument;
  using ExpressionArguments = boss::expressions::ExpressionArguments;
  using boss::expressions::atoms::Span;
  auto const v1 = GENERATE(take(3, random<std::int64_t>(1, 100)));
  auto const v2 = GENERATE(take(3, random<std::int64_t>(1, 100)));
  auto const e = "UnevaluatedPlus"_(v1, v2);
  CHECK(e.getHead().getName() == "UnevaluatedPlus");
  CHECK(e.getArguments().at(0) == v1);
  CHECK(e.getArguments().at(1) == v2);

  SECTION("static expression arguments") {
    auto staticArgumentExpression =
        boss::expressions::ComplexExpressionWithStaticArguments<std::int64_t, std::int64_t>(
            "UnevaluatedPlus"_, {v1, v2});
    CHECK(e == staticArgumentExpression);
  }

  SECTION("dynamic in-place initialized expression arguments") {
    auto spanArgumentExpression =
        boss::expressions::ComplexExpression("UnevaluatedPlus"_, {}, ExpressionArguments(v1, v2));
    CHECK(e == spanArgumentExpression);
  }

  SECTION("span expression arguments") {
    std::array<int64_t, 2> values = {v1, v2};
    SpanArguments args;
    args.emplace_back(Span<int64_t>(values.data(), 2, nullptr));
    auto spanArgumentExpression =
        boss::expressions::ComplexExpression("UnevaluatedPlus"_, {}, {}, std::move(args));
    CHECK(e == spanArgumentExpression);
  }

  SECTION("in-place initialized span expression arguments") {
    std::array<int64_t, 2> values = {v1, v2};
    auto spanArgumentExpression = boss::expressions::ComplexExpression(
        "UnevaluatedPlus"_, {}, {}, SpanArguments(Span<int64_t>(values.data(), 2, nullptr)));
    CHECK(e == spanArgumentExpression);
  }

  SECTION("nested span expression arguments") {
    std::array<int64_t, 2> values = {v1, v2};
    SpanArguments args;
    args.emplace_back(Span<int64_t const>(values.data(), 2, nullptr));
    auto nested = boss::expressions::ComplexExpression("UnevaluatedPlus"_, {}, {}, std::move(args));
    boss::expressions::ExpressionArguments subExpressions;
    subExpressions.emplace_back(std::move(nested));
    auto spanArgumentExpression =
        boss::expressions::ComplexExpression("UnevaluatedPlus"_, {}, std::move(subExpressions), {});
    CHECK("UnevaluatedPlus"_("UnevaluatedPlus"_(v1, v2)) == spanArgumentExpression);
  }
}

TEST_CASE("Expressions with static Arguments", "[expressions]") {
  SECTION("Atomic type subexpressions") {
    auto v1 = GENERATE(take(3, random<std::int64_t>(1, 100)));
    auto v2 = GENERATE(take(3, random<std::int64_t>(1, 100)));
    auto const e = boss::ComplexExpressionWithStaticArguments<std::int64_t, std::int64_t>(
        "UnevaluatedPlus"_, {v1, v2}, {}, {});
    CHECK(e.getHead().getName() == "UnevaluatedPlus");
    CHECK(e.getArguments().at(0) == v1);
    CHECK(e.getArguments().at(1) == v2);
  }
  SECTION("Complex subexpressions") {
    auto v1 = GENERATE(take(3, random<std::int64_t>(1, 100)));
    auto const e = boss::ComplexExpressionWithStaticArguments<
        boss::ComplexExpressionWithStaticArguments<std::int64_t>>(
        {"Duh"_,
         boss::ComplexExpressionWithStaticArguments<std::int64_t>{"UnevaluatedPlus"_, {v1}, {}, {}},
         {},
         {}});
    CHECK(e.getHead().getName() == "Duh");
    // TODO: this check should be enabled but requires a way to construct argument wrappers
    // from statically typed expressions
    // std::visit(
    //     [](auto&& arg) {
    //       CHECK(std::is_same_v<decltype(arg), boss::expressions::ComplexExpression>);
    //     },
    //     e.getArguments().at(0).getArgument());
  }
}

TEST_CASE("Expression Transformation", "[expressions]") {
  auto v1 = GENERATE(take(3, random<std::int64_t>(1, 100)));
  auto v2 = GENERATE(take(3, random<std::int64_t>(1, 100)));
  auto e = "UnevaluatedPlus"_(v1, v2);
  REQUIRE(*std::begin(e.getArguments()) == v1);
  get<std::int64_t>(*std::begin(e.getArguments()))++;
  REQUIRE(*std::begin(e.getArguments()) == v1 + 1);
  std::transform(std::make_move_iterator(std::begin(e.getArguments())),
                 std::make_move_iterator(std::end(e.getArguments())), e.getArguments().begin(),
                 [](auto&& e) { return get<std::int64_t>(e) + 1; });

  CHECK(e.getArguments().at(0) == v1 + 2);
  CHECK(e.getArguments().at(1) == v2 + 1);
}

TEST_CASE("Expression without arguments", "[expressions]") {
  auto const& e = "UnevaluatedPlus"_();
  CHECK(e.getHead().getName() == "UnevaluatedPlus");
}

class DummyAtom {
public:
  friend std::ostream& operator<<(std::ostream& s, DummyAtom const& /*unused*/) {
    return s << "dummy";
  }
};

TEST_CASE("Expression cast to more general expression system", "[expressions]") {
  auto a = boss::ExtensibleExpressionSystem<>::Expression("howdie"_());
  auto b = (boss::ExtensibleExpressionSystem<DummyAtom>::Expression)std::move(a);
  CHECK(
      get<boss::ExtensibleExpressionSystem<DummyAtom>::ComplexExpression>(b).getHead().getName() ==
      "howdie");
  auto& srcExpr = get<boss::ExtensibleExpressionSystem<DummyAtom>::ComplexExpression>(b);
  auto const& cexpr = std::decay_t<decltype(srcExpr)>(std::move(srcExpr));
  auto const& args = cexpr.getArguments();
  CHECK(args.empty());
}

TEST_CASE("Complex expression's argument cast to more general expression system", "[expressions]") {
  auto a = "List"_("howdie"_(1, 2, 3));
  auto const& b1 =
      (boss::ExtensibleExpressionSystem<DummyAtom>::Expression)(std::move(a).getArgument(0));
  CHECK(
      get<boss::ExtensibleExpressionSystem<DummyAtom>::ComplexExpression>(b1).getHead().getName() ==
      "howdie");
  auto b2 = get<boss::ExtensibleExpressionSystem<DummyAtom>::ComplexExpression>(b1).cloneArgument(
      1, CloneReason::FOR_TESTING);
  CHECK(get<int32_t>(b2) == 2);
}

TEST_CASE("Extract typed arguments from complex expression (using std::accumulate)",
          "[expressions]") {
  auto exprBase = "List"_("howdie"_(), 1, "unknown"_, "hello world"s);
  auto const& expr0 =
      boss::ExtensibleExpressionSystem<DummyAtom>::ComplexExpression(std::move(exprBase));
  auto str = [](auto const& expr) {
    auto const& args = expr.getArguments();
    return std::accumulate(
        args.begin(), args.end(), expr.getHead().getName(),
        [](auto const& accStr, auto const& arg) {
          return accStr + "_" +
                 visit(boss::utilities::overload(
                           [](auto const& value) { return std::to_string(value); },
                           [](DummyAtom const& /*value*/) { return ""s; },
                           [](boss::ExtensibleExpressionSystem<DummyAtom>::ComplexExpression const&
                                  expr) { return expr.getHead().getName(); },
                           [](boss::Symbol const& symbol) { return symbol.getName(); },
                           [](std::string const& str) { return str; }),
                       arg);
        });
  }(expr0);
  CHECK(str == "List_howdie_1_unknown_hello world");
}

TEST_CASE("Extract typed arguments from complex expression (manual iteration)", "[expressions]") {
  auto exprBase = "List"_("howdie"_(), 1, "unknown"_, "hello world"s);
  auto const& expr0 =
      boss::ExtensibleExpressionSystem<DummyAtom>::ComplexExpression(std::move(exprBase));
  auto str = [](auto const& expr) {
    auto const& args = expr.getArguments();
    auto size = args.size();
    auto accStr = expr.getHead().getName();
    for(int idx = 0; idx < size; ++idx) {
      accStr +=
          "_" +
          visit(boss::utilities::overload(
                    [](auto const& value) { return std::to_string(value); },
                    [](DummyAtom const& /*value*/) { return ""s; },
                    [](boss::ExtensibleExpressionSystem<DummyAtom>::ComplexExpression const& expr) {
                      return expr.getHead().getName();
                    },
                    [](boss::Symbol const& symbol) { return symbol.getName(); },
                    [](std::string const& str) { return str; }),
                args.at(idx));
    }
    return accStr;
  }(expr0);
  CHECK(str == "List_howdie_1_unknown_hello world");
}

TEST_CASE("Merge two complex expressions", "[expressions]") {
  auto delimeters = "List"_("_"_(), "_"_(), "_"_(), "_"_());
  auto expr = "List"_("howdie"_(), 1, "unknown"_, "hello world"s);
  auto delimetersIt = std::make_move_iterator(delimeters.getArguments().begin());
  auto delimetersItEnd = std::make_move_iterator(delimeters.getArguments().end());
  auto exprIt = std::make_move_iterator(expr.getArguments().begin());
  auto exprItEnd = std::make_move_iterator(expr.getArguments().end());
  auto args = boss::ExpressionArguments();
  for(; delimetersIt != delimetersItEnd && exprIt != exprItEnd; ++delimetersIt, ++exprIt) {
    args.emplace_back(std::move(*delimetersIt));
    args.emplace_back(std::move(*exprIt));
  }
  auto e = boss::ComplexExpression("List"_, std::move(args));
  auto str = std::accumulate(
      e.getArguments().begin(), e.getArguments().end(), e.getHead().getName(),
      [](auto const& accStr, auto const& arg) {
        return accStr + visit(boss::utilities::overload(
                                  [](auto const& value) { return std::to_string(value); },
                                  [](boss::ComplexExpression const& expr) {
                                    return expr.getHead().getName();
                                  },
                                  [](boss::Symbol const& symbol) { return symbol.getName(); },
                                  [](std::string const& str) { return str; }),
                              arg);
      });
  CHECK(str == "List_howdie_1_unknown_hello world");
}

TEST_CASE("Merge a static and a dynamic complex expressions", "[expressions]") {
  auto delimeters = "List"_("_"s, "_"s, "_"s, "_"s);
  auto expr = "List"_("howdie"_(), 1, "unknown"_, "hello world"s);
  auto delimetersIt = std::make_move_iterator(delimeters.getArguments().begin());
  auto delimetersItEnd = std::make_move_iterator(delimeters.getArguments().end());
  auto exprIt = std::make_move_iterator(expr.getArguments().begin());
  auto exprItEnd = std::make_move_iterator(expr.getArguments().end());
  auto args = boss::ExpressionArguments();
  for(; delimetersIt != delimetersItEnd && exprIt != exprItEnd; ++delimetersIt, ++exprIt) {
    args.emplace_back(std::move(*delimetersIt));
    args.emplace_back(std::move(*exprIt));
  }
  auto e = boss::ComplexExpression("List"_, std::move(args));
  auto str = std::accumulate(
      e.getArguments().begin(), e.getArguments().end(), e.getHead().getName(),
      [](auto const& accStr, auto const& arg) {
        return accStr + visit(boss::utilities::overload(
                                  [](auto const& value) { return std::to_string(value); },
                                  [](boss::ComplexExpression const& expr) {
                                    return expr.getHead().getName();
                                  },
                                  [](boss::Symbol const& symbol) { return symbol.getName(); },
                                  [](std::string const& str) { return str; }),
                              arg);
      });
  CHECK(str == "List_howdie_1_unknown_hello world");
}

TEST_CASE("holds_alternative for complex expression's arguments", "[expressions]") {
  auto const& expr = "List"_("howdie"_(), (int8_t)1, 1, (int64_t)1, "unknown"_, "hello world"s);
  CHECK(holds_alternative<boss::ComplexExpression>(expr.getArguments().at(0)));
  CHECK(holds_alternative<int8_t>(expr.getArguments().at(1)));
  CHECK(holds_alternative<int32_t>(expr.getArguments().at(2)));
  CHECK(holds_alternative<int64_t>(expr.getArguments().at(3)));
  CHECK(holds_alternative<boss::Symbol>(expr.getArguments().at(4)));
  CHECK(holds_alternative<std::string>(expr.getArguments().at(5)));
}

TEST_CASE("get_if for complex expression's arguments", "[expressions]") {
  auto const& expr = "List"_("howdie"_(), 1, "unknown"_, "hello world"s);
  auto const& arg0 = expr.getArguments().at(0);
  auto const& arg1 = expr.getArguments().at(1);
  auto const& arg2 = expr.getArguments().at(2);
  auto const& arg3 = expr.getArguments().at(3);
  CHECK(get_if<boss::ComplexExpression>(&arg0) != nullptr);
  CHECK(get_if<int32_t>(&arg1) != nullptr);
  CHECK(get_if<boss::Symbol>(&arg2) != nullptr);
  CHECK(get_if<std::string>(&arg3) != nullptr);
  auto const& arg0args = get<boss::ComplexExpression>(arg0).getArguments();
  CHECK(arg0args.empty());
}

TEST_CASE("move expression's arguments to a new expression", "[expressions]") {
  auto expr = "List"_("howdie"_(), 1, "unknown"_, "hello world"s);
  auto&& movedExpr = std::move(expr);
  boss::ExpressionArguments args = movedExpr.getArguments();
  auto expr2 = boss::ComplexExpression(std::move(movedExpr.getHead()), std::move(args)); // NOLINT
  CHECK(get<boss::ComplexExpression>(expr2.getArguments().at(0)) == "howdie"_());
  CHECK(get<int32_t>(expr2.getArguments().at(1)) == 1);
  CHECK(get<boss::Symbol>(expr2.getArguments().at(2)) == "unknown"_);
  CHECK(get<std::string>(expr2.getArguments().at(3)) == "hello world"s);
}

TEST_CASE("copy expression's arguments to a new expression", "[expressions]") {
  auto expr = "List"_("howdie"_(), 1, "unknown"_, "hello world"s);
  auto args =
      expr.getArguments(); // TODO: this one gets the reference to the arguments
                           // when it should be a copy.
                           // Any modification/move of args will be reflected in expr's arguments!
  get<int32_t>(args.at(1)) = 2;
  auto expr2 = boss::ComplexExpression(expr.getHead(), args);
  get<int32_t>(args.at(1)) = 3;
  auto expr3 = boss::ComplexExpression(expr.getHead(), std::move(args)); // NOLINT
  // CHECK(get<int64_t>(expr.getArguments().at(1)) == 1); // fails for now (see above TODO)
  CHECK(get<int32_t>(expr2.getArguments().at(1)) == 2);
  CHECK(get<int32_t>(expr3.getArguments().at(1)) == 3);
}

TEST_CASE("copy non-const expression's arguments to ExpressionArguments", "[expressions]") {
  auto expr = "List"_("howdie"_(), 1, "unknown"_, "hello world"s);
  boss::ExpressionArguments args = expr.getArguments(); // TODO: why is it moved?
  get<int32_t>(args.at(1)) = 2;
  auto expr2 = boss::ComplexExpression(expr.getHead(), std::move(args));
  // CHECK(get<int64_t>(expr.getArguments().at(1)) == 1); // fails because args was moved (see TODO)
  CHECK(get<int32_t>(expr2.getArguments().at(1)) == 2);
}

TEST_CASE("copy const expression's arguments to ExpressionArguments)", "[expressions]") {
  auto const& expr = "List"_("howdie"_(), 1, "unknown"_, "hello world"s);
  boss::ExpressionArguments args = expr.getArguments();
  get<int32_t>(args.at(1)) = 2;
  auto expr2 = boss::ComplexExpression(expr.getHead(), std::move(args));
  CHECK(get<int32_t>(expr.getArguments().at(1)) == 1);
  CHECK(get<int32_t>(expr2.getArguments().at(1)) == 2);
}

TEST_CASE("move and dispatch expression's arguments", "[expressions]") {
  auto expr = "List"_("howdie"_(), 1, "unknown"_, "hello world"s);
  std::vector<boss::Symbol> symbols;
  std::vector<boss::Expression> otherExpressions;
  for(auto&& arg : (boss::ExpressionArguments)std::move(expr).getArguments()) {
    visit(boss::utilities::overload(
              [&otherExpressions](auto&& value) {
                otherExpressions.emplace_back(std::forward<decltype(value)>(value));
              },
              [&symbols](boss::Symbol&& symbol) { symbols.emplace_back(std::move(symbol)); }),
          std::move(arg));
  }
  CHECK(symbols.size() == 1);
  CHECK(symbols[0] == "unknown"_);
  CHECK(otherExpressions.size() == 3);
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("Complex Expressions with numeric Spans", "[spans]", std::int32_t, std::int64_t,
                   std::float_t, std::double_t) {
  auto input = GENERATE(take(3, chunk(5, random<TestType>(1, 1000))));
  auto argument = vector<TestType>(input);
  auto s = boss::Span<TestType>(std::move(argument));
  auto vectorExpression = "duh"_(std::move(s));
  REQUIRE(vectorExpression.getArguments().size() == input.size());
  for(auto i = 0U; i < input.size(); i++) {
    CHECK(vectorExpression.getArguments().at(i) == input.at(i));
    CHECK(vectorExpression.getArguments()[i] == input[i]);
  }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("Complex Expressions with non-owning numeric Spans", "[spans]", std::int32_t,
                   std::int64_t, std::float_t, std::double_t) {
  auto input = GENERATE(take(3, chunk(5, random<TestType>(1, 1000))));
  auto argument = vector<TestType>(input);
  auto s = boss::Span<TestType>(argument);
  auto vectorExpression = "duh"_(std::move(s));
  REQUIRE(vectorExpression.getArguments().size() == input.size());
  for(auto i = 0U; i < input.size(); i++) {
    CHECK(vectorExpression.getArguments().at(i) == input.at(i));
    CHECK(vectorExpression.getArguments()[i] == input[i]);
  }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("Complex Expressions with non-owning const numeric Spans", "[spans]",
                   std::int32_t, std::int64_t, std::float_t, std::double_t) {
  auto input = GENERATE(take(3, chunk(5, random<TestType>(1, 1000))));
  auto const argument = vector<TestType>(input);
  auto s = boss::Span<TestType const>(argument);
  auto const vectorExpression = "duh"_(std::move(s));
  REQUIRE(vectorExpression.getArguments().size() == input.size());
  for(auto i = 0U; i < input.size(); i++) {
    CHECK(vectorExpression.getArguments().at(i) == input.at(i));
    CHECK(vectorExpression.getArguments()[i] == input[i]);
  }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("Cloning Expressions with numeric Spans", "[spans][clone]", std::int32_t,
                   std::int64_t, std::float_t, std::double_t) {
  auto input = GENERATE(take(3, chunk(5, random<TestType>(1, 1000))));
  auto vectorExpression = "duh"_(boss::Span<TestType>(vector(input)));
  auto clonedVectorExpression = vectorExpression.clone(CloneReason::FOR_TESTING);
  for(auto i = 0U; i < input.size(); i++) {
    CHECK(clonedVectorExpression.getArguments().at(i) == input.at(i));
    CHECK(vectorExpression.getArguments()[i] == input[i]);
  }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("Complex Expressions with Spans", "[spans]", std::string, boss::Symbol) {
  using std::literals::string_literals::operator""s;
  auto vals = GENERATE(take(3, chunk(5, values({"a"s, "b"s, "c"s, "d"s, "e"s, "f"s, "g"s, "h"s}))));
  auto input = vector<TestType>();
  std::transform(begin(vals), end(vals), std::back_inserter(input),
                 [](auto argument) { return TestType(argument); });
  auto vectorExpression = "duh"_(boss::Span<TestType>(std::move(input)));
  for(auto i = 0U; i < vals.size(); i++) {
    CHECK(vectorExpression.getArguments().at(0) == TestType(vals.at(0)));
    CHECK(vectorExpression.getArguments()[0] == TestType(vals[0]));
  }
}

TEST_CASE("Basics", "[basics]") { // NOLINT
  auto engine = boss::engines::BootstrapEngine();
  REQUIRE(!librariesToTest.empty());
  auto eval = [&engine](boss::Expression&& expression) mutable {
    return engine.evaluate("EvaluateInEngines"_("List"_(GENERATE(from_range(librariesToTest))),
                                                std::move(expression)));
  };

  SECTION("CatchingErrors") {
    CHECK_THROWS_MATCHES(
        engine.evaluate("EvaluateInEngines"_("List"_(9), 5)), std::bad_variant_access,
        Message("expected and actual type mismatch in expression \"9\", expected string"));
  }

  SECTION("Atomics") {
    CHECK(get<std::int32_t>(eval(boss::Expression(9))) == 9); // NOLINT
  }

  SECTION("Addition") {
    CHECK(get<std::int32_t>(eval("Plus"_(5, 4))) == 9); // NOLINT
    CHECK(get<std::int32_t>(eval("Plus"_(5, 2, 2))) == 9);
    CHECK(get<std::int32_t>(eval("Plus"_(5, 2, 2))) == 9);
    CHECK(get<std::int32_t>(eval("Plus"_("Plus"_(2, 3), 2, 2))) == 9);
    CHECK(get<std::int32_t>(eval("Plus"_("Plus"_(3, 2), 2, 2))) == 9);
  }

  SECTION("Strings") {
    CHECK(get<string>(eval("StringJoin"_((string) "howdie", (string) " ", (string) "world"))) ==
          "howdie world");
  }

  SECTION("Floats") {
    auto const twoAndAHalf = 2.5F;
    auto const two = 2.0F;
    auto const quantum = 0.001F;
    CHECK(std::fabs(get<float>(eval("Plus"_(twoAndAHalf, twoAndAHalf))) - two * twoAndAHalf) <
          quantum);
  }

  SECTION("Doubles") {
    auto const twoAndAHalf = 2.5;
    auto const two = 2.0;
    auto const quantum = 0.001;
    CHECK(std::fabs(get<double>(eval("Plus"_(twoAndAHalf, twoAndAHalf))) - two * twoAndAHalf) <
          quantum);
  }

  SECTION("Booleans") {
    CHECK(get<bool>(eval("Greater"_(5, 2))));
    CHECK(!get<bool>(eval("Greater"_(2, 5))));
  }

  SECTION("Symbols") {
    CHECK(get<boss::Symbol>(eval("Symbol"_((string) "x"))).getName() == "x");
    auto expression = get<boss::ComplexExpression>(
        eval("UndefinedFunction"_(9))); // NOLINT(readability-magic-numbers)

    CHECK(expression.getHead().getName() == "UndefinedFunction");
    CHECK(get<std::int32_t>(expression.getArguments().at(0)) == 9);

    CHECK(get<std::string>(
              get<boss::ComplexExpression>(eval("UndefinedFunction"_((string) "Hello World!")))
                  .getArguments()
                  .at(0)) == "Hello World!");
  }

  SECTION("Interpolation") {
    auto thing = GENERATE(
        take(1, chunk(3, filter([](int i) { return i % 2 == 1; }, random(1, 1000))))); // NOLINT
    std::sort(begin(thing), end(thing));
    auto y = GENERATE(
        take(1, chunk(3, filter([](int i) { return i % 2 == 1; }, random(1, 1000))))); // NOLINT

    auto interpolationTable = "Table"_("Column"_("x"_, "List"_(thing[0], thing[1], thing[2])),
                                       "Column"_("y"_, "List"_(y[0], "Interpolate"_("x"_), y[2])));

    auto expectedProjectX = "Table"_("Column"_("x"_, "List"_(thing[0], thing[1], thing[2])));
    auto expectedProjectY = "Table"_("Column"_("y"_, "List"_(y[0], (y[0] + y[2]) / 2, y[2])));

    CHECK(eval("Project"_(interpolationTable.clone(CloneReason::FOR_TESTING), "As"_("x"_, "x"_))) ==
          expectedProjectX);
    CHECK(eval("Project"_(interpolationTable.clone(CloneReason::FOR_TESTING), "As"_("y"_, "y"_))) ==
          expectedProjectY);
  }

  SECTION("Relational (Ints)") {
    SECTION("Selection") {
      auto intTable = "Table"_("Column"_("Value"_, "List"_(2, 3, 1, 4, 1))); // NOLINT
      auto result = eval("Select"_(std::move(intTable), "Where"_("Greater"_("Value"_, 3))));
      CHECK(result == "Table"_("Column"_("Value"_, "List"_(4))));
    }

    SECTION("Projection") {
      auto intTable = "Table"_("Column"_("Value"_, "List"_(10, 20, 30, 40, 50))); // NOLINT

      SECTION("Plus") {
        CHECK(eval("Project"_(intTable.clone(CloneReason::FOR_TESTING),
                              "As"_("Result"_, "Plus"_("Value"_, "Value"_)))) ==
              "Table"_("Column"_("Result"_, "List"_(20, 40, 60, 80, 100)))); // NOLINT
      }

      SECTION("Greater") {
        CHECK(eval("Project"_(intTable.clone(CloneReason::FOR_TESTING),
                              "As"_("Result"_, "Greater"_("Value"_, 25)))) ==
              "Table"_("Column"_("Result"_, "List"_(false, false, true, true, true)))); // NOLINT
        CHECK(eval("Project"_(intTable.clone(CloneReason::FOR_TESTING),
                              "As"_("Result"_, "Greater"_(45, "Value"_)))) ==
              "Table"_("Column"_("Result"_, "List"_(true, true, true, true, false)))); // NOLINT
      }

      SECTION("Logic") {
        CHECK(eval("Project"_(
                  intTable.clone(CloneReason::FOR_TESTING),
                  "As"_("Result"_, "And"_("Greater"_("Value"_, 25), "Greater"_(45, "Value"_))))) ==
              "Table"_("Column"_("Result"_, "List"_(false, false, true, true, false)))); // NOLINT
      }
    }

    SECTION("Join") {
      auto const dataSetSize = 10;
      std::vector<int64_t> vec1(dataSetSize);
      std::vector<int64_t> vec2(dataSetSize);
      std::iota(vec1.begin(), vec1.end(), 0);
      std::iota(vec2.begin(), vec2.end(), dataSetSize);

      auto adjacency1 = "Table"_("Column"_("From"_, "List"_(boss::Span<int64_t>(vector(vec1)))),
                                 "Column"_("To"_, "List"_(boss::Span<int64_t>(vector(vec2)))));
      auto adjacency2 = "Table"_("Column"_("From2"_, "List"_(boss::Span<int64_t>(vector(vec2)))),
                                 "Column"_("To2"_, "List"_(boss::Span<int64_t>(vector(vec1)))));

      auto result = eval("Join"_(std::move(adjacency1), std::move(adjacency2),
                                 "Where"_("Equal"_("To"_, "From2"_))));

      INFO(result);
      CHECK(get<boss::ComplexExpression>(result).getArguments().size() == dataSetSize);
    }
  }

  SECTION("Relational (Strings)") {
    auto customerTable = "Table"_("Column"_("FirstName"_, "List"_("John", "Sam", "Barbara")),
                                  "Column"_("LastName"_, "List"_("McCarthy", "Madden", "Liskov")));

    SECTION("Selection") {
      auto sam = eval("Select"_(customerTable.clone(CloneReason::FOR_TESTING),
                                "Where"_("StringContainsQ"_("LastName"_, "Madden"))));
      CHECK(sam == "Table"_("Column"_("FirstName"_, "List"_("Sam")),
                            "Column"_("LastName"_, "List"_("Madden"))));
    }

    SECTION("Aggregation") {
      SECTION("ConstantGroup") {
        auto result =
            eval("Group"_(customerTable.clone(CloneReason::FOR_TESTING), "Function"_(0), "Count"_));
        INFO(result);
        CHECK(get<boss::ComplexExpression>(result).getArguments().size() == 2);
        CHECK(get<boss::ComplexExpression>(
                  get<boss::ComplexExpression>(
                      get<boss::ComplexExpression>(result).getArguments().at(0))
                      .getArguments()
                      .at(1)) == "List"_(0));
        CHECK(get<boss::ComplexExpression>(
                  get<boss::ComplexExpression>(
                      get<boss::ComplexExpression>(result).getArguments().at(1))
                      .getArguments()
                      .at(1)) == "List"_(3));
      }

      SECTION("NoGroup") {
        auto result = eval("Group"_(customerTable.clone(CloneReason::FOR_TESTING), "Count"_));
        INFO(result);
        CHECK(get<boss::ComplexExpression>(result).getArguments().size() == 1);
        CHECK(get<boss::ComplexExpression>(
                  get<boss::ComplexExpression>(
                      get<boss::ComplexExpression>(result).getArguments().at(0))
                      .getArguments()
                      .at(1)) == "List"_(3));
      }

      SECTION("Select+Group") {
        auto result = eval("Group"_("Select"_(customerTable.clone(CloneReason::FOR_TESTING),
                                              "Where"_("StringContainsQ"_("LastName"_, "Madden"))),
                                    "Function"_(0), "Count"_));
        INFO(result);
        CHECK(get<boss::ComplexExpression>(result).getArguments().size() == 2);
        CHECK(get<boss::ComplexExpression>(
                  get<boss::ComplexExpression>(
                      get<boss::ComplexExpression>(result).getArguments().at(0))
                      .getArguments()
                      .at(1)) == "List"_(0));
        CHECK(get<boss::ComplexExpression>(
                  get<boss::ComplexExpression>(
                      get<boss::ComplexExpression>(result).getArguments().at(1))
                      .getArguments()
                      .at(1)) == "List"_(1));
      }
    }
  }

  SECTION("Relational (empty table)") {
    auto emptyCustomerTable =
        "Table"_("Column"_("ID"_, "List"_()), "Column"_("FirstName"_, "List"_()),
                 "Column"_("LastName"_, "List"_()), "Column"_("BirthYear"_, "List"_()),
                 "Column"_("Country"_, "List"_()));
    auto emptySelect =
        eval("Select"_(emptyCustomerTable.clone(CloneReason::FOR_TESTING), "Function"_(true)));
    CHECK(emptySelect == emptyCustomerTable);
  }

  SECTION("Relational (multiple types)") {
    auto customerTable = "Table"_("Column"_("ID"_, "List"_(1, 2, 3)), // NOLINT
                                  "Column"_("FirstName"_, "List"_("John", "Sam", "Barbara")),
                                  "Column"_("LastName"_, "List"_("McCarthy", "Madden", "Liskov")),
                                  "Column"_("BirthYear"_, "List"_(1927, 1976, 1939)), // NOLINT
                                  "Column"_("Country"_, "List"_("USA", "USA", "USA")));

    SECTION("Selection") {
      auto fullTable =
          eval("Select"_(customerTable.clone(CloneReason::FOR_TESTING), "Function"_(true)));
      CHECK(fullTable == customerTable);

      auto none =
          eval("Select"_(customerTable.clone(CloneReason::FOR_TESTING), "Function"_(false)));
      CHECK(none == "Table"_("Column"_("ID"_, "List"_()), "Column"_("FirstName"_, "List"_()),
                             "Column"_("LastName"_, "List"_()), "Column"_("BirthYear"_, "List"_()),
                             "Column"_("Country"_, "List"_())));

      auto usa = eval("Select"_(customerTable.clone(CloneReason::FOR_TESTING),
                                "Where"_("StringContainsQ"_("Country"_, "USA"))));
      CHECK(usa == customerTable);

      auto madden = eval("Select"_(customerTable.clone(CloneReason::FOR_TESTING),
                                   "Where"_("StringContainsQ"_("LastName"_, "Madden"))));
      CHECK(madden == "Table"_("Column"_("ID"_, "List"_(2)), // NOLINT
                               "Column"_("FirstName"_, "List"_("Sam")),
                               "Column"_("LastName"_, "List"_("Madden")),
                               "Column"_("BirthYear"_, "List"_(1976)), // NOLINT
                               "Column"_("Country"_, "List"_("USA"))));

      auto john = eval("Select"_(customerTable.clone(CloneReason::FOR_TESTING),
                                 "Where"_("StringContainsQ"_("FirstName"_, "John"))));
      CHECK(john == "Table"_("Column"_("ID"_, "List"_(1)), // NOLINT
                             "Column"_("FirstName"_, "List"_("John")),
                             "Column"_("LastName"_, "List"_("McCarthy")),
                             "Column"_("BirthYear"_, "List"_(1927)), // NOLINT
                             "Column"_("Country"_, "List"_("USA"))));

      auto id3 = eval(
          "Select"_(customerTable.clone(CloneReason::FOR_TESTING), "Where"_("Equal"_("ID"_, 3))));
      CHECK(id3 == "Table"_("Column"_("ID"_, "List"_(3)), // NOLINT
                            "Column"_("FirstName"_, "List"_("Barbara")),
                            "Column"_("LastName"_, "List"_("Liskov")),
                            "Column"_("BirthYear"_, "List"_(1939)), // NOLINT
                            "Column"_("Country"_, "List"_("USA"))));

      auto notFound = eval("Select"_(customerTable.clone(CloneReason::FOR_TESTING),
                                     "Where"_("Equal"_("BirthYear"_, 0))));
      CHECK(notFound == "Table"_("Column"_("ID"_, "List"_()), "Column"_("FirstName"_, "List"_()),
                                 "Column"_("LastName"_, "List"_()),
                                 "Column"_("BirthYear"_, "List"_()),
                                 "Column"_("Country"_, "List"_())));
    }

    SECTION("Projection") {
      auto fullnames =
          eval("Project"_(customerTable.clone(CloneReason::FOR_TESTING),
                          "As"_("FirstName"_, "FirstName"_, "LastName"_, "LastName"_)));
      CHECK(fullnames == "Table"_("Column"_("FirstName"_, "List"_("John", "Sam", "Barbara")),
                                  "Column"_("LastName"_, "List"_("McCarthy", "Madden", "Liskov"))));
      auto firstNames = eval("Project"_(customerTable.clone(CloneReason::FOR_TESTING),
                                        "As"_("FirstName"_, "FirstName"_)));
      CHECK(firstNames == "Table"_("Column"_("FirstName"_, "List"_("John", "Sam", "Barbara"))));
      auto lastNames = eval("Project"_(customerTable.clone(CloneReason::FOR_TESTING),
                                       "As"_("LastName"_, "LastName"_)));
      CHECK(lastNames == "Table"_("Column"_("LastName"_, "List"_("McCarthy", "Madden", "Liskov"))));
    }

    SECTION("Sorting") {
      auto sortedByID =
          eval("Sort"_("Select"_(customerTable.clone(CloneReason::FOR_TESTING), "Function"_(true)),
                       "By"_("ID"_)));
      CHECK(sortedByID == customerTable);

      auto sortedByLastName =
          eval("Sort"_("Select"_(customerTable.clone(CloneReason::FOR_TESTING), "Function"_(true)),
                       "By"_("LastName"_)));
      CHECK(sortedByLastName ==
            "Table"_("Column"_("ID"_, "List"_(3, 2, 1)), // NOLINT
                     "Column"_("FirstName"_, "List"_("Barbara", "Sam", "John")),
                     "Column"_("LastName"_, "List"_("Liskov", "Madden", "McCarthy")),
                     "Column"_("BirthYear"_, "List"_(1939, 1976, 1927)), // NOLINT
                     "Column"_("Country"_, "List"_("USA", "USA", "USA"))));
    }

    SECTION("Aggregation") {
      auto countRows = eval("Group"_("Customer"_, "Function"_(0), "Count"_));
      INFO(countRows);
      CHECK(get<boss::ComplexExpression>(countRows).getArguments().size() == 2);
      CHECK(get<boss::ComplexExpression>(
                get<boss::ComplexExpression>(
                    get<boss::ComplexExpression>(countRows).getArguments().at(0))
                    .getArguments()
                    .at(1)) == "List"_(0));
      CHECK(get<boss::ComplexExpression>(
                get<boss::ComplexExpression>(
                    get<boss::ComplexExpression>(countRows).getArguments().at(1))
                    .getArguments()
                    .at(1)) == "List"_(3));
    }
  }
}

namespace {
int64_t operator""_i64(char c) { return static_cast<int64_t>(c); };
} // namespace

TEST_CASE("TPC-H", "[tpch]") {
  auto engine = boss::engines::BootstrapEngine();
  REQUIRE(!librariesToTest.empty());
  auto eval = [&engine](boss::Expression&& expression) mutable {
    return engine.evaluate("EvaluateInEngines"_("List"_(GENERATE(from_range(librariesToTest))),
                                                std::move(expression)));
  };

  auto nation = "Table"_("Column"_("N_NATIONKEY"_, "List"_(1, 2, 3, 4)), // NOLINT
                         "Column"_("N_REGIONKEY"_, "List"_(1, 1, 2, 3)), // NOLINT
                         "Column"_("N_NAME"_, "List"_("ALGERIA", "ARGENTINA", "BRAZIL", "CANADA")));

  auto part =
      "Table"_("Column"_("P_PARTKEY"_, "List"_(1, 1, 2, 3)),                         // NOLINT
               "Column"_("P_RETAILPRICE"_, "List"_(100.01, 100.01, 100.01, 100.01)), // NOLINT
               "Column"_("P_NAME"_, "List"_("spring green yellow purple cornsilk",
                                            "cornflower chocolate smoke green pink",
                                            "moccasin green thistle khaki floral",
                                            "green blush tomato burlywood seashell")));

  auto supplier = "Table"_("Column"_("S_SUPPKEY"_, "List"_(1, 1, 2, 3)),    // NOLINT
                           "Column"_("S_NATIONKEY"_, "List"_(1, 1, 2, 3))); // NOLINT

  auto partsupp =
      "Table"_("Column"_("PS_PARTKEY"_, "List"_(1, 1, 2, 3)),                         // NOLINT
               "Column"_("PS_SUPPKEY"_, "List"_(1, 1, 2, 3)),                         // NOLINT
               "Column"_("PS_SUPPLYCOST"_, "List"_(771.64, 993.49, 337.09, 357.84))); // NOLINT

  auto customer = "Table"_(
      "Column"_("C_CUSTKEY"_, "List"_(4, 7, 1, 4)),                       // NOLINT
      "Column"_("C_NATIONKEY"_, "List"_(15, 13, 1, 4)),                   // NOLINT
      "Column"_("C_ACCTBAL"_, "List"_(711.56, 121.65, 7498.12, 2866.83)), // NOLINT
      "Column"_("C_NAME"_, "List"_("Customer#000000001", "Customer#000000002", "Customer#000000003",
                                   "Customer#000000004")),
      "Column"_("C_MKTSEGMENT"_, "List"_("AUTOMOBILE", "MACHINERY", "HOUSEHOLD", "BUILDING")));

  auto orders = "Table"_(
      "Column"_("O_ORDERKEY"_, "List"_(1, 1, 2, 3)),
      "Column"_("O_CUSTKEY"_, "List"_(4, 7, 1, 4)),                                    // NOLINT
      "Column"_("O_TOTALPRICE"_, "List"_(178821.73, 154260.84, 202660.52, 155680.60)), // NOLINT
      "Column"_("O_ORDERDATE"_, "List"_("DateObject"_("1998-01-24"), "DateObject"_("1992-05-01"),
                                        "DateObject"_("1992-12-21"), "DateObject"_("1994-06-18"))),
      "Column"_("O_SHIPPRIORITY"_, "List"_(1, 1, 1, 1))); // NOLINT

  auto lineitem = "Table"_(
      "Column"_("L_ORDERKEY"_, "List"_(1, 1, 2, 3)),                                 // NOLINT
      "Column"_("L_PARTKEY"_, "List"_(1, 2, 3, 4)),                                  // NOLINT
      "Column"_("L_SUPPKEY"_, "List"_(1, 2, 3, 4)),                                  // NOLINT
      "Column"_("L_RETURNFLAG"_, "List"_("N", "N", "A", "A")),                       // NOLINT
      "Column"_("L_LINESTATUS"_, "List"_("O", "O", "F", "F")),                       // NOLINT
      "Column"_("L_RETURNFLAG_INT"_, "List"_('N'_i64, 'N'_i64, 'A'_i64, 'A'_i64)),   // NOLINT
      "Column"_("L_LINESTATUS_INT"_, "List"_('O'_i64, 'O'_i64, 'F'_i64, 'F'_i64)),   // NOLINT
      "Column"_("L_QUANTITY"_, "List"_(17, 21, 8, 5)),                               // NOLINT
      "Column"_("L_EXTENDEDPRICE"_, "List"_(17954.55, 34850.16, 7712.48, 25284.00)), // NOLINT
      "Column"_("L_DISCOUNT"_, "List"_(0.10, 0.05, 0.06, 0.06)),                     // NOLINT
      "Column"_("L_TAX"_, "List"_(0.02, 0.06, 0.02, 0.06)),                          // NOLINT
      "Column"_("L_SHIPDATE"_, "List"_("DateObject"_("1992-03-13"), "DateObject"_("1994-04-12"),
                                       "DateObject"_("1996-02-28"), "DateObject"_("1994-12-31"))));

  SECTION("Q1 (Select only)") {
    auto output = eval("Select"_(
        "Project"_(lineitem.clone(CloneReason::FOR_TESTING), "As"_("L_SHIPDATE"_, "L_SHIPDATE"_)),
        "Where"_("Greater"_("DateObject"_("1998-08-31"), "L_SHIPDATE"_))));
    CHECK(output ==
          eval("Table"_("Column"_(
              "L_SHIPDATE"_, "List"_("DateObject"_("1992-03-13"), "DateObject"_("1994-04-12"),
                                     "DateObject"_("1996-02-28"), "DateObject"_("1994-12-31"))))));
  }

  SECTION("Q1 (Project only)") {
    auto output = eval("Project"_(
        "Project"_(lineitem.clone(CloneReason::FOR_TESTING),
                   "As"_("L_QUANTITY"_, "L_QUANTITY"_, "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_,
                         "calc1"_, "Minus"_(1.0, "L_DISCOUNT"_), "calc2"_, "Plus"_("L_TAX"_, 1.0),
                         "L_DISCOUNT"_, "L_DISCOUNT"_)),
        "As"_("L_QUANTITY"_, "L_QUANTITY"_, "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_, "disc_price"_,
              "Times"_("L_EXTENDEDPRICE"_, "calc1"_), "charge"_,
              "Times"_("L_EXTENDEDPRICE"_, "calc1"_, "calc2"_), "L_DISCOUNT"_, "L_DISCOUNT"_)));
    CHECK(output ==
          "Table"_("Column"_("L_QUANTITY"_, "List"_(17, 21, 8, 5)), // NOLINT
                   "Column"_("L_EXTENDEDPRICE"_,
                             "List"_(17954.55, 34850.16, 7712.48, 25284.00)), // NOLINT
                   "Column"_("disc_price"_,
                             "List"_(17954.55 * (1.0 - 0.10), 34850.16 * (1.0 - 0.05),    // NOLINT
                                     7712.48 * (1.0 - 0.06), 25284.00 * (1.0 - 0.06))),   // NOLINT
                   "Column"_("charge"_, "List"_(17954.55 * (1.0 - 0.10) * (0.02 + 1.0),   // NOLINT
                                                34850.16 * (1.0 - 0.05) * (0.06 + 1.0),   // NOLINT
                                                7712.48 * (1.0 - 0.06) * (0.02 + 1.0),    // NOLINT
                                                25284.00 * (1.0 - 0.06) * (0.06 + 1.0))), // NOLINT
                   "Column"_("L_DISCOUNT"_, "List"_(0.10, 0.05, 0.06, 0.06))));           // NOLINT
  }

  SECTION("Q1 (Select-Project only)") {
    auto output = eval("Project"_(
        "Project"_(
            "Project"_("Select"_("Project"_(lineitem.clone(CloneReason::FOR_TESTING),
                                            "As"_("L_QUANTITY"_, "L_QUANTITY"_, "L_DISCOUNT"_,
                                                  "L_DISCOUNT"_, "L_SHIPDATE"_, "L_SHIPDATE"_,
                                                  "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_, "L_TAX"_,
                                                  "L_TAX"_)),
                                 "Where"_("Greater"_("DateObject"_("1998-08-31"), "L_SHIPDATE"_))),
                       "As"_("L_QUANTITY"_, "L_QUANTITY"_, "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_,
                             "L_DISCOUNT"_, "L_DISCOUNT"_, "calc1"_, "Minus"_(1.0, "L_DISCOUNT"_),
                             "calc2"_, "Plus"_("L_TAX"_, 1.0))),
            "As"_("L_QUANTITY"_, "L_QUANTITY"_, "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_,
                  "L_DISCOUNT"_, "L_DISCOUNT"_, "disc_price"_,
                  "Times"_("L_EXTENDEDPRICE"_, "calc1"_), "calc2"_, "calc2"_)),
        "As"_("L_QUANTITY"_, "L_QUANTITY"_, "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_, "L_DISCOUNT"_,
              "L_DISCOUNT"_, "disc_price"_, "disc_price"_, "calc"_,
              "Times"_("disc_price"_, "calc2"_))));
    CHECK(output ==
          "Table"_("Column"_("L_QUANTITY"_, "List"_(17, 21, 8, 5)), // NOLINT
                   "Column"_("L_EXTENDEDPRICE"_,
                             "List"_(17954.55, 34850.16, 7712.48, 25284.00)), // NOLINT
                   "Column"_("L_DISCOUNT"_, "List"_(0.10, 0.05, 0.06, 0.06)), // NOLINT
                   "Column"_("disc_price"_,
                             "List"_(17954.55 * (1.0 - 0.10), 34850.16 * (1.0 - 0.05),    // NOLINT
                                     7712.48 * (1.0 - 0.06), 25284.00 * (1.0 - 0.06))),   // NOLINT
                   "Column"_("calc"_, "List"_(17954.55 * (1.0 - 0.10) * (0.02 + 1.0),     // NOLINT
                                              34850.16 * (1.0 - 0.05) * (0.06 + 1.0),     // NOLINT
                                              7712.48 * (1.0 - 0.06) * (0.02 + 1.0),      // NOLINT
                                              25284.00 * (1.0 - 0.06) * (0.06 + 1.0))))); // NOLINT
  }

  SECTION("Q1 (No Order, No Strings)") {
    auto output = eval("Group"_(
        "Project"_(
            "Project"_(
                "Project"_(
                    "Select"_(
                        "Project"_(lineitem.clone(CloneReason::FOR_TESTING),
                                   "As"_("L_QUANTITY"_, "L_QUANTITY"_, "L_DISCOUNT"_, "L_DISCOUNT"_,
                                         "L_SHIPDATE"_, "L_SHIPDATE"_, "L_EXTENDEDPRICE"_,
                                         "L_EXTENDEDPRICE"_, "L_RETURNFLAG_INT"_,
                                         "L_RETURNFLAG_INT"_, "L_LINESTATUS_INT"_,
                                         "L_LINESTATUS_INT"_, "L_TAX"_, "L_TAX"_)),
                        "Where"_("Greater"_("DateObject"_("1998-08-31"), "L_SHIPDATE"_))),
                    "As"_("L_RETURNFLAG_INT"_, "L_RETURNFLAG_INT"_, "L_LINESTATUS_INT"_,
                          "L_LINESTATUS_INT"_, "L_QUANTITY"_, "L_QUANTITY"_, "L_EXTENDEDPRICE"_,
                          "L_EXTENDEDPRICE"_, "L_DISCOUNT"_, "L_DISCOUNT"_, "calc1"_,
                          "Minus"_(1.0, "L_DISCOUNT"_), "calc2"_, "Plus"_("L_TAX"_, 1.0))),
                "As"_("L_RETURNFLAG_INT"_, "L_RETURNFLAG_INT"_, "L_LINESTATUS_INT"_,
                      "L_LINESTATUS_INT"_, "L_QUANTITY"_, "L_QUANTITY"_, "L_EXTENDEDPRICE"_,
                      "L_EXTENDEDPRICE"_, "L_DISCOUNT"_, "L_DISCOUNT"_, "disc_price"_,
                      "Times"_("L_EXTENDEDPRICE"_, "calc1"_), "calc2"_, "calc2"_)),
            "As"_("L_RETURNFLAG_INT"_, "L_RETURNFLAG_INT"_, "L_LINESTATUS_INT"_,
                  "L_LINESTATUS_INT"_, "L_QUANTITY"_, "L_QUANTITY"_, "L_EXTENDEDPRICE"_,
                  "L_EXTENDEDPRICE"_, "L_DISCOUNT"_, "L_DISCOUNT"_, "disc_price"_, "disc_price"_,
                  "calc"_, "Times"_("disc_price"_, "calc2"_))),
        "By"_("L_RETURNFLAG_INT"_, "L_LINESTATUS_INT"_),
        "As"_("SUM_QTY"_, "Sum"_("L_QUANTITY"_), "SUM_BASE_PRICE"_, "Sum"_("L_EXTENDEDPRICE"_),
              "SUM_DISC_PRICE"_, "Sum"_("disc_price"_), "SUM_CHARGES"_, "Sum"_("calc"_), "AVG_QTY"_,
              "Avg"_("L_QUANTITY"_), "AVG_PRICE"_, "Avg"_("L_EXTENDEDPRICE"_), "AVG_DISC"_,
              "Avg"_("l_discount"_), "COUNT_ORDER"_, "Count"_("*"_))));
    CHECK(output ==
          "Table"_("Column"_("L_RETURNFLAG_INT"_, "List"_('N'_i64, 'A'_i64)), // NOLINT
                   "Column"_("L_LINESTATUS_INT"_, "List"_('O'_i64, 'F'_i64)), // NOLINT
                   "Column"_("SUM_QTY"_, "List"_(17 + 21, 8 + 5)),            // NOLINT
                   "Column"_("SUM_BASE_PRICE"_,
                             "List"_(17954.55 + 34850.16, 7712.48 + 25284.00)), // NOLINT
                   "Column"_("SUM_DISC_PRICE"_,
                             "List"_(17954.55 * (1.0 - 0.10) + 34850.16 * (1.0 - 0.05),  // NOLINT
                                     7712.48 * (1.0 - 0.06) + 25284.00 * (1.0 - 0.06))), // NOLINT
                   "Column"_("SUM_CHARGES"_,
                             "List"_(17954.55 * (1.0 - 0.10) * (0.02 + 1.0) +             // NOLINT
                                         34850.16 * (1.0 - 0.05) * (0.06 + 1.0),          // NOLINT
                                     7712.48 * (1.0 - 0.06) * (0.02 + 1.0) +              // NOLINT
                                         25284.00 * (1.0 - 0.06) * (0.06 + 1.0))),        // NOLINT
                   "Column"_("AVG_PRICE"_, "List"_((17954.55 + 34850.16) / 2,             // NOLINT
                                                   (7712.48 + 25284.00) / 2)),            // NOLINT
                   "Column"_("AVG_DISC"_, "List"_((0.10 + 0.05) / 2, (0.06 + 0.06) / 2)), // NOLINT
                   "Column"_("COUNT_ORDER"_, "List"_(2, 2))));                            // NOLINT
  }

  SECTION("Q1 (No Order)") {
    auto output = eval("Group"_(
        "Project"_(
            "Project"_(
                "Project"_(
                    "Select"_(
                        "Project"_(lineitem.clone(CloneReason::FOR_TESTING),
                                   "As"_("L_QUANTITY"_, "L_QUANTITY"_, "L_DISCOUNT"_, "L_DISCOUNT"_,
                                         "L_SHIPDATE"_, "L_SHIPDATE"_, "L_EXTENDEDPRICE"_,
                                         "L_EXTENDEDPRICE"_, "L_RETURNFLAG"_, "L_RETURNFLAG"_,
                                         "L_LINESTATUS"_, "L_LINESTATUS"_, "L_TAX"_, "L_TAX"_)),
                        "Where"_("Greater"_("DateObject"_("1998-08-31"), "L_SHIPDATE"_))),
                    "As"_("L_RETURNFLAG"_, "L_RETURNFLAG"_, "L_LINESTATUS"_, "L_LINESTATUS"_,
                          "L_QUANTITY"_, "L_QUANTITY"_, "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_,
                          "L_DISCOUNT"_, "L_DISCOUNT"_, "calc1"_, "Minus"_(1.0, "L_DISCOUNT"_),
                          "calc2"_, "Plus"_("L_TAX"_, 1.0))),
                "As"_("L_RETURNFLAG"_, "L_RETURNFLAG"_, "L_LINESTATUS"_, "L_LINESTATUS"_,
                      "L_QUANTITY"_, "L_QUANTITY"_, "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_,
                      "L_DISCOUNT"_, "L_DISCOUNT"_, "disc_price"_,
                      "Times"_("L_EXTENDEDPRICE"_, "calc1"_), "calc2"_, "calc2"_)),
            "As"_("L_RETURNFLAG"_, "L_RETURNFLAG"_, "L_LINESTATUS"_, "L_LINESTATUS"_, "L_QUANTITY"_,
                  "L_QUANTITY"_, "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_, "L_DISCOUNT"_,
                  "L_DISCOUNT"_, "disc_price"_, "disc_price"_, "calc"_,
                  "Times"_("disc_price"_, "calc2"_))),
        "By"_("L_RETURNFLAG"_, "L_LINESTATUS"_),
        "As"_("SUM_QTY"_, "Sum"_("L_QUANTITY"_), "SUM_BASE_PRICE"_, "Sum"_("L_EXTENDEDPRICE"_),
              "SUM_DISC_PRICE"_, "Sum"_("disc_price"_), "SUM_CHARGES"_, "Sum"_("calc"_), "AVG_QTY"_,
              "Avg"_("L_QUANTITY"_), "AVG_PRICE"_, "Avg"_("L_EXTENDEDPRICE"_), "AVG_DISC"_,
              "Avg"_("l_discount"_), "COUNT_ORDER"_, "Count"_("*"_))));
    CHECK(output ==
          "Table"_("Column"_("L_RETURNFLAG"_, "List"_("N", "A")),  // NOLINT
                   "Column"_("L_LINESTATUS"_, "List"_("O", "F")),  // NOLINT
                   "Column"_("SUM_QTY"_, "List"_(17 + 21, 8 + 5)), // NOLINT
                   "Column"_("SUM_BASE_PRICE"_,
                             "List"_(17954.55 + 34850.16, 7712.48 + 25284.00)), // NOLINT
                   "Column"_("SUM_DISC_PRICE"_,
                             "List"_(17954.55 * (1.0 - 0.10) + 34850.16 * (1.0 - 0.05),  // NOLINT
                                     7712.48 * (1.0 - 0.06) + 25284.00 * (1.0 - 0.06))), // NOLINT
                   "Column"_("SUM_CHARGES"_,
                             "List"_(17954.55 * (1.0 - 0.10) * (0.02 + 1.0) +             // NOLINT
                                         34850.16 * (1.0 - 0.05) * (0.06 + 1.0),          // NOLINT
                                     7712.48 * (1.0 - 0.06) * (0.02 + 1.0) +              // NOLINT
                                         25284.00 * (1.0 - 0.06) * (0.06 + 1.0))),        // NOLINT
                   "Column"_("AVG_PRICE"_, "List"_((17954.55 + 34850.16) / 2,             // NOLINT
                                                   (7712.48 + 25284.00) / 2)),            // NOLINT
                   "Column"_("AVG_DISC"_, "List"_((0.10 + 0.05) / 2, (0.06 + 0.06) / 2)), // NOLINT
                   "Column"_("COUNT_ORDER"_, "List"_(2, 2))));                            // NOLINT
  }

  SECTION("Q1") {
    auto output = eval("Order"_(
        "Group"_(
            "Project"_(
                "Project"_(
                    "Project"_(
                        "Select"_("Project"_(lineitem.clone(CloneReason::FOR_TESTING),
                                             "As"_("L_QUANTITY"_, "L_QUANTITY"_, "L_DISCOUNT"_,
                                                   "L_DISCOUNT"_, "L_SHIPDATE"_, "L_SHIPDATE"_,
                                                   "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_,
                                                   "L_RETURNFLAG"_, "L_RETURNFLAG"_,
                                                   "L_LINESTATUS"_, "L_LINESTATUS"_, "L_TAX"_,
                                                   "L_TAX"_)),
                                  "Where"_("Greater"_("DateObject"_("1998-08-31"), "L_SHIPDATE"_))),
                        "As"_("L_RETURNFLAG"_, "L_RETURNFLAG"_, "L_LINESTATUS"_, "L_LINESTATUS"_,
                              "L_QUANTITY"_, "L_QUANTITY"_, "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_,
                              "L_DISCOUNT"_, "L_DISCOUNT"_, "calc1"_, "Minus"_(1.0, "L_DISCOUNT"_),
                              "calc2"_, "Plus"_("L_TAX"_, 1.0))),
                    "As"_("L_RETURNFLAG"_, "L_RETURNFLAG"_, "L_LINESTATUS"_, "L_LINESTATUS"_,
                          "L_QUANTITY"_, "L_QUANTITY"_, "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_,
                          "L_DISCOUNT"_, "L_DISCOUNT"_, "disc_price"_,
                          "Times"_("L_EXTENDEDPRICE"_, "calc1"_), "calc2"_, "calc2"_)),
                "As"_("L_RETURNFLAG"_, "L_RETURNFLAG"_, "L_LINESTATUS"_, "L_LINESTATUS"_,
                      "L_QUANTITY"_, "L_QUANTITY"_, "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_,
                      "L_DISCOUNT"_, "L_DISCOUNT"_, "disc_price"_, "disc_price"_, "calc"_,
                      "Times"_("disc_price"_, "calc2"_))),
            "By"_("L_RETURNFLAG"_, "L_LINESTATUS"_),
            "As"_("SUM_QTY"_, "Sum"_("L_QUANTITY"_), "SUM_BASE_PRICE"_, "Sum"_("L_EXTENDEDPRICE"_),
                  "SUM_DISC_PRICE"_, "Sum"_("disc_price"_), "SUM_CHARGES"_, "Sum"_("calc"_),
                  "AVG_QTY"_, "Avg"_("L_QUANTITY"_), "AVG_PRICE"_, "Avg"_("L_EXTENDEDPRICE"_),
                  "AVG_DISC"_, "Avg"_("l_discount"_), "COUNT_ORDER"_, "Count"_("*"_))),
        "By"_("L_RETURNFLAG"_, "L_LINESTATUS"_)));
    CHECK(output ==
          "Table"_("Column"_("L_RETURNFLAG"_, "List"_("A", "N")),  // NOLINT
                   "Column"_("L_LINESTATUS"_, "List"_("F", "O")),  // NOLINT
                   "Column"_("SUM_QTY"_, "List"_(8 + 5, 17 + 21)), // NOLINT
                   "Column"_("SUM_BASE_PRICE"_,
                             "List"_(7712.48 + 25284.00, 17954.55 + 34850.16)), // NOLINT
                   "Column"_("SUM_DISC_PRICE"_,
                             "List"_(7712.48 * (1.0 - 0.06) + 25284.00 * (1.0 - 0.06),    // NOLINT
                                     17954.55 * (1.0 - 0.10) + 34850.16 * (1.0 - 0.05))), // NOLINT
                   "Column"_("SUM_CHARGES"_,
                             "List"_(7712.48 * (1.0 - 0.06) * (0.02 + 1.0) +              // NOLINT
                                         25284.00 * (1.0 - 0.06) * (0.06 + 1.0),          // NOLINT
                                     17954.55 * (1.0 - 0.10) * (0.02 + 1.0) +             // NOLINT
                                         34850.16 * (1.0 - 0.05) * (0.06 + 1.0))),        // NOLINT
                   "Column"_("AVG_PRICE"_, "List"_((7712.48 + 25284.00) / 2,              // NOLINT
                                                   (17954.55 + 34850.16) / 2)),           // NOLINT
                   "Column"_("AVG_DISC"_, "List"_((0.06 + 0.06) / 2, (0.10 + 0.05) / 2)), // NOLINT
                   "Column"_("COUNT_ORDER"_, "List"_(2, 2))));                            // NOLINT
  }

  SECTION("Q6 (No Grouping)") {
    auto output = eval("Project"_(
        "Select"_(
            "Project"_(lineitem.clone(CloneReason::FOR_TESTING),
                       "As"_("L_QUANTITY"_, "L_QUANTITY"_, "L_DISCOUNT"_, "L_DISCOUNT"_,
                             "L_SHIPDATE"_, "L_SHIPDATE"_, "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_)),
            "Where"_("And"_("Greater"_(24, "L_QUANTITY"_),      // NOLINT
                            "Greater"_("L_DISCOUNT"_, 0.0499),  // NOLINT
                            "Greater"_(0.07001, "L_DISCOUNT"_), // NOLINT
                            "Greater"_("DateObject"_("1995-01-01"), "L_SHIPDATE"_),
                            "Greater"_("L_SHIPDATE"_, "DateObject"_("1993-12-31"))))),
        "As"_("revenue"_, "Times"_("L_EXTENDEDPRICE"_, "L_DISCOUNT"_))));
    CHECK(output ==
          "Table"_("Column"_("revenue"_, "List"_(34850.16 * 0.05, 25284.00 * 0.06)))); // NOLINT
  }

  SECTION("Q6") {
    auto output = eval("Group"_(
        "Project"_(
            "Select"_("Project"_(lineitem.clone(CloneReason::FOR_TESTING),
                                 "As"_("L_QUANTITY"_, "L_QUANTITY"_, "L_DISCOUNT"_, "L_DISCOUNT"_,
                                       "L_SHIPDATE"_, "L_SHIPDATE"_, "L_EXTENDEDPRICE"_,
                                       "L_EXTENDEDPRICE"_)),
                      "Where"_("And"_("Greater"_(24, "L_QUANTITY"_),      // NOLINT
                                      "Greater"_("L_DISCOUNT"_, 0.0499),  // NOLINT
                                      "Greater"_(0.07001, "L_DISCOUNT"_), // NOLINT
                                      "Greater"_("DateObject"_("1995-01-01"), "L_SHIPDATE"_),
                                      "Greater"_("L_SHIPDATE"_, "DateObject"_("1993-12-31"))))),
            "As"_("revenue"_, "Times"_("L_EXTENDEDPRICE"_, "L_DISCOUNT"_))),
        "Sum"_("revenue"_)));
    CHECK(output ==
          "Table"_("Column"_("revenue"_, "List"_(34850.16 * 0.05 + 25284.00 * 0.06)))); // NOLINT
  }

  SECTION("Q3 (No Strings)") {
    auto output = eval("Top"_(
        "Group"_(
            "Project"_(
                "Join"_(
                    "Project"_(
                        "Join"_(
                            "Project"_(
                                "Select"_("Project"_(customer.clone(CloneReason::FOR_TESTING),
                                                     "As"_("C_CUSTKEY"_, "C_CUSTKEY"_, "C_ACCTBAL"_,
                                                           "C_ACCTBAL"_)),
                                          "Where"_("Equal"_("C_ACCTBAL"_, 2866.83))), // NOLINT
                                "As"_("C_CUSTKEY"_, "C_CUSTKEY"_)),
                            "Select"_(
                                "Project"_(orders.clone(CloneReason::FOR_TESTING),
                                           "As"_("O_ORDERKEY"_, "O_ORDERKEY"_, "O_ORDERDATE"_,
                                                 "O_ORDERDATE"_, "O_CUSTKEY"_, "O_CUSTKEY"_,
                                                 "O_SHIPPRIORITY"_, "O_SHIPPRIORITY"_)),
                                "Where"_("Greater"_("DateObject"_("1995-03-15"), "O_ORDERDATE"_))),
                            "Where"_("Equal"_("C_CUSTKEY"_, "O_CUSTKEY"_))),
                        "As"_("O_ORDERKEY"_, "O_ORDERKEY"_, "O_ORDERDATE"_, "O_ORDERDATE"_,
                              "O_CUSTKEY"_, "O_CUSTKEY"_, "O_SHIPPRIORITY"_, "O_SHIPPRIORITY"_)),
                    "Project"_(
                        "Select"_("Project"_(lineitem.clone(CloneReason::FOR_TESTING),
                                             "As"_("L_ORDERKEY"_, "L_ORDERKEY"_, "L_DISCOUNT"_,
                                                   "L_DISCOUNT"_, "L_SHIPDATE"_, "L_SHIPDATE"_,
                                                   "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_)),
                                  "Where"_("Greater"_("L_SHIPDATE"_, "DateObject"_("1993-03-15")))),
                        "As"_("L_ORDERKEY"_, "L_ORDERKEY"_, "L_DISCOUNT"_, "L_DISCOUNT"_,
                              "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_)),
                    "Where"_("Equal"_("O_ORDERKEY"_, "L_ORDERKEY"_))),
                "As"_("Expr1009"_, "Times"_("L_EXTENDEDPRICE"_, "Minus"_(1.0, "L_DISCOUNT"_)),
                      "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_, "L_ORDERKEY"_, "L_ORDERKEY"_,
                      "O_ORDERDATE"_, "O_ORDERDATE"_, "O_SHIPPRIORITY"_, "O_SHIPPRIORITY"_)),
            "By"_("L_ORDERKEY"_, "O_ORDERDATE"_, "O_SHIPPRIORITY"_),
            "As"_("revenue"_, "Sum"_("Expr1009"_))),
        "By"_("revenue"_, "desc"_, "O_ORDERDATE"_), 10)); // NOLINT
    CHECK(output == "Dummy"_());
  }

  SECTION("Q3") {
    auto output = eval("Top"_(
        "Group"_(
            "Project"_(
                "Join"_(
                    "Project"_(
                        "Join"_(
                            "Project"_(
                                "Select"_(
                                    "Project"_(customer.clone(CloneReason::FOR_TESTING),
                                               "As"_("C_CUSTKEY"_, "C_CUSTKEY"_, "C_MKTSEGMENT"_,
                                                     "C_MKTSEGMENT"_)),
                                    "Where"_("StringContainsQ"_("C_MKTSEGMENT"_, "BUILDING"))),
                                "As"_("C_CUSTKEY"_, "C_CUSTKEY"_)),
                            "Select"_(
                                "Project"_(orders.clone(CloneReason::FOR_TESTING),
                                           "As"_("O_ORDERKEY"_, "O_ORDERKEY"_, "O_ORDERDATE"_,
                                                 "O_ORDERDATE"_, "O_CUSTKEY"_, "O_CUSTKEY"_,
                                                 "O_SHIPPRIORITY"_, "O_SHIPPRIORITY"_)),
                                "Where"_("Greater"_("DateObject"_("1995-03-15"), "O_ORDERDATE"_))),
                            "Where"_("Equal"_("C_CUSTKEY"_, "O_CUSTKEY"_))),
                        "As"_("O_ORDERKEY"_, "O_ORDERKEY"_, "O_ORDERDATE"_, "O_ORDERDATE"_,
                              "O_CUSTKEY"_, "O_CUSTKEY"_, "O_SHIPPRIORITY"_, "O_SHIPPRIORITY"_)),
                    "Project"_(
                        "Select"_("Project"_(lineitem.clone(CloneReason::FOR_TESTING),
                                             "As"_("L_ORDERKEY"_, "L_ORDERKEY"_, "L_DISCOUNT"_,
                                                   "L_DISCOUNT"_, "L_SHIPDATE"_, "L_SHIPDATE"_,
                                                   "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_)),
                                  "Where"_("Greater"_("L_SHIPDATE"_, "DateObject"_("1993-03-15")))),
                        "As"_("L_ORDERKEY"_, "L_ORDERKEY"_, "L_DISCOUNT"_, "L_DISCOUNT"_,
                              "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_)),
                    "Where"_("Equal"_("O_ORDERKEY"_, "L_ORDERKEY"_))),
                "As"_("Expr1009"_, "Times"_("L_EXTENDEDPRICE"_, "Minus"_(1.0, "L_DISCOUNT"_)),
                      "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_, "L_ORDERKEY"_, "L_ORDERKEY"_,
                      "O_ORDERDATE"_, "O_ORDERDATE"_, "O_SHIPPRIORITY"_, "O_SHIPPRIORITY"_)),
            "By"_("L_ORDERKEY"_, "O_ORDERDATE"_, "O_SHIPPRIORITY"_),
            "As"_("revenue"_, "Sum"_("Expr1009"_))),
        "By"_("revenue"_, "desc"_, "O_ORDERDATE"_), 10)); // NOLINT
    CHECK(output == "Dummy"_());
  }

  SECTION("Q9 (No Strings)") {
    auto output = eval("Order"_(
        "Group"_(
            "Project"_(
                "Join"_(
                    "Project"_(
                        "Join"_(
                            "Project"_(
                                "Join"_(
                                    "Project"_(
                                        "Select"_(
                                            "Project"_(part.clone(CloneReason::FOR_TESTING),
                                                       "As"_("P_PARTKEY"_, "P_PARTKEY"_,
                                                             "P_RETAILPRICE"_, "P_RETAILPRICE"_)),
                                            "Where"_("Equal"_("P_RETAILPRICE"_, 100.01))), // NOLINT
                                        "As"_("P_PARTKEY"_, "P_PARTKEY"_)),
                                    "Project"_(
                                        "Join"_(
                                            "Project"_(
                                                "Join"_(
                                                    "Project"_(
                                                        nation.clone(CloneReason::FOR_TESTING),
                                                        "As"_("N_REGIONKEY"_, "N_REGIONKEY"_,
                                                              "N_NATIONKEY"_, "N_NATIONKEY"_)),
                                                    "Project"_(
                                                        supplier.clone(CloneReason::FOR_TESTING),
                                                        "As"_("S_SUPPKEY"_, "S_SUPPKEY"_,
                                                              "S_NATIONKEY"_, "S_NATIONKEY"_)),
                                                    "Where"_(
                                                        "Equal"_("N_NATIONKEY"_, "S_NATIONKEY"_))),
                                                "As"_("N_REGIONKEY"_, "N_REGIONKEY"_, "S_SUPPKEY"_,
                                                      "S_SUPPKEY"_)),
                                            "Project"_(partsupp.clone(CloneReason::FOR_TESTING),
                                                       "As"_("PS_PARTKEY"_, "PS_PARTKEY"_,
                                                             "PS_SUPPKEY"_, "PS_SUPPKEY"_,
                                                             "PS_SUPPLYCOST"_, "PS_SUPPLYCOST"_)),
                                            "Where"_("Equal"_("S_SUPPKEY"_, "PS_SUPPKEY"_))),
                                        "As"_("N_REGIONKEY"_, "N_REGIONKEY"_, "PS_PARTKEY"_,
                                              "PS_PARTKEY"_, "PS_SUPPKEY"_, "PS_SUPPKEY"_,
                                              "PS_SUPPLYCOST"_, "PS_SUPPLYCOST"_)),
                                    "Where"_("Equal"_("P_PARTKEY"_, "PS_PARTKEY"_))),
                                "As"_("N_REGIONKEY"_, "N_REGIONKEY"_, "PS_PARTKEY"_, "PS_PARTKEY"_,
                                      "PS_SUPPKEY"_, "PS_SUPPKEY"_, "PS_SUPPLYCOST"_,
                                      "PS_SUPPLYCOST"_)),
                            "Project"_(lineitem.clone(CloneReason::FOR_TESTING),
                                       "As"_("L_PARTKEY"_, "L_PARTKEY"_, "L_SUPPKEY"_, "L_SUPPKEY"_,
                                             "L_ORDERKEY"_, "L_ORDERKEY"_, "L_EXTENDEDPRICE"_,
                                             "L_EXTENDEDPRICE"_, "L_DISCOUNT"_, "L_DISCOUNT"_,
                                             "L_QUANTITY"_, "L_QUANTITY"_)),
                            "Where"_("Equal"_("List"_("PS_PARTKEY"_, "PS_SUPPKEY"_),
                                              "List"_("L_PARTKEY"_, "L_SUPPKEY"_)))),
                        "As"_("N_REGIONKEY"_, "N_REGIONKEY"_, "PS_SUPPLYCOST"_, "PS_SUPPLYCOST"_,
                              "L_ORDERKEY"_, "L_ORDERKEY"_, "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_,
                              "L_DISCOUNT"_, "L_DISCOUNT"_, "L_QUANTITY"_, "L_QUANTITY"_)),
                    "Project"_(orders.clone(CloneReason::FOR_TESTING),
                               "As"_("O_ORDERKEY"_, "O_ORDERKEY"_, "O_ORDERDATE"_, "O_ORDERDATE"_)),
                    "Where"_("Equal"_("O_ORDERKEY"_, "L_ORDERKEY"_))),
                "As"_("nation"_, "N_REGIONKEY"_, "O_YEAR"_, "Year"_("O_ORDERDATE"_), "amount"_,
                      "Minus"_("Times"_("L_EXTENDEDPRICE"_, "Minus"_(1.0, "L_DISCOUNT"_)),
                               "Times"_("PS_SUPPLYCOST"_, "L_QUANTITY"_)))),
            "By"_("nation"_, "O_YEAR"_), "Sum"_("amount"_)),
        "By"_("nation"_, "O_YEAR"_, "desc"_)));
    CHECK(output == "Dummy"_());
  }

  SECTION("Q9") {
    auto output = eval("Order"_(
        "Group"_(
            "Project"_(
                "Join"_(
                    "Project"_(
                        "Join"_(
                            "Project"_(
                                "Join"_(
                                    "Project"_(
                                        "Select"_("Project"_(part.clone(CloneReason::FOR_TESTING),
                                                             "As"_("P_PARTKEY"_, "P_PARTKEY"_,
                                                                   "P_NAME"_, "P_NAME"_)),
                                                  "Where"_("StringContainsQ"_("P_NAME"_, "green"))),
                                        "As"_("P_PARTKEY"_, "P_PARTKEY"_)),
                                    "Project"_(
                                        "Join"_(
                                            "Project"_(
                                                "Join"_(
                                                    "Project"_(
                                                        nation.clone(CloneReason::FOR_TESTING),
                                                        "As"_("N_NAME"_, "N_NAME"_, "N_NATIONKEY"_,
                                                              "N_NATIONKEY"_)),
                                                    "Project"_(
                                                        supplier.clone(CloneReason::FOR_TESTING),
                                                        "As"_("S_SUPPKEY"_, "S_SUPPKEY"_,
                                                              "S_NATIONKEY"_, "S_NATIONKEY"_)),
                                                    "Where"_(
                                                        "Equal"_("N_NATIONKEY"_, "S_NATIONKEY"_))),
                                                "As"_("N_NAME"_, "N_NAME"_, "S_SUPPKEY"_,
                                                      "S_SUPPKEY"_)),
                                            "Project"_(partsupp.clone(CloneReason::FOR_TESTING),
                                                       "As"_("PS_PARTKEY"_, "PS_PARTKEY"_,
                                                             "PS_SUPPKEY"_, "PS_SUPPKEY"_,
                                                             "PS_SUPPLYCOST"_, "PS_SUPPLYCOST"_)),
                                            "Where"_("Equal"_("S_SUPPKEY"_, "PS_SUPPKEY"_))),
                                        "As"_("N_NAME"_, "N_NAME"_, "PS_PARTKEY"_, "PS_PARTKEY"_,
                                              "PS_SUPPKEY"_, "PS_SUPPKEY"_, "PS_SUPPLYCOST"_,
                                              "PS_SUPPLYCOST"_)),
                                    "Where"_("Equal"_("P_PARTKEY"_, "PS_PARTKEY"_))),
                                "As"_("N_NAME"_, "N_NAME"_, "PS_PARTKEY"_, "PS_PARTKEY"_,
                                      "PS_SUPPKEY"_, "PS_SUPPKEY"_, "PS_SUPPLYCOST"_,
                                      "PS_SUPPLYCOST"_)),
                            "Project"_(lineitem.clone(CloneReason::FOR_TESTING),
                                       "As"_("L_PARTKEY"_, "L_PARTKEY"_, "L_SUPPKEY"_, "L_SUPPKEY"_,
                                             "L_ORDERKEY"_, "L_ORDERKEY"_, "L_EXTENDEDPRICE"_,
                                             "L_EXTENDEDPRICE"_, "L_DISCOUNT"_, "L_DISCOUNT"_,
                                             "L_QUANTITY"_, "L_QUANTITY"_)),
                            "Where"_("Equal"_("List"_("PS_PARTKEY"_, "PS_SUPPKEY"_),
                                              "List"_("L_PARTKEY"_, "L_SUPPKEY"_)))),
                        "As"_("N_NAME"_, "N_NAME"_, "PS_SUPPLYCOST"_, "PS_SUPPLYCOST"_,
                              "L_ORDERKEY"_, "L_ORDERKEY"_, "L_EXTENDEDPRICE"_, "L_EXTENDEDPRICE"_,
                              "L_DISCOUNT"_, "L_DISCOUNT"_, "L_QUANTITY"_, "L_QUANTITY"_)),
                    "Project"_(orders.clone(CloneReason::FOR_TESTING),
                               "As"_("O_ORDERKEY"_, "O_ORDERKEY"_, "O_ORDERDATE"_, "O_ORDERDATE"_)),
                    "Where"_("Equal"_("O_ORDERKEY"_, "L_ORDERKEY"_))),
                "As"_("nation"_, "N_NAME"_, "O_YEAR"_, "Year"_("O_ORDERDATE"_), "amount"_,
                      "Minus"_("Times"_("L_EXTENDEDPRICE"_, "Minus"_(1.0, "L_DISCOUNT"_)),
                               "Times"_("PS_SUPPLYCOST"_, "L_QUANTITY"_)))),
            "By"_("nation"_, "O_YEAR"_), "Sum"_("amount"_)),
        "By"_("nation"_, "O_YEAR"_, "desc"_)));
    CHECK(output == "Dummy"_());
  }

  SECTION("Q18 (No Strings)") {
    auto output = eval("Top"_(
        "Group"_(
            "Project"_(
                "Join"_("Select"_("Group"_("Project"_(lineitem.clone(CloneReason::FOR_TESTING),
                                                      "As"_("L_ORDERKEY"_, "L_ORDERKEY"_,
                                                            "L_QUANTITY"_, "L_QUANTITY"_)),
                                           "By"_("L_ORDERKEY"_),
                                           "As"_("sum_l_quantity"_, "Sum"_("L_QUANTITY"_))),
                                  "Where"_("Greater"_("sum_l_quantity"_, 1.0))),
                        "Project"_(
                            "Join"_("Project"_(customer.clone(CloneReason::FOR_TESTING),
                                               "As"_("C_ACCTBAL"_, "C_ACCTBAL"_, "C_CUSTKEY"_,
                                                     "C_CUSTKEY"_)),
                                    "Project"_(orders.clone(CloneReason::FOR_TESTING),
                                               "As"_("O_ORDERKEY"_, "O_ORDERKEY"_, "O_CUSTKEY"_,
                                                     "O_CUSTKEY"_, "O_ORDERDATE"_, "O_ORDERDATE"_,
                                                     "O_TOTALPRICE"_, "O_TOTALPRICE"_)),
                                    "Where"_("Equal"_("C_CUSTKEY"_, "O_CUSTKEY"_))),
                            "As"_("C_ACCTBAL"_, "C_ACCTBAL"_, "O_ORDERKEY"_, "O_ORDERKEY"_,
                                  "O_CUSTKEY"_, "O_CUSTKEY"_, "O_ORDERDATE"_, "O_ORDERDATE"_,
                                  "O_TOTALPRICE"_, "O_TOTALPRICE"_)),
                        "Where"_("Equal"_("L_ORDERKEY"_, "O_ORDERKEY"_))),
                "As"_("O_ORDERKEY"_, "O_ORDERKEY"_, "O_ORDERDATE"_, "O_ORDERDATE"_, "O_TOTALPRICE"_,
                      "O_TOTALPRICE"_, "C_ACCTBAL"_, "C_ACCTBAL"_, "O_CUSTKEY"_, "O_CUSTKEY"_,
                      "sum_l_quantity"_, "sum_l_quantity"_)),
            "By"_("C_ACCTBAL"_, "O_CUSTKEY"_, "O_ORDERKEY"_, "O_ORDERDATE"_, "O_TOTALPRICE"_),
            "Sum"_("sum_l_quantity"_)),
        "By"_("O_TOTALPRICE"_, "desc"_, "O_ORDERDATE"_), 100)); // NOLINT
    CHECK(output == "Dummy"_());
  }

  SECTION("Q18") {
    auto output = eval("Top"_(
        "Group"_("Project"_(
                     "Join"_("Select"_("Group"_("Project"_(lineitem.clone(CloneReason::FOR_TESTING),
                                                           "As"_("L_ORDERKEY"_, "L_ORDERKEY"_,
                                                                 "L_QUANTITY"_, "L_QUANTITY"_)),
                                                "By"_("L_ORDERKEY"_),
                                                "As"_("sum_l_quantity"_, "Sum"_("L_QUANTITY"_))),
                                       "Where"_("Greater"_("sum_l_quantity"_, 1.0))), // NOLINT
                             "Project"_("Join"_("Project"_(customer.clone(CloneReason::FOR_TESTING),
                                                           "As"_("C_NAME"_, "C_NAME"_, "C_CUSTKEY"_,
                                                                 "C_CUSTKEY"_)),
                                                "Project"_(orders.clone(CloneReason::FOR_TESTING),
                                                           "As"_("O_ORDERKEY"_, "O_ORDERKEY"_,
                                                                 "O_CUSTKEY"_, "O_CUSTKEY"_,
                                                                 "O_ORDERDATE"_, "O_ORDERDATE"_,
                                                                 "O_TOTALPRICE"_, "O_TOTALPRICE"_)),
                                                "Where"_("Equal"_("C_CUSTKEY"_, "O_CUSTKEY"_))),
                                        "As"_("C_NAME"_, "C_NAME"_, "O_ORDERKEY"_, "O_ORDERKEY"_,
                                              "O_CUSTKEY"_, "O_CUSTKEY"_, "O_ORDERDATE"_,
                                              "O_ORDERDATE"_, "O_TOTALPRICE"_, "O_TOTALPRICE"_)),
                             "Where"_("Equal"_("L_ORDERKEY"_, "O_ORDERKEY"_))),
                     "As"_("O_ORDERKEY"_, "O_ORDERKEY"_, "O_ORDERDATE"_, "O_ORDERDATE"_,
                           "O_TOTALPRICE"_, "O_TOTALPRICE"_, "C_NAME"_, "C_NAME"_, "O_CUSTKEY"_,
                           "O_CUSTKEY"_, "sum_l_quantity"_, "sum_l_quantity"_)),
                 "By"_("C_NAME"_, "O_CUSTKEY"_, "O_ORDERKEY"_, "O_ORDERDATE"_, "O_TOTALPRICE"_),
                 "Sum"_("sum_l_quantity"_)),
        "By"_("O_TOTALPRICE"_, "desc"_, "O_ORDERDATE"_), 100)); // NOLINT
    CHECK(output == "Dummy"_());
  }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("Summation of numeric Spans", "[spans]", std::int32_t, std::int64_t,
                   std::float_t, std::double_t) {
  auto engine = boss::engines::BootstrapEngine();
  REQUIRE(!librariesToTest.empty());
  auto eval = [&engine](auto&& expression) mutable {
    return engine.evaluate("EvaluateInEngines"_("List"_(GENERATE(from_range(librariesToTest))),
                                                std::forward<decltype(expression)>(expression)));
  };

  auto input = GENERATE(take(3, chunk(50, random<TestType>(1, 1000))));
  auto sum = std::accumulate(begin(input), end(input), TestType());

  auto result = eval("Plus"_(boss::Span<TestType>(vector(input))));
  if constexpr(std::is_floating_point_v<TestType>) {
    CHECK(get<TestType>(result) == Catch::Detail::Approx((TestType)sum));
  } else {
    CHECK(get<TestType>(result) == sum);
  }
}

TEST_CASE("Expression Serialization") {
  auto const plans = std::array<boss::Expression, 8>{
      "Yo"_,
      "Howdie"_("Yo"_(5, 17, "duh"_(3)), "Five"_(6), 9, 1),
      "Howdie"_(1, 4, 9, "You"_(1, 3), 9, 3),
      "Top"_("Group"_(
                 "Project"_(
                     "Join"_("Select"_("Group"_("Project"_("lineitem"_,
                                                           "As"_("L_ORDERKEY"_, "L_ORDERKEY"_,
                                                                 "L_QUANTITY"_, "L_QUANTITY"_)),
                                                "By"_("L_ORDERKEY"_),
                                                "As"_("sum_l_quantity"_, "Sum"_("L_QUANTITY"_))),
                                       "Where"_("Greater"_("sum_l_quantity"_, 1.0))), // NOLINT
                             "Project"_(
                                 "Join"_("Project"_("customer"_, "As"_("C_NAME"_, "C_NAME"_,
                                                                       "C_CUSTKEY"_, "C_CUSTKEY"_)),
                                         "Project"_("orders"_,
                                                    "As"_("O_ORDERKEY"_, "O_ORDERKEY"_,
                                                          "O_CUSTKEY"_, "O_CUSTKEY"_,
                                                          "O_ORDERDATE"_, "O_ORDERDATE"_,
                                                          "O_TOTALPRICE"_, "O_TOTALPRICE"_)),
                                         "Where"_("Equal"_("C_CUSTKEY"_, "O_CUSTKEY"_))),
                                 "As"_("C_NAME"_, "C_NAME"_, "O_ORDERKEY"_, "O_ORDERKEY"_,
                                       "O_CUSTKEY"_, "O_CUSTKEY"_, "O_ORDERDATE"_, "O_ORDERDATE"_,
                                       "O_TOTALPRICE"_, "O_TOTALPRICE"_)),
                             "Where"_("Equal"_("L_ORDERKEY"_, "O_ORDERKEY"_))),
                     "As"_("O_ORDERKEY"_, "O_ORDERKEY"_, "O_ORDERDATE"_, "O_ORDERDATE"_,
                           "O_TOTALPRICE"_, "O_TOTALPRICE"_, "C_NAME"_, "C_NAME"_, "O_CUSTKEY"_,
                           "O_CUSTKEY"_, "sum_l_quantity"_, "sum_l_quantity"_)),
                 "By"_("C_NAME"_, "O_CUSTKEY"_, "O_ORDERKEY"_, "O_ORDERDATE"_, "O_TOTALPRICE"_),
                 "Sum"_("sum_l_quantity"_)),
             "By"_("O_TOTALPRICE"_, "desc"_, "O_ORDERDATE"_), 100),
      "Table"_("Something"_(5, 17, "Sum"_(3, 9, 2)), "Else"_(6, "Date"_())),
      "Table"_(1, 5, 9),
      Expression(3),
      "SetDefaultEnginePipeline"_(
          "/Users/hlgr/Temp/BOSSWolframEngine/Debug/libBOSSWolframEngine.so")};
  for(auto const& plan : plans) {
    CHECK(boss::serialization::SerializedExpression(plan.clone(CloneReason::FOR_TESTING))
              .deserialize() == plan);
  }
}

TEST_CASE("Laxy Expression Serialization") {
  auto const plans = std::array<boss::Expression, 8>{
      "HiThere"_(1, 4, 9, "You"_(1, 3), 9, 3),
      "Howdie"_("Yo"_(5, 17, "duh"_(3)), "Five"_(6), 9, 1),
      "Yo"_,
      "Top"_("Group"_(
                 "Project"_(
                     "Join"_("Select"_("Group"_("Project"_("lineitem"_,
                                                           "As"_("L_ORDERKEY"_, "L_ORDERKEY"_,
                                                                 "L_QUANTITY"_, "L_QUANTITY"_)),
                                                "By"_("L_ORDERKEY"_),
                                                "As"_("sum_l_quantity"_, "Sum"_("L_QUANTITY"_))),
                                       "Where"_("Greater"_("sum_l_quantity"_, 1.0))), // NOLINT
                             "Project"_(
                                 "Join"_("Project"_("customer"_, "As"_("C_NAME"_, "C_NAME"_,
                                                                       "C_CUSTKEY"_, "C_CUSTKEY"_)),
                                         "Project"_("orders"_,
                                                    "As"_("O_ORDERKEY"_, "O_ORDERKEY"_,
                                                          "O_CUSTKEY"_, "O_CUSTKEY"_,
                                                          "O_ORDERDATE"_, "O_ORDERDATE"_,
                                                          "O_TOTALPRICE"_, "O_TOTALPRICE"_)),
                                         "Where"_("Equal"_("C_CUSTKEY"_, "O_CUSTKEY"_))),
                                 "As"_("C_NAME"_, "C_NAME"_, "O_ORDERKEY"_, "O_ORDERKEY"_,
                                       "O_CUSTKEY"_, "O_CUSTKEY"_, "O_ORDERDATE"_, "O_ORDERDATE"_,
                                       "O_TOTALPRICE"_, "O_TOTALPRICE"_)),
                             "Where"_("Equal"_("L_ORDERKEY"_, "O_ORDERKEY"_))),
                     "As"_("O_ORDERKEY"_, "O_ORDERKEY"_, "O_ORDERDATE"_, "O_ORDERDATE"_,
                           "O_TOTALPRICE"_, "O_TOTALPRICE"_, "C_NAME"_, "C_NAME"_, "O_CUSTKEY"_,
                           "O_CUSTKEY"_, "sum_l_quantity"_, "sum_l_quantity"_)),
                 "By"_("C_NAME"_, "O_CUSTKEY"_, "O_ORDERKEY"_, "O_ORDERDATE"_, "O_TOTALPRICE"_),
                 "Sum"_("sum_l_quantity"_)),
             "By"_("O_TOTALPRICE"_, "desc"_, "O_ORDERDATE"_), 100),
      "Table"_("Something"_(5, 17, "Sum"_(3, 9, 2)), "Else"_(6, "Date"_())),
      "Table"_(1, 5, 9),
      Expression(3),
      "SetDefaultEnginePipeline"_(
          "/Users/hlgr/Temp/BOSSWolframEngine/Debug/libBOSSWolframEngine.so")};

  {
    auto e = boss::serialization::SerializedExpression(Expression(3));
    CHECK(!(e.lazilyDeserialize() == "Thingy"_));
  }
  {
    auto e = boss::serialization::SerializedExpression(
        "H"_("O"_("W"_(1, 5, 9)), "D"_("I"_(6, 1), "E"_(2))));
    CHECK(e.lazilyDeserialize() ==
          (Expression) "H"_("O"_("W"_(1, 5, 9)), "D"_("I"_(6, 1), "E"_(2))));
  }
  {
    auto e = boss::serialization::SerializedExpression("Table"_(1, 5, 9));
    CHECK(!(e.lazilyDeserialize() == "Table"_(1, 5, 10)));
  }

  for(auto const& plan : plans) {
    auto e = boss::serialization::SerializedExpression(plan.clone(CloneReason::FOR_TESTING));
    CHECK(e.lazilyDeserialize() == plan);
    CHECK(!(e.lazilyDeserialize() == "Thingy"_));
  }
}

int main(int argc, char* argv[]) {
  Catch::Session session;
  session.cli(session.cli() | Catch::clara::Opt(librariesToTest, "library")["--library"]);
  auto const returnCode = session.applyCommandLine(argc, argv);
  if(returnCode != 0) {
    return returnCode;
  }
  return session.run();
}
// NOLINTEND(readability-function-cognitive-complexity)
// NOLINTEND(bugprone-exception-escape)
// NOLINTEND(readability-magic-numbers)
