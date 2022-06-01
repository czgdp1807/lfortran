module corr
contains
function corr_2_cdp_cdp(x, dim, mask) result(res)
  complex(8), intent(in) :: x(:, :)
  integer, intent(in) :: dim
  logical, intent(in), optional :: mask
  complex(8) :: res(merge(size(x, 1), size(x, 2), mask = 1<dim)&
                      , merge(size(x, 1), size(x, 2), mask = 1<dim))

  integer :: i, j
  complex(8) :: mean_(merge(size(x, 1), size(x, 2), mask = 1<dim))
  complex(8) :: center(size(x, 1),size(x, 2))

  select case(dim)
    case(1)
      do i = 1, size(x, 1)
        center(i, :) = x(i, :) - mean_
      end do
        res = matmul( transpose(conjg(center)), center)
    case(2)
      do i = 1, size(x, 2)
        center(:, i) = x(:, i) - mean_
      end do
        res = matmul( center, transpose(conjg(center)))
    case default
      error stop
  end select

  do i = 1, size(res, 1)
    do j = 1, size(res, 2)
      res(j, i) = res(j, i) * mean_(i) * mean_(j)
    end do
  end do

end function corr_2_cdp_cdp
end module

program matrix_02_tranpose
end program