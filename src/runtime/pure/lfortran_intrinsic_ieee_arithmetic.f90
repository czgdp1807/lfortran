module lfortran_intrinsic_ieee_arithmetic
implicit none

type IEEE_CLASS_TYPE
end type

type(ieee_class_type), parameter :: ieee_negative_denormal
type(ieee_class_type), parameter :: ieee_negative_inf
type(ieee_class_type), parameter :: ieee_negative_normal
type(ieee_class_type), parameter :: ieee_negative_zero
type(ieee_class_type), parameter :: ieee_positive_denormal
type(ieee_class_type), parameter :: ieee_positive_inf
type(ieee_class_type), parameter :: ieee_positive_normal
type(ieee_class_type), parameter :: ieee_positive_zero
type(ieee_class_type), parameter :: ieee_quiet_nan
type(ieee_class_type), parameter :: ieee_signaling_nan

contains

elemental function ieee_class(x) result(y)
    real :: x
    type(ieee_class_type) :: y
end function

elemental function ieee_value(x, class) result(y)
    real :: x
    type(ieee_class_type) :: class
    real :: y
end function

elemental function ieee_is_nan(x) result(y)
    real :: x
    logical :: y
end function

end module
