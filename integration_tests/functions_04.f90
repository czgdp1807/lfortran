module stdlib_ascii

    implicit none

    interface to_lower
        module procedure :: to_lower
    end interface to_lower

contains

    pure function to_lower(string) result(lower_string)
        character(len=*), intent(in) :: string
        character(len=len(string)) :: lower_string
        integer :: i

        do i = 1, len(string)
            lower_string(i:i) = string(i:i)
        end do

    end function to_lower

end module stdlib_ascii
