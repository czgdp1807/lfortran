program expr2
implicit none

integer :: x(5)
x(2) = 4
print *, x(2), size(x)
print *, get_size(x)

contains

function get_size(x) result(r)
    integer, intent(in) :: x(:)
    integer :: r
    r = size(x)
    return
end function

end program
