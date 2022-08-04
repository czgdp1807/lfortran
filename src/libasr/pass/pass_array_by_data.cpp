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

        Allocator& al;
        std::map< ASR::symbol_t*, std::pair<ASR::symbol_t*, Vec<size_t>> > proc2newproc;

    public:

        void visit_Program(const ASR::Program_t& x) {
            ASR::Program_t& xx = const_cast<ASR::Program_t&>(x);
        }
};

void pass_array_by_data(Allocator &al, ASR::TranslationUnit_t &unit) {
    ArrayDimIntrinsicCallsVisitor v;
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}

} // namespace LFortran
