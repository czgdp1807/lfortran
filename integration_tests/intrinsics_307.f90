program intrinsics_307
    implicit none
    real, parameter :: x1 = imag((1.0, 2.0))
    real(8), parameter :: x2 = imag((1.0_8, 2.0_8))
    real, parameter :: ar1(3) = imag([(1.0, 2.0), (3.0, 4.0), (5.0, 6.0)])
    real(8), parameter :: ar2(3) = imag([(1.0_8, 2.0_8), (3.0_8, 4.0_8), (5.0_8, 6.0_8)])
    complex :: x = (1.0, 2.0)
    complex(8) :: y = (1.0_8, 2.0_8)
    complex :: z(3) = [(1.0, 2.0), (3.0, 4.0), (5.0, 6.0)]

    print *, x1
    if (x1 /= 2.0) error stop
    print *, x2
    if (x2 /= 2.0_8) error stop
    print *, imag(x)
    if (imag(x) /= 2.0) error stop
    print *, imag(y)
    if (imag(y) /= 2.0_8) error stop
    print *, ar1
    if (any(ar1 /= [2.0, 4.0, 6.0])) error stop
    print *, ar2
    if (any(ar2 /= [2.0_8, 4.0_8, 6.0_8])) error stop
    print *, imag(x)
    if (imag(x) /= 2.0) error stop
    print *, imag(y)
    if (imag(y) /= 2.0_8) error stop
    print *, imag(z)
    if (any(imag(z) /= [2.0, 4.0, 6.0])) error stop
end program