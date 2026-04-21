#pragma once

#include "backend/glsl/types.h"
#include "common/target_info.h"

namespace stc::glsl {

class GLSLTargetInfo final : public TargetInfo {
private:
    GLSLTypes gl_types;

public:
    GLSLTargetInfo(const GLSLTargetInfo&)            = delete;
    GLSLTargetInfo& operator=(const GLSLTargetInfo&) = delete;
    GLSLTargetInfo(GLSLTargetInfo&&)                 = delete;
    GLSLTargetInfo& operator=(GLSLTargetInfo&&)      = delete;

    static const GLSLTargetInfo& get(TypePool& type_pool) {
        static GLSLTargetInfo instance = create_instance(GLSLTypes{type_pool});

        return instance;
    }

    bool valid_ctor_call(TypeId target, const TypeList& arg_types) const override;
    bool can_implicit_cast(TypeId src_ty, TypeId dest_ty) const override;

private:
    explicit GLSLTargetInfo(GlobalList builtin_globals, FnList builtin_fns, GLSLTypes types)
        : TargetInfo{std::move(builtin_globals), std::move(builtin_fns)},
          gl_types{std::move(types)} {}

    static GLSLTargetInfo create_instance(GLSLTypes types);

    static GlobalList create_globals(GLSLTypes& types);
    static FnList create_fns(GLSLTypes& types);
};

} // namespace stc::glsl
