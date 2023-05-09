#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/arr_slice.h>

#include <vector>
#include <utility>


namespace LCompilers {

/*
This ASR pass replaces array slice with do loops and array expression assignments.
The function `pass_replace_arr_slice` transforms the ASR tree in-place.

Converts:

    x = y(1:3)

to:

    do i = 1, 3
        x(i) = y(i)
    end do
*/

class ReplaceArraySection: public ASR::BaseExprReplacer<ReplaceArraySection> {

    private:

    Allocator& al;
    Vec<ASR::stmt_t*>& pass_result;
    size_t slice_counter;

    public:

    SymbolTable* current_scope;

    ReplaceArraySection(Allocator& al_, Vec<ASR::stmt_t*>& pass_result_) :
    al(al_), pass_result(pass_result_), slice_counter(0), current_scope(nullptr) {}

    ASR::ttype_t* get_array_from_slice(ASR::ArraySection_t* x) {
        Vec<ASR::dimension_t> m_dims;
        m_dims.reserve(al, x->n_args);
        for( size_t i = 0; i < x->n_args; i++ ) {
            if( x->m_args[i].m_step != nullptr ) {
                ASR::dimension_t curr_dim;
                curr_dim.loc = x->base.base.loc;
                curr_dim.m_start = nullptr;
                curr_dim.m_length = nullptr;
                m_dims.push_back(al, curr_dim);
            }
        }

        ASR::ttype_t* t2 = ASRUtils::type_get_past_pointer(x->m_type);
        ASR::ttype_t* new_type = ASRUtils::duplicate_type(al, t2, &m_dims);

        return ASRUtils::TYPE(ASR::make_Pointer_t(al, x->base.base.loc, new_type));
    }

    void replace_ArraySection(ASR::ArraySection_t* x) {
        LCOMPILERS_ASSERT(current_scope != nullptr);
        const Location& loc = x->base.base.loc;
        std::string new_name = "__libasr__created__section__" + std::to_string(slice_counter) + "_slice";
        slice_counter += 1;
        char* new_var_name = s2c(al, new_name);
        ASR::ttype_t* slice_asr_type = get_array_from_slice(x);
        ASR::asr_t* slice_asr = ASR::make_Variable_t(al, loc,
            current_scope, new_var_name, nullptr, 0,
            ASR::intentType::Local, nullptr, nullptr, ASR::storage_typeType::Default,
            slice_asr_type, nullptr, ASR::abiType::Source, ASR::accessType::Public,
            ASR::presenceType::Required, false);
        ASR::symbol_t* slice_sym = ASR::down_cast<ASR::symbol_t>(slice_asr);
        current_scope->add_symbol(std::string(new_var_name), slice_sym);
        ASR::expr_t* slice_var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, slice_sym));
        pass_result.push_back(al, ASRUtils::STMT(ASR::make_Associate_t(
            al, loc, slice_var, *current_expr)));
        *current_expr = slice_var;
    }

};

class ArraySectionVisitor : public ASR::CallReplacerOnExpressionsVisitor<ArraySectionVisitor>
{
    private:

        Allocator& al;
        ReplaceArraySection replacer;
        Vec<ASR::stmt_t*> pass_result;

    public:

        ArraySectionVisitor(Allocator& al_) :
        al(al_), replacer(al_, pass_result) {
            pass_result.reserve(al_, 1);
        }

        void call_replacer() {
            replacer.current_expr = current_expr;
            replacer.current_scope = current_scope;
            replacer.replace_expr(*current_expr);
        }

        void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
            Vec<ASR::stmt_t*> body;
            body.reserve(al, n_body);
            for (size_t i=0; i<n_body; i++) {
                pass_result.n = 0;
                visit_stmt(*m_body[i]);
                for (size_t j=0; j < pass_result.size(); j++) {
                    body.push_back(al, pass_result[j]);
                }
                body.push_back(al, m_body[i]);
            }
            m_body = body.p;
            n_body = body.size();
        }

        void visit_Assignment(const ASR::Assignment_t &x) {
            ASR::expr_t** current_expr_copy_9 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_value));
            this->call_replacer();
            current_expr = current_expr_copy_9;
            this->visit_expr(*x.m_value);
            if (x.m_overloaded) {
                this->visit_stmt(*x.m_overloaded);
            }
        }

        void visit_Associate(const ASR::Associate_t& /*x*/) {
            // Associating a slice to a pointer array
            // should happen only in the backends.
        }

};

void pass_replace_arr_slice(Allocator &al, ASR::TranslationUnit_t &unit,
                            const LCompilers::PassOptions& /*pass_options*/) {
    ArraySectionVisitor v(al);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor w(al);
    w.visit_TranslationUnit(unit);
}

} // namespace LCompilers
