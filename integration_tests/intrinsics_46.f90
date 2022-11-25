program test_ichar_iachar
  integer :: i, j
  integer(8) :: li, lj, li1, lj1
  i = ichar(' ')
  j = iachar(' ')
  li = ichar('a', 8)
  lj = iachar('a', 8)
  li1 = ichar('b', kind=8)
  lj1 = iachar('b', kind=8)
  print *, i, j, li, li1, lj, lj1
end program test_ichar_iachar
