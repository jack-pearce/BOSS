#pragma once
#include "BOSS.h"
#include "Engine.hpp"
#include "Expression.hpp"

template <typename... Args>
auto freeExpression(Args&&... args) -> decltype(freeBOSSExpression(std::forward<Args>(args)...)) {
  return freeBOSSExpression(std::forward<Args>(args)...);
}

struct BOSSExpression {
  boss::Expression delegate;
};
struct BOSSSymbol {
  boss::Symbol delegate;
};

namespace boss {
expressions::Expression evaluate(expressions::Expression&& expr);
}
