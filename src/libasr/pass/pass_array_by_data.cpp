#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/pass_array_by_data.h>

#include <vector>
#include <utility>


namespace LFortran {

class PassArrayByDataSubroutineVisitor : public PassUtils::PassVisitor<PassArrayByDataSubroutineVisitor>
{
    private:

        ASR::ExprStmtDuplicator node_duplicator;
        SymbolTable* current_proc_scope;
        bool is_editing_procedure;

    public:

        std::map< ASR::symbol_t*, std::pair<ASR::symbol_t*, std::vector<size_t>> > proc2newproc;

        PassArrayByDataSubroutineVisitor(Allocator& al_) : PassVisitor(al_, nullptr),
        node_duplicator(al_), current_proc_scope(nullptr), is_editing_procedure(false)
        {}

        void visit_Var(const ASR::Var_t& x) {
            if( !is_editing_procedure ) {
                return ;
            }
            ASR::Var_t& xx = const_cast<ASR::Var_t&>(x);
            ASR::symbol_t* x_sym = xx.m_v;
            std::string x_sym_name = std::string(ASRUtils::symbol_name(x_sym));
            if( current_proc_scope->get_symbol(x_sym_name) != x_sym ) {
                xx.m_v = current_proc_scope->get_symbol(x_sym_name);
            }
        }

        template <typename T>
        ASR::symbol_t* insert_new_procedure(T* x, std::vector<size_t>& indices) {
            SymbolTable* new_symtab = al.make_new<SymbolTable>(current_scope);
            for( auto& item: x->m_symtab->get_scope() ) {
                ASR::Variable_t* arg = ASR::down_cast<ASR::Variable_t>(item.second);
                ASR::symbol_t* new_arg = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t(al,
                                            arg->base.base.loc, new_symtab, s2c(al, item.first),
                                            arg->m_intent, arg->m_symbolic_value, arg->m_value,
                                            arg->m_storage, arg->m_type, arg->m_abi, arg->m_access,
                                            arg->m_presence, arg->m_value_attr));
                new_symtab->add_symbol(item.first, new_arg);
            }
            Vec<ASR::stmt_t*> new_body;
            new_body.reserve(al, x->n_body);
            node_duplicator.allow_procedure_calls = true;
            for( size_t i = 0; i < x->n_body; i++ ) {
                node_duplicator.success = true;
                ASR::stmt_t* new_stmt = node_duplicator.duplicate_stmt(x->m_body[i]);
                LFORTRAN_ASSERT(node_duplicator.success);
                new_body.push_back(al, new_stmt);
            }
            Vec<ASR::expr_t*> new_args;
            std::string suffix = "";
            new_args.reserve(al, x->n_args);
            for( size_t i = 0; i < x->n_args; i++ ) {
                ASR::Variable_t* arg = ASRUtils::EXPR2VAR(x->m_args[i]);
                if( std::find(indices.begin(), indices.end(), i) !=
                    indices.end() ) {
                    suffix += "_" + std::string(arg->m_name);
                }
                ASR::expr_t* new_arg = ASRUtils::EXPR(ASR::make_Var_t(al,
                                        arg->base.base.loc, new_symtab->get_symbol(
                                                        std::string(arg->m_name))));
                new_args.push_back(al, new_arg);
            }
            ASR::symbol_t* new_symbol = nullptr;
            std::string new_name = std::string(x->m_name) + suffix;
            if( ASR::is_a<ASR::Subroutine_t>( *((ASR::symbol_t*) x) ) ) {
                std::string new_bindc_name = "";
                if( x->m_bindc_name ) {
                    new_bindc_name = std::string(x->m_bindc_name) + suffix;
                }
                ASR::asr_t* new_subrout = ASR::make_Subroutine_t(al, x->base.base.loc,
                                            new_symtab, s2c(al, new_name), new_args.p,
                                            new_args.size(), new_body.p, new_body.size(), x->m_abi,
                                            x->m_access, x->m_deftype, s2c(al, new_bindc_name),
                                            x->m_pure, x->m_module);
                new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
            }
            current_scope->add_symbol(new_name, new_symbol);
            proc2newproc[(ASR::symbol_t*) x] = std::make_pair(new_symbol, indices);
            return new_symbol;
        }

