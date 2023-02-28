#ifndef LIBASR_PASS_INTRINSIC_FUNCTION_H
#define LIBASR_PASS_INTRINSIC_FUNCTION_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_intrinsic_function(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions& pass_options);

    ASR::expr_t *eval_sin(Allocator &al, const Location &loc, ASR::expr_t* arg);
    ASR::expr_t *eval_log_gamma(Allocator &al, const Location &loc, ASR::expr_t* arg);

} // namespace LCompilers

#endif // LIBASR_PASS_INTRINSIC_FUNCTION_H
