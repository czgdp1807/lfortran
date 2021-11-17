module stdlib_string_type
    type :: string_type
        sequence
        private
        character(len=:), allocatable :: raw
    end type string_type

    interface write(formatted)
        module procedure :: write_formatted
    end interface
end module

program string_14
end program