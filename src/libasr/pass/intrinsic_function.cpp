#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/intrinsic_function.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <utility>


namespace LFortran {

using ASR::down_cast;
using ASR::is_a;

/*

This ASR pass replaces the IntrinsicFunction node with a call to an
implementation in the surface language (ASR).

Call this pass if you do not want to implement intrinsic functions directly
in the backend.

*/

class ReplaceIntrinsicFunction: public ASR::BaseExprReplacer<ReplaceIntrinsicFunction> {

    private:

    Allocator& al;

    public:

    ReplaceIntrinsicFunction(Allocator& al_) : al(al_)
    {}

    void replace_IntrinsicFunction(ASR::IntrinsicFunction_t* x) {
        switch (x->m_intrinsic_id) {
            case (static_cast<int64_t>(ASRUtils::IntrinsicFunctions::LogGamma)) : {
                for (size_t i=0; i < x->n_args; i++) {
                    // TODO: handle the case if the argument changes
                    replace_expr(x->m_args[i]);
                }
                // FIXME!
                ASR::symbol_t* new_func_sym = nullptr;

                Vec<ASR::call_arg_t> new_args;
                new_args.reserve(al, x->n_args);

                ASR::expr_t* new_call = ASRUtils::EXPR(ASR::make_FunctionCall_t(al,
                                            x->base.base.loc, new_func_sym, new_func_sym,
                                            new_args.p, new_args.size(), x->m_type, nullptr,
                                            nullptr));
                *current_expr = new_call;
                // TODO: here we must get access to the pure ASR implementation
                // of LogGamma, as provided by the frontend, let's say
                // it is assigned to this "fc":
                // ASR::FunctionCall_t *fc = ...;
                // TODO: Return "fc" instead of "self" here
                //throw LCompilersException("TODO: change IntrinsicFunction to FunctionCall");
                break;
            }
            default : {
                throw LCompilersException("Intrinsic function not implemented");
            }
        }
    }

};

/*
The following visitor calls the above replacer i.e., ReplaceFunctionCalls
on expressions present in ASR so that FunctionCall get replaced everywhere
and we don't end up with false positives.
*/
class ReplaceIntrinsicFunctionVisitor : public ASR::CallReplacerOnExpressionsVisitor<ReplaceIntrinsicFunctionVisitor>
{
    private:

        ReplaceIntrinsicFunction replacer;

    public:

        ReplaceIntrinsicFunctionVisitor(Allocator& al_) : replacer(al_) {}

        void call_replacer() {
            replacer.current_expr = current_expr;
            replacer.replace_expr(*current_expr);
        }

};

void pass_replace_intrinsic_function(Allocator &al, ASR::TranslationUnit_t &unit,
                             const LCompilers::PassOptions& /*pass_options*/) {
    ReplaceIntrinsicFunctionVisitor v(al);
    v.visit_TranslationUnit(unit);
}


} // namespace LFortran
