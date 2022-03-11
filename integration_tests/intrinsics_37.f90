program intrinsics_37
real(8) :: tsource(2, 3), fsource(2, 3), ar1(2, 3), ar2(2, 3)
logical :: mask(2, 3), mask_vec(6)
real(8) :: real_mask(2, 3), real_mask_vec(6)
integer :: i, j

mask = .true.
real_mask = 0.0
mask_vec = reshape(mask, shape(mask_vec))
mask_vec = .false.
real_mask_vec = reshape(real_mask, shape(real_mask_vec))
real_mask_vec = 1.0
print *, mask, real_mask
mask = reshape(mask_vec, shape(mask))
real_mask = reshape(real_mask_vec, shape(real_mask))
print *, mask, real_mask

end program