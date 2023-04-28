program test
real x

call sub(x, 1, *100, *200)
print*, "Success:", x
stop

100 print*, "Negative input value"
stop

200 print*, "Input value too large"
stop

end

subroutine sub(x, i, *, *)
  real, intent(out) :: x
  integer, intent(in) :: i
  if (i<0) return 1
  if (i>10) return 2
  x = i
end subroutine
