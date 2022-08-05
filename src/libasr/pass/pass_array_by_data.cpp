#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/update_array_dim_intrinsic_calls.h>

#include <vector>
#include <utility>


namespace LFortran {

class PassArrayByDataSubroutineVisitor : public PassUtils::PassVisitor<PassArrayByDataSubroutineVisitor>
{
    private:

        std::map< ASR::symbol_t*, std::pair<ASR::symbol_t*, std::vector<size_t>> > proc2newproc;

    public:

        PassArrayByDataSubroutineVisitor(Allocator& al_) : PassVisitor(al, nullptr)
        {}

        template <typename T>
        void insert_new_procedure(T* x, std::vector<size_t>& indices) {
            Vec<ASR::expr_t*> new_args;
            std::string suffix = "";
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
                    suffix += "_" + arg_name;
                    Vec<ASR::dimension_t> new_dims;
                    new_dims.reserve(al, n_dims);
                    for( size_t j = 0, k = 0; j < n_dims; j++ ) {
                        ASR::dimension_t new_dim;
                        new_dim.loc = dims[j].loc;
                        new_dim.m_start = dim_variables[k];
                        new_dim.m_length = dim_variables[k + 1];
                        k += 2;
                    }
                    ASR::ttype_t* new_type = ASRUtils::duplicate_type(al, arg->m_type, &new_dims);
                    arg->m_type = new_type;
                    for( size_t k = 0; k < 2 * n_dims; k++ ) {
                        new_args.push_back(al, dim_variables[k]);
                    }
                }
            }
            ASR::symbol_t* new_symbol = nullptr;
            std::string new_name = std::string(x->m_name) + suffix;
            if( x->type == ASR::symbolType::Subroutine ) {
                std::string new_bindc_name = std::string(x->m_bindc_name) + suffix;
                ASR::asr_t* new_subrout = ASR::make_Subroutine_t(al, x->base.base.loc,
                                            x->m_symtab, s2c(al, new_name), new_args.p,
                                            new_args.size(), x->m_body, x->n_body, x->m_abi,
                                            x->m_access, x->m_deftype, s2c(al, new_bindc_name),
                                            x->m_pure, x->m_module);
                new_symbol = ASR::down_cast<ASR::symbol_t>(new_subrout);
            }
            current_scope->add_symbol(new_name, new_symbol);
            proc2newproc[(ASR::symbol_t*) x] = std::make_pair(new_symbol, indices);

        }

        void visit_Program(const ASR::Program_t& x) {
            ASR::Program_t& xx = const_cast<ASR::Program_t&>(x);
            current_scope = xx.m_symtab;
            for( auto& item: xx.m_symtab->get_scope() ) {
                if( ASR::is_a<ASR::Subroutine_t>(*item.second) ) {
                    ASR::Subroutine_t* subrout = ASR::down_cast<ASR::Subroutine_t>(item.second);
                    std::vector<size_t> arg_indices;
                    if( ASRUtils::is_pass_array_by_data_possible(subrout, arg_indices) ) {
                        insert_new_procedure(subrout, arg_indices);
                    }
                }
            }
        }
};

void pass_array_by_data(Allocator &al, ASR::TranslationUnit_t &unit) {
    PassArrayByDataSubroutineVisitor v(al);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}

} // namespace LFortran
