#include <algorithm>

#include "ast/scope.h"
#include "errors.h"
#include "ast/tokens/token.h"

namespace stride::ast
{
    bool Scope::is_symbol_defined(const Symbol& symbol) const
    {
        return std::ranges::find_if(this->symbols->begin(), this->symbols->end(), [&](const Symbol& s)
        {
            return s.value == symbol.value;
        }) != this->symbols->end();
    }

    void Scope::try_define_symbol(const SourceFile& source, const size_t source_offset, const Symbol& symbol) const
    {
        if (this->is_symbol_defined(symbol))
        {
            throw parsing_error(
                make_source_error(
                    source,
                    ErrorType::SEMANTIC_ERROR,
                    "Symbol already defined in this scope",
                    source_offset,
                    symbol.value.size()
                )
            );
        }
        this->symbols->push_back(symbol);
    }
}
