#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/nested_vars.h>
#include <libasr/pass/pass_utils.h>
#include <unordered_map>

namespace LCompilers {

using ASR::down_cast;

/*

TODO: add docs

*/

class NestedVarVisitor : public ASR::BaseWalkVisitor<NestedVarVisitor>
{
public:
    Allocator &al;
    size_t nesting_depth = 0;
    SymbolTable* current_scope;
    std::map<ASR::symbol_t*, std::set<ASR::symbol_t*>> nesting_map;

    NestedVarVisitor(Allocator& al_): al(al_) {
        current_scope = nullptr;
    };

    ASR::symbol_t *cur_func_sym = nullptr;
    ASR::symbol_t *par_func_sym = nullptr;

    template<typename T>
    void visit_procedure(const T &x) {
        nesting_depth++;
        bool is_func_visited = false;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                par_func_sym = cur_func_sym;
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(
                    item.second);
                if (!is_func_visited) {
                    is_func_visited = true;
                    for (size_t i = 0; i < x.n_body; i++) {
                        visit_stmt(*x.m_body[i]);
                    }
                }
                visit_Function(*s);
            }
        }
        if (!is_func_visited) {
            is_func_visited = true;
            for (size_t i = 0; i < x.n_body; i++) {
                visit_stmt(*x.m_body[i]);
            }
        }
        nesting_depth--;
    }


    void visit_Program(const ASR::Program_t &x) {
        ASR::symbol_t *cur_func_sym_copy = cur_func_sym;
        cur_func_sym = (ASR::symbol_t*)(&x);
        current_scope = x.m_symtab;
        visit_procedure(x);
        cur_func_sym = cur_func_sym_copy;
    }

    void visit_Function(const ASR::Function_t &x) {
        ASR::symbol_t *cur_func_sym_copy = cur_func_sym;
        cur_func_sym = (ASR::symbol_t*)(&x);
        current_scope = x.m_symtab;
        visit_procedure(x);
        cur_func_sym = cur_func_sym_copy;
    }

    void visit_Var(const ASR::Var_t &x) {
        // Only attempt if we are actually in a nested function
        if (nesting_depth > 1) {
            ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(
                    ASRUtils::symbol_get_past_external(x.m_v));
            // If the variable is not defined in the current scope, it is a
            // "needed global" since we need to be able to access it from the
            // nested procedure.
            if (current_scope && current_scope->get_symbol(v->m_name) == nullptr) {
                nesting_map[par_func_sym].insert(x.m_v);
            }
        }
    }
};


class ReplacerNestedVars: public ASR::BaseExprReplacer<ReplacerNestedVars> {
private:
    Allocator &al;
public:
    SymbolTable *current_scope;
    std::map<ASR::symbol_t*, std::pair<std::string, ASR::symbol_t*>> nested_var_to_ext_var;
    bool skip_replace=false;
    ReplacerNestedVars(Allocator &_al) : al(_al) {}

    void replace_Var(ASR::Var_t* x) {
        if (nested_var_to_ext_var.find(x->m_v) != nested_var_to_ext_var.end()) {
            std::string m_name = nested_var_to_ext_var[x->m_v].first;
            ASR::symbol_t *t = nested_var_to_ext_var[x->m_v].second;
            char *fn_name = ASRUtils::symbol_name(t);
            std::string sym_name = fn_name;
            if (current_scope->get_symbol(sym_name) != nullptr) {
                if (!skip_replace) {
                    x->m_v = current_scope->get_symbol(sym_name);
                }
                return;
            }
            ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
                al, t->base.loc,
                /* a_symtab */ current_scope,
                /* a_name */ fn_name,
                t,
                s2c(al, m_name), nullptr, 0, fn_name,
                ASR::accessType::Public
                );
            ASR::symbol_t *ext_sym = ASR::down_cast<ASR::symbol_t>(fn);
            current_scope->add_symbol(sym_name, ext_sym);
            if (!skip_replace) {
                x->m_v = ext_sym;
            }
        }
    }
};

class ReplaceNestedVisitor: public ASR::CallReplacerOnExpressionsVisitor<ReplaceNestedVisitor> {
    private:

    Allocator& al;
    ReplacerNestedVars replacer;

    public:
    size_t nesting_depth = 0;
    std::map<ASR::symbol_t*, std::set<ASR::symbol_t*>> &nesting_map;
    std::map<ASR::symbol_t*, std::pair<std::string, ASR::symbol_t*>> nested_var_to_ext_var;

    ReplaceNestedVisitor(Allocator& al_,
        std::map<ASR::symbol_t*, std::set<ASR::symbol_t*>> &n_map) : al(al_),
        replacer(al_), nesting_map(n_map) {}


    void call_replacer(bool skip_replace=false) {
        replacer.current_expr = current_expr;
        replacer.current_scope = current_scope;
        replacer.skip_replace = skip_replace;
        replacer.replace_expr(*current_expr);
    }


    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        current_scope = x.m_global_scope;
        SymbolTable* current_scope_copy = current_scope;

        // Add the nested vars by creating a new module

