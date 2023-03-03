#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/asr_utils.h>

#include <cmath>
#include <string>

namespace LCompilers {

namespace ASRUtils {

enum class IntrinsicFunctions : int64_t {
    Sin,
    Cos,
    Gamma,
    LogGamma,
    // ...
};

namespace UnaryIntrinsicFunction {

static inline ASR::expr_t* instantiate_functions(Allocator &al, const Location &loc,
    SymbolTable *global_scope, std::string new_name,
    ASR::ttype_t *arg_type, Vec<ASR::call_arg_t>& new_args,
    ASR::expr_t *value) {
    std::string c_func_name;
    if (ASRUtils::extract_kind_from_ttype_t(arg_type) == 4) {
        c_func_name = "_lfortran_s" + new_name;
    } else {
        c_func_name = "_lfortran_d" + new_name;
    }
    new_name = "_lcompilers_" + new_name;
    // Check if Function is already defined.
    {
        std::string new_func_name = new_name;
        int i = 1;
        while (global_scope->get_symbol(new_func_name) != nullptr) {
            ASR::symbol_t *s = global_scope->get_symbol(new_func_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            if (ASRUtils::types_equal(ASRUtils::expr_type(f->m_return_var),
                    arg_type)) {
                return ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc, s,
                    s, new_args.p, new_args.size(), arg_type, value, nullptr));
            } else {
                new_func_name += std::to_string(i);
                i++;
            }
        }
    }
    new_name = global_scope->get_unique_name(new_name);
    SymbolTable *fn_symtab = al.make_new<SymbolTable>(global_scope);

    Vec<ASR::expr_t*> args;
    {
        args.reserve(al, 1);
        ASR::symbol_t *arg = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
            al, loc, fn_symtab, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
            nullptr, nullptr, ASR::storage_typeType::Default, arg_type,
            ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false));
        fn_symtab->add_symbol(s2c(al, "x"), arg);
        args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg)));
    }

    ASR::symbol_t *return_var = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
        al, loc, fn_symtab, s2c(al, new_name), nullptr, 0, ASRUtils::intent_return_var,
        nullptr, nullptr, ASR::storage_typeType::Default, arg_type,
        ASR::abiType::Source, ASR::Public, ASR::presenceType::Required, false));
    fn_symtab->add_symbol(s2c(al, new_name), return_var);

    Vec<ASR::stmt_t*> body;
    body.reserve(al, 1);

    Vec<char *> dep;
    dep.reserve(al, 1);

    {
        SymbolTable *fn_symtab_1 = al.make_new<SymbolTable>(fn_symtab);
        Vec<ASR::expr_t*> args_1;
        {
            args_1.reserve(al, 1);
            ASR::symbol_t *arg = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
                al, loc, fn_symtab_1, s2c(al, "x"), nullptr, 0, ASR::intentType::In,
                nullptr, nullptr, ASR::storage_typeType::Default, arg_type,
                ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, true));
            fn_symtab_1->add_symbol(s2c(al, "x"), arg);
            args_1.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg)));
        }

        ASR::symbol_t *return_var_1 = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(
            al, loc, fn_symtab_1, s2c(al, c_func_name), nullptr, 0, ASRUtils::intent_return_var,
            nullptr, nullptr, ASR::storage_typeType::Default, arg_type,
            ASR::abiType::BindC, ASR::Public, ASR::presenceType::Required, false));
        fn_symtab_1->add_symbol(s2c(al, c_func_name), return_var_1);

        ASR::symbol_t *s =  ASR::down_cast<ASR::symbol_t>(
            ASRUtils::make_Function_t_util(al, loc, fn_symtab_1,
            s2c(al, c_func_name), nullptr, 0, args_1.p, args_1.n, nullptr, 0,
            ASRUtils::EXPR(ASR::make_Var_t(al, loc, return_var_1)),
            ASR::abiType::BindC, ASR::accessType::Public,
            ASR::deftypeType::Interface, s2c(al, c_func_name), false, false,
            false, false, false, nullptr, 0, nullptr, 0, false, false, false));
        fn_symtab->add_symbol(c_func_name, s);
        dep.push_back(al, s2c(al, c_func_name));
        Vec<ASR::call_arg_t> call_args;
        {
            call_args.reserve(al, 1);
            ASR::call_arg_t arg;
            arg.m_value = args[0];
            call_args.push_back(al, arg);
        }
        body.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, loc,
            ASRUtils::EXPR(ASR::make_Var_t(al, loc, return_var)),
            ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc, s, s,
            call_args.p, call_args.n, arg_type, nullptr, nullptr)), nullptr)));
    }

    ASR::asr_t* new_subrout = ASRUtils::make_Function_t_util(al, loc,
        fn_symtab, s2c(al, new_name), dep.p, dep.n, args.p, args.n, body.p, body.n,
        ASRUtils::EXPR(ASR::make_Var_t(al, loc, return_var)),
        ASR::abiType::Source, ASR::accessType::Public,
        ASR::deftypeType::Implementation, nullptr, false, false, false,
        false, false, nullptr, 0, nullptr, 0, false, false, false);
    ASR::symbol_t *new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
    global_scope->add_symbol(new_name, new_symbol);
    return ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc, new_symbol,
        new_symbol, new_args.p, new_args.size(), arg_type, value, nullptr));
}

} // namespace UnaryIntrinsicFunction

