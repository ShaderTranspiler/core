using Libdl

const LIB_PATH = joinpath(@__DIR__, "..", "build-rel", "lib", "libSTC.so")

cpp_free(ptr) = ccall((:stc_jl_free, LIB_PATH), Cvoid, (Ptr{Cvoid},), ptr)

function test_call(input_expr)
    c_str = ccall((:stc_jl_print_expr, LIB_PATH), Ptr{Cchar}, (Any,), input_expr)

    if c_str == C_NULL
        @warn "An error occured during the cpp lib call"
        return "julia default return string"
    end

    str = unsafe_string(c_str)

    ccall((:stc_jl_free, LIB_PATH), Cvoid, (Ptr{Cchar},), c_str)

    return str
end

test_call(:(
    if x == 0
        println("x is zero")
    elseif x < 0
        println("x is less than zero")
    else
        println("x is greater than zero")
    end
))
