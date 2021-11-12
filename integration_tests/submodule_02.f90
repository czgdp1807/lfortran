! Reference - http://fortranwiki.org/fortran/show/Submodules
module points
  type :: point
     real :: x, y
  end type point

  interface
     module function point_dist_func(a, b) result(distance)
       type(point), intent(in) :: a, b
       real :: distance
     end function point_dist_func
  end interface

  interface
     module subroutine point_dist_subrout(a, b, distance)
       type(point), intent(in) :: a, b
       real, intent(out) :: distance
     end subroutine point_dist_subrout
  end interface
end module points

submodule (points) points_a
contains
  module function point_dist_func(a, b) result(distance)
    type(point), intent(in) :: a, b
    real :: distance
    distance = sqrt((a%x - b%x)**2 + (a%y - b%y)**2)
  end function point_dist_func

  module subroutine point_dist_subrout(a, b, distance)
    type(point), intent(in) :: a, b
    real, intent(out) :: distance
    distance = sqrt((a%x - b%x)**2 + (a%y - b%y)**2)
  end subroutine point_dist_subrout
end submodule points_a

program submodules_02
implicit none
end program