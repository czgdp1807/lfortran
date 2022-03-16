program intrinsics_37
! real(8) :: tsource(2, 3), fsource(2, 3), ar1(2, 3), ar2(2, 3)
logical :: mask(164, 193), mask_vec(6)
! real(8) :: real_mask(2, 3), real_mask_vec(6)
! integer :: i, j, mask_shape(32), real_mask_shape(32)
integer :: mask_shape(32)
! complex :: a, b
! a = (1.0, 1.0)
! b = a
! print *, b

! mask = .true.
! real_mask = 0.0
print *, size(mask_vec)
mask_shape = shape(mask)
print *, mask_shape(0), mask_shape(1)
! mask_vec = reshape(mask, mask_shape)
! mask_vec = .false.
! real_mask_shape = shape(real_mask_vec)
! real_mask_vec = reshape(real_mask, real_mask_shape)
! real_mask_vec = 1.0
! print *, mask, real_mask
! mask = reshape(mask_vec, shape(mask))
! real_mask = reshape(real_mask_vec, shape(real_mask))
! print *, mask, real_mask

end program