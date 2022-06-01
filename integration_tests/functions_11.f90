module corr
contains
function diag_cdp(v) result(res)
  complex(8), intent(in) :: v(:)
  complex(8) :: res(size(v),size(v))
end function diag_cdp

function diag_cdp_use(x) result(r)
  complex(8), intent(in) :: x(:)
  complex(8) :: r
  print *, diag_cdp(r)
end function

end module

program functions_11
end program