namespace LogGamma {

static inline ASR::expr_t *eval_log_gamma(Allocator &al, const Location &loc, Vec<ASR::expr_t*>& args) {
    double rv = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
    double val = lgamma(rv);
    ASR::ttype_t *t = ASRUtils::expr_type(args[0]);
    return ASRUtils::EXPR(ASR::make_RealConstant_t(al, loc, val, t));
}

static inline ASR::asr_t* create_LogGamma(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> /*err*/) {
    int64_t intrinsic_id = static_cast<int64_t>(ASRUtils::IntrinsicFunctions::LogGamma);
    int64_t overload_id = 0;
    ASR::expr_t *value = nullptr;
    ASR::expr_t *arg_value = ASRUtils::expr_value(args[0]);
    ASR::ttype_t *type = ASRUtils::expr_type(args[0]);
    if (arg_value) {
        Vec<ASR::expr_t*> arg_values;
        arg_values.reserve(al, 1);
        arg_values.push_back(al, arg_value);
        value = eval_log_gamma(al, loc, arg_values);
    }
    return ASR::make_IntrinsicFunction_t(al, loc, intrinsic_id,
        args.p, args.n, overload_id, type, value);
}

static inline ASR::expr_t* instantiate_LogGamma(Allocator &al, const Location &loc,
    SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
    Vec<ASR::call_arg_t>& new_args, ASR::expr_t* compile_time_value) {
    LCOMPILERS_ASSERT(arg_types.size() == 1);
    ASR::ttype_t* arg_type = arg_types[0];
    return UnaryIntrinsicFunction::instantiate_functions(
        al, loc, scope, "log_gamma", arg_type, new_args,
        compile_time_value);
}

} // namespace LogGamma

namespace Sin {

static inline ASR::expr_t *eval_sin(Allocator &al, const Location &loc, Vec<ASR::expr_t*>& args) {
    LCOMPILERS_ASSERT(args.size() == 1);
    double rv = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
    double val = sin(rv);
    ASR::ttype_t *t = ASRUtils::expr_type(args[0]);
    return ASRUtils::EXPR(ASR::make_RealConstant_t(al, loc, val, t));
}

static inline ASR::asr_t* create_Sin(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    ASR::ttype_t *type = ASRUtils::expr_type(args[0]);
    if (!ASRUtils::is_real(*type) && !ASRUtils::is_complex(*type)) {
        err("`x` argument of `sin` must be real or complex",
            args[0]->base.loc);
    }
    ASR::expr_t *value = nullptr;
    ASR::expr_t *arg_value = ASRUtils::expr_value(args[0]);
    if (arg_value) {
        Vec<ASR::expr_t*> arg_values;
        arg_values.reserve(al, 1);
        arg_values.push_back(al, arg_value);
        value = eval_sin(al, loc, arg_values);
    }
    int64_t intrinsic_id = static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Sin);
    int64_t overload_id = 0;
    return ASR::make_IntrinsicFunction_t(al, loc,
        intrinsic_id, args.p, args.n, overload_id, type, value);
}

static inline ASR::expr_t* instantiate_Sin(Allocator &al, const Location &loc,
    SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
    Vec<ASR::call_arg_t>& new_args, ASR::expr_t* compile_time_value) {
    LCOMPILERS_ASSERT(arg_types.size() == 1);
    ASR::ttype_t* arg_type = arg_types[0];
    return UnaryIntrinsicFunction::instantiate_functions(
        al, loc, scope, "sin", arg_type, new_args,
        compile_time_value);
}

} // namespace Sin

typedef ASR::expr_t* (*impl_function)(
    Allocator&, const Location &,
    SymbolTable*, Vec<ASR::ttype_t*>&,
    Vec<ASR::call_arg_t>&, ASR::expr_t*);

typedef ASR::expr_t* (*eval_intrinsic_function)(
    Allocator&, const Location &,
    Vec<ASR::expr_t*>&);

typedef ASR::asr_t* (*create_intrinsic_function)(
    Allocator&, const Location&,
    Vec<ASR::expr_t*>&,
    const std::function<void (const std::string &, const Location &)>);

namespace IntrinsicFunctionRegistry {

    static const std::map<int64_t, impl_function>& intrinsic_function_by_id_db = {
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::LogGamma),
            &LogGamma::instantiate_LogGamma},

        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Sin),
            &Sin::instantiate_Sin}
    };

    static const std::map<std::string,
        std::pair<create_intrinsic_function,
                    eval_intrinsic_function>>& intrinsic_function_by_name_db = {
                {"log_gamma", {&LogGamma::create_LogGamma, &LogGamma::eval_log_gamma}},
                {"sin", {&Sin::create_Sin, &Sin::eval_sin}}
    };

    static inline bool is_intrinsic_function(const std::string& name) {
        return intrinsic_function_by_name_db.find(name) != intrinsic_function_by_name_db.end();
    }

    static inline bool is_intrinsic_function(int64_t id) {
        return intrinsic_function_by_id_db.find(id) != intrinsic_function_by_id_db.end();
    }

    static inline create_intrinsic_function get_create_function(const std::string& name) {
        return intrinsic_function_by_name_db.at(name).first;
    }

    static inline impl_function get_instantiate_function(int64_t id) {
        return intrinsic_function_by_id_db.at(id);
    }

}

} // namespace ASRUtils

} // namespace LCompilers