        for (auto &it: nesting_map) {
            // Iterate on each function with nested vars and create a context in
            // a new module.
            current_scope = al.make_new<SymbolTable>(current_scope_copy);
            std::string module_name = "__lcompilers_created__nested_context__" + std::string(
                                    ASRUtils::symbol_name(it.first)) + "_";
            std::map<ASR::symbol_t*, std::string> sym_to_name;
            module_name = current_scope->get_unique_name(module_name);
            for (auto &it2: it.second) {
                std::string new_ext_var = module_name + std::string(ASRUtils::symbol_name(it2));
                ASR::ttype_t* type = ASR::down_cast<ASR::Variable_t>(it2)->m_type;
                new_ext_var = current_scope->get_unique_name(new_ext_var);
                ASR::expr_t *sym_expr = PassUtils::create_auxiliary_variable(
                        it2->base.loc, new_ext_var,
                        al, current_scope, type, ASR::intentType::Local);
                ASR::symbol_t* sym = ASR::down_cast<ASR::Var_t>(sym_expr)->m_v;
                nested_var_to_ext_var[it2] = std::make_pair(module_name, sym);
            }
            ASR::asr_t *tmp = ASR::make_Module_t(al, x.base.base.loc,
                                            /* a_symtab */ current_scope,
                                            /* a_name */ s2c(al, module_name),
                                            nullptr,
                                            0,
                                            false, false);
            ASR::symbol_t* mod_sym = ASR::down_cast<ASR::symbol_t>(tmp);
            x.m_global_scope->add_symbol(module_name, mod_sym);
        }
        replacer.nested_var_to_ext_var = nested_var_to_ext_var;

        current_scope = x.m_global_scope;
        for (auto &a : x.m_global_scope->get_scope()) {
            this->visit_symbol(*a.second);
        }
        current_scope = current_scope_copy;
    }

    void visit_Program(const ASR::Program_t &x) {
        nesting_depth++;
        ASR::Program_t& xx = const_cast<ASR::Program_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
        transform_stmts(xx.m_body, xx.n_body);
        current_scope = current_scope_copy;
        nesting_depth--;
    }

    void visit_Function(const ASR::Function_t &x) {
        nesting_depth++;
        ASR::Function_t& xx = const_cast<ASR::Function_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
        visit_ttype(*x.m_function_signature);
        for (size_t i=0; i<x.n_args; i++) {
            ASR::expr_t** current_expr_copy_0 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_args[i]));
            call_replacer(nesting_depth == 1);
            current_expr = current_expr_copy_0;
            if( x.m_args[i] )
            visit_expr(*x.m_args[i]);
        }
        transform_stmts(xx.m_body, xx.n_body);
        if (x.m_return_var) {
            ASR::expr_t** current_expr_copy_1 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_return_var));
            call_replacer();
            current_expr = current_expr_copy_1;
            if( x.m_return_var )
            visit_expr(*x.m_return_var);
        }
        current_scope = current_scope_copy;
        nesting_depth--;
    }

};

class AssignNestedVars: public PassUtils::PassVisitor<AssignNestedVars> {
public:
    std::map<ASR::symbol_t*, std::pair<std::string, ASR::symbol_t*>> &nested_var_to_ext_var;

    AssignNestedVars(Allocator &al_,
    std::map<ASR::symbol_t*, std::pair<std::string, ASR::symbol_t*>> &nv) :
    PassVisitor(al_, nullptr), nested_var_to_ext_var(nv)
    {
        pass_result.reserve(al, 1);
    }

    void visit_Function(const ASR::Function_t &x) {
        ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
        SymbolTable* current_scope_copy = current_scope;
        current_scope = xx.m_symtab;

        for (size_t i=0; i<x.n_args; i++) {
            if (ASR::is_a<ASR::Var_t>(*x.m_args[i])) {
                ASR::symbol_t* sym = ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v;
                if (nested_var_to_ext_var.find(sym) != nested_var_to_ext_var.end()) {
                    std::string m_name = nested_var_to_ext_var[sym].first;
                    ASR::symbol_t *t = nested_var_to_ext_var[sym].second;
                    char *fn_name = ASRUtils::symbol_name(t);
                    std::string sym_name_ext = fn_name;
                    ASR::symbol_t *ext_sym = current_scope->get_symbol(sym_name_ext);
                    LCOMPILERS_ASSERT(ext_sym!=nullptr);
                    ASR::expr_t *target = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, ext_sym));
                    ASR::stmt_t *assignment = ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc,
                        target, x.m_args[i], nullptr));
                    pass_result.push_back(al, assignment);
                }
            }
        }
        transform_stmts(xx.m_body, xx.n_body);

        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                visit_Function(*s);
            }
            if (ASR::is_a<ASR::Block_t>(*item.second)) {
                ASR::Block_t *s = ASR::down_cast<ASR::Block_t>(item.second);
                visit_Block(*s);
            }
            if (ASR::is_a<ASR::AssociateBlock_t>(*item.second)) {
                ASR::AssociateBlock_t *s = ASR::down_cast<ASR::AssociateBlock_t>(item.second);
                visit_AssociateBlock(*s);
            }
        }
        current_scope = current_scope_copy;
    }
};

void pass_nested_vars(Allocator &al, ASR::TranslationUnit_t &unit,
    const LCompilers::PassOptions& /*pass_options*/) {
    NestedVarVisitor v(al);
    v.visit_TranslationUnit(unit);
    ReplaceNestedVisitor w(al, v.nesting_map);
    w.visit_TranslationUnit(unit);
    AssignNestedVars z(al, w.nested_var_to_ext_var);
    z.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor x(al);
    x.visit_TranslationUnit(unit);
    // {
    //
    //     for (auto &it: v.nesting_map) {
    //         std::cout<<"func name " << ASRUtils::symbol_name(it.first) << '\n';
    //         for (auto &it2: it.second) {
    //             std::cout<<ASRUtils::symbol_name(it2)<<' ';
    //         }
    //     }
    // }
}


} // namespace LCompilers