        template <typename T>
        void edit_new_procedure(T* x, std::vector<size_t>& indices) {
            Vec<ASR::expr_t*> new_args;
            new_args.reserve(al, x->n_args);
            for( size_t i = 0; i < x->n_args; i++ ) {
                new_args.push_back(al, x->m_args[i]);
                if( std::find(indices.begin(), indices.end(), i) !=
                    indices.end() ) {
                    ASR::Variable_t* arg = ASRUtils::EXPR2VAR(x->m_args[i]);
                    ASR::dimension_t* dims = nullptr;
                    int n_dims = ASRUtils::extract_dimensions_from_ttype(arg->m_type, dims);
                    Vec<ASR::expr_t*> dim_variables;
                    std::string arg_name = std::string(arg->m_name);
                    PassUtils::create_vars(dim_variables, 2 * n_dims, arg->base.base.loc, al,
                                           x->m_symtab, arg_name);
                    Vec<ASR::dimension_t> new_dims;
                    new_dims.reserve(al, n_dims);
                    for( int j = 0, k = 0; j < n_dims; j++ ) {
                        ASR::dimension_t new_dim;
                        new_dim.loc = dims[j].loc;
                        new_dim.m_start = dim_variables[k];
                        new_dim.m_length = dim_variables[k + 1];
                        new_dims.push_back(al, new_dim);
                        k += 2;
                    }
                    ASR::ttype_t* new_type = ASRUtils::duplicate_type(al, arg->m_type, &new_dims);
                    arg->m_type = new_type;
                    for( int k = 0; k < 2 * n_dims; k++ ) {
                        new_args.push_back(al, dim_variables[k]);
                    }
                }
            }

            x->m_args = new_args.p;
            x->n_args = new_args.size();

            is_editing_procedure = true;
            current_proc_scope = x->m_symtab;
            for( size_t i = 0; i < x->n_body; i++ ) {
                visit_stmt(*x->m_body[i]);
            }
            is_editing_procedure = false;
            current_proc_scope = nullptr;
        }

        void visit_Program(const ASR::Program_t& x) {
            ASR::Program_t& xx = const_cast<ASR::Program_t&>(x);
            current_scope = xx.m_symtab;
            for( auto& item: xx.m_symtab->get_scope() ) {
                if( ASR::is_a<ASR::Subroutine_t>(*item.second) ) {
                    ASR::Subroutine_t* subrout = ASR::down_cast<ASR::Subroutine_t>(item.second);
                    std::vector<size_t> arg_indices;
                    if( ASRUtils::is_pass_array_by_data_possible(subrout, arg_indices) ) {
                        ASR::Subroutine_t* new_subrout = ASR::down_cast<ASR::Subroutine_t>(
                                                            insert_new_procedure(subrout, arg_indices));
                        edit_new_procedure(new_subrout, arg_indices);
                    }
                }
            }
        }
};

class ReplaceSubroutineCallsVisitor : public PassUtils::PassVisitor<ReplaceSubroutineCallsVisitor>
{
    private:

        PassArrayByDataSubroutineVisitor& v;

    public:

        ReplaceSubroutineCallsVisitor(Allocator& al_, PassArrayByDataSubroutineVisitor& v_): PassVisitor(al_, nullptr),
        v(v_) {
            pass_result.reserve(al, 1);
        }

        void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
            ASR::symbol_t* subrout_sym = x.m_name;
            if( v.proc2newproc.find(subrout_sym) == v.proc2newproc.end() ) {
                return ;
            }

            ASR::symbol_t* new_subrout_sym = v.proc2newproc[subrout_sym].first;
            std::vector<size_t>& indices = v.proc2newproc[subrout_sym].second;

            Vec<ASR::call_arg_t> new_args;
            new_args.reserve(al, x.n_args);
            for( size_t i = 0; i < x.n_args; i++ ) {
                new_args.push_back(al, x.m_args[i]);
                if( std::find(indices.begin(), indices.end(), i) == indices.end() ) {
                    continue ;
                }

                Vec<ASR::expr_t*> dim_vars;
                dim_vars.reserve(al, 2);
                ASRUtils::get_dimensions(x.m_args[i].m_value, dim_vars, al);
                for( size_t j = 0; j < dim_vars.size(); j++ ) {
                    ASR::call_arg_t dim_var;
                    dim_var.loc = dim_vars[j]->base.loc;
                    dim_var.m_value = dim_vars[j];
                    new_args.push_back(al, dim_var);
                }
            }

            ASR::stmt_t* new_call = ASRUtils::STMT(ASR::make_SubroutineCall_t(al,
                                        x.base.base.loc, new_subrout_sym, new_subrout_sym,
                                        new_args.p, new_args.size(), x.m_dt));
            pass_result.push_back(al, new_call);
        }
};

void pass_array_by_data(Allocator &al, ASR::TranslationUnit_t &unit) {
    PassArrayByDataSubroutineVisitor v(al);
    v.visit_TranslationUnit(unit);
    ReplaceSubroutineCallsVisitor u(al, v);
    u.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}

} // namespace LFortran
