#pragma once

#include "backend/glsl/types.h"
#include "common/backend_info.h"

namespace stc::glsl {

class GLSLBackendInfo final : public BackendInfo {
public:
    GLSLBackendInfo(const GLSLBackendInfo&)            = delete;
    GLSLBackendInfo& operator=(const GLSLBackendInfo&) = delete;
    GLSLBackendInfo(GLSLBackendInfo&&)                 = delete;
    GLSLBackendInfo& operator=(GLSLBackendInfo&&)      = delete;

    static const GLSLBackendInfo& get(GLSLTypes& types) {
        static GLSLBackendInfo instance = create_instance(types);
        return instance;
    }

    bool can_implicit_cast(TypeId src_ty, TypeId dest_ty) const override;

private:
    explicit GLSLBackendInfo(GlobalList builtin_globals, FnList builtin_fns)
        : BackendInfo{std::move(builtin_globals), std::move(builtin_fns)} {}

    static GLSLBackendInfo create_instance(GLSLTypes& types);

    static GlobalList create_globals(GLSLTypes& types);
    static FnList create_fns(GLSLTypes& types);
};

} // namespace stc::glsl
