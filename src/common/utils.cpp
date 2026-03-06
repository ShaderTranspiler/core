#include <iostream>

#include "common/utils.h"

namespace stc {

std::nullptr_t report(std::string_view msg, bool is_warning, std::ostream& out) {
    out << (is_warning ? "warning: " : "error: ") << msg << '\n';

    return nullptr;
}

std::nullptr_t error(std::string_view msg, std::ostream& out) {
    return report(msg, false, out);
}

std::nullptr_t warning(std::string_view msg, std::ostream& out) {
    return report(msg, true, out);
}

} // namespace stc