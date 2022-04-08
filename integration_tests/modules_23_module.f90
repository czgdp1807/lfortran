module string_module
implicit none
    type :: string_type
    sequence
    private
    character(len=:), allocatable :: raw
    end type string_type

    interface string_type
        pure elemental module function new_string(string) result(new)
            character(len=*), intent(in), optional :: string
            type(string_type) :: new
        end function new_string
        pure elemental module function new_string_from_integer_int8(val) result(new)
            integer(1), intent(in) :: val
            type(string_type) :: new
        end function new_string_from_integer_int8
        pure elemental module function new_string_from_integer_int16(val) result(new)
            integer(2), intent(in) :: val
            type(string_type) :: new
        end function new_string_from_integer_int16
        pure elemental module function new_string_from_integer_int32(val) result(new)
            integer(4), intent(in) :: val
            type(string_type) :: new
        end function new_string_from_integer_int32
        pure elemental module function new_string_from_integer_int64(val) result(new)
            integer(8), intent(in) :: val
            type(string_type) :: new
        end function new_string_from_integer_int64
    end interface string_type

    interface assignment(=)
    module procedure :: assign_char_to_string
    end interface assignment(=)

contains

    elemental subroutine assign_char_to_string(lhs, rhs)
    type(string_type), intent(inout) :: lhs
    character(len=*), intent(in) :: rhs
    lhs%raw = rhs
    end subroutine assign_char_to_string

    elemental function len_string(string) result(length)
            type(string_type), intent(in) :: string
            integer :: length

            if (allocated(string%raw)) then
                length = len(string%raw)
            else
                length = 0
            end if

    end function len_string

    elemental function adjustl_string(string) result(adjusted_string)
    type(string_type), intent(in) :: string
    type(string_type) :: adjusted_string

    adjusted_string%raw = adjustl(maybe(string))

    end function adjustl_string

    pure function maybe(string) result(maybe_string)
        type(string_type), intent(in) :: string
        character(len=len_string(string)) :: maybe_string
        if (allocated(string%raw)) then
            maybe_string = string%raw
        else
            maybe_string = ''
        end if
    end function maybe

end module