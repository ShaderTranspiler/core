#include "types/types.h"

namespace stc::types {

bool Type::has_qualifiers(Qualifier filter) const {
    return has_any(qualifiers, filter);
}

std::string Type::to_string() const {
    return "TypeId: " + std::to_string(base_type);
}

} // namespace stc::